#include <stdio.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "tasks.h"

int uart_lock;
char* shadow;
uint64_t m_counter;
uint64_t i_counter;
uint64_t g_counter;

/* Core_0 thread */
int main(void)
{
  //================== Initialisation ==================//
  ght_set_numberofcheckers(6);
  ROCC_INSTRUCTION_S (1, 0x02, 0x01); // Enabling FI
  ROCC_INSTRUCTION (1, 0x67); // Reset FI

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

  // se: 00, end_id: 0x02, scheduling: rr, start_id: 0x01
  ght_cfg_se (0x00, 0x02, 0x01, 0x01);
  // se: 01, end_id: 0x03, scheduling: rr, start_id: 0x03
  ght_cfg_se (0x01, 0x03, 0x01, 0x03);
  // se: 02, end_id: 0x06, scheduling: rr, start_id: 0x04
  ght_cfg_se (0x02, 0x06, 0x01, 0x04);

  
  ght_cfg_mapper (0b01, 0b111);

  while (ght_get_initialisation() == 0){

  }


  lock_acquire(&uart_lock);
  printf("C0: Test is now started: \r\n");
  lock_release(&uart_lock);
  ght_set_satp_priv();
  ght_set_status (0x01); // ght: start



  //===================== Execution =====================//
  __asm__(
          "li   t0,   0x81000000;"         // write pointer
          "li   t1,   0x55555000;"         // data
          "li   t2,   0x81000000;"         // Read pointer
          "j    .loop_store;");

  __asm__(
          ".loop_store:"
          "li   a5,   0x81000FFF;"
          "sw         t1,   (t0);"
          "addi t1,   t1,   1;"            // data + 1
          "addi t0,   t0,   4;"            // write address + 4
          "blt  t0,   a5,  .loop_store;"
          "li   t0,   0x82000000;"
          "li   t2,   0x81000000;"
          "j    .loop_load;");

  __asm__(
          ".loop_load:"
          "li   a5,   0x82000FFF;"
          "lw   t1,   (t2);"
          "sw         t1,   (t0);"
          "addi t0,   t0,   4;"
          "addi t2,   t2,   4;"
          "blt  t0,   a5,  .loop_load;");

  __asm__(
          ".loop_load2:"
          "li   a5,   0x82000FFF;"
          "lw   t1,   (t2);"
          "sw         t1,   (t0);"
          "addi t0,   t0,   4;"
          "addi t2,   t2,   4;"
          "blt  t0,   a5,  .loop_load;");



  //=================== Post execution ===================//
  ght_set_status (0x02);
  uint64_t status;
  while (((status = ght_get_status()) < 0x1FFFF) || (ght_get_buffer_status() != GHT_EMPTY)){

  }

  lock_acquire(&uart_lock);
  printf("All tests are done! Status: %x \r\n", status);
  lock_release(&uart_lock);

  ght_unset_satp_priv();
  ght_set_status (0x00);

  m_counter = debug_mcounter(); // Merged instructions
  i_counter = debug_icounter(); // Instructions committed
  g_counter = debug_gcounter(); // Global cycle counter

  lock_acquire(&uart_lock);
  printf("Debug, m-counter: %x \r\n", m_counter);
  printf("Debug, i-counter: %x \r\n", i_counter);
  printf("Debug, g-counter: %x \r\n", g_counter);

  for (int j = 0; j < 0x40; j++) {
    uint64_t latency = ght_readFIU(j);
    printf("Detection latency for %x is %d cycles \r\n",j, latency);
  }
  lock_release(&uart_lock);




  while (ght_get_initialisation() == 1){
    
  }

  return 0;
}

/* Core_1 & 2 thread */
int __main(void)
{
  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));
  
  switch (Hart_id){
      case 0x01:
        task_PerfCounter(Hart_id);
      break;

      case 0x02:
        task_PerfCounter(Hart_id);
      break;

      case 0x03:
        task_PerfCounter(Hart_id);
      break;

      case 0x04:
        task_PerfCounter(Hart_id);
      break;

      case 0x05:
        task_PerfCounter(Hart_id);
      break;

      case 0x06:
        task_PerfCounter(Hart_id);
      break;


      case 0x07:
        task_PerfCounter(Hart_id);
      break;

      default:
      break;
  }
  
  idle();
  return 0;
}