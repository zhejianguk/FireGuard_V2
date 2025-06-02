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
#include <time.h>

#define  USE_HA 0

void gcStartup (void) __attribute__ ((constructor));
void gcCleanup (void) __attribute__ ((destructor));

pthread_t threads[NUM_CORES-1];

void* thread_pmc_gc(void* args){
    uint64_t hart_id = (uint64_t) args;
    //================== Initialisation ==================//
	// GC variables
	uint64_t base = 0;
	uint64_t end = 0x10000000000L;
	uint64_t perfc = 0;
	uint64_t addr;
	uint64_t addr1;
	uint64_t addr2;
	uint64_t addr3;
	uint64_t addr4;
	uint64_t addr5;
	uint64_t addr6;
	uint64_t addr7;
	uint64_t addr8;

	if (gc_pthread_setaffinity(hart_id) != 0){
		printf ("[Rocket-C%x-PMC]: pthread_setaffinity failed.", hart_id);
	}

	ghe_go();
	if (USE_HA == 1) {
		ROCC_INSTRUCTION_S (1, 0x01, 0x52);
	}
	ghe_initailised();

	//===================== Execution =====================//
	while (ghe_checkght_status() != 0x02){
if (USE_HA == 0) {
		while (ghe_get_bufferdepth() > 0){
			ROCC_INSTRUCTION (1, 0x0A);
			ROCC_INSTRUCTION_D (1, addr1, 0x0D);
			ROCC_INSTRUCTION (1, 0x0A);
			if (addr1 > base && addr1 < end){
				perfc = perfc + 1;
			}
		}
}
	}	
	//=================== Post execution ===================//
	if (hart_id == 0){
		printf("[Rocket-C%x-PMC]: Completed, PMC = %lx! \r\n", hart_id, perfc);
	}

	ghe_deinitailised();
	ghe_release();

	return NULL;
}

struct timespec start, end;
void gcStartup (void)
{
    //================== Initialisation ==================//
    // Bound current thread to BOOM
    if (gc_pthread_setaffinity(BOOM_ID) != 0) {
		printf ("[Boom-C%x]: pthread_setaffinity failed.", BOOM_ID);
	}

    // GC threads
    for (uint64_t i = 0; i < NUM_CORES - 1; i++) {
		pthread_create(&threads[i], NULL, thread_pmc_gc, (void *) (i+1));
	}

	while (ght_get_initialisation() == 0){
 	}
	
	// struct sched_param fifo_param;
  	// fifo_param.sched_priority=99;
 	// int s = pthread_setschedparam(pthread_self(), SCHED_FIFO, &fifo_param);
   	// if (s != 0) {
	// 	printf("Error: Scheduling policy! %x. \r\n", s);
   	// }

	printf("[Boom-%x]: Test is now started: \r\n", BOOM_ID);
	clock_gettime(CLOCK_MONOTONIC_RAW, &start); // get start time
	ght_set_satp_priv();
	ght_set_status_01 (); // ght: start
    //===================== Execution =====================//
}
  
void gcCleanup (void)
{	
	//=================== Post execution ===================//
	ght_set_status_02 ();
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    // GC threads.
	for (uint64_t i = 0; i < NUM_CORES-1; i++) {
		pthread_join(threads[i], NULL);
	}

	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("==== Execution time: %f seconds ==== \r\n", elapsed);

	
	if (GC_DEBUG == 1){
		printf("[Boom-%x]: Test is now completed: \r\n", BOOM_ID);
	}

	ght_unset_satp_priv();
	ght_set_status_00 ();
}

