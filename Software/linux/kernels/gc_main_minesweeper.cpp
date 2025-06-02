#ifndef _GNU_SOURCE
	#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "libraries/ght.h"
#include "libraries/gc_top.h"
#include "libraries/ghe.h"
#include <time.h>
// This is a silly include, due to the deadline, we will fix them later
#include "/home/centos/asplos22-minesweeper-reproduce/minesweeper-public/central_quarantine.h"

struct timespec start, end;
uint64_t csr_cycle[2];
uint64_t csr_instret[2];


extern __attribute__((weak)) CentralQuarantine central_quarantine;

void gcStartup (void) __attribute__ ((constructor));
void gcCleanup (void) __attribute__ ((destructor));

void report_error(uint64_t offset){
	printf("**Error** accesses!\r\n");
}


pthread_t threads[NUM_CORES-1];
void* thread_minesweeper_gc(void* args){
    uint64_t hart_id = (uint64_t) args;
	// GC variables
	char bits = 0;
	char bits1 = 0;
	char bits2 = 0;

	uint64_t Address = 0;
	uint64_t Header = 0;
	uint64_t Address1 = 0;
	uint64_t Header1 = 0;
	uint64_t Address2 = 0;
	uint64_t Header2 = 0;
	char* shadow = (char*) central_quarantine.quarantineShadow.marks; 
	
	//================== Initialisation ==================//
	if (gc_pthread_setaffinity(hart_id) != 0){
		printf ("[Rocket-C%x-MW]: pthread_setaffinity failed.", hart_id);
	}
	
	printf ("[Rocket-C%x-MW]: Initialised. \r\n", hart_id);

	
	asm volatile("fence rw, rw;");
	ghe_go();
	ghe_initailised();
	//===================== Execution =====================//
	
	while (ghe_checkght_status() != 0x02){
		// Run analysis
		uint32_t buffer_depth = ghe_get_bufferdepth();
		while (buffer_depth > 9){
			ROCC_INSTRUCTION_D (1, Address1, 0x0D);
			ROCC_INSTRUCTION_D (1, Address2, 0x0D);
			bits1 = shadow[(Address1)>>7];
			if (bits1 != 0){
				if(bits1 & (1<<((Address1 >> 7)&8))) {
					report_error(1);
				}
			}
			bits2 = shadow[(Address2)>>7];
			if (bits2 != 0){
				if(bits2 & (1<<((Address2 >> 7)&8))) {
					report_error(0);
				}
			}

			ROCC_INSTRUCTION_D (1, Address1, 0x0D);
			ROCC_INSTRUCTION_D (1, Address2, 0x0D);
			bits1 = shadow[(Address1)>>7];
			if (bits1 != 0){
				if(bits1 & (1<<((Address1 >> 7)&8))) {
					report_error(1);
				}
			}
			bits2 = shadow[(Address2)>>7];
			if (bits2 != 0){
				if(bits2 & (1<<((Address2 >> 7)&8))) {
					report_error(0);
				}
			}

			ROCC_INSTRUCTION_D (1, Address1, 0x0D);
			ROCC_INSTRUCTION_D (1, Address2, 0x0D);
			bits1 = shadow[(Address1)>>7];
			if (bits1 != 0){
				if(bits1 & (1<<((Address1 >> 7)&8))) {
					report_error(1);
				}
			}
			bits2 = shadow[(Address2)>>7];
			if (bits2 != 0){
				if(bits2 & (1<<((Address2 >> 7)&8))) {
					report_error(0);
				}
			}
			
			ROCC_INSTRUCTION_D (1, Address1, 0x0D);
			ROCC_INSTRUCTION_D (1, Address2, 0x0D);
			bits1 = shadow[(Address1)>>7];
			if (bits1 != 0){
				if(bits1 & (1<<((Address1 >> 7)&8))) {
					report_error(1);
				}
			}
			bits2 = shadow[(Address2)>>7];
			if (bits2 != 0){
				if(bits2 & (1<<((Address2 >> 7)&8))) {
					report_error(0);
				}
			}
			
			ROCC_INSTRUCTION_D (1, Address1, 0x0D);
			ROCC_INSTRUCTION_D (1, Address2, 0x0D);
			bits1 = shadow[(Address1)>>7];
			if (bits1 != 0){
				if(bits1 & (1<<((Address1 >> 7)&8))) {
					report_error(1);
				}
			}
			bits2 = shadow[(Address2)>>7];
			if (bits2 != 0){
				if(bits2 & (1<<((Address2 >> 7)&8))) {
					report_error(0);
				}
			}
			buffer_depth = ghe_get_bufferdepth();
		}

		if (buffer_depth > 0){
			switch (buffer_depth){
				case 9:
					ROCC_INSTRUCTION_D (1, Address, 0x0D); 
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}

				case 8:
					ROCC_INSTRUCTION_D (1, Address, 0x0D); 
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}

				case 7:
					ROCC_INSTRUCTION_D (1, Address, 0x0D); 
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}

				case 6: 
					ROCC_INSTRUCTION_D (1, Address, 0x0D); 
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}

				case 5: 
					ROCC_INSTRUCTION_D (1, Address, 0x0D); 
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}

				case 4: 
					ROCC_INSTRUCTION_D (1, Address, 0x0D);
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}
				case 3: 
					ROCC_INSTRUCTION_D (1, Address, 0x0D); 
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}

				case 2: 
					ROCC_INSTRUCTION_D (1, Address, 0x0D); 
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}

				case 1: 
					ROCC_INSTRUCTION_D (1, Address, 0x0D); 
					bits = shadow[(Address)>>7];
					if (bits != 0){
						if(bits & (1<<((Address >> 7)&8))) {
							report_error(0);
						}
					}
					break;

				default:
					printf("Error: duff's device");
					exit(0);
			}
		}

		// Dedicated for shadowstack
		if (ghe_checkght_status() == 0x04) {
			ghe_complete();
			while(ghe_checkght_status() == 0x04) {
				ghe_complete();
			}
			ghe_go();
		}
	}

	//=================== Post execution ===================//
	ghe_deinitailised();
	ghe_release();

	return NULL; 
}


