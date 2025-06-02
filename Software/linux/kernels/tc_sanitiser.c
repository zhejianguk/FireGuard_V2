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
#include "malloc.h"
#include "sys/mman.h"
#include "libraries/gc_top.h"

char* shadow;
int uart_lock;
int if_tasks_initalised[NUM_CORES] = {0};
int global_counter;

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

	ghm_cfg_agg(0x07);
	// Insepct load operations 
	// index: 0x01 
	// Func: 0x00; 0x01; 0x02; 0x03; 0x04; 0x05
	// Opcode: 0x03
	// Data path: MemCaluc
	ght_cfg_filter(0x02, 0x00, 0x03, 0x02); // lb
	ght_cfg_filter(0x02, 0x01, 0x03, 0x02); // lh
	ght_cfg_filter(0x02, 0x02, 0x03, 0x02); // lw 
	ght_cfg_filter(0x02, 0x03, 0x03, 0x02); // ld
	ght_cfg_filter(0x02, 0x04, 0x03, 0x02); // lbu
	ght_cfg_filter(0x02, 0x05, 0x03, 0x02); // lhu

	// Insepct store operations 
	// index: 0x02 
	// Func: 0x00; 0x01; 0x02; 0x03
	// Opcode: 0x23
	// Data path: MemCaluc
	ght_cfg_filter(0x02, 0x00, 0x23, 0x03); // sb
	ght_cfg_filter(0x02, 0x01, 0x23, 0x03); // sh
	ght_cfg_filter(0x02, 0x02, 0x23, 0x03); // sw
	ght_cfg_filter(0x02, 0x03, 0x23, 0x03); // sd


	ght_cfg_se (0x00, 0x00, 0x01, 0x00);
	ght_cfg_se (0x01, 0x06, 0x01, 0x01);
	ght_cfg_se (0x02, 0x00, 0x01, 0x00);
	ght_cfg_se (0x03, 0x00, 0x01, 0x00);
	
	ght_cfg_mapper (0x01, 0b0000);
	ght_cfg_mapper (0x02, 0b0010);
	ght_cfg_mapper (0x03, 0b0000);

	lock_acquire(&uart_lock);
	printf("[Boom-%x]: Test is now started: \r\n", hart_id);
	lock_release(&uart_lock);

	ght_set_status_01 (); // ght: start
	//===================== Execution =====================//
	uint64_t sum_temp = 0;
	uint64_t sum = 0;
	for (uint64_t i = 0; i < 170000; i++ ){
    	sum_temp = task_synthetic_malloc(i);
    	sum = sum + sum_temp;
		global_counter = i;
	}

	//=================== Post execution ===================//
	uint64_t status;
	ght_set_status_02 ();

	while (((status = ght_get_status()) < 0x1FFFF) || (ght_get_buffer_status() != GHT_EMPTY)){
	}

	if_tasks_initalised[proc_id] = 0;
	ght_unset_satp_priv ();
	ght_set_status_00 ();
	
	lock_acquire(&uart_lock);
	printf("[Boom-%x]: All tests are done! Status: %llx; Sum: %llx. \r\n", hart_id, status, sum);
	lock_release(&uart_lock);
	
	while (or_gate(if_tasks_initalised, NUM_CORES) == 1){
	}

	return NULL;
}

void* thread_sanitiser(void* args){
	uint64_t hart_id = (uint64_t) args;
	uint64_t proc_id = remapping_hart_id(hart_id);

	// GC variables
	uint64_t Address = 0x0;
  	uint64_t Err_Cnt = 0x0;
	uint64_t Header = 0x0;
	uint64_t PC = 0x0;
	uint64_t Inst = 0x0;

	if (gc_pthread_setaffinity(proc_id) != 0){
		lock_acquire(&uart_lock);
		printf ("[Rocket-C%x]: pthread_setaffinity failed.", hart_id);
		lock_release(&uart_lock);
	} else{
		ghe_go();
		if_tasks_initalised[proc_id] = 1;
	}


	//================== Initialisation ==================//
	while (and_gate(if_tasks_initalised, NUM_CORES) == 0){
	}


	//===================== Execution =====================//
	while (ghe_checkght_status() != 0x02){
		while (ghe_status() != GHE_EMPTY){
		
		ROCC_INSTRUCTION_D (1, Header, 0x0A);    
		ROCC_INSTRUCTION_D (1, Address, 0x0D);
		asm volatile("fence rw, rw;");

		PC = Header >> 32;
		Inst = Header & 0xFFFFFFFF;

		char bits = shadow[(Address)>>7];
		
		if(!bits) continue;

		if(bits & (1<<((Address&0x70)>>4))) {
			lock_acquire(&uart_lock);
			printf("[Rocket-C%x-Sani]: **Error** illegal accesses at %llx, PC: %x, Inst: %x. Global counter: %d. \r\n", hart_id, Address, PC, Inst, global_counter);
			lock_release(&uart_lock);
			Err_Cnt ++;
			// return -1;
			}
		}

		// Dedicated for shadowstack 
		if (ghe_checkght_status() == 0x04) {
			ghe_complete();
			while((ghe_checkght_status() == 0x04)) {
				// Wait big core to re-start
			}
			ghe_go();
		}
	}
	//=================== Post execution ===================//
	ghe_release();
	if_tasks_initalised[proc_id] = 0;
	lock_acquire(&uart_lock);
	printf("Rocket-C%x-Sani]: Completed, %d illegal accesses are detected. \r\n", hart_id, Err_Cnt);
	lock_release(&uart_lock);

	while (or_gate(if_tasks_initalised, NUM_CORES) == 1){
	}


	return NULL;
}

int main(){
	pthread_t threads[NUM_CORES];

	uint64_t map_size = (long long) 4*1024*1024*1024*sizeof(char);

	// shadow memory
	shadow = mmap(NULL,
				map_size,
				PROT_WRITE | PROT_READ,
				MAP_PRIVATE | MAP_ANON | MAP_NORESERVE,
				-1, 0);

	if(shadow == NULL) {
		lock_acquire(&uart_lock);
		printf("[Boom]: Error! memory not allocated.");
		lock_release(&uart_lock);
		exit(0);
	} 
	asm volatile("fence rw, rw;");
	
	pthread_create(&threads[0], NULL, thread_boom, (void *) 0);	

	for (uint64_t i = 1; i < NUM_CORES; i++) {
		pthread_create(&threads[i], NULL, thread_sanitiser, (void *) i);
	}
	

	for (uint64_t i = 0; i < NUM_CORES; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}