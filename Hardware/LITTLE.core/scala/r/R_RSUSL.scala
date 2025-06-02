package freechips.rocketchip.r

import chisel3._
import chisel3.util._
import chisel3.experimental.{BaseModule}
import freechips.rocketchip.guardiancouncil._

case class R_RSUSLParams(
  xLen: Int,
  numARFS: Int
)

class R_RSUSLIO(params: R_RSUSLParams) extends Bundle {
  val arfs_out = Output(UInt(params.xLen.W))
  val farfs_out = Output(UInt(params.xLen.W))
  val arfs_idx_out = Output(UInt(8.W))
  val arfs_valid_out = Output(UInt(1.W))

  val pcarf_out = Output(UInt(40.W))
  val fcsr_out = Output(UInt(8.W))
  val pfarf_valid_out = Output(UInt(1.W))
  val core_hang_up = Output(UInt(1.W))

  val arfs_merge = Input(UInt((params.xLen*2).W))
  val arfs_index = Input(UInt(8.W))
  val arfs_if_ARFS = Input(UInt(1.W))
  val arfs_if_CPS = Input(UInt(1.W))
  val copy_arfs = Input(UInt(1.W))
  val rsu_status = Output(UInt(2.W))
  val clear_ic_status = Input(UInt(1.W))

  val cdc_ready = Output(UInt(1.W))

  val do_cp_check = Input(UInt(1.W))
  val if_cp_check_completed = Output(UInt(1.W))
  val core_arfs_in = Input(Vec(params.numARFS, UInt(params.xLen.W)))
  val core_farfs_in = Input(Vec(params.numARFS, UInt(params.xLen.W)))
  val elu_cp_deq = Input(UInt(1.W))
  val elu_cp_data = Output(UInt((4*params.xLen+8).W))
  val elu_status = Output(UInt(1.W))
}

trait HasR_RSUSLIO extends BaseModule {
  val params: R_RSUSLParams
  val io = IO(new R_RSUSLIO(params))
}

class R_RSUSL(val params: R_RSUSLParams) extends Module with HasR_RSUSLIO {
  // Revisit: move it to the instruction counter
  val rsu_status                                  = RegInit(0.U(2.W))

  /* Loading snapshot from RSU Master */
  val arfs_ss                                     = Reg(Vec(params.numARFS, UInt(params.xLen.W)))
  val farfs_ss                                    = Reg(Vec(params.numARFS, UInt(params.xLen.W)))
  val pcarf_ss                                    = RegInit(0.U(40.W))
  val fcsr_ss                                     = RegInit(0.U(8.W))

  val if_RSU_packet                               = WireInit(0.U(1.W))
  val packet_valid                                = RegInit(0.U(1.W)) 
  val packet_index                                = RegInit(0.U(8.W))
  val packet_arfs                                 = RegInit(0.U(params.xLen.W))
  val packet_farfs                                = RegInit(0.U(params.xLen.W))

  if_RSU_packet                                  := Mux(io.arfs_if_ARFS.asBool && io.arfs_if_CPS.asBool, 1.U, 0.U) 
  packet_valid                                   := Mux(if_RSU_packet === 1.U, 1.U, 0.U)
  packet_arfs                                    := Mux(if_RSU_packet === 1.U, io.arfs_merge(63,0), 0.U)
  packet_farfs                                   := Mux(if_RSU_packet === 1.U, io.arfs_merge(127,64), 0.U)
  packet_index                                   := Mux(if_RSU_packet === 1.U, io.arfs_index, 0.U)

  when (packet_valid === 1.U){
    for (i <- 0 until params.numARFS){
      arfs_ss(i)                                 := Mux(packet_index === i.U, packet_arfs, arfs_ss(i))
      farfs_ss(i)                                := Mux(packet_index === i.U, packet_farfs, farfs_ss(i))
     }
      pcarf_ss                                   := Mux(packet_index === 0x20.U, packet_arfs(39,0), pcarf_ss)
      fcsr_ss                                    := Mux(packet_index === 0x20.U, packet_farfs(7,0), fcsr_ss)
  }

  /* Loading snapshot at End of Check Point from RSU Master */
  val arfs_ss_ECP                                 = Reg(Vec(params.numARFS, UInt(params.xLen.W)))
  val farfs_ss_ECP                                = Reg(Vec(params.numARFS, UInt(params.xLen.W)))
  val pcarf_ss_ECP                                = RegInit(0.U(40.W))
  val fcsr_ss_ECP                                 = RegInit(0.U(8.W))

  val if_RSU_packet_ECP                           = WireInit(0.U(1.W))
  val packet_valid_ECP                            = RegInit(0.U(1.W)) 
  val packet_index_ECP                            = RegInit(0.U(8.W))
  val packet_arfs_ECP                             = RegInit(0.U(params.xLen.W))
  val packet_farfs_ECP                            = RegInit(0.U(params.xLen.W))
  
  if_RSU_packet_ECP                              := Mux(io.arfs_if_ARFS.asBool && !io.arfs_if_CPS.asBool, 1.U, 0.U) 
  packet_valid_ECP                               := Mux(if_RSU_packet_ECP === 1.U, 1.U, 0.U)
  packet_arfs_ECP                                := Mux(if_RSU_packet_ECP === 1.U, io.arfs_merge(63,0), 0.U)
  packet_farfs_ECP                               := Mux(if_RSU_packet_ECP === 1.U, io.arfs_merge(127,64), 0.U)
  packet_index_ECP                               := Mux(if_RSU_packet_ECP === 1.U, io.arfs_index, 0.U)

