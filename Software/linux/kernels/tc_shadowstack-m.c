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

dequeue  queues_header[NUM_CORES];
dequeue  queues_payload[NUM_CORES];

dequeue  shadow_agg_header;
dequeue  shadow_agg_payload;

uint64_t core_s = 0x01;
uint64_t core_e = 0x06;

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
	// se: 0x03, end_id: 0x06, scheduling: fp, start_id: 0x01
	ght_cfg_se (0x03, core_e, 0x03, core_s);
	ght_cfg_se (0x03, 0x06, 0x0f, 0x01); // Reset SE 0x03

	
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
		asm volatile("fence rw, rw;");
	}

	//=================== Post execution ===================//
	uint64_t status;
	ght_set_status_02 ();

	while (((status = ght_get_status()) < 0x1FFFF) || (ght_get_buffer_status() != GHT_EMPTY)){
	}

	lock_acquire(&uart_lock);
	printf("[Boom-%x]: All tests are done! Status: %llx; Sum: %llx. \r\n", hart_id, status, sum);
	lock_release(&uart_lock);

	ght_unset_satp_priv();
	ght_set_status_00 ();
	if_tasks_initalised[proc_id] = 0;

	return NULL;
}

void* thread_shadowstack_m_pre(void* args){
	uint64_t hart_id = (uint64_t) args;
	uint64_t proc_id = remapping_hart_id(hart_id);

	// GC variables
	uint64_t Header = 0x0;
	uint64_t Pc = 0x0;  
	uint64_t Opcode = 0x0;
	uint64_t Func = 0x0;
	uint64_t Inst = 0x0;
	uint64_t Rd = 0x0;
	uint64_t RS1 = 0x0;
	uint64_t Payload = 0x0;
	uint64_t PayloadPush = 0x0;
	uint64_t PayloadPull = 0x0;
	uint64_t Header_index = (hart_id << 32);
	uint64_t cnt = 0x00;
	uint64_t S_Header = 0x00;
	uint64_t S_Payload = 0x00;

	dequeue  shadow_header;
  	dequeue  shadow_payload;
  	initialize(&shadow_header);
  	initialize(&shadow_payload);

	asm volatile("fence rw, rw;");
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
		while (ghe_status() != GHE_EMPTY) {
      		ROCC_INSTRUCTION_D (1, Header, 0x0A);
      		ROCC_INSTRUCTION_D (1, Payload, 0x0D);

      		Inst = Header & 0xFFFFFFFF;
      		Pc = Header >> 32;
      		Opcode = Header & 0x7F;
      		Func = (Header & 0x7000) >> 12;
      		Rd = (Header & 0xF80) >> 7;
      		RS1 = (Header & 0xF8000) >> 15;
      
      		// Push -- a function is called
      		if ((((Opcode == 0x6F) || (Opcode == 0x67)) && (Rd == 0x01)) || // + comprised inst
          		((Opcode == 0x02) && (Func == 0x01) && (Rd != 0x00) && ((RS1 & 0x01) == 0X01))) {
        		PayloadPush = Payload;
        		if (full(&shadow_payload) == 0) {
          			enqueueF(&shadow_header, Inst);
          			enqueueF(&shadow_payload, PayloadPush);
        		} else {
          			lock_acquire(&uart_lock);
          			printf("[C%x SS]: **Error** shadow stack is full. \r\n", hart_id);
          			lock_release(&uart_lock);
        		}
     		}

      		// Pull -- a function is returned
      		if (((Opcode == 0x67) && (Rd == 0x00) && (RS1 == 0x01)) || // + comprised inst
          		((Opcode == 0X02) && (Func == 0x00 ) && (Rd == 0x01) && ((RS1 & 0x01) == 0X01))) {
        		PayloadPull = Payload;
        		if (empty(&shadow_payload) == 1) {
          			// Send it to AGG
          			while (ghe_agg_status() == GHE_FULL) {
          			}
          			S_Header = Inst | Header_index;
          			S_Payload = Payload;
          			ghe_agg_push (S_Header, S_Payload);
				} else {
					u_int64_t comp = dequeueF(&shadow_payload);
					dequeueF(&shadow_header);
					if (comp != PayloadPull){
						lock_acquire(&uart_lock);
						printf("[C%x SS]: **Error**  Expected: %x. v.s. Pulled: %x. PC: %x. Inst: %x. -- G-Counter = %x. \r\n", hart_id, comp, PayloadPull, Pc, Inst, global_counter);
						lock_release(&uart_lock);
					} else {
						// Paried
					}
				}
			}
		}

		// If all push and pull are handled
   		if ((ghe_sch_status() == 0x01) && (ghe_status() == GHE_EMPTY)) {
			// Send unpaired pushes 
			while ((empty(&shadow_payload) == 0)) {
				S_Header = dequeueR(&shadow_header);
				S_Header = S_Header | Header_index;
				S_Payload = dequeueR(&shadow_payload);
				while (ghe_agg_status() == GHE_FULL) {
				}
				ghe_agg_push (S_Header, S_Payload);
			}
		
			// Send termination flag
			while (ghe_agg_status() == GHE_FULL) {
			}
      		ghe_agg_push ((0xFFFFFFFF|Header_index), 0x0);

			while ((ghe_sch_status() == 0x01) && (ghe_status() == GHE_EMPTY)) {
				// Wait the core to be waked up
			}
      	}
	}

	//=================== Post execution ===================//
	lock_acquire(&uart_lock);
    printf("[C%x SS]: Completed. No error is detected\r\n", hart_id);
   	lock_release(&uart_lock);
 	
	ghe_release();
	if_tasks_initalised[proc_id] = 0;

	// This should not be required -- but we add it for unhandled inst
	while (ghe_status() != GHE_EMPTY){
		ROCC_INSTRUCTION (1, 0x0D);
	}

	return NULL;
}

