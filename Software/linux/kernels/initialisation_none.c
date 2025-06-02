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

uint64_t csr_cycle[2];
uint64_t csr_instret[2];


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

	csr_cycle[0] = read_csr_safe(cycle);
	csr_instret[0]  = read_csr_safe(instret);
	printf("Cycles: %ld \r\n", csr_cycle[0]);
	printf("Insts: %ld \r\n", csr_instret[0]);

	csr_cycle[1] = read_csr_safe(cycle);
	csr_instret[1]  = read_csr_safe(instret);
	printf("Cycles: %ld \r\n", csr_cycle[1]);
	printf("Insts: %ld \r\n", csr_instret[1]);


	printf("==== Insts v.s. Cycles ==== \r\n");
	int64_t cycle = csr_cycle[1] - csr_cycle[0];
	int64_t instret = csr_instret[1] - csr_instret[0];
	printf("Cycles: %ld \r\n", cycle);
	printf("Insts: %ld \r\n", instret);



	printf("[Boom-%x]: Initialisation for PMC is now completed, number of Checkers: %d!\r\n", BOOM_ID, NUM_CORES-1);
	printf("[Boom-%x]: Simulating %d-width event filter!\r\n", BOOM_ID, FILTERWIDTH);
}

