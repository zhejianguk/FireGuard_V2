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
#include "libraries/deque.h"
#include "libraries/gc_top.h"

#define GC_Debug 0 

int uart_lock;
int if_tasks_initalised[NUM_CORES] = {0};
int global_counter;
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
	// Insepct JAL 
	// inst_index: 0x03 
	// Func: 0x00 - 0x07
	// Opcode: 0x6F
	// Data path: ALU + JALR
	ght_cfg_filter(0x03, 0x00, 0x6F, 0x01);
	ght_cfg_filter(0x03, 0x01, 0x6F, 0x01);
	ght_cfg_filter(0x03, 0x02, 0x6F, 0x01);
	ght_cfg_filter(0x03, 0x03, 0x6F, 0x01);
	ght_cfg_filter(0x03, 0x04, 0x6F, 0x01);
	ght_cfg_filter(0x03, 0x05, 0x6F, 0x01);
	ght_cfg_filter(0x03, 0x06, 0x6F, 0x01);
	ght_cfg_filter(0x03, 0x07, 0x6F, 0x01);

	// Insepct JALR 
	// inst_index: 0x03 
	// Func: 0x00
	// Opcode: 0x67
	// Data path: ALU + JALR
	ght_cfg_filter(0x03, 0x00, 0x67, 0x01);

	// Insepct Special RET 
	// inst_index: 0x03
	// Func: 0x00
	// Opcode: 0x02
	// Data path: ALU + JALR
	ght_cfg_filter(0x03, 0x00, 0x02, 0x01);

	// Insepct Special JALR 
	// inst_index: 0x03
	// Func: 0x01
	// Opcode: 0x02
	// Data path: ALU + JALR
	ght_cfg_filter(0x03, 0x01, 0x02, 0x01);

	ght_cfg_se (0x00, 0x00, 0x01, 0x00);
	ght_cfg_se (0x01, 0x00, 0x01, 0x00);
	ght_cfg_se (0x02, 0x00, 0x01, 0x00);
	// se: 0x03, end_id: 0x01, scheduling: rr, start_id: 0x01
	ght_cfg_se (0x03, 0x01, 0x01, 0x01);


	
	ght_cfg_mapper (0x01, 0b0000);
	ght_cfg_mapper (0x02, 0b0000);
	// inst_index: 0x03 se: 0x03
	ght_cfg_mapper (0x03, 0b1000);

	lock_acquire(&uart_lock);
	printf("[Boom-%x]: Test is now started: \r\n", hart_id);
	lock_release(&uart_lock);

	ght_set_status_01 (); // start monitoring
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

	lock_acquire(&uart_lock);
	printf("[Boom-%x]: All tests are done! Status: %llx; Sum: %llx. \r\n", hart_id, status, sum);
	lock_release(&uart_lock);
	ght_unset_satp_priv ();
	ght_set_status_00 ();

	return NULL;
}

void* thread_shadowstack_s(void* args){
	uint64_t hart_id = (uint64_t) args;
	uint64_t proc_id = remapping_hart_id(hart_id);

	// GC variables
	uint64_t Header = 0x0;  
	uint64_t Opcode = 0x0;
	uint64_t Func = 0x0;
	uint64_t Rd = 0x0;
	uint64_t RS1 = 0x0;
	uint64_t Payload = 0x0;
	uint64_t PC = 0x0;
	uint64_t Inst = 0x0;
	uint64_t PayloadPush = 0x0;
	uint64_t PayloadPull = 0x0;

	dequeue  shadow_header;
 	dequeue  shadow_payload;
  	initialize(&shadow_header);
  	initialize(&shadow_payload);

	//================== Initialisation ==================//
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
			ROCC_INSTRUCTION_D (1, Payload, 0x0D);

			Opcode = Header & 0x7F;
			Func = (Header & 0x7000) >> 12;
			Rd = (Header & 0xF80) >> 7;
			RS1 = (Header & 0xF8000) >> 15;
			PC = Header >> 32;
			Inst = Header & 0xFFFFFFFF;
		
			// Push -- a function is called
			if ((((Opcode == 0x6F) || (Opcode == 0x67)) && (Rd == 0x01)) || // + comprised inst
				((Opcode == 0x02) && (Func == 0x01) && (Rd != 0x00) && ((RS1 & 0x01) == 0X01))) {
				PayloadPush = Payload;
				if (full(&shadow_payload) == 0) {
					enqueueF(&shadow_header, Header);
					enqueueF(&shadow_payload, PayloadPush);
					if (GC_Debug) {
						lock_acquire(&uart_lock);
						printf("[C%x SS]: <<Pushed>> Expected: %x.                        PC: %x. Inst: %x. \r\n", hart_id, PayloadPush, PC, Inst);
						lock_release(&uart_lock);
					}
				}
			}

			// Pull -- a function is returned
			if (((Opcode == 0x67) && (Rd == 0x00) && (RS1 == 0x01)) || // + comprised inst
				((Opcode == 0X02) && (Func == 0x00 ) && (Rd == 0x01) && ((RS1 & 0x01) == 0X01))) {
				PayloadPull = Payload;
				if (empty(&shadow_payload) == 1) {
					lock_acquire(&uart_lock);
					printf("[C%x SS]: ==Empty== Uninteded: %x.                        PC: %x. Inst: %x. \r\n", hart_id, PayloadPull, PC, Inst);
					lock_release(&uart_lock);
				} else {
					u_int64_t comp = dequeueF(&shadow_payload);
					dequeueF(&shadow_header);
				
					if (comp != PayloadPull){
						lock_acquire(&uart_lock);
						printf("[C%x SS]: **Error**  Expected: %x. v.s. Pulled: %x. PC: %x. Inst: %x. \r\n", hart_id, comp, PayloadPull, PC, Inst);
						lock_release(&uart_lock);
						// return -1;
					} else {
						if (GC_Debug) {
						lock_acquire(&uart_lock);
						printf("[C%x SS]: --Paried-- Expected: %x. v.s. Pulled: %x. PC: %x. Inst: %x. \r\n", hart_id, comp, PayloadPull, PC, Inst);
						lock_release(&uart_lock);
						}
					}
				}
			}
		}
	}

	//=================== Post execution ===================//
	lock_acquire(&uart_lock);
    printf("[C%x SS]: Completed. No error is detected\r\n", hart_id);
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
		pthread_create(&threads[i], NULL, thread_shadowstack_s, (void *) i);
	}
	
	for (uint64_t i = 0; i < NUM_CORES; i++) {
		pthread_join(threads[i], NULL);
	}
	
	return 0;
}