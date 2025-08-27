#include <stdio.h>
#include <stddef.h> /* for offsetof */
#include <string.h>
#include "dr_api.h"
#include "dr_ir_macros.h"
#include "dr_tools.h"
#include "drmgr.h"
#include "drsyms.h"
#include "drreg.h"
#include "drutil.h"
#include "drx.h"

#include "../interval_tree.h"

enum {
  REF_TYPE_READ = 0,
  REF_TYPE_WRITE = 1,
};

// from openssl target
static int *instrument_enable;
static struct interval_tree *wp_ref = NULL;
static uint64_t *wp_access_count;
static uint64_t *wp_hit_count;

// flag variables
static const char *binname = NULL;
static client_id_t client_id;
static void *mutex;      /* for multithread support */
static uint64 num_refs;  /* keep a global memory reference count */
static bool print_instr; /* for testing */

static int tls_idx;

#define MINSERT instrlist_meta_preinsert

static void
memory_accessed_verbose(app_pc acc_start, uint size, app_pc pc)
{
  DR_ASSERT(wp_ref);
  void *drcontext = dr_get_current_drcontext();
  // per_thread_t *data;

  instr_t instr;
  instr_init(drcontext, &instr);

  decode(drcontext, pc, &instr);
  instr_disassemble(drcontext, &instr, STDERR);
  dr_fprintf(STDERR, " @ %p\n", pc);


  // data = drmgr_get_tls_field(drcontext, tls_idx);
  (*wp_access_count)++;
  if (is_in_interval2(wp_ref, (uint64_t)acc_start, (uint64_t)acc_start + size)) {
    dr_fprintf(STDERR, "\tHIT: [" PIFX ", +%lu]\n", acc_start, size);
    (*wp_hit_count)++;
  }
}

static void
memory_accessed(app_pc acc_start, uint size)
{
  DR_ASSERT(wp_ref != NULL);
  void *drcontext = dr_get_current_drcontext();

  (*wp_access_count)++;
  if (is_in_interval2(wp_ref, (uint64_t)acc_start, (uint64_t)acc_start + size)) {
    //dr_fprintf(STDERR, "\tHIT: [" PIFX ", +%lu]\n", acc_start, size);
    (*wp_hit_count)++;
  }
}

/* insert inline code to add a memory reference info entry into the buffer */
static void
instrument_mem(void *drcontext, instrlist_t *ilist, instr_t *where, opnd_t ref,
    bool write)
{
  /* We need two scratch registers */
  reg_id_t reg_addr, reg_tmp;
  if (drreg_reserve_register(drcontext, ilist, where, NULL, &reg_addr) !=
      DRREG_SUCCESS ||
      drreg_reserve_register(drcontext, ilist, where, NULL, &reg_tmp) !=
      DRREG_SUCCESS) {
    DR_ASSERT(false); /* cannot recover */
    return;
  }
  bool ok = drutil_insert_get_mem_addr(drcontext, ilist, where, ref, reg_addr, reg_tmp);
  DR_ASSERT(ok);
  uint size = drutil_opnd_mem_size_in_bytes(ref, where);
  app_pc pc = instr_get_app_pc(where);
  if (print_instr)
    dr_insert_clean_call(drcontext, ilist, where, (void *)memory_accessed_verbose, false, 3, opnd_create_reg(reg_addr), OPND_CREATE_INT32(size), OPND_CREATE_INTPTR(pc));
  else
    dr_insert_clean_call(drcontext, ilist, where, (void *)memory_accessed, false, 2, opnd_create_reg(reg_addr), OPND_CREATE_INT32(size));

  /* Restore scratch registers */
  if (drreg_unreserve_register(drcontext, ilist, where, reg_addr) != DRREG_SUCCESS ||
      drreg_unreserve_register(drcontext, ilist, where, reg_tmp) != DRREG_SUCCESS)
    DR_ASSERT(false);
}

/* For each memory reference app instr, we insert inline code to fill the buffer
 * with an instruction entry and memory reference entries.
 */
static dr_emit_flags_t
event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *where,
    bool for_trace, bool translating, void *user_data)
{
  int i;
  // dr_fprintf(STDERR, "Instrumenting %p\n", instr_get_app_pc(where));

  /* Insert code to add an entry for each data memory reference opnd. */
  instr_t *instr_operands = drmgr_orig_app_instr_for_operands(drcontext);
  if (instr_operands == NULL ||
      !((instr_reads_memory(instr_operands) || instr_writes_memory(instr_operands))
        && !instr_is_cti(instr_operands)))
    return DR_EMIT_DEFAULT;
  DR_ASSERT(instr_is_app(instr_operands));

  if (IF_AARCHXX_OR_RISCV64_ELSE(instr_is_exclusive_store(instr_operands), false))
    // DR_ASSERT_MSG(false, "Unexpected Exclusive Store encountered\n");
    return DR_EMIT_DEFAULT; /* skip */

  for (i = 0; i < instr_num_srcs(instr_operands); i++) {
    const opnd_t src = instr_get_src(instr_operands, i);
    if (opnd_is_memory_reference(src)) {
      instrument_mem(drcontext, bb, where, src, false);
    }
  }

  for (i = 0; i < instr_num_dsts(instr_operands); i++) {
    const opnd_t dst = instr_get_dst(instr_operands, i);
    if (opnd_is_memory_reference(dst)) {
      instrument_mem(drcontext, bb, where, dst, true);
    }
  }

  return DR_EMIT_DEFAULT;
}

/* We transform string loops into regular loops so we can more easily
 * monitor every memory reference they make.
 */
