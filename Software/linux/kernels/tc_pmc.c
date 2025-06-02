#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#include "libraries/spin_lock.h"
#include "libraries/ght.h"
#include "libraries/ghe.h"
#include "libraries/gc_top.h"

int uart_lock;
int if_tasks_initalised[NUM_CORES] = {0};
int agg_core_id = 0x07;

void* thread_boom(void* args){
	uint64_t hart_id = (uint64_t) args;
	uint64_t proc_id = remapping_hart_id(hart_id);

	if (gc_pthread_setaffinity(proc_id) != 0){
		lock_acquire(&uart_lock);
		printf ("[Boom-C%x]: pthread_setaffinity failed.", hart_id);
		lock_release(&uart_lock);
	} else{
		ght_set_satp_priv ();
		if_tasks_initalised[proc_id] = 1;
	}

	//================== Initialisation ==================//
	while (and_gate(if_tasks_initalised, NUM_CORES) == 0){
	}
	
	ghm_cfg_agg(agg_core_id);

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

	// se: 00, end_id: 0x01, scheduling: rr, start_id: 0x01
  	ght_cfg_se (0x00, 0x01, 0x01, 0x01);
	// se: 01, end_id: 0x03, scheduling: rr, start_id: 0x02
	ght_cfg_se (0x01, 0x03, 0x01, 0x02);
	// se: 02, end_id: 0x06, scheduling: rr, start_id: 0x04
	ght_cfg_se (0x02, 0x06, 0x01, 0x04);
	// se: 03, end_id: 0x00, scheduling: rr, start_id: 00
	ght_cfg_se (0x03, 0x00, 0x01, 0x00);
	
	ght_cfg_mapper (0x01, 0b0111);
	ght_cfg_mapper (0x02, 0b0000);
	ght_cfg_mapper (0x03, 0b0000);
	
	lock_acquire(&uart_lock);
	printf("[Boom-%x]: Test is now started: \r\n", hart_id);
	lock_release(&uart_lock);
	ght_set_status_01 (); // ght: start
	//===================== Execution =====================//
	int sum_temp = 0;
	int sum = 0;
	for (int i = 0; i < 170000; i++ ){
    	sum_temp = task_synthetic_malloc(i);
    	sum = sum + sum_temp;
	}

	//=================== Post execution ===================//
	uint64_t status;
	ght_set_status_02 ();

	while (((status = ght_get_status()) < 0x1FFFF) || (ght_get_buffer_status() != GHT_EMPTY)){
	}


	lock_acquire(&uart_lock);
	printf("[Boom-%x]: All tests are done! Status: %x, Sum: %llx \r\n", hart_id, status, sum);
	lock_release(&uart_lock);

	ght_unset_satp_priv();
	ght_set_status_00 ();
	
	return NULL;
}

void* thread_pmc(void* args){
	uint64_t hart_id = (uint64_t) args;
	uint64_t proc_id = remapping_hart_id(hart_id);

	// GC variables
	uint64_t perfc = 0;
	uint64_t Payload = 0x0;
	uint64_t ecounter;

	if (gc_pthread_setaffinity(proc_id) != 0){
		lock_acquire(&uart_lock);
		printf ("[Boom-C%x]: pthread_setaffinity failed.", hart_id);
		lock_release(&uart_lock);
	} else{
		ghe_go();
		if_tasks_initalised[proc_id] = 1;
	}

	//================== Initialisation ==================//
	while (and_gate(if_tasks_initalised, NUM_CORES) == 0){
	}

	//===================== Execution =====================// 
	while (ghe_checkght_status() != 0x02) {
		while (ghe_status() != GHE_EMPTY){
			ROCC_INSTRUCTION_D (1, Payload, 0x0D);
			perfc = perfc + 1;
		}
	}
	//=================== Post execution ===================//
	lock_acquire(&uart_lock);
	printf("[Rocket-C%x-PMC]: Completed, PMC = %x! \r\n", hart_id, perfc);
	lock_release(&uart_lock);
	ghe_release();
	
	// This should not be required -- but we add it for unhandled inst
	while (ghe_status() != GHE_EMPTY){
		ROCC_INSTRUCTION (1, 0x0D);
	}

	if (hart_id == agg_core_id) {
		for (int i = 0; i< NUM_CORES - 1; i++){
			ROCC_INSTRUCTION_S(1, 0x01 << (i-1), 0x21);
		}
	}

	return NULL;
}

int main(){
	pthread_t threads[NUM_CORES];

	pthread_create(&threads[0], NULL, thread_boom, (void *) 0);	

	for (uint64_t i = 1; i < NUM_CORES; i++) {
		pthread_create(&threads[i], NULL, thread_pmc, (void *) i);
	}
	
	for (uint64_t i = 0; i < NUM_CORES; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}