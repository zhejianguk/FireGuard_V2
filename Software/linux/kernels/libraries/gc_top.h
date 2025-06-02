#ifndef GC_TOP_H
#define GC_TOP_H

#define NUM_CORES 5
#define GC_DEBUG 0
#define FILTERWIDTH 0x00

#define BOOM_ID 0
#define CHK1_ID 1
#define CHK2_ID 2
#define CHK3_ID 3
#define CHK4_ID 4
#define CHK5_ID 5
#define CHK6_ID 6
#define CHK7_ID 7

#define AGG_CORE_ID (NUM_CORES - 1)

#define read_csr_safe(reg) ({ register long __tmp asm("a0"); \
        asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
        __tmp; })
        
#endif