static dr_emit_flags_t
event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace,
    bool translating)
{
  if (!drutil_expand_rep_string(drcontext, bb)) {
    DR_ASSERT(false);
    /* in release build, carry on: we'll just miss per-iter refs */
  }
  if (!drx_expand_scatter_gather(drcontext, bb, NULL)) {
    DR_ASSERT(false);
  }
  return DR_EMIT_DEFAULT;
}
static bool
print_symbol(const char *name, size_t modoffs, void *data) {
    dr_fprintf(STDERR, "symbol: %s, offset: 0x%zx\n", name, modoffs);
    return true; // continue enumeration
}

static void
event_module_load(void *drcontext, const module_data_t *mod, bool loaded)
{
  const char *name = dr_module_preferred_name(mod);
  dr_fprintf(STDERR, "name: %s\n", name);
  if (strcmp(name, binname) != 0)
    return;

  // enumerate symbols in this module
  // drsym_error_t res = drsym_enumerate_symbols(mod->full_path, print_symbol, NULL, 0);
  // if (res != DRSYM_SUCCESS) {
  //   dr_fprintf(STDERR, "drsym_enumerate_symbols() failed: %d\n", res);
  //   DR_ASSERT(false);
  // }

  size_t wp_modoffs;
  if (drsym_lookup_symbol(mod->full_path, "dr_watched", &wp_modoffs, 0) == DRSYM_SUCCESS) {
    dr_fprintf(STDERR, "Resolved `dr_watched` at %p + %p\n", mod->start, (app_pc)wp_modoffs);
  } else {
    DR_ASSERT_MSG(false, "Failed to resolve `dr_watched` symbol\n");
  }
  size_t flag_modoffs;
  if (drsym_lookup_symbol(mod->full_path, "dr_instrument_enable", &flag_modoffs, DRSYM_DEMANGLE) == DRSYM_SUCCESS) {
    dr_fprintf(STDERR, "Resolved `dr_instrument_enable` at %p + %p\n", mod->start, (app_pc)flag_modoffs);
  } else {
    DR_ASSERT_MSG(false, "Failed to resolve `dr_instrument_enable` symbol\n");
  }
  size_t access_count_modoffs;
  if (drsym_lookup_symbol(mod->full_path, "dr_access_count", &access_count_modoffs, 0) == DRSYM_SUCCESS) {
    dr_fprintf(STDERR, "Resolved `dr_access_count` at %p + %p\n", mod->start, (app_pc)access_count_modoffs);
  } else {
    DR_ASSERT_MSG(false, "Failed to resolve `dr_access_count` symbol\n");
  }
  size_t hit_count_modoffs;
  if (drsym_lookup_symbol(mod->full_path, "dr_hit_count", &hit_count_modoffs, 0) == DRSYM_SUCCESS) {
    dr_fprintf(STDERR, "Resolved `dr_hit_count` at %p + %p\n", mod->start, (app_pc)hit_count_modoffs);
  } else {
    DR_ASSERT_MSG(false, "Failed to resolve `dr_hit_count` symbol\n");
  }
  wp_ref = (struct interval_tree *)(mod->start + wp_modoffs);
  instrument_enable = (int *)(mod->start + flag_modoffs);
  wp_access_count = (uint64_t *)(mod->start + access_count_modoffs);
  wp_hit_count = (uint64_t *)(mod->start + hit_count_modoffs);
}

static void
event_module_unload(void *drcontext, const module_data_t *mod) {
  wp_ref = NULL;
  instrument_enable = NULL;
}

static void
event_exit(void)
{
  dr_log(NULL, DR_LOG_ALL, 1, "Client 'memtrace' num refs seen: " SZFMT "\n", num_refs);

  if (!drmgr_unregister_module_load_event(event_module_load) ||
      !drmgr_unregister_module_unload_event(event_module_unload) ||
      !drmgr_unregister_bb_app2app_event(event_bb_app2app) ||
      !drmgr_unregister_bb_insertion_event(event_app_instruction) ||
      drreg_exit() != DRREG_SUCCESS)
    DR_ASSERT(false);

  drsym_exit();
  drutil_exit();
  drmgr_exit();
  drx_exit();
}

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
  /* We need 2 reg slots beyond drreg's eflags slots => 3 slots */
  drreg_options_t ops = { sizeof(ops), 3, false };
  dr_set_client_name("Watchpoint hit counter", "http://dynamorio.org/issues");

  for (size_t i = 1; i < argc; i++) {
    if (strncmp(argv[i], "-print-instr", 12) == 0) {
      print_instr = true;
    }  else if (strncmp(argv[i], "-binname", 12) == 0 && i + 1 < argc) {
      binname = argv[++i];
    } else {
      dr_fprintf(STDERR, "Warning: ignore options %s\n", argv[i]);
      dr_abort();
    }
  }
  DR_ASSERT(binname);

  // dr_fprintf(STDOUT, "Initializing..\n");

  if (!drmgr_init() || drsym_init(0) != DRSYM_SUCCESS || drreg_init(&ops) != DRREG_SUCCESS || !drutil_init() ||
      !drx_init())
    DR_ASSERT(false);

  // dr_fprintf(STDOUT, "Register Events..\n");

  // /* register events */
  dr_register_exit_event(event_exit);
  if (!drmgr_register_module_load_event(event_module_load) ||
      !drmgr_register_module_unload_event(event_module_unload) ||
      !drmgr_register_bb_app2app_event(event_bb_app2app, NULL) ||
      !drmgr_register_bb_instrumentation_event(NULL /*analysis_func*/,
        event_app_instruction, NULL))
    DR_ASSERT(false);
}
