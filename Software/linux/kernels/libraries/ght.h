#ifndef __GHT_H_INCLUDED_
#define __GHT_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"

#define TRUE 0x01
#define FALSE 0x00


#define GHT_FULL 0x02
#define GHT_EMPTY 0x01


/************** Debug features **************/
static inline uint64_t debug_ecounter ()
{
  uint64_t ecounter;
  ROCC_INSTRUCTION_D (1, ecounter, 0x22);
  return ecounter;
}

static inline void debug_bp_reset ()
{
  ROCC_INSTRUCTION (1, 0x2d);
}

static inline uint64_t debug_bp_checker ()
{
  uint64_t bp_checker;
  ROCC_INSTRUCTION_D (1, bp_checker, 0x1d);
  return bp_checker;
}

static inline uint64_t debug_bp_cdc ()
{
  uint64_t bp_cdc;
  ROCC_INSTRUCTION_D (1, bp_cdc, 0x1e);
  return bp_cdc;
}

static inline uint64_t debug_bp_filter ()
{
  uint64_t bp_filter;
  ROCC_INSTRUCTION_D (1, bp_filter, 0x1f);
  return bp_filter;
}

static inline void ght_debug_filter_width (uint64_t width)
{
  uint64_t set_debug_width;
  set_debug_width = ((width & 0xf)<<4) | 0x05;
  ROCC_INSTRUCTION_SS (1, set_debug_width, 0X02, 0x06);
}
/************** End of Debug features **************/

static inline void ght_set_status_00 ()
{
  ROCC_INSTRUCTION (1, 0x30);
}

static inline void ght_set_status_01 ()
{
  ROCC_INSTRUCTION (1, 0x31);
}

static inline void ght_set_status_02 ()
{
  ROCC_INSTRUCTION (1, 0x32);
}

static inline void ght_set_status_04 ()
{
  ROCC_INSTRUCTION (1, 0x34);
}

static inline void ght_set_status_06 ()
{
  ROCC_INSTRUCTION (1, 0x36);
}

static inline void ght_set_critial (uint64_t critial_or_not)
{
  ROCC_INSTRUCTION_S (1, critial_or_not, 0x39);
}

static inline uint64_t ght_check_critial ()
{
  uint64_t critial_or_not;
  ROCC_INSTRUCTION_D (1, critial_or_not, 0x49);
  return critial_or_not;
}


static inline uint64_t ght_get_status ()
{
  uint64_t get_status;
  ROCC_INSTRUCTION_DSS (1, get_status, 0X00, 0X00, 0x06);
  return get_status;
}

static inline uint64_t ght_get_satp ()
{
  uint64_t get_satp;
  ROCC_INSTRUCTION_DSS (1, get_satp, 0X00, 0X00, 0x17);
  return get_satp;
}

static inline uint64_t ght_get_priv ()
{
  uint64_t get_priv;
  ROCC_INSTRUCTION_DSS (1, get_priv, 0X00, 0X00, 0x18);
  return get_priv;
}

static inline void ght_set_satp_priv ()
 {
   ROCC_INSTRUCTION_S (1, 0x01, 0x16);
 }

 static inline void ght_unset_satp_priv ()
 {
   ROCC_INSTRUCTION_S (1, 0x02, 0x16);
 }

/*
static inline uint64_t ght_get_buffer_status ()
{
  uint64_t get_buffer_status;
  ROCC_INSTRUCTION_DSS (1, get_buffer_status, 0X00, 0X00, 0x08);
  return get_buffer_status;
}
*/

static inline void ght_cfg_filter (uint64_t index, uint64_t func, uint64_t opcode, uint64_t sel_d)
{
  uint64_t set_ref;
  set_ref = ((index & 0x1f)<<4) | ((sel_d & 0xf)<<17) | ((opcode & 0x7f)<<21) | ((func & 0xf)<<28) | 0x02;
  ROCC_INSTRUCTION_SS (1, set_ref, 0X02, 0x06);
}

static inline void ght_cfg_filter_rvc (uint64_t index, uint64_t func, uint64_t opcode, uint64_t sel_d)
{
  uint64_t set_ref;
  set_ref = ((index & 0x1f)<<4) | ((sel_d & 0xf)<<17) | ((opcode & 0x7f)<<21) | (((func|0x08) & 0xf)<<28) | 0x02;
  ROCC_INSTRUCTION_SS (1, set_ref, 0X02, 0x06);
}


static inline void ght_cfg_se (uint64_t se_id, uint64_t end_id, uint64_t policy, uint64_t start_id)
{
  uint64_t set_se;
  set_se = ((se_id & 0x1f)<<4) | ((start_id & 0xf)<<17) | ((policy & 0x7f)<<21) | ((end_id & 0xf)<<28) | 0x04;
  ROCC_INSTRUCTION_SS (1, set_se, 0X02, 0x06);
}

static inline void ght_cfg_mapper (uint64_t inst_type, uint64_t ses_receiving_inst)
{
  uint64_t set_mapper;
  set_mapper = ((inst_type & 0x1f)<<4) | ((ses_receiving_inst & 0xFFFF)<<16) | 0x03;
  ROCC_INSTRUCTION_SS (1, set_mapper, 0X02, 0x06);
}

static inline void ghm_cfg_agg (uint64_t agg_core_id)
{
  uint64_t agg_core_set;
  agg_core_set = ((agg_core_id & 0xffff)<<16) | 0x08;
  ROCC_INSTRUCTION_SS (1, agg_core_set, 0X02, 0x06);
}

/*
uint64_t task_synthetic_malloc (uint64_t base)
{
  uint64_t *ptr = NULL;
  uint64_t ptr_size = 32;
  uint64_t sum = 0;
    
  ptr = (uint64_t*) malloc(ptr_size * sizeof(uint64_t));


  // if memory cannot be allocated
  if(ptr == NULL) {
    printf("Error! memory not allocated. \r\n");
    exit(0);
  }

  for (uint64_t i = 0; i < ptr_size; i++)
  {
    *(ptr + i) = base + i;
    *(ptr + i) = *(ptr + i) + *(ptr + ptr_size - i - 1);
  }

  for (uint64_t i = 0; i < ptr_size; i++)
  {
    sum = sum + *(ptr+i);
  }

  free(ptr);

  return sum;
}
*/

static inline void ght_set_num_of_checkers (uint64_t num)
{
  ROCC_INSTRUCTION_S (1, num, 0x1c);
}


static inline uint64_t ght_get_initialisation ()
{
  uint64_t get_status;
  ROCC_INSTRUCTION_D (1, get_status, 0x1b);
  return get_status;
}


 static inline uint64_t ght_readFIU (uint64_t sel)
 {
   uint64_t latency;

   ROCC_INSTRUCTION_DS (1, latency, sel, 0x68);

   return latency;
 }

 #endif 