  when (packet_valid_ECP === 1.U){
    for (i <- 0 until params.numARFS){
      arfs_ss_ECP(i)                             := Mux(packet_index_ECP === i.U, packet_arfs_ECP, arfs_ss_ECP(i))
      farfs_ss_ECP(i)                            := Mux(packet_index_ECP === i.U, packet_farfs_ECP, farfs_ss_ECP(i))
     }
      pcarf_ss_ECP                               := Mux(packet_index_ECP === 0x20.U, packet_arfs_ECP(39,0), pcarf_ss_ECP)
      fcsr_ss_ECP                                := Mux(packet_index_ECP === 0x20.U, packet_farfs_ECP(7,0), fcsr_ss_ECP)
  }

  rsu_status                                     := Mux(io.clear_ic_status.asBool, 0.U, Mux(packet_index === 0x20.U, 1.U, Mux(packet_index_ECP === 0x20.U, 3.U, rsu_status)))

  /* Applying snapshot to the core */
  val apply_snapshot                              = RegInit(0.U(1.W))
  val apply_counter                               = RegInit(20.U(8.W))

  when ((io.copy_arfs === 0x01.U) && (apply_snapshot === 0.U)) {
    apply_snapshot                               := 1.U
    apply_counter                                := 0.U
  } .elsewhen (apply_snapshot === 1.U){
    apply_snapshot                               := Mux(apply_counter === 0x20.U, 0.U, 1.U)
    apply_counter                                := Mux(apply_counter === 0x20.U, 0.U, apply_counter + 1.U)
  } .otherwise {
    apply_snapshot                               := apply_snapshot
    apply_counter                                := apply_counter
  }

  io.arfs_out                                    := Mux(((apply_snapshot === 1.U) && (apply_counter =/= 0x20.U)), arfs_ss(apply_counter), 0.U)
  io.farfs_out                                   := Mux(((apply_snapshot === 1.U) && (apply_counter =/= 0x20.U)), farfs_ss(apply_counter), 0.U)
  io.arfs_idx_out                                := Mux(((apply_snapshot === 1.U) && (apply_counter =/= 0x20.U)), apply_counter, 0.U)
  io.arfs_valid_out                              := Mux(((apply_snapshot === 1.U) && (apply_counter =/= 0x20.U)), 1.U, 0.U)

  io.pcarf_out                                   := pcarf_ss
  io.fcsr_out                                    := Mux(((apply_snapshot === 1.U) && (apply_counter === 0x20.U)), fcsr_ss, 0.U)
  io.pfarf_valid_out                             := Mux(((apply_snapshot === 1.U) && (apply_counter === 0x20.U)), 1.U, 0.U)

  io.cdc_ready                                   := packet_valid | packet_valid_ECP
  io.core_hang_up                                := apply_snapshot

  io.rsu_status                                  := rsu_status

  val u_channel                                   = Module (new GH_FIFO(FIFOParams((4*params.xLen+8), 4)))
  val channel_enq_valid                           = WireInit(false.B)
  val channel_enq_data                            = WireInit(0.U((4*params.xLen+8).W))
  val channel_deq_ready                           = WireInit(false.B)
  val channel_deq_data                            = WireInit(0.U((4*params.xLen+8).W))
  val channel_empty                               = WireInit(true.B)
  val channel_full                                = WireInit(false.B)

  u_channel.io.enq_valid                          := channel_enq_valid
  u_channel.io.enq_bits                           := channel_enq_data
  u_channel.io.deq_ready                          := channel_deq_ready
  channel_deq_data                                := u_channel.io.deq_bits
  channel_empty                                   := u_channel.io.empty
  channel_full                                    := u_channel.io.full

  val if_check_completed                           = RegInit(0.U(1.W))
  val checking_counter                             = RegInit(0.U(8.W))

  if_check_completed                              := Mux(!io.do_cp_check.asBool, 0.U, Mux(checking_counter === 0x19.U, 1.U, if_check_completed))
  io.if_cp_check_completed                        := if_check_completed

  when (!io.do_cp_check.asBool){
    checking_counter                              := 0.U
  } .otherwise {
    checking_counter                              := Mux(checking_counter === 0x19.U, checking_counter, checking_counter+1.U)
  }
  channel_enq_valid                               := io.do_cp_check.asBool && !if_check_completed.asBool && ((io.core_arfs_in(checking_counter) =/= arfs_ss_ECP(checking_counter)) ||  (io.core_farfs_in(checking_counter) =/= farfs_ss_ECP(checking_counter)))
  channel_enq_data                                := Mux(channel_enq_valid.asBool, Cat(checking_counter, farfs_ss_ECP(checking_counter), io.core_farfs_in(checking_counter), arfs_ss_ECP(checking_counter), io.core_arfs_in(checking_counter)), 0.U)
  channel_deq_ready                               := io.elu_cp_deq.asBool
  io.elu_cp_data                                  := channel_deq_data
  io.elu_status                                   := ~channel_empty
}