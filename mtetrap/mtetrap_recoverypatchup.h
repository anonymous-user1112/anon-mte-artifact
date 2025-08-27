#define data_watchpoint_tag ((unsigned char) 0x01)
#define mte_untag_insts_offset ((unsigned int) 4)
#define mte_rerun_inst_offset ((unsigned int) 28)
#define mte_retag_insts_offset ((unsigned int) 36)
#define mte_sp_restore_inst_offset ((unsigned int) 60)
#define mte_br_inst_offset ((unsigned int) 64)
#define mte_recovery_code_size ((unsigned int) 72)
// #define mte_rerun_inst_offset ((unsigned int) 32)
// #define mte_retag_insts_offset ((unsigned int) 40)
// #define mte_sp_restore_inst_offset ((unsigned int) 68)
// #define mte_br_inst_offset ((unsigned int) 72)
// #define mte_recovery_code_size ((unsigned int) 80)

#define insert_chosen_tag(ptr, tag) (((unsigned long)ptr) | (((unsigned long)tag) << 56))

#ifdef __KERNEL__
#define write_program_addr(dst, src, len) if (copy_to_user(dst, src, len)) { return 2; }
#define read_program_addr(dst, src, len) if (copy_from_user(dst, src, len)) { return 3; }
#define doprint printk
#else
#define write_program_addr(dst, src, len) memcpy(dst, src, len)
#define read_program_addr(dst, src, len) memcpy(dst, src, len)
#define doprint printf
#endif

static int patchup_mte_app_recovery(
  unsigned char* memory_addr,
  unsigned char** p_inst_addr,
  unsigned char** p_stack_addr,
  unsigned char * recovery_code_addr,
  unsigned long* access_count_addr,
  unsigned char * pre_recovery_code_addr)
{
  const unsigned long memory_addr_val = (unsigned long) memory_addr;
  unsigned char* stack_ptr = *p_stack_addr;
  unsigned char* inst_addr = *p_inst_addr;

  const unsigned long tag_mask = 0xff0000000000000f;
  const unsigned long untagged_memory_addr_val = memory_addr_val & (~tag_mask);
  const unsigned long tagged_memory_addr_val = insert_chosen_tag(untagged_memory_addr_val, data_watchpoint_tag);

  const unsigned short ptr_2byte_1 = (unsigned short)(untagged_memory_addr_val);
  const unsigned short ptr_2byte_2 = (unsigned short)(untagged_memory_addr_val >> 16);
  const unsigned short ptr_2byte_3 = (unsigned short)(untagged_memory_addr_val >> 32);
  const unsigned short untaggedptr_2byte_4 = (unsigned short)(untagged_memory_addr_val >> 48);
  const unsigned short taggedptr_2byte_4 = (unsigned short)(tagged_memory_addr_val >> 48);

  const unsigned int ptr_shifted_1 = 0x06940000 | ((unsigned int)ptr_2byte_1);
  const unsigned int ptr_shifted_2 = 0x07950000 | ((unsigned int)ptr_2byte_2);
  const unsigned int ptr_shifted_3 = 0x07960000 | ((unsigned int)ptr_2byte_3);
  const unsigned int untaggedptr_shifted_4 = 0x07970000 | ((unsigned int)untaggedptr_2byte_4);
  const unsigned int taggedptr_shifted_4 = 0x07970000 | ((unsigned int)taggedptr_2byte_4);

  // Begin the recovery process
  const unsigned int untagged_insts [] = {
    ptr_shifted_1 << 5,
    ptr_shifted_2 << 5,
    ptr_shifted_3 << 5,
    untaggedptr_shifted_4 << 5,
  };

  const unsigned int tagged_insts [] = {
    ptr_shifted_1 << 5,
    ptr_shifted_2 << 5,
    ptr_shifted_3 << 5,
    taggedptr_shifted_4 << 5,
  };

  const long min_offset = -0x7ffffff;
  const long max_offset =  0x7ffffff;
  const unsigned long offset_mask = 0xfffffff;

  // Vars used later
  unsigned int faulting_inst_copy = 0;
  unsigned char* new_stack_ptr = 0;
  unsigned char* next_instr_addr = 0;
  unsigned char* br_inst_addr = 0;
  long offset = 0;
  unsigned long offset_compressed = 0;
  unsigned int br_rel_inst = 0;
  unsigned char stack_immediate = 0;

  unsigned long access_count_val = 0;

  // doprint("[mtetrap] Recovery: stage 0\n");

  if (access_count_addr) {
    read_program_addr(&access_count_val, access_count_addr, sizeof(unsigned long));
    access_count_val++;
    write_program_addr(access_count_addr, &access_count_val, sizeof(unsigned long));
  }

  // 1. Copy the untagged pointer into the recovery page
  write_program_addr(&(recovery_code_addr[mte_untag_insts_offset]), untagged_insts, sizeof(untagged_insts));

  // 2. Copy the faulting instruction to the recovery page
  // doprint("[mtetrap] Recovery: stage 2\n");
  read_program_addr(&faulting_inst_copy, inst_addr, sizeof(unsigned int));
  write_program_addr(&(recovery_code_addr[mte_rerun_inst_offset]), &faulting_inst_copy, sizeof(unsigned int));

  // 3. Copy the tagged pointer into the recovery page
  // doprint("Recovery: stage 3\n");
  write_program_addr(&(recovery_code_addr[mte_retag_insts_offset]), tagged_insts, sizeof(tagged_insts));

  // 4. Make room on the stack for a new var
  // doprint("Recovery: stage 4\n");
  new_stack_ptr = stack_ptr - sizeof(void*);
  stack_immediate = sizeof(void*);
  if (((unsigned long)new_stack_ptr) % 16 != 0) {
    stack_immediate += 8;
    new_stack_ptr -= 8;
  }
  *p_stack_addr = new_stack_ptr;

  // The stack room immediate is a 12-bit constant stored in bits 21 to 10 in the instruction at mte_sp_restore_inst_offset
  // However, since we only want write values 8 (1000) or 16 (10000), we can restrict our writes from bit 10 to bit 15
  // Additionally, bits 8 and 9 are fixed at 1.
  // So we just have to write a single uint8_t from bits 8 to 16
  stack_immediate = (stack_immediate << 2) | ((unsigned char)3);
  write_program_addr(&(recovery_code_addr[mte_sp_restore_inst_offset + 1]), &stack_immediate, sizeof(stack_immediate));

  // 5. Compute and write the offset of the PC after the faulting instruction so we can return to it
  // doprint("Recovery: stage 5\n");
  next_instr_addr = inst_addr + 4;
  br_inst_addr = &(recovery_code_addr[mte_br_inst_offset]);
  offset = next_instr_addr - br_inst_addr;
  if(offset < min_offset || offset > max_offset) {
    return 1;
  }

  offset_compressed = (((unsigned long) offset) & offset_mask) >> 2;
  br_rel_inst = 0x14000000 | ((unsigned int) offset_compressed);
  write_program_addr(br_inst_addr, &br_rel_inst, sizeof(br_rel_inst));

  // 6. Set the PC to resume as the recovery page
  // doprint("Recovery: stage 6\n");
  if (pre_recovery_code_addr != NULL)
    *p_inst_addr = pre_recovery_code_addr;
  else
    *p_inst_addr = recovery_code_addr;

  // doprint("Recovery: finished\n");

// #ifndef __KERNEL__
//   __builtin___clear_cache(recovery_code_addr, recovery_code_addr + mte_recovery_code_size);
// #endif

  return 0;
}