void clear_queue(int index)
{
  	while (empty(&queues_header[index]) == 0) {
		uint64_t Header_q = dequeueR(&queues_header[index]);
		uint64_t Payload_q = dequeueR(&queues_payload[index]);
		Header_q = Header_q & 0xFFFFFFFF;
		uint64_t inst = Header_q & 0xFFFFFFFF;
		uint64_t Opcode_q = Header_q & 0x7F;
		uint64_t Func_q = (Header_q & 0x7000) >> 12;
		uint64_t Rd_q = (Header_q & 0xF80) >> 7;
		uint64_t RS1_q = (Header_q & 0xF8000) >> 15;
		uint64_t PayloadPush_q = 0x0;
		uint64_t PayloadPull_q = 0x0;
    
             
		if ((((Opcode_q == 0x6F) || (Opcode_q == 0x67)) && (Rd_q == 0x01)) || // + comprised inst
				((Opcode_q == 0x02) && (Func_q == 0x01) && (Rd_q != 0x00) && ((RS1_q & 0x01) == 0X01))) {
			PayloadPush_q = Payload_q;
			if (full(&shadow_agg_payload) == 0) {
				enqueueF(&shadow_agg_header, Header_q);
				enqueueF(&shadow_agg_payload, PayloadPush_q);
				if (GC_Debug){
					lock_acquire(&uart_lock);
					printf("[AGG SS]: <<Pushed>> Expected: %x.                        Inst: %x. \r\n", PayloadPush_q, inst);
					lock_release(&uart_lock);
				}
			} else {
				lock_acquire(&uart_lock);
				printf("[AGG SS]: **Error** shadow stack is full. -- Queue index: %x.\r\n", index);
				lock_release(&uart_lock);
			}
		}

		if (((Opcode_q == 0x67) && (Rd_q == 0x00) && (RS1_q == 0x01)) || // + comprised inst
			((Opcode_q == 0X02) && (Func_q == 0x00 ) && (Rd_q == 0x01) && ((RS1_q & 0x01) == 0X01))) {
			PayloadPull_q = Payload_q;
			if (empty(&shadow_agg_payload) == 1) {
				printf("[AGG SS]: **Error** unintended pull. Addr: %x. Inst:%x. -- Queue index: %x. -- G-Counter = %x. \r\n", PayloadPull_q, Header_q, index, global_counter);
			} else {
				u_int64_t comp_q = dequeueF(&shadow_agg_payload);
				dequeueF(&shadow_agg_header);
				if (comp_q != PayloadPull_q){
					lock_acquire(&uart_lock);
					printf("[AGG SS]: **Error - CF ** Pulled: %x v.s. Expected: %x. Inst: %x. -- Queue index: %x. -- G-Counter = %x. \r\n", PayloadPull_q, comp_q, Header_q, index, global_counter);
					lock_release(&uart_lock);
				} else {
					// Successfully paired
					if (GC_Debug){
						lock_acquire(&uart_lock);
						printf("[AGG SS]: --Paried-- Expected: %x. v.s. Pulled: %x. Inst: %x. \r\n", comp_q, PayloadPull_q, inst);
						lock_release(&uart_lock);
					}
				}
			}
		}
	}
}

