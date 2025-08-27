#ifndef ARM_V8_TIMING_H
#define ARM_V8_TIMING_H

#include <stdint.h>

#define ARMV8_PMCR_E            (1 << 0) /* Enable all counters */
#define ARMV8_PMCR_P            (1 << 1) /* Reset all counters */
#define ARMV8_PMCR_C            (1 << 2) /* Cycle counter reset */

#define ARMV8_PMUSERENR_EN      (1 << 0) /* EL0 access enable */
#define ARMV8_PMUSERENR_CR      (1 << 2) /* Cycle counter read enable */
#define ARMV8_PMUSERENR_ER      (1 << 3) /* Event counter read enable */

#define ARMV8_PMCNTENSET_EL0_EN (1 << 31) /* Performance Monitors Count Enable Set register */

inline uint64_t
arm_v8_get_timing(void);

inline void
arm_v8_timing_init(void);

inline void
arm_v8_timing_terminate(void);

inline void
arm_v8_reset_timing(void);

#endif  /*ARM_V8_TIMING_H*/