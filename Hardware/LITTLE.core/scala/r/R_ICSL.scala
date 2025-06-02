package freechips.rocketchip.r

import chisel3._
import chisel3.util._
import chisel3.experimental.{BaseModule}
import freechips.rocketchip.guardiancouncil._

case class R_ICSLParams(
  width_of_ic: Int
)

class R_ICSLIO(params: R_ICSLParams) extends Bundle {
  val ic_counter                                 = Input(UInt((params.width_of_ic).W))
  val icsl_run                                   = Input(UInt(1.W))
  val new_commit                                 = Input(UInt(1.W))
  val if_correct_process                         = Input(UInt(1.W))
  val returned_to_special_address_valid          = Input(UInt(1.W))

  val icsl_checkermode                           = Output(UInt(1.W))
  val clear_ic_status                            = Output(UInt(1.W))
  val if_overtaking                              = Output(UInt(1.W))
  val if_ret_special_pc                          = Output(UInt(1.W))
  val if_rh_cp_pc                                = Output(UInt(1.W))
  val if_check_completed                         = Input(UInt(1.W))
  val icsl_status                                = Output(UInt(2.W))
}

trait HasR_ICSLIO extends BaseModule {
  val params: R_ICSLParams
  val io = IO(new R_ICSLIO(params))
}

class R_ICSL (val params: R_ICSLParams) extends Module with HasR_ICSLIO {
  val ic_counter_shadow                          = WireInit(0.U((params.width_of_ic-1).W))
  val ic_counter_done                            = WireInit(0.U(1.W))
  val icsl_run                                   = WireInit(0.U(1.W))
  
  val icsl_checkermode                           = WireInit(0.U(1.W))
  val clear_ic_status                            = WireInit(0.U(1.W))
  val if_overtaking                              = RegInit(0.U(1.W))
  val if_ret_special_pc                          = RegInit(0.U(1.W))
  val if_rh_cp_pc                                = WireInit(0.U(1.W))

  val sl_counter                                 = RegInit(0.U(params.width_of_ic.W))
  val if_instants_overtaking                     = Mux((io.if_correct_process.asBool && io.new_commit.asBool && ((sl_counter + 1.U) >= ic_counter_shadow) && ic_counter_done.asBool), 1.U, 0.U)


  val fsm_reset :: fsm_nonchecking :: fsm_checking :: fsm_postchecking :: Nil = Enum(4)
  val fsm_state                                  = RegInit(fsm_reset)

  switch (fsm_state) {
    is (fsm_reset) {
      sl_counter                                := 0.U
      clear_ic_status                           := 1.U
      icsl_checkermode                          := 0.U
      if_rh_cp_pc                               := 0.U
      fsm_state                                 := fsm_nonchecking
    }

    is (fsm_nonchecking) {
      sl_counter                                := sl_counter
      clear_ic_status                           := 0.U
      icsl_checkermode                          := 0.U
      if_rh_cp_pc                               := 0.U
      fsm_state                                 := Mux(icsl_run.asBool, fsm_checking, fsm_nonchecking)
    }

    is (fsm_checking){
      sl_counter                                := Mux(io.if_correct_process.asBool && io.new_commit.asBool, sl_counter + 1.U, sl_counter)
      clear_ic_status                           := 0.U
      icsl_checkermode                          := 1.U
      if_rh_cp_pc                               := 0.U
      fsm_state                                 := Mux(if_instants_overtaking.asBool, fsm_postchecking, fsm_checking)
    }

    is (fsm_postchecking){
      sl_counter                                := sl_counter
      clear_ic_status                           := 0.U
      icsl_checkermode                          := 1.U
      if_rh_cp_pc                               := 1.U
      fsm_state                                 := Mux(io.returned_to_special_address_valid.asBool, fsm_reset, fsm_postchecking)
    }

  }

  ic_counter_shadow                             := io.ic_counter(params.width_of_ic-2,0) + 1.U // The checker core requires to run one more insts due to the custom jump
  ic_counter_done                               := io.ic_counter(params.width_of_ic-1)
  if_overtaking                                 := Mux((if_instants_overtaking.asBool ||  (sl_counter >= ic_counter_shadow)), 1.U, 0.U)
  

  if_ret_special_pc                             := Mux(io.if_check_completed.asBool, 1.U, 0.U)
  
  icsl_run                                      := io.icsl_run
  io.clear_ic_status                            := clear_ic_status
  io.icsl_checkermode                           := icsl_checkermode & io.if_correct_process 
  io.if_overtaking                              := if_overtaking
  io.if_ret_special_pc                          := if_ret_special_pc
  io.if_rh_cp_pc                                := if_rh_cp_pc
  io.icsl_status                                := Mux(fsm_state === fsm_nonchecking, 1.U, 0.U)
}
