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
	
	// Insepct load operations 
	// index: 0x01 
	// Func: 0x00; 0x01; 0x02; 0x03; 0x04; 0x05
	// Opcode: 0x03
	// Data path: N/U
	ght_cfg_filter(0x01, 0x00, 0x03, 0x02); // lb
	ght_cfg_filter(0x01, 0x01, 0x03, 0x02); // lh
	ght_cfg_filter(0x01, 0x02, 0x03, 0x02); // lw
	ght_cfg_filter(0x01, 0x03, 0x03, 0x02); // ld
	ght_cfg_filter(0x01, 0x04, 0x03, 0x02); // lbu
	ght_cfg_filter(0x01, 0x05, 0x03, 0x02); // lhu

	// se: 00, end_id: NUM_CORES-1, scheduling: rr, start_id: 0x01
  	ght_cfg_se (0x00, NUM_CORES-1, 0x01, 0x01);
	ght_cfg_mapper (0x01, 0b0001);

	/* Simulating a N-width filter*/
	ght_debug_filter_width (FILTERWIDTH);

	printf("[Boom-%x]: Initialisation for PMC is now completed, number of Checkers: %d!\r\n", BOOM_ID, NUM_CORES-1);
	printf("[Boom-%x]: Simulating %d-width event filter!\r\n", BOOM_ID, FILTERWIDTH);
}