uint64_t nxt_target (uint64_t c_current, uint64_t c_start, uint64_t c_end)
{
	uint64_t c_nxt;
	if (c_current == c_end) {
    	c_nxt = c_start;
  	} else {
    	c_nxt = c_current + 1;
  	}
  	return c_nxt;
}



void* thread_shadowstack_m_agg(void* args){
	uint64_t hart_id = (uint64_t) args;
	uint64_t proc_id = remapping_hart_id(hart_id);

	uint64_t CurrentTarget = core_s;

  	initialize(&shadow_agg_header);
  	initialize(&shadow_agg_payload);

	for (int i = 0; i < NUM_CORES; i ++)
  	{
    	initialize(&queues_header[i]);
    	initialize(&queues_payload[i]);
  	}
	asm volatile("fence rw, rw;");


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
	if (GC_Debug){
  		lock_acquire(&uart_lock);
  		printf("[AGG SS]: == Current Target: %x. == \r\n", CurrentTarget);
  		lock_release(&uart_lock); 
	}
	
	while ((ghe_checkght_status() != 0x02) || (or_gate_checkercores(if_tasks_initalised, NUM_CORES) == 1)) {
		while (ghe_status() != GHE_EMPTY){
			uint64_t Header = 0x0;  
			uint64_t Opcode = 0x0;
			uint64_t Func = 0x0;
			uint64_t Rd = 0x0;
			uint64_t RS1 = 0x0;
			uint64_t Payload = 0x0;
			uint64_t PayloadPush = 0x0;
			uint64_t PayloadPull = 0x0;

			ROCC_INSTRUCTION_D (1, Header, 0x0A);
			ROCC_INSTRUCTION_D (1, Payload, 0x0D);
			uint64_t from = (Header>>32) & 0xF;
			uint64_t inst = Header & 0xFFFFFFFF;

			if (from == CurrentTarget){
				Opcode = inst & 0x7F;
				Func = (Header & 0x7000) >> 12;
				Rd = (inst & 0xF80) >> 7;
				RS1 = (Header & 0xF8000) >> 15;

				// Push -- a function is pushed
				if ((((Opcode == 0x6F) || (Opcode == 0x67)) && (Rd == 0x01)) || // + comprised inst
					((Opcode == 0x02) && (Func == 0x01) && ((RS1 & 0x01) == 0X01))) {
					PayloadPush = Payload;
					if (full(&shadow_agg_payload) == 0) {
						enqueueF(&shadow_agg_header, Header);
						enqueueF(&shadow_agg_payload, PayloadPush);
						if (GC_Debug){
							lock_acquire(&uart_lock);
							printf("[AGG SS]: <<Pushed>> Expected: %x.                        Inst: %x. \r\n", PayloadPush, inst);
							lock_release(&uart_lock);
						}
					} else {
						lock_acquire(&uart_lock);
						printf("[AGG SS]: **Error** shadow stack is full. -- Queue index: %x. \r\n", CurrentTarget);
						lock_release(&uart_lock);
					}
				}

				// Pull -- a function is pulled
				if (((Opcode == 0x67) && (Rd == 0x00) && (RS1 == 0x01)) || // + comprised inst
					((Opcode == 0X02) && (Func == 0x00 ) && (Rd == 0x01) && ((RS1 & 0x01) == 0X01))) {
					PayloadPull = Payload; 
					if (empty(&shadow_agg_payload) == 1) {
						printf("[AGG SS]: **Error** unintended pull. Addr: %x. Inst:%x. -- Queue index: %x. -- G-Counter = %x. \r\n", PayloadPull, inst, CurrentTarget, global_counter);
					} else {
						u_int64_t comp = dequeueF(&shadow_agg_payload);
						dequeueF(&shadow_agg_header);
					
						if (comp != PayloadPull){
						lock_acquire(&uart_lock);
						printf("[AGG SS]: **Error - MF ** Pulled: %x v.s. Expected: %x. Inst:%x. -- Queue index: %x -- G-Counter = %x.\r\n", PayloadPull, comp, inst, CurrentTarget, global_counter);
						lock_release(&uart_lock);
						} else {
						// Successfully paired
							if (GC_Debug){
								lock_acquire(&uart_lock);
								printf("[AGG SS]: --Paried-- Expected: %x. v.s. Pulled: %x. Inst: %x. \r\n", comp, PayloadPull, inst);
								lock_release(&uart_lock);
							}
						}
					}
				}
				
				// Clear queue
				if ((Opcode == 0x7F) && (Rd == 0x1F)) {
					clear_queue(CurrentTarget);
					ROCC_INSTRUCTION_S(1, 0x01 << (CurrentTarget-1), 0x21);
					CurrentTarget = nxt_target(CurrentTarget, core_s, core_e);
					if (GC_Debug){
						lock_acquire(&uart_lock);
						printf("[AGG SS]: == Current Target: %x. == \r\n", CurrentTarget);
						lock_release(&uart_lock); 
					}

					while ((empty(&queues_header[CurrentTarget]) == 0) && 
						   ((queueT(&queues_header[CurrentTarget]) & 0xFFFFFFFF) == 0xFFFFFFFF)) {
						u_int64_t useless;
						useless = dequeueF(&queues_header[CurrentTarget]);
						useless = dequeueF(&queues_payload[CurrentTarget]);
						clear_queue(CurrentTarget);
						ROCC_INSTRUCTION_S(1, 0x01 << (CurrentTarget-1), 0x21);
						CurrentTarget = nxt_target(CurrentTarget, core_s, core_e);
						if (GC_Debug){
							lock_acquire(&uart_lock);
							printf("[AGG SS]: == Current Target: %x. == \r\n", CurrentTarget);
							lock_release(&uart_lock); 
						}
					}
					clear_queue(CurrentTarget);
				}          
			} else {
				if (full(&queues_header[from]) == 0) {
          			enqueueF(&queues_header[from], Header);
					enqueueF(&queues_payload[from], Payload);
        		} else {
          			lock_acquire(&uart_lock);
          			printf("[AGG SS-%x]: **Error** Asynchronous Full!! from %x, Header = %x, Payload = %x. -- G-Counter = %x.\r\n", hart_id, from, Header, Payload, global_counter);
          			lock_release(&uart_lock);
        		}
			}
		}
	}
	
	//=================== Post execution ===================//
	lock_acquire(&uart_lock);
	printf("[AGG SS-C%x]: Completed. No error is detected!\r\n", hart_id);
   	lock_release(&uart_lock);

	ghe_release();
	if_tasks_initalised[proc_id] = 0;

	// This should not be required -- but we add it for unhandled inst
	while (ghe_status() != GHE_EMPTY){
		ROCC_INSTRUCTION (1, 0x0D);
	}

	for (int i = 0; i< NUM_CORES - 1; i++){
		ROCC_INSTRUCTION_S(1, 0x01 << (i-1), 0x21);
	}


	return NULL;
}


int main(){
	pthread_t threads[NUM_CORES];

	pthread_create(&threads[0], NULL, thread_boom, (void *) 0);	

	for (uint64_t i = 1; i < NUM_CORES; i++) {
		if (i == (NUM_CORES - 1)) {
			pthread_create(&threads[i], NULL, thread_shadowstack_m_agg, (void *) i);
		} else {
			pthread_create(&threads[i], NULL, thread_shadowstack_m_pre, (void *) i);
		}
	}

	for (uint64_t i = 0; i < NUM_CORES; i++) {
		pthread_join(threads[i], NULL);
	}
	
	return 0;
}