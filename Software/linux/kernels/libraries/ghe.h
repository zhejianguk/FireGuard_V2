#include <stdint.h>
#include "rocc.h"

#define GHE_FULL 0x02
#define GHE_EMPTY 0x01


/*
static inline uint64_t debug_icounter ()
{
  uint64_t icounter;
  ROCC_INSTRUCTION_D (1, icounter, 0x1a);
  return icounter;
}

static inline uint64_t debug_gcounter ()
{
  uint64_t gcounter;
  ROCC_INSTRUCTION_D (1, gcounter, 0x23);
  return gcounter;
}

static inline uint64_t debug_mcounter ()
{
  uint64_t mcounter;
  ROCC_INSTRUCTION_D (1, mcounter, 0x19);
  return mcounter;
}
*/

static inline uint64_t ghe_status ()
{
  uint64_t status;
  ROCC_INSTRUCTION_D (1, status, 0x00);
  return status; 
  // 0b01: empty; 
  // 0b10: full;
  // 0b00: data buffered;
  // 0b11: error
}


static inline uint64_t ghe_checkght_status ()
{
  uint64_t status;
  ROCC_INSTRUCTION_D (1, status, 0x07);
  return status; 
}


static inline void ghe_complete ()
{
  ROCC_INSTRUCTION (1, 0x41);
}

static inline void ghe_release ()
{
  ROCC_INSTRUCTION (1, 0x43);
}

static inline void ghe_go ()
{
  ROCC_INSTRUCTION (1, 0x40);
}

static inline uint64_t ghe_agg_status ()
{
  uint64_t status;
  ROCC_INSTRUCTION_D (1, status, 0x10);
  return status; 
  // 0b01: empty; 
  // 0b10: full;
  // 0b00: data buffered;
  // 0b11: error
}

static inline void ghe_agg_push (uint64_t header, uint64_t payload)
{
  ROCC_INSTRUCTION_SS (1, header, payload, 0x11);
}

static inline uint64_t ghe_sch_status ()
{
  uint64_t status;
  ROCC_INSTRUCTION_D (1, status, 0x20);
  return status; 
  // 0b01: empty; 
  // 0b10: full;
  // 0b00: data buffered;
  // 0b11: error
}

int gc_pthread_setaffinity(uint64_t phart_id){
	pthread_t thread_id = pthread_self();
	cpu_set_t cpu_id;
	int s;

	CPU_ZERO(&cpu_id);
	CPU_SET(phart_id, &cpu_id); 
	s = pthread_setaffinity_np(thread_id, sizeof(cpu_id), &cpu_id);
	
	return s;
} 


static inline void ghe_initailised ()
{
  ROCC_INSTRUCTION (1, 0x51);
}

static inline void ghe_deinitailised ()
{
  ROCC_INSTRUCTION (1, 0x50);
}



static inline uint64_t ghe_get_bufferdepth ()
{
  uint64_t depth;
  ROCC_INSTRUCTION_D (1, depth, 0x25);
  return depth;
}

/*
static inline uint64_t ghe_get_fifocounter ()
{
  uint64_t counter;
  ROCC_INSTRUCTION_D (1, counter, 0x26);
  return counter;
}

static inline uint64_t ghe_get_fifodcounter ()
{
  uint64_t dcounter;
  ROCC_INSTRUCTION_D (1, dcounter, 0x27);
  return dcounter;
}
*/

static inline uint64_t ghe_get_fifocache0 ()
{
  uint64_t fifocache;
  ROCC_INSTRUCTION_D (1, fifocache, 0x28);
  return fifocache;
}

static inline uint64_t ghe_get_fifocache1 ()
{
  uint64_t fifocache;
  ROCC_INSTRUCTION_D (1, fifocache, 0x29);
  return fifocache;
}
