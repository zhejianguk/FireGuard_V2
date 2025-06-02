#ifndef _GNU_SOURCE
	#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif

#include <stack>
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

#define  USE_HA 1

void gcStartup (void) __attribute__ ((constructor));
void gcCleanup (void) __attribute__ ((destructor));

pthread_t threads[NUM_CORES-1];

void* thread_shadowstack_gc(void* args){
	uint64_t hart_id = (uint64_t) args;
	std::deque<uint64_t> shadow;

	// GC variables
	uint64_t Payload;
	//================== Initialisation ==================//
	if (gc_pthread_setaffinity(hart_id) != 0){
		printf ("Pthread_setaffinity failed.");
	}

	ghe_go();
	if (USE_HA == 1) {
		ROCC_INSTRUCTION_S (1, 0x01, 0x52);
	}
	ghe_initailised();
	//===================== Execution =====================//
	while (ghe_checkght_status() != 0x02){
if (USE_HA == 0) {
		while (ghe_status() != GHE_EMPTY){
			ROCC_INSTRUCTION_D (1, Payload, 0x0D);
			uint64_t type = Payload & 0x03;

			if (type == 1) {
				shadow.push_front(Payload);
			} else if (type == 2) {
				if (!shadow.empty()) {
					uint64_t comp = shadow.front() + 1;
					shadow.pop_front();
					if (comp != Payload){
						printf("Exp:%x v.s. Pul:%x! \r\n", comp, Payload);
					}
				}
			}
		}
}
		// Dedicated for shadowstack
		/*
		if (ghe_checkght_status() == 0x04) {
			ghe_complete();
			while(ghe_checkght_status() == 0x04) {
				ghe_complete();
			}
			ghe_go();
		}
		*/
	}
	//=================== Post execution ===================//
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
		printf ("[BOOM-C%x]: pthread_setaffinity failed.", BOOM_ID);
	}
	
	for (uint64_t i = 0; i < NUM_CORES - 1; i++) {
		pthread_create(&threads[i], NULL, thread_shadowstack_gc, (void *) (i+1));
	}

	while (ght_get_initialisation() == 0){
 	}
	
	printf("[Boom-C-%x]: Test is now started: \r\n", BOOM_ID);
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
	
	for (uint64_t i = 0; i < NUM_CORES-1; i++) {
		pthread_join(threads[i], NULL);
	}

	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("==== Execution time: %f seconds ==== \r\n", elapsed);

	if (GC_DEBUG == 1){
		printf("[BOOM-C-%x]: Test is now completed! \r\n", BOOM_ID);
	}

	ght_unset_satp_priv();
	ght_set_status_00 ();
}