void gcStartup (void)
{
    // Bound current thread to BOOM
	// It is not required, as we bound it in the minesweeper

	// GC threads
    for (uint64_t i = 0; i < NUM_CORES - 1; i++) {
		pthread_create(&threads[i], NULL, thread_minesweeper_gc, (void *) (i+1));
	}


	while (ght_get_initialisation() == 0){
 	}

	printf("[Boom-C-%x]: Idle for checkers being intialised... \r\n", BOOM_ID);
	printf("[Boom-C-%x]: Test is now started: \r\n", BOOM_ID);

	ROCC_INSTRUCTION_S (1, 0x02, 0x01); // Enabling FI
  	ROCC_INSTRUCTION (1, 0x67); // Reset FI

	clock_gettime(CLOCK_MONOTONIC_RAW, &start); // get start time
	ght_set_satp_priv();
	ght_set_status_01 (); // ght: start
}
  
void gcCleanup (void)
{	
	//=================== Post execution ===================//
	ght_set_status_02 ();
	clock_gettime(CLOCK_MONOTONIC_RAW, &end); // get end time

	// GC threads.
	for (uint64_t i = 0; i < NUM_CORES-1; i++) {
		pthread_join(threads[i], NULL);
	}

	printf("\r\n ======== Execution Report ========\r\n");
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("[Execution time] \r\n");
	printf("Seconds: %f \r\n \r\n", elapsed);

	printf("[Detection latency (unit: cycles)] \r\n");

	for (int j = 0; j < 0x40; j++) {
     uint64_t latency = ght_readFIU(j);
     printf("%d \r\n", latency);
    }

	printf("\r\n ======== End of Report ========\r\n");

	printf ("[Boom-C%x]: Completed!\r\n", BOOM_ID);

	ght_unset_satp_priv();
	ght_set_status_00 ();
}