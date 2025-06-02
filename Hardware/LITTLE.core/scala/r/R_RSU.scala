package freechips.rocketchip.r

import chisel3._
import chisel3.util._
import chisel3.experimental.{BaseModule}

case class R_RSUParams(
  xLen: Int,
  numARFS: Int,
  scalarWidth: Int
)

class R_RSUIO(params: R_RSUParams) extends Bundle {
  val arfs_in = Input(Vec(params.numARFS, UInt(params.xLen.W)))
  val farfs_in = Input(Vec(params.numARFS, UInt(params.xLen.W)))
  val pcarf_in = Input(UInt(40.W))
  val fcsr_in = Input(UInt(8.W))
  val snapshot = Input(UInt(1.W))
  val merge = Input(UInt(1.W))
  val ght_filters_ready = Input(UInt(1.W))
  val core_hang_up = Output(UInt(1.W))
  val rsu_merging = Output(UInt(1.W))
  val arfs_merge = Output(Vec(params.scalarWidth, UInt((params.xLen*2).W)))
  val arfs_index = Output(Vec(params.scalarWidth, UInt(8.W)))
  val ic_crnt_target = Input(UInt(5.W))
  val arfs_pidx = Output(Vec(params.scalarWidth, UInt(8.W)))
  val rsu_busy = Output(UInt(1.W))
}

trait HasR_RSUIO extends BaseModule {
  val params: R_RSUParams
  val io = IO(new R_RSUIO(params))
}

class R_RSU(val params: R_RSUParams) extends Module with HasR_RSUIO {
  val pcarf_ss                                    = RegInit(0.U(40.W))
  val arfs_ss                                     = Reg(Vec(params.numARFS, UInt(params.xLen.W)))
  val farfs_ss                                    = Reg(Vec(params.numARFS, UInt(params.xLen.W)))
  val fcsr_ss                                     = RegInit(0.U(8.W))

  val merging                                     = RegInit(0.U(1.W))
  val merge_counter                               = RegInit(0.U(8.W)) // 32/4 + 1 (PC) = 9 Packets in total
  val doSnapshot                                  = RegInit(0.U(1.W))
  val doMerge                                     = RegInit(0.U(1.W))

  doSnapshot                                     := io.snapshot 
  doMerge                                        := io.merge 

  
  when (doSnapshot === 1.U) {
      for (i <- 0 until params.numARFS) {
        arfs_ss(i)                               := io.arfs_in(i)
        farfs_ss(i)                              := io.farfs_in(i)
      }
      pcarf_ss                                   := io.pcarf_in
      fcsr_ss                                    := io.fcsr_in
  }

  when ((doMerge === 1.U) && (merging === 0.U)){
    merging                                      := 1.U
    merge_counter                                := 0.U
  } .elsewhen ((merging === 1.U) && (io.ght_filters_ready === 1.U)) {
    merging                                      := Mux (merge_counter === 8.U, 0.U, 1.U)
    merge_counter                                := Mux (merge_counter === 8.U, 0.U, merge_counter + 1.U)
  } .otherwise {
    merging                                      := merging
    merge_counter                                := merge_counter
  }

  io.core_hang_up                                := io.snapshot|doSnapshot

  val zeros_24bits                                = WireInit(0.U(24.W))
  val zeros_56bits                                = WireInit(0.U(56.W))
  val seven_3bits                                 = WireInit(7.U(3.W))

  for (w <- 0 until params.scalarWidth) {
    if (w == 0) {
      io.arfs_merge(w)                             := MuxCase(0.U,
                                                      Array(((merging === 1.U) && (merge_counter =/= 8.U)) -> Cat(farfs_ss(merge_counter<<2), arfs_ss(merge_counter<<2)),
                                                            ((merging === 1.U) && (merge_counter === 8.U)) -> Cat(zeros_56bits, fcsr_ss, zeros_24bits, pcarf_ss)
                                                          )
                                                          )

      io.arfs_index(w)                             := Mux((merging === 1.U), (merge_counter<<2), 0.U)
      io.arfs_pidx(w)                              := Mux((merging === 1.U), Cat(io.ic_crnt_target(4,0), seven_3bits), 0.U)
    } else {
      io.arfs_merge(w)                             := MuxCase(0.U,
                                                      Array(((merging === 1.U) && (merge_counter =/= 8.U)) -> Cat(farfs_ss((merge_counter<<2)+w.U), arfs_ss((merge_counter<<2)+w.U)),
                                                            ((merging === 1.U) && (merge_counter === 8.U)) -> 0.U
                                                          )
                                                          )

      io.arfs_index(w)                             := Mux(((merging === 1.U) && (merge_counter =/= 8.U)), ((merge_counter<<2) + w.U), 0.U)
      io.arfs_pidx(w)                              := Mux(((merging === 1.U) && (merge_counter =/= 8.U)), Cat(io.ic_crnt_target(4,0), seven_3bits), 0.U)
    }
  }
  
  io.rsu_merging                                   := merging
  io.rsu_busy                                      := Mux(io.snapshot.asBool || io.merge.asBool || doSnapshot.asBool || doMerge.asBool || merging.asBool, 1.U, 0.U)




}