#ifndef _GNU_SOURCE
	#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#include "libraries/ght.h"
#include "libraries/ghe.h"
#include "libraries/gc_top.h"
#include "libraries/spin_lock.h"

  
void main (void)
{	
   	//================== Initialisation ==================//
    // Bound current thread to BOOM
    if (gc_pthread_setaffinity(BOOM_ID) != 0) {
		printf ("[Boom-C%x]: pthread_setaffinity failed.", BOOM_ID);
	}

    /*=====================*/
    /*  GC configurations  */
    /*=====================*/
    ghm_cfg_agg(AGG_CORE_ID);
	ght_set_num_of_checkers(NUM_CORES-1);
	
	// Insepct JAL 
	// inst_index: 0x01
	// Func: 0x00 - 0x07
	// Opcode: 0x6F
	// Data path: ALU + JALR
	ght_cfg_filter(0x01, 0x00, 0x6F, 0x01);
	ght_cfg_filter(0x01, 0x01, 0x6F, 0x01);
	ght_cfg_filter(0x01, 0x02, 0x6F, 0x01);
	ght_cfg_filter(0x01, 0x03, 0x6F, 0x01);
	ght_cfg_filter(0x01, 0x04, 0x6F, 0x01);
	ght_cfg_filter(0x01, 0x05, 0x6F, 0x01);
	ght_cfg_filter(0x01, 0x06, 0x6F, 0x01);
	ght_cfg_filter(0x01, 0x07, 0x6F, 0x01);

	// Insepct JALR 
	// inst_index: 0x01
	// Func: 0x00
	// Opcode: 0x67
	// Data path: ALU + JALR
	ght_cfg_filter(0x01, 0x00, 0x67, 0x01);

	// Insepct Special RET 
  	// inst_index: 0x01
  	// Func: 0x00
  	// Opcode: 0x02
  	// Data path: ALU + JALR
  	ght_cfg_filter_rvc(0x01, 0x00, 0x02, 0x01);

	// Insepct Special JALR 
	// inst_index: 0x01
	// Func: 0x01
	// Opcode: 0x02
	// Data path: ALU + JALR
	ght_cfg_filter_rvc(0x01, 0x01, 0x02, 0x01);

	// se: 00, end_id: NUM_CORES-2, scheduling: rr, start_id: 0x01
	// The last checker is used for agg
	ght_cfg_se (0x00, NUM_CORES-2, 0x01, 0x01); 
	
	ght_cfg_mapper (0x03, 0b0001);

	printf("[Boom-%x]: Initialisation for ShadowStack (SS) is now completed, number of Checkers: %d!\r\n", BOOM_ID, NUM_CORES-1);
}

