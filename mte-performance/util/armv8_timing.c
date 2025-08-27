#include "armv8_timing.h"

uint64_t arm_v8_get_timing(void)
{
  uint64_t result = 0;

  asm volatile("msr %0, PMCCNTR_EL0" : "=r" (result));

  return result;
}

void arm_v8_timing_init(void)
{
  uint32_t value = 0;

  /* Enable Performance Counter */
  asm volatile("MRS %0, PMCR_EL0" : "=r" (value));
  value |= ARMV8_PMCR_E; /* Enable */
  value |= ARMV8_PMCR_C; /* Cycle counter reset */
  value |= ARMV8_PMCR_P; /* Reset all counters */
  asm volatile("MSR PMCR_EL0, %0" : : "r" (value));

  /* Enable cycle counter register */
  asm volatile("MRS %0, PMCNTENSET_EL0" : "=r" (value));
  value |= ARMV8_PMCNTENSET_EL0_EN;
  asm volatile("MSR PMCNTENSET_EL0, %0" : : "r" (value));
}

void arm_v8_timing_terminate(void)
{
  uint32_t value = 0;
  uint32_t mask = 0;

  /* Disable Performance Counter */
  asm volatile("MRS %0, PMCR_EL0" : "=r" (value));
  mask = 0;
  mask |= ARMV8_PMCR_E; /* Enable */
  mask |= ARMV8_PMCR_C; /* Cycle counter reset */
  mask |= ARMV8_PMCR_P; /* Reset all counters */
  asm volatile("MSR PMCR_EL0, %0" : : "r" (value & ~mask));

  /* Disable cycle counter register */
  asm volatile("MRS %0, PMCNTENSET_EL0" : "=r" (value));
  mask = 0;
  mask |= ARMV8_PMCNTENSET_EL0_EN;
  asm volatile("MSR PMCNTENSET_EL0, %0" : : "r" (value & ~mask));
}

void arm_v8_reset_timing(void)
{
  uint32_t value = 0;
  asm volatile("MRS %0, PMCR_EL0" : "=r" (value));
  value |= ARMV8_PMCR_C; /* Cycle counter reset */
  asm volatile("MSR PMCR_EL0, %0" : : "r" (value));
}


