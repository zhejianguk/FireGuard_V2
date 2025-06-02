package freechips.rocketchip.r

import chisel3._
import chisel3.util._
import chisel3.experimental.{BaseModule}
import freechips.rocketchip.guardiancouncil._

case class R_ELUParams(
  nEntries: Int,
  xLen: Int,
  wAddr: Int
)

class R_ELUIO(params: R_ELUParams) extends Bundle {
  val lsl_req_valid = Input(UInt(1.W))
  val lsl_req_addr = Input(UInt(params.wAddr.W))
  val lsl_req_cmd = Input(UInt(2.W))
  val lsl_req_data = Input(UInt(params.xLen.W))
  val lsl_req_size = Input(UInt(2.W)) // 11: 64 bits; 10: 32 bits; 01: 16 bits; 00: 8 bits.
  val lsl_req_ready = Input(UInt(1.W))
  val lsl_req_kill = Input(UInt(1.W))

  val lsl_resp_valid = Input(UInt(1.W))
  val lsl_resp_addr = Input(UInt(params.wAddr.W))
  val lsl_resp_data = Input(UInt(params.xLen.W))
  val wb_pc = Input(UInt(params.wAddr.W))
  val wb_inst = Input(UInt(32.W))

  val elu_deq = Input(UInt(1.W))
  val elu_data = Output(UInt((2*params.xLen+3*params.wAddr).W))
  val elu_status = Output(UInt(1.W))
}

trait HasR_ELUIO extends BaseModule {
  val params: R_ELUParams
  val io = IO(new R_ELUIO(params))
}

class R_ELU (val params: R_ELUParams) extends Module with HasR_ELUIO {
  val ld_valid_reg            = RegInit(false.B)
  val ld_valid                = WireInit(false.B)
  val st_valid                = RegInit(false.B)
  val req_addr_reg            = RegInit(0.U(params.wAddr.W))
  val req_data_reg            = RegInit(0.U(params.xLen.W))
  val size                    = RegInit(0.U(2.W))

  val req_data_wire           = WireInit(0.U(params.xLen.W))
  val resp_data_wire          = WireInit(0.U(params.xLen.W))

  val if_amo                  = WireInit(false.B)
  if_amo                     := (io.wb_inst(6,0) === 0x2F.U) && ((io.wb_inst(14,12) === 0x2.U) || (io.wb_inst(14,12) === 0x3.U))
  val if_amo_sc               = if_amo && (io.wb_inst(31,27) === 0x03.U)

  ld_valid_reg               := !io.lsl_req_kill.asBool && io.lsl_req_ready.asBool && io.lsl_req_valid.asBool && ((io.lsl_req_cmd === 0x01.U) || (io.lsl_req_cmd === 0x03.U))
  ld_valid                   := ld_valid_reg && !if_amo_sc
  st_valid                   := !io.lsl_req_kill.asBool && io.lsl_req_ready.asBool && io.lsl_req_valid.asBool && (io.lsl_req_cmd === 0x02.U)
  req_addr_reg               := io.lsl_req_addr
  req_data_reg               := io.lsl_req_data
  size                       := io.lsl_req_size(0)

  val zeros_32bits            = WireInit(0.U(32.W))
  val zeros_48bits            = WireInit(0.U(48.W))
  val zeros_56bits            = WireInit(0.U(56.W))

  req_data_wire              := MuxCase(0.U, 
                                    Array((size === 3.U)      -> req_data_reg,
                                          (size === 2.U)      -> Cat(zeros_32bits, req_data_reg(31,0)),
                                          (size === 1.U)      -> Cat(zeros_48bits, req_data_reg(15,0)),
                                          (size === 0.U)      -> Cat(zeros_56bits, req_data_reg(7,0))
                                          )
                                          )

  resp_data_wire             := MuxCase(0.U, 
                                  Array((size === 3.U)      -> io.lsl_resp_data,
                                        (size === 2.U)      -> Cat(zeros_32bits, io.lsl_resp_data(31,0)),
                                        (size === 1.U)      -> Cat(zeros_48bits, io.lsl_resp_data(15,0)),
                                        (size === 0.U)      -> Cat(zeros_56bits, io.lsl_resp_data(7,0))
                                        )
                                        )

  val err_ld                  = WireInit(false.B)
  val err_st                  = WireInit(false.B)
  err_ld                     := Mux(ld_valid && (req_addr_reg =/= io.lsl_resp_addr), true.B, false.B)
  err_st                     := Mux(st_valid && ((req_addr_reg =/= io.lsl_resp_addr) || (req_data_wire =/= resp_data_wire)), true.B, false.B)

  val err_log_ld              = WireInit(0.U((2*params.xLen + 3*params.wAddr).W))
  val err_log_st              = WireInit(0.U((2*params.xLen + 3*params.wAddr).W))
  val err_log                 = WireInit(0.U((2*params.xLen + 3*params.wAddr).W))

  err_log_ld                 := Cat(io.wb_pc, resp_data_wire, 0x7777777.U, io.lsl_resp_addr, req_addr_reg) // load data is not compared
  err_log_st                 := Cat(io.wb_pc, resp_data_wire, req_data_wire, io.lsl_resp_addr, req_addr_reg)
  err_log                    := MuxCase(0.U, 
                                    Array(err_ld              -> err_log_ld,
                                          err_st              -> err_log_st
                                          )
                                          )
  // Revisit: ELU does not handle overflow, as it should rarely happen
  val u_channel               = Module (new GH_FIFO(FIFOParams((2*params.xLen+3*params.wAddr), params.nEntries)))
  val channel_enq_valid       = WireInit(false.B)
  val channel_enq_data        = WireInit(0.U((2*params.xLen+3*params.wAddr).W))
  val channel_deq_ready       = WireInit(false.B)
  val channel_deq_data        = WireInit(0.U((2*params.xLen+3*params.wAddr).W))
  val channel_empty           = WireInit(true.B)
  val channel_full            = WireInit(false.B)
  val channel_nearfull        = WireInit(0.U(1.W))

  u_channel.io.enq_valid     := channel_enq_valid
  u_channel.io.enq_bits      := channel_enq_data
  u_channel.io.deq_ready     := channel_deq_ready
  channel_deq_data           := u_channel.io.deq_bits
  channel_empty              := u_channel.io.empty
  channel_full               := u_channel.io.full
  channel_nearfull           := u_channel.io.status_threeslots

  channel_enq_valid          := err_ld | err_st
  channel_enq_data           := err_log
  channel_deq_ready          := io.elu_deq
  io.elu_data                := channel_deq_data
  io.elu_status              := ~channel_empty
}
