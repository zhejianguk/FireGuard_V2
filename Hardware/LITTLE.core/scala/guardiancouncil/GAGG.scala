package freechips.rocketchip.guardiancouncil


import chisel3._
import chisel3.util._
import chisel3.experimental.{BaseModule}
import org.chipsalliance.cde.config.{Field, Parameters}
import freechips.rocketchip.subsystem.{BaseSubsystem, HierarchicalLocation, HasTiles, TLBusWrapperLocation}
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.util.{ClockDivider2}

case class GAGGParams(
  number_of_little_cores: Int,
  width_GH_packet: Int
)


class GAGGIO(params: GAGGParams) extends Bundle {
  val agg_packet_in                              = Input(Vec(params.number_of_little_cores, UInt(params.width_GH_packet.W)))
  val agg_buffer_full                            = Output(Vec(params.number_of_little_cores, UInt(1.W)))
  val agg_core_status                            = Input(Vec(params.number_of_little_cores,UInt(2.W)))
  val agg_core_id                                = Input(UInt(16.W))

  val sch_na_in                                  = Input(Vec(params.number_of_little_cores, UInt(1.W)))
  val sch_na_out                                 = Output(UInt(params.number_of_little_cores.W))
  val fi_d                                       = Input(Vec(params.number_of_little_cores, UInt(57.W)))
  val fi_d_out                                   = Output(UInt((params.number_of_little_cores * 57).W))

  val sch_do_refresh                             = Input(Vec(params.number_of_little_cores, UInt(32.W)))
  val sch_refresh_out                            = Output(Vec(params.number_of_little_cores, UInt(1.W)))

  val agg_packet_outs                            = Output(Vec(params.number_of_little_cores, UInt(params.width_GH_packet.W)))
  val agg_no_packet_inflight                     = Output(UInt(1.W))
}

trait HasGAGGIO extends BaseModule {
  val params: GAGGParams
  val io = IO(new GAGGIO(params))
}

//==========================================================
// Implementations
//==========================================================
class GAGG (val params: GAGGParams)(implicit p: Parameters) extends LazyModule
{
  lazy val module = new Impl
  class Impl extends LazyModuleImp(this) {
    val io                                         = IO(new GAGGIO(params))
    val u_agg                                      = Module (new GHT_AGG(GHM_AGG_Params(params.number_of_little_cores, params.width_GH_packet)))

    val packet_out_wires                           = WireInit (VecInit(Seq.fill(params.number_of_little_cores)(0.U(params.width_GH_packet.W))))
    var sch_na                                     = WireInit(0.U(params.number_of_little_cores.W))
    val sch_na_in_wires                            = WireInit(VecInit(Seq.fill(params.number_of_little_cores)(0.U(params.number_of_little_cores.W))))
    val zeros_nbit                                 = WireInit(0.U((params.number_of_little_cores - 1).W))
    val agg_core_id                                = io.agg_core_id - 1.U
    val do_refresh                                 = WireInit(0.U(32.W))

        
    // Routing
    for (i <- 0 to params.number_of_little_cores - 1) {
      packet_out_wires(i)                         := Mux((agg_core_id === i.U), u_agg.io.agg_packet_out, 0.U)
                                                         
      u_agg.io.agg_packet_in(i)                   := io.agg_packet_in(i)
      io.agg_buffer_full(i)                       := u_agg.io.agg_buffer_full(i)
    }
    u_agg.io.agg_core_full                        := io.agg_core_status(agg_core_id)(1)
    do_refresh                                    := io.sch_do_refresh(agg_core_id)

   

    val collecting_checker_status                  = io.agg_core_status.reduce(_&_)
    val if_checkers_empty                          = collecting_checker_status(0)
    val if_agg_empty                               = Mux((packet_out_wires.reduce(_|_) === 0.U), 1.U, 0.U)

    val if_no_inflight_packets                     = if_checkers_empty & if_agg_empty & u_agg.io.agg_empty

