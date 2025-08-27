#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/version.h>

#include "mtetrap_module.h"

#include "mtetrap_recoverypatchup.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("XXXXXXXX Narayan");
MODULE_DESCRIPTION("A simple example Linux module.");
MODULE_VERSION("0.01");

pid_t pid_vnr_fake(struct pid *pid);

pid_t pid_vnr_fake(struct pid *pid)
{
	return 0;
}

#define pid_vnr_temp pid_vnr_fake

// KProbe at https://elixir.bootlin.com/linux/v6.8/source/arch/arm64/mm/fault.c#L763
// do_tag_check_fault

static struct kprobe kp = {
  .symbol_name  = "do_tag_check_fault",
};

static int device_open(struct inode *inode, struct file *file) {
  // printk(KERN_ALERT "[mtetrap] Main open!\n");
  return 0;
}

static int device_release(struct inode *inode, struct file *file) {
  // printk(KERN_ALERT "[mtetrap] Main release!\n");
  return 0;
}

unsigned long g_trap_handler_pid = 0;
unsigned long g_trap_handler = 0;
unsigned long g_access_count_addr = 0;
unsigned long g_trap_handler_pre = 0;
unsigned long g_kernel_execution_skip = 0;

static unsigned long look_for_next_ret(char* addr) {
  unsigned int diff = 0;

  do {
    unsigned int inst = 0;
    memcpy(&inst, addr, sizeof(unsigned int));

    // check for return instruction
    if (inst == 0xd65f03c0) {
      return diff;
    }

    addr += 4;
    diff += 4;

    if (diff > 10000) {
      printk("[mtetrap] ERROR: couldn't find the ret instruction\n");
      return 0;
    }
  } while(true);
}

static int __kprobes kprobe_handler_pre(struct kprobe *p, struct pt_regs *regs)
{
  // printk("[mtetrap] kprobe_handler_pre\n");

  if (g_trap_handler_pid == pid_vnr_temp(get_pid(task_tgid(current))))
  {
    // regs parameter to the original function is the third parameter, i.e. in reg x2
    unsigned long param_far = regs_get_kernel_argument(regs, 0);
    unsigned long memory_addr = untagged_addr(param_far);
    unsigned long param_regs_addr = regs_get_kernel_argument(regs, 2);
    struct pt_regs* param_regs = (struct pt_regs*) param_regs_addr;

    // printk("[mtetrap] kprobe_handler_pre kern PC: %p (%llu), SP: %p (%llu)\n", (void*) regs->pc, regs->pc, (void*) regs->sp, regs->sp);
    // printk("[mtetrap] kprobe_handler_pre user PC: %p (%llu), SP: %p (%llu)\n", (void*) param_regs->pc, param_regs->pc, (void*) param_regs->sp, param_regs->sp);

    if (g_trap_handler != 0) {
      // Redirect the app to the app's custom trap handler
      int patchup_error = patchup_mte_app_recovery(
        (unsigned char*) memory_addr,
        (unsigned char**) &(param_regs->pc),
        (unsigned char**) &(param_regs->sp),
        (unsigned char*) g_trap_handler,
        (unsigned long*) g_access_count_addr,
        (unsigned char*) g_trap_handler_pre
      );

      if (patchup_error != 0) {
        printk("[mtetrap] kprobe_handler_pre: Got patchup error: %d\n", patchup_error);
      } else {
        // Redirect the kernel execution to skip the regular trap
        if (g_kernel_execution_skip == 0) {
          g_kernel_execution_skip = look_for_next_ret((char*) regs->pc);
        }

        // printk("[mtetrap] kprobe_handler_pre: Execution skip: %d, Curr pc: %lu\n", (int) g_kernel_execution_skip, (unsigned long) (regs->pc));
        regs->pc += g_kernel_execution_skip;
        regs_set_return_value(regs, 0);

        // printk("[mtetrap] kprobe_handler_pre: New pc: %lu\n", (unsigned long) (regs->pc));

        // Return 1 since kernel execution has been redirected
        return 1;
      }
    }
  } else {
      // printk("[mtetrap] kprobe_handler_pre: Other pid\n");
  }

  return 0;
}

static void __kprobes kprobe_handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags) {
  // printk("[mtetrap] kprobe_handler_post\n");
}

static long attach_kprobe(void) {
  int r2 = 0;
  kp.pre_handler = kprobe_handler_pre;
  kp.post_handler = kprobe_handler_post;

  r2 = register_kprobe(&kp);
  if (r2 < 0) {
    printk(KERN_ALERT "[mtetrap] Error: register_kprobe failed, returned %d\n", r2);
    return r2;
  }
  printk("[mtetrap] Planted kprobe at %p\n", kp.addr);

  return 0;
}

static long ioctl_setredirect(struct ioctl_setredirect_params param) {
  // printk(KERN_ALERT "[mtetrap] ioctl_setredirect start: %lu!\n", param);
  g_trap_handler_pid = pid_vnr_temp(get_pid(task_tgid(current)));
  g_trap_handler = param.trap_handler;
  g_access_count_addr = param.access_count_addr;
  g_trap_handler_pre = param.trap_handler_pre;
  return 0;
}

static int trigger_page_fault(unsigned long user_address)
{
  struct vm_area_struct *vma;
  struct page *page;
  int ret;

  vma = find_vma(current->mm, user_address);
  if (!vma)
    return -EFAULT;

  printk(KERN_INFO "[mtetrap] vm_flags: %lx\n", vma->vm_flags);

  if (!(vma->vm_flags & VM_MAYEXEC))
    return -EACCES;

  // Presumably FOLL_WRITE will set PG_dirty of page->flags
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0))
  ret = get_user_pages(user_address, 1, FOLL_WRITE, &page);
#else
  ret = get_user_pages(user_address, 1, FOLL_WRITE, &page, NULL);
#endif

  if (ret < 1) {
    printk(KERN_ERR "[mtetrap] Failed to trigger page fault: %d\n", ret);
    return -EFAULT;
  }
  printk(KERN_INFO "[mtetrap] page->flags: %lx\n", page->flags);

  // Successfully handled the page fault; release the page immediately
  put_page(page);
  printk(KERN_INFO "[mtetrap] Page fault triggered and handled for address: %px\n", user_address);
  return 0;
}

static long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
  int ret;
  struct ioctl_setredirect_params param = {0};
  switch (ioctl_num) {
    case MTETRAP_IOCTL_CMD_SETREDIRECT:
      if (ioctl_param != 0) {
        if (copy_from_user(&(param.trap_handler), (char*) ioctl_param, sizeof(unsigned long))) {
          return -1;
        }
        ret = trigger_page_fault(param.trap_handler);
        if (ret)
          return ret;

        if (copy_from_user(&(param.access_count_addr), (char*) (ioctl_param + 8), sizeof(unsigned long))) {
          return -1;
        }
        if (copy_from_user(&(param.trap_handler_pre), (char*) ioctl_param + 16, sizeof(unsigned long))) {
          return -1;
        }
      }
      return ioctl_setredirect(param);

    default:
      return -1;
  }
  return 0;
}

static struct file_operations f_ops = { .owner = THIS_MODULE,
                                       .unlocked_ioctl = device_ioctl,
                                       .open = device_open,
                                       .release = device_release};

static struct miscdevice misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MTETRAP_DEVICE_NAME,
    .fops = &f_ops,
    .mode = S_IRWXUGO,
};

static int __init my_module_init(void) {
  int r = 0;
  int r2 = 0;
  printk(KERN_ALERT "[mtetrap] Init start!\n");

  /* Register device */
  r = misc_register(&misc_dev);
  if (r != 0) {
    printk(KERN_ALERT "[mtetrap] Error: Failed registering device with %d\n", r);
    return 1;
  }

  r2 = attach_kprobe();
  if (r2 != 0) {
    misc_deregister(&misc_dev);
  }

  printk(KERN_ALERT "[mtetrap] Init complete.\n");
  return r2;
}

static void __exit my_module_exit(void) {
  // printk(KERN_ALERT "[mtetrap] Exit start!\n");
  unregister_kprobe(&kp);
  misc_deregister(&misc_dev);
  // printk(KERN_ALERT "[mtetrap] Exit complete!\n");
}

module_init(my_module_init);
module_exit(my_module_exit);
