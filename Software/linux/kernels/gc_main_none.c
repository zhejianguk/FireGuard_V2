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

struct timespec start, end;
uint64_t csr_cycle[2];
uint64_t csr_instret[2];


/* Apply the constructor attribute to myStartupFun() so that it
     is executed before main() */
void gcStartup (void) __attribute__ ((constructor));


 /* Apply the destructor attribute to myCleanupFun() so that it
    is executed after main() */
void gcCleanup (void) __attribute__ ((destructor));
 
void gcStartup (void)
{
    // Bound current thread to BOOM
    if (gc_pthread_setaffinity(BOOM_ID) != 0){
		printf ("[Boom-C%x]: Pthread_setaffinity failed.", BOOM_ID);
	} else{
		printf ("[Boom-C%x]: Initialised!\r\n", BOOM_ID);
	}

	csr_cycle[0] = read_csr_safe(cycle);
	csr_instret[0]  = read_csr_safe(instret);
	printf("Cycles: %ld \r\n", csr_cycle[0]);
	printf("Insts: %ld \r\n", csr_instret[0]);

	clock_gettime(CLOCK_MONOTONIC_RAW, &start); // get start time
}
  
void gcCleanup (void)
{	
	csr_cycle[1] = read_csr_safe(cycle);
	csr_instret[1]  = read_csr_safe(instret);
	clock_gettime(CLOCK_MONOTONIC_RAW, &end); // get end time
	double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; // calculate elapsed time in seconds
	

	printf("==== Execution time: %f seconds ==== \r\n", elapsed);
	printf("Cycles: %ld \r\n", csr_cycle[1]);
	printf("Insts: %ld \r\n", csr_instret[1]);

	printf("==== Insts v.s. Cycles ==== \r\n");
	int64_t cycle = csr_cycle[1] - csr_cycle[0];
	int64_t instret = csr_instret[1] - csr_instret[0];
	printf("Cycles: %ld \r\n", cycle);
	printf("Insts: %ld \r\n", instret);




	printf ("[Boom-C%x]: Completed!\r\n", BOOM_ID);

}