    for(i <- 0 to params.number_of_little_cores - 1) {
      io.agg_packet_outs(i)                        := packet_out_wires(i)
      sch_na_in_wires(i)                          := Cat(zeros_nbit, io.sch_na_in(i))
      io.sch_refresh_out(i)                       := do_refresh(i)
    }


    for(i <- 0 to params.number_of_little_cores - 1) {
      sch_na                                      = sch_na | (sch_na_in_wires(i) << i)
    }
    io.sch_na_out                                := sch_na
    io.agg_no_packet_inflight                    := if_no_inflight_packets
    io.fi_d_out                                  := io.fi_d.reverse.reduce(Cat(_,_))
  }
}

case class GAGGCoreLocated(loc: HierarchicalLocation) extends Field[Option[GAGGParams]](None)

object GAGGCore {

  def attach(params: GAGGParams, subsystem: BaseSubsystem with HasTiles, where: TLBusWrapperLocation)
            (implicit p: Parameters) {
    val number_of_ghes                             = subsystem.tile_agg_packet_in_EPNodes.size

    val agg_empty_SRNode                           = BundleBridgeSource[UInt](Some(() => UInt(1.W)))
    val ghm_agg_core_id_SKNode                     = BundleBridgeSink[UInt](Some(() => UInt(16.W)))
    var agg_packet_out_SRNodes                     = Seq[BundleBridgeSource[UInt]]()

    ghm_agg_core_id_SKNode                        := subsystem.tile_ghm_agg_core_id_EPNode

    for (i <- 0 to number_of_ghes-1) {
      val agg_packet_out_SRNode                    = BundleBridgeSource[UInt]()
      agg_packet_out_SRNodes                       = agg_packet_out_SRNodes :+ agg_packet_out_SRNode
      subsystem.tile_agg_packet_in_EPNodes(i)     := agg_packet_out_SRNodes(i)
    }

    // Agg
    var report_fi_detection_in_SKNodes             = Seq[BundleBridgeSink[UInt]]()
    var report_fi_detection_out_SRNodes            = Seq[BundleBridgeSource[UInt]]()

    var agg_packet_in_SKNodes                      = Seq[BundleBridgeSink[UInt]]()
    var agg_buffer_full_out_SRNodes                = Seq[BundleBridgeSource[UInt]]()
    var agg_core_status_in_SKNodes                 = Seq[BundleBridgeSink[UInt]]()

    var ghm_ght_sch_na_in_SKNodes                  = Seq[BundleBridgeSink[UInt]]()
    var ghm_ghe_sch_refresh_out_SRNodes            = Seq[BundleBridgeSource[UInt]]()
    var ghm_ght_sch_dorefresh_in_SKNodes           = Seq[BundleBridgeSink[UInt]]()

    for (i <- 0 to number_of_ghes-1) {
      val report_fi_detection_in_SKNode            = BundleBridgeSink[UInt]()
      report_fi_detection_in_SKNodes               = report_fi_detection_in_SKNodes :+ report_fi_detection_in_SKNode
      report_fi_detection_in_SKNodes(i)           := subsystem.tile_report_fi_detection_EPNodes(i)

      val report_fi_detection_out_SRNode           = BundleBridgeSource[UInt]()
      report_fi_detection_out_SRNodes              = report_fi_detection_out_SRNodes :+ report_fi_detection_out_SRNode
      subsystem.tile_report_fi_detection_in_EPNodes(i) := report_fi_detection_out_SRNodes(i)

      val agg_packet_in_SKNode                     = BundleBridgeSink[UInt]()
      agg_packet_in_SKNodes                        = agg_packet_in_SKNodes :+ agg_packet_in_SKNode
      agg_packet_in_SKNodes(i)                    := subsystem.tile_agg_packet_out_EPNodes(i)

      val agg_buffer_full_out_SRNode               = BundleBridgeSource[UInt]()
      agg_buffer_full_out_SRNodes                  = agg_buffer_full_out_SRNodes :+ agg_buffer_full_out_SRNode
      subsystem.tile_agg_buffer_full_in_EPNodes(i):= agg_buffer_full_out_SRNodes(i)

      val agg_core_status_in_SKNode                = BundleBridgeSink[UInt]()
      agg_core_status_in_SKNodes                   = agg_core_status_in_SKNodes :+ agg_core_status_in_SKNode
      agg_core_status_in_SKNodes(i)               := subsystem.tile_agg_core_status_out_EPNodes(i)

      val ghm_ght_sch_na_in_SKNode                 = BundleBridgeSink[UInt]()
      ghm_ght_sch_na_in_SKNodes                    = ghm_ght_sch_na_in_SKNodes :+ ghm_ght_sch_na_in_SKNode
      ghm_ght_sch_na_in_SKNodes(i)                := subsystem.tile_ght_sch_na_out_EPNodes(i)

      val ghm_ghe_sch_refresh_out_SRNode           = BundleBridgeSource[UInt]()
      ghm_ghe_sch_refresh_out_SRNodes              = ghm_ghe_sch_refresh_out_SRNodes :+ ghm_ghe_sch_refresh_out_SRNode
      subsystem.tile_ghe_sch_refresh_in_EPNodes(i):= ghm_ghe_sch_refresh_out_SRNodes(i)

      val ghm_ght_sch_dorefresh_in_SKNode          = BundleBridgeSink[UInt]()
      ghm_ght_sch_dorefresh_in_SKNodes             = ghm_ght_sch_dorefresh_in_SKNodes :+ ghm_ght_sch_dorefresh_in_SKNode
      ghm_ght_sch_dorefresh_in_SKNodes(i)         := subsystem.tile_ght_sch_dorefresh_EPNodes(i)
    }
    subsystem.tile_agg_empty_EPNode               := agg_empty_SRNode

    val sch_na_SRNode                              = BundleBridgeSource[UInt](Some(() => UInt(16.W)))
    subsystem.tile_sch_na_EPNode                  := sch_na_SRNode

    val bus = subsystem.locateTLBusWrapper(where)
    val gagg = LazyModule (new GAGG (GAGGParams (params.number_of_little_cores, params.width_GH_packet)))

    
    InModuleBody {
      // val clk_div = Module(new ClockDivider2)
      // clk_div.io.clk_in                           := bus.module.clock
      // gagg.module.clock                           := clk_div.io.clk_out
      gagg.module.clock                           := bus.module.clock
      gagg.module.io.agg_core_id                  := ghm_agg_core_id_SKNode.bundle 

      for (i <- 0 to number_of_ghes-1) {
        if (i == 0) { // The big core
          // GHE is not connected to the big core
          agg_packet_out_SRNodes(i).bundle        := 0.U 
          agg_buffer_full_out_SRNodes(i).bundle   := 0.U
          report_fi_detection_out_SRNodes(i).bundle := gagg.module.io.fi_d_out
        } else {// -1 big core
          agg_packet_out_SRNodes(i).bundle        := gagg.module.io.agg_packet_outs(i-1)
          gagg.module.io.agg_core_status(i-1)     := agg_core_status_in_SKNodes(i).bundle
          gagg.module.io.sch_do_refresh(i-1)      := ghm_ght_sch_dorefresh_in_SKNodes(i).bundle
          gagg.module.io.agg_packet_in(i-1)       := agg_packet_in_SKNodes(i).bundle
          agg_buffer_full_out_SRNodes(i).bundle   := gagg.module.io.agg_buffer_full(i-1)
          gagg.module.io.sch_na_in(i-1)           := ghm_ght_sch_na_in_SKNodes(i).bundle
          ghm_ghe_sch_refresh_out_SRNodes(i).bundle:= gagg.module.io.sch_refresh_out(i-1)
          
          gagg.module.io.fi_d(i-1)                := report_fi_detection_in_SKNodes(i).bundle
          report_fi_detection_out_SRNodes(i).bundle := 0.U
        }
      }
      agg_empty_SRNode.bundle                     := gagg.module.io.agg_no_packet_inflight
      sch_na_SRNode.bundle                        := gagg.module.io.sch_na_out
    }
    gagg
  }
}