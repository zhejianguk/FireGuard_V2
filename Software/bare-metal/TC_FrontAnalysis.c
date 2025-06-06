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
  ght_set_numberofcheckers(4);
  ROCC_INSTRUCTION (1, 0x67); // Reset Performance Counter

  

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
  ght_cfg_mapper (0b01, 0b11);


  lock_acquire(&uart_lock);
  printf("C0: Test is now started: \r\n");
  lock_release(&uart_lock);
  
  ght_set_satp_priv();
  ROCC_INSTRUCTION_S (1, 0x01, 0x01); // Enabling Front Performance Analysis
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
  ROCC_INSTRUCTION_S (1, 0x00, 0x01); // Disabling Front Performance Analysis

  lock_acquire(&uart_lock);
  printf("All tests are done! Status: %x \r\n");
  lock_release(&uart_lock);

  ght_unset_satp_priv();
  ght_set_status (0x00);
  
  lock_acquire(&uart_lock);
  m_counter = debug_mcounter(); // Merged instructions
  i_counter = debug_icounter(); // Instructions committed
  g_counter = debug_gcounter(); // Global cycle counter
  printf("Debug, m-counter: %x \r\n", m_counter);
  printf("Debug, i-counter: %x \r\n", i_counter);
  printf("Debug, g-counter: %x \r\n", g_counter);
  lock_release(&uart_lock);



  lock_acquire(&uart_lock);
  ROCC_INSTRUCTION (1, 0x67); // Reset Performance Counter
  printf("Reset Performance Counter \r\n");

  m_counter = debug_mcounter(); // Merged instructions
  i_counter = debug_icounter(); // Instructions committed
  g_counter = debug_gcounter(); // Global cycle counter
  printf("Debug, m-counter: %x \r\n", m_counter);
  printf("Debug, i-counter: %x \r\n", i_counter);
  printf("Debug, g-counter: %x \r\n", g_counter);
  lock_release(&uart_lock);


  return 0;
}

/* Core_1 & 2 thread */
int __main(void)
{  
  idle();
  return 0;
}