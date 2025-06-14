// See LICENSE.Berkeley for license details.
// See LICENSE.SiFive for license details.

package freechips.rocketchip.tile

import chisel3._
import chisel3.util._
import chisel3.util.HasBlackBoxResource
import chisel3.experimental.IntParam
import org.chipsalliance.cde.config._
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.rocket._
import freechips.rocketchip.tilelink._
import freechips.rocketchip.util.InOrderArbiter
//===== GuardianCouncil Function: Start ====//
import freechips.rocketchip.guardiancouncil._
//===== GuardianCouncil Function: End   ====//

case object BuildRoCC extends Field[Seq[Parameters => LazyRoCC]](Nil)

class RoCCInstruction extends Bundle {
  val funct = Bits(7.W)
  val rs2 = Bits(5.W)
  val rs1 = Bits(5.W)
  val xd = Bool()
  val xs1 = Bool()
  val xs2 = Bool()
  val rd = Bits(5.W)
  val opcode = Bits(7.W)
}

class RoCCCommand(implicit p: Parameters) extends CoreBundle()(p) {
  val inst = new RoCCInstruction
  val rs1 = Bits(xLen.W)
  val rs2 = Bits(xLen.W)
  val status = new MStatus
}

class RoCCResponse(implicit p: Parameters) extends CoreBundle()(p) {
  val rd = Bits(5.W)
  val data = Bits(xLen.W)
}

class RoCCCoreIO(val nRoCCCSRs: Int = 0)(implicit p: Parameters) extends CoreBundle()(p) {
  val cmd = Flipped(Decoupled(new RoCCCommand))
  val resp = Decoupled(new RoCCResponse)
  val mem = new HellaCacheIO
  val busy = Output(Bool())
  val interrupt = Output(Bool())
  val exception = Input(Bool())
  //===== GuardianCouncil Function: Start ====//
  val ghe_packet_in = Input(UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W))
  val ghe_status_in = Input(UInt(32.W))
  val bigcore_comp  = Input(UInt(3.W))
  val ghe_event_out = Output(UInt(5.W))
  val ght_mask_out = Output(UInt(1.W))
  val ght_status_out = Output(UInt(32.W))
  val ght_cfg_out = Output(UInt(32.W))
  val ght_cfg_valid = Output(UInt(1.W))
  val debug_bp_reset = Output(UInt(1.W))

  val agg_packet_out = Output(UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W))
  val report_fi_detection_out = Output(UInt(57.W))
  val fi_sel_out = Output(UInt(8.W))
  val agg_buffer_full = Input(UInt(1.W))
  val agg_core_status = Output(UInt(2.W))
  val ght_sch_na = Output(UInt(1.W))
  val ght_sch_refresh = Input(UInt(1.W))
  val ght_sch_dorefresh = Output(UInt(32.W))
  val ght_buffer_status = Input(UInt(2.W))

  val ght_satp_ppn  = Input(UInt(44.W))
  val ght_sys_mode  = Input(UInt(2.W))
  val if_correct_process = Output(UInt(1.W))
  
  val debug_mcounter = Input(UInt(64.W))
  val debug_icounter = Input(UInt(64.W))
  val debug_gcounter = Input(UInt(64.W))

  val debug_bp_checker = Input(UInt(64.W))
  val debug_bp_cdc = Input(UInt(64.W))
  val debug_bp_filter = Input(UInt(64.W))
  val fi_latency = Input(UInt(64.W))

  /* R Features */
  val t_value_out = Output(UInt(15.W))
  val icctrl_out = Output(UInt(4.W))
  val arf_copy_out = Output(UInt(1.W))
  val rsu_status_in = Input(UInt(2.W))
  val s_or_r_out = Output(UInt(2.W))
  val elu_data_in = Input(UInt(264.W))
  val elu_deq_out = Output(UInt(1.W))
  val elu_sel_out = Output(UInt(1.W))
  val record_pc_out = Output(UInt(1.W))
  val elu_status_in = Input(UInt(2.W))
  val gtimer_reset_out = Output(UInt(1.W))
  //===== GuardianCouncil Function: End   ====//
  val csrs = Input(Vec(nRoCCCSRs, new CustomCSRIO))
}

class RoCCIO(val nPTWPorts: Int, nRoCCCSRs: Int)(implicit p: Parameters) extends RoCCCoreIO(nRoCCCSRs)(p) {
  val ptw = Vec(nPTWPorts, new TLBPTWIO)
  val fpu_req = Decoupled(new FPInput)
  val fpu_resp = Flipped(Decoupled(new FPResult))
}

/** Base classes for Diplomatic TL2 RoCC units **/
abstract class LazyRoCC(
  val opcodes: OpcodeSet,
  val nPTWPorts: Int = 0,
  val usesFPU: Boolean = false,
  val roccCSRs: Seq[CustomCSR] = Nil
)(implicit p: Parameters) extends LazyModule {
  val module: LazyRoCCModuleImp
  require(roccCSRs.map(_.id).toSet.size == roccCSRs.size)
  val atlNode: TLNode = TLIdentityNode()
  val tlNode: TLNode = TLIdentityNode()
}

class LazyRoCCModuleImp(outer: LazyRoCC) extends LazyModuleImp(outer) {
  val io = IO(new RoCCIO(outer.nPTWPorts, outer.roccCSRs.size))
}

/** Mixins for including RoCC **/

trait HasLazyRoCC extends CanHavePTW { this: BaseTile =>
  val roccs = p(BuildRoCC).map(_(p))
  val roccCSRs = roccs.map(_.roccCSRs) // the set of custom CSRs requested by all roccs
  require(roccCSRs.flatten.map(_.id).toSet.size == roccCSRs.flatten.size,
    "LazyRoCC instantiations require overlapping CSRs")
  roccs.map(_.atlNode).foreach { atl => tlMasterXbar.node :=* atl }
  roccs.map(_.tlNode).foreach { tl => tlOtherMastersNode :=* tl }

  nPTWPorts += roccs.map(_.nPTWPorts).sum
  nDCachePorts += roccs.size
}

trait HasLazyRoCCModule extends CanHavePTWModule
    with HasCoreParameters { this: RocketTileModuleImp with HasFpuOpt =>

  val (respArb, cmdRouter) = if(outer.roccs.nonEmpty) {
    val respArb = Module(new RRArbiter(new RoCCResponse()(outer.p), outer.roccs.size))
    val cmdRouter = Module(new RoccCommandRouter(outer.roccs.map(_.opcodes))(outer.p))
    outer.roccs.zipWithIndex.foreach { case (rocc, i) =>
      rocc.module.io.ptw ++=: ptwPorts
      rocc.module.io.cmd <> cmdRouter.io.out(i)
      val dcIF = Module(new SimpleHellaCacheIF()(outer.p))
      dcIF.io.requestor <> rocc.module.io.mem
      dcachePorts += dcIF.io.cache
      respArb.io.in(i) <> Queue(rocc.module.io.resp)
    //===== GuardianCouncil Function: Start ====//
      rocc.module.io.ghe_packet_in := cmdRouter.io.ghe_packet_in
      rocc.module.io.ghe_status_in := cmdRouter.io.ghe_status_in
      rocc.module.io.bigcore_comp  := cmdRouter.io.bigcore_comp
      cmdRouter.io.ght_mask_in := rocc.module.io.ght_mask_out
      cmdRouter.io.ght_status_in := rocc.module.io.ght_status_out
      cmdRouter.io.ghe_event_in := rocc.module.io.ghe_event_out
      cmdRouter.io.ght_cfg_in := rocc.module.io.ght_cfg_out
      cmdRouter.io.ght_cfg_valid_in := rocc.module.io.ght_cfg_valid
      cmdRouter.io.debug_bp_reset_in := rocc.module.io.debug_bp_reset

      cmdRouter.io.agg_packet_in := rocc.module.io.agg_packet_out
      cmdRouter.io.report_fi_detection_in := rocc.module.io.report_fi_detection_out
      rocc.module.io.agg_buffer_full := cmdRouter.io.agg_buffer_full
      cmdRouter.io.agg_core_status_in := rocc.module.io.agg_core_status
      cmdRouter.io.ght_sch_na_in := rocc.module.io.ght_sch_na
      rocc.module.io.ght_sch_refresh := cmdRouter.io.ght_sch_refresh
      rocc.module.io.ght_buffer_status := cmdRouter.io.ght_buffer_status
      cmdRouter.io.ght_sch_dorefresh_in := rocc.module.io.ght_sch_dorefresh
      cmdRouter.io.if_correct_process_in := rocc.module.io.if_correct_process

      /* R Features */
      cmdRouter.io.icctrl_in := rocc.module.io.icctrl_out
      cmdRouter.io.t_value_in := rocc.module.io.t_value_out
      cmdRouter.io.s_or_r_in := rocc.module.io.s_or_r_out
      cmdRouter.io.arf_copy_in := rocc.module.io.arf_copy_out
      cmdRouter.io.record_pc_in := rocc.module.io.record_pc_out
      cmdRouter.io.gtimer_reset_in := rocc.module.io.gtimer_reset_out
      rocc.module.io.rsu_status_in := cmdRouter.io.rsu_status_in
      rocc.module.io.ght_satp_ppn := cmdRouter.io.ght_satp_ppn
      rocc.module.io.ght_sys_mode := cmdRouter.io.ght_sys_mode
      rocc.module.io.elu_data_in := cmdRouter.io.elu_data_in
      cmdRouter.io.elu_deq_in := rocc.module.io.elu_deq_out
      cmdRouter.io.elu_sel_in := rocc.module.io.elu_sel_out
      rocc.module.io.elu_status_in := cmdRouter.io.elu_status_in
      //===== GuardianCouncil Function: End   ====//
    }

    fpuOpt foreach { fpu =>
      val nFPUPorts = outer.roccs.count(_.usesFPU)
      if (usingFPU && nFPUPorts > 0) {
        val fpArb = Module(new InOrderArbiter(new FPInput()(outer.p), new FPResult()(outer.p), nFPUPorts))
        val fp_rocc_ios = outer.roccs.filter(_.usesFPU).map(_.module.io)
        fpArb.io.in_req <> fp_rocc_ios.map(_.fpu_req)
        fp_rocc_ios.zip(fpArb.io.in_resp).foreach {
          case (rocc, arb) => rocc.fpu_resp <> arb
        }
        fpu.io.cp_req <> fpArb.io.out_req
        fpArb.io.out_resp <> fpu.io.cp_resp
      } else {
        fpu.io.cp_req.valid := false.B
        fpu.io.cp_resp.ready := false.B
      }
    }
    (Some(respArb), Some(cmdRouter))
  } else {
    (None, None)
  }
  val roccCSRIOs = outer.roccs.map(_.module.io.csrs)
}

class AccumulatorExample(opcodes: OpcodeSet, val n: Int = 4)(implicit p: Parameters) extends LazyRoCC(opcodes) {
  override lazy val module = new AccumulatorExampleModuleImp(this)
}

class AccumulatorExampleModuleImp(outer: AccumulatorExample)(implicit p: Parameters) extends LazyRoCCModuleImp(outer)
    with HasCoreParameters {
  val regfile = Mem(outer.n, UInt(xLen.W))
  val busy = RegInit(VecInit(Seq.fill(outer.n){false.B}))

  val cmd = Queue(io.cmd)
  val funct = cmd.bits.inst.funct
  val addr = cmd.bits.rs2(log2Up(outer.n)-1,0)
  val doWrite = funct === 0.U
  val doRead = funct === 1.U
  val doLoad = funct === 2.U
  val doAccum = funct === 3.U
  val memRespTag = io.mem.resp.bits.tag(log2Up(outer.n)-1,0)

  // datapath
  val addend = cmd.bits.rs1
  val accum = regfile(addr)
  val wdata = Mux(doWrite, addend, accum + addend)

  when (cmd.fire() && (doWrite || doAccum)) {
    regfile(addr) := wdata
  }

  when (io.mem.resp.valid) {
    regfile(memRespTag) := io.mem.resp.bits.data
    busy(memRespTag) := false.B
  }

  // control
  when (io.mem.req.fire()) {
    busy(addr) := true.B
  }

  val doResp = cmd.bits.inst.xd
  val stallReg = busy(addr)
  val stallLoad = doLoad && !io.mem.req.ready
  val stallResp = doResp && !io.resp.ready

  cmd.ready := !stallReg && !stallLoad && !stallResp
    // command resolved if no stalls AND not issuing a load that will need a request

  // PROC RESPONSE INTERFACE
  io.resp.valid := cmd.valid && doResp && !stallReg && !stallLoad
    // valid response if valid command, need a response, and no stalls
  io.resp.bits.rd := cmd.bits.inst.rd
    // Must respond with the appropriate tag or undefined behavior
  io.resp.bits.data := accum
    // Semantics is to always send out prior accumulator register value

  io.busy := cmd.valid || busy.reduce(_||_)
    // Be busy when have pending memory requests or committed possibility of pending requests
  io.interrupt := false.B
    // Set this true to trigger an interrupt on the processor (please refer to supervisor documentation)

  // MEMORY REQUEST INTERFACE
  io.mem.req.valid := cmd.valid && doLoad && !stallReg && !stallResp
  io.mem.req.bits.addr := addend
  io.mem.req.bits.tag := addr
  io.mem.req.bits.cmd := M_XRD // perform a load (M_XWR for stores)
  io.mem.req.bits.size := log2Ceil(8).U
  io.mem.req.bits.signed := false.B
  io.mem.req.bits.data := 0.U // we're not performing any stores...
  io.mem.req.bits.phys := false.B
  io.mem.req.bits.dprv := cmd.bits.status.dprv
  io.mem.req.bits.dv := cmd.bits.status.dv
}

class  TranslatorExample(opcodes: OpcodeSet)(implicit p: Parameters) extends LazyRoCC(opcodes, nPTWPorts = 1) {
  override lazy val module = new TranslatorExampleModuleImp(this)
}

class TranslatorExampleModuleImp(outer: TranslatorExample)(implicit p: Parameters) extends LazyRoCCModuleImp(outer)
    with HasCoreParameters {
  val req_addr = Reg(UInt(coreMaxAddrBits.W))
  val req_rd = Reg(chiselTypeOf(io.resp.bits.rd))
  val req_offset = req_addr(pgIdxBits - 1, 0)
  val req_vpn = req_addr(coreMaxAddrBits - 1, pgIdxBits)
  val pte = Reg(new PTE)

  val s_idle :: s_ptw_req :: s_ptw_resp :: s_resp :: Nil = Enum(4)
  val state = RegInit(s_idle)

  io.cmd.ready := (state === s_idle)

  when (io.cmd.fire()) {
    req_rd := io.cmd.bits.inst.rd
    req_addr := io.cmd.bits.rs1
    state := s_ptw_req
  }

  private val ptw = io.ptw(0)

  when (ptw.req.fire()) { state := s_ptw_resp }

  when (state === s_ptw_resp && ptw.resp.valid) {
    pte := ptw.resp.bits.pte
    state := s_resp
  }

  when (io.resp.fire()) { state := s_idle }

  ptw.req.valid := (state === s_ptw_req)
  ptw.req.bits.valid := true.B
  ptw.req.bits.bits.addr := req_vpn

  io.resp.valid := (state === s_resp)
  io.resp.bits.rd := req_rd
  io.resp.bits.data := Mux(pte.leaf(), Cat(pte.ppn, req_offset), -1.S(xLen.W).asUInt)

  io.busy := (state =/= s_idle)
  io.interrupt := false.B
  io.mem.req.valid := false.B
}

class  CharacterCountExample(opcodes: OpcodeSet)(implicit p: Parameters) extends LazyRoCC(opcodes) {
  override lazy val module = new CharacterCountExampleModuleImp(this)
  override val atlNode = TLClientNode(Seq(TLMasterPortParameters.v1(Seq(TLMasterParameters.v1("CharacterCountRoCC")))))
}

class CharacterCountExampleModuleImp(outer: CharacterCountExample)(implicit p: Parameters) extends LazyRoCCModuleImp(outer)
  with HasCoreParameters
  with HasL1CacheParameters {
  val cacheParams = tileParams.dcache.get

  private val blockOffset = blockOffBits
  private val beatOffset = log2Up(cacheDataBits/8)

  val needle = Reg(UInt(8.W))
  val addr = Reg(UInt(coreMaxAddrBits.W))
  val count = Reg(UInt(xLen.W))
  val resp_rd = Reg(chiselTypeOf(io.resp.bits.rd))

  val addr_block = addr(coreMaxAddrBits - 1, blockOffset)
  val offset = addr(blockOffset - 1, 0)
  val next_addr = (addr_block + 1.U) << blockOffset.U

  val s_idle :: s_acq :: s_gnt :: s_check :: s_resp :: Nil = Enum(5)
  val state = RegInit(s_idle)

  val (tl_out, edgesOut) = outer.atlNode.out(0)
  val gnt = tl_out.d.bits
  val recv_data = Reg(UInt(cacheDataBits.W))
  val recv_beat = RegInit(0.U(log2Up(cacheDataBeats+1).W))

  val data_bytes = VecInit(Seq.tabulate(cacheDataBits/8) { i => recv_data(8 * (i + 1) - 1, 8 * i) })
  val zero_match = data_bytes.map(_ === 0.U)
  val needle_match = data_bytes.map(_ === needle)
  val first_zero = PriorityEncoder(zero_match)

  val chars_found = PopCount(needle_match.zipWithIndex.map {
    case (matches, i) =>
      val idx = Cat(recv_beat - 1.U, i.U(beatOffset.W))
      matches && idx >= offset && i.U <= first_zero
  })
  val zero_found = zero_match.reduce(_ || _)
  val finished = Reg(Bool())

  io.cmd.ready := (state === s_idle)
  io.resp.valid := (state === s_resp)
  io.resp.bits.rd := resp_rd
  io.resp.bits.data := count
  tl_out.a.valid := (state === s_acq)
  tl_out.a.bits := edgesOut.Get(
                       fromSource = 0.U,
                       toAddress = addr_block << blockOffset,
                       lgSize = lgCacheBlockBytes.U)._2
  tl_out.d.ready := (state === s_gnt)

  when (io.cmd.fire()) {
    addr := io.cmd.bits.rs1
    needle := io.cmd.bits.rs2
    resp_rd := io.cmd.bits.inst.rd
    count := 0.U
    finished := false.B
    state := s_acq
  }

  when (tl_out.a.fire()) { state := s_gnt }

  when (tl_out.d.fire()) {
    recv_beat := recv_beat + 1.U
    recv_data := gnt.data
    state := s_check
  }

  when (state === s_check) {
    when (!finished) {
      count := count + chars_found
    }
    when (zero_found) { finished := true.B }
    when (recv_beat === cacheDataBeats.U) {
      addr := next_addr
      state := Mux(zero_found || finished, s_resp, s_acq)
      recv_beat := 0.U
    } .otherwise {
      state := s_gnt
    }
  }

  when (io.resp.fire()) { state := s_idle }

  io.busy := (state =/= s_idle)
  io.interrupt := false.B
  io.mem.req.valid := false.B
  // Tie off unused channels
  tl_out.b.ready := true.B
  tl_out.c.valid := false.B
  tl_out.e.valid := false.B
}

class BlackBoxExample(opcodes: OpcodeSet, blackBoxFile: String)(implicit p: Parameters)
    extends LazyRoCC(opcodes) {
  override lazy val module = new BlackBoxExampleModuleImp(this, blackBoxFile)
}

class BlackBoxExampleModuleImp(outer: BlackBoxExample, blackBoxFile: String)(implicit p: Parameters)
    extends LazyRoCCModuleImp(outer)
    with RequireSyncReset
    with HasCoreParameters {

  val blackbox = {
    val roccIo = io
    Module(
      new BlackBox( Map( "xLen" -> IntParam(xLen),
                         "PRV_SZ" -> IntParam(PRV.SZ),
                         "coreMaxAddrBits" -> IntParam(coreMaxAddrBits),
                         "dcacheReqTagBits" -> IntParam(roccIo.mem.req.bits.tag.getWidth),
                         "M_SZ" -> IntParam(M_SZ),
                         "mem_req_bits_size_width" -> IntParam(roccIo.mem.req.bits.size.getWidth),
                         "coreDataBits" -> IntParam(coreDataBits),
                         "coreDataBytes" -> IntParam(coreDataBytes),
                         "paddrBits" -> IntParam(paddrBits),
                         "vaddrBitsExtended" -> IntParam(vaddrBitsExtended),
                         "FPConstants_RM_SZ" -> IntParam(FPConstants.RM_SZ),
                         "fLen" -> IntParam(fLen),
                         "FPConstants_FLAGS_SZ" -> IntParam(FPConstants.FLAGS_SZ)
                   ) ) with HasBlackBoxResource {
        val io = IO( new Bundle {
                      val clock = Input(Clock())
                      val reset = Input(Reset())
                      val rocc = chiselTypeOf(roccIo)
                    })
        override def desiredName: String = blackBoxFile
        addResource(s"/vsrc/$blackBoxFile.v")
      }
    )
  }

  blackbox.io.clock := clock
  blackbox.io.reset := reset
  blackbox.io.rocc.cmd <> io.cmd
  io.resp <> blackbox.io.rocc.resp
  io.mem <> blackbox.io.rocc.mem
  io.busy := blackbox.io.rocc.busy
  io.interrupt := blackbox.io.rocc.interrupt
  blackbox.io.rocc.exception := io.exception
  io.ptw <> blackbox.io.rocc.ptw
  io.fpu_req <> blackbox.io.rocc.fpu_req
  blackbox.io.rocc.fpu_resp <> io.fpu_resp

}

class OpcodeSet(val opcodes: Seq[UInt]) {
  def |(set: OpcodeSet) =
    new OpcodeSet(this.opcodes ++ set.opcodes)

  def matches(oc: UInt) = opcodes.map(_ === oc).reduce(_ || _)
}

object OpcodeSet {
  def custom0 = new OpcodeSet(Seq("b0001011".U))
  def custom1 = new OpcodeSet(Seq("b0101011".U))
  def custom2 = new OpcodeSet(Seq("b1011011".U))
  def custom3 = new OpcodeSet(Seq("b1111011".U))
  def all = custom0 | custom1 | custom2 | custom3
}

class RoccCommandRouter(opcodes: Seq[OpcodeSet])(implicit p: Parameters)
    extends CoreModule()(p) {
  val io = new Bundle {
    val in = Flipped(Decoupled(new RoCCCommand))
    val out = Vec(opcodes.size, Decoupled(new RoCCCommand))
    val busy = Output(Bool())
    //===== GuardianCouncil Function: Start ====//
    val ghe_packet_in = Input(UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W))
    val ghe_status_in = Input(UInt(32.W))
    val bigcore_comp  = Input(UInt(3.W))
    val ghe_event_in = Input(UInt(5.W))
    val ghe_event_out = Output(UInt(5.W))
    val ght_mask_out  = Output(UInt(1.W))
    val ght_mask_in = Input(UInt(1.W))
    val ght_status_out  = Output(UInt(32.W))
    val ght_status_in = Input(UInt(32.W))
    val ght_cfg_out = Output(UInt(32.W))
    val ght_cfg_in = Input(UInt(32.W))
    val ght_cfg_valid = Output(UInt(1.W))
    val ght_cfg_valid_in = Input(UInt(1.W))

    val debug_bp_reset = Output(UInt(1.W))
    val debug_bp_reset_in = Input(UInt(1.W))

    val agg_packet_out = Output(UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W))
    val agg_packet_in  = Input(UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W))
    val report_fi_detection_out = Output(UInt(57.W))
    val report_fi_detection_in  = Input(UInt(57.W))
    val agg_buffer_full = Input(UInt(1.W))
    val agg_core_status_out = Output(UInt(2.W))
    val agg_core_status_in = Input(UInt(2.W))

    val ght_sch_na_in = Input(UInt(1.W))
    val ght_sch_na_out = Output(UInt(1.W))
    val ght_sch_refresh = Input(UInt(1.W))
    val ght_buffer_status = Input(UInt(2.W))

    val ght_sch_dorefresh_in = Input(UInt(32.W))
    val ght_sch_dorefresh_out = Output(UInt(32.W))

    val if_correct_process_in = Input(UInt(1.W))
    val if_correct_process_out = Output(UInt(1.W))

    /* R Features */
    val icctrl_out = Output(UInt(4.W))
    val icctrl_in = Input(UInt(4.W))
    val t_value_out = Output(UInt(15.W))
    val t_value_in = Input(UInt(15.W))
    val arf_copy_out = Output(UInt(1.W))
    val arf_copy_in = Input(UInt(1.W))
    val record_pc_out = Output(UInt(1.W))
    val record_pc_in = Input(UInt(1.W))
    val gtimer_reset_out = Output(UInt(1.W))
    val gtimer_reset_in = Input(UInt(1.W))

    val rsu_status_in = Input(UInt(2.W))
    val s_or_r_out = Output(UInt(2.W))
    val s_or_r_in = Input(UInt(2.W))
    val ght_satp_ppn  = Input(UInt(44.W))
    val ght_sys_mode  = Input(UInt(2.W))
    val elu_data_in = Input(UInt(264.W))
    val elu_deq_out = Output(UInt(1.W))
    val elu_deq_in = Input(UInt(1.W))
    val elu_sel_out = Output(UInt(1.W))
    val elu_sel_in = Input(UInt(1.W))
    val elu_status_in = Input(UInt(2.W))
    //===== GuardianCouncil Function: End   ====//
  }

  val cmd = io.in
  val cmdReadys = io.out.zip(opcodes).map { case (out, opcode) =>
    val me = opcode.matches(cmd.bits.inst.opcode)
    out.valid := cmd.valid && me
    out.bits := cmd.bits
    out.ready && me
  }
  cmd.ready := cmdReadys.reduce(_ || _)
  io.busy := cmd.valid
  //===== GuardianCouncil Function: Start ====//
  io.ghe_event_out := io.ghe_event_in
  io.ght_mask_out := io.ght_mask_in
  io.ght_status_out := io.ght_status_in
  io.ght_cfg_out := io.ght_cfg_in
  io.ght_cfg_valid := io.ght_cfg_valid_in
  io.debug_bp_reset := io.debug_bp_reset_in

  io.agg_packet_out := io.agg_packet_in
  io.report_fi_detection_out := io.report_fi_detection_in
  io.agg_core_status_out := io.agg_core_status_in
  io.ght_sch_na_out := io.ght_sch_na_in
  io.ght_sch_dorefresh_out := io.ght_sch_dorefresh_in

  io.if_correct_process_out := io.if_correct_process_in

  /* R Features */
  io.icctrl_out := io.icctrl_in
  io.t_value_out := io.t_value_in
  io.arf_copy_out := io.arf_copy_in
  io.record_pc_out := io.record_pc_in
  io.gtimer_reset_out := io.gtimer_reset_in
  io.s_or_r_out := io.s_or_r_in
  io.elu_deq_out := io.elu_deq_in
  io.elu_sel_out := io.elu_sel_in
  //===== GuardianCouncil Function: End   ====//
  assert(PopCount(cmdReadys) <= 1.U,
    "Custom opcode matched for more than one accelerator")
}

class RoccCommandRouterBoom(opcodes: Seq[OpcodeSet])(implicit p: Parameters)
    extends CoreModule()(p) {
  val io = new Bundle {
    val in = Flipped(Decoupled(new RoCCCommand))
    val out = Vec(opcodes.size, Decoupled(new RoCCCommand))
    val busy = Output(Bool())
    //===== GuardianCouncil Function: Start ====//
    val ghe_packet_in = Input(UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W))
    val ghe_status_in = Input(UInt(32.W))
    val bigcore_comp  = Input(UInt(3.W))
    val ghe_event_in = Input(UInt(5.W))
    val ghe_event_out = Output(UInt(5.W))
    val ght_mask_out  = Output(UInt(1.W))
    val ght_mask_in = Input(UInt(1.W))
    val ght_status_out  = Output(UInt(32.W))
    val ght_status_in = Input(UInt(32.W))
    val ght_cfg_out = Output(UInt(32.W))
    val ght_cfg_in = Input(UInt(32.W))
    val ght_cfg_valid = Output(UInt(1.W))
    val ght_cfg_valid_in = Input(UInt(1.W))

    val debug_bp_reset = Output(UInt(1.W))
    val debug_bp_reset_in = Input(UInt(1.W))
    

    val agg_packet_out = Output(UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W))
    val agg_packet_in  = Input(UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W))
    val agg_buffer_full = Input(UInt(1.W))
    val agg_core_status_out = Output(UInt(2.W))
    val agg_core_status_in = Input(UInt(2.W))

    val ght_sch_na_in = Input(UInt(1.W))
    val ght_sch_na_out = Output(UInt(1.W))
    val ght_sch_refresh = Input(UInt(1.W))
    val ght_buffer_status = Input(UInt(2.W))

    val ght_sch_dorefresh_in = Input(UInt(32.W))
    val ght_sch_dorefresh_out = Output(UInt(32.W))

    val ght_satp_ppn  = Input(UInt(44.W))
    val ght_sys_mode  = Input(UInt(2.W))

    val if_correct_process_in = Input(UInt(1.W))
    val if_correct_process_out = Output(UInt(1.W))

    val debug_mcounter  = Input(UInt(64.W))
    val debug_icounter  = Input(UInt(64.W))
    val debug_gcounter  = Input(UInt(64.W))

    val debug_bp_checker = Input(UInt(64.W))
    val debug_bp_cdc = Input(UInt(64.W))
    val debug_bp_filter = Input(UInt(64.W))

    /* R Features */
    val icctrl_out = Output(UInt(4.W))
    val icctrl_in = Input(UInt(4.W))
    val t_value_out = Output(UInt(15.W))
    val t_value_in = Input(UInt(15.W))
    val arf_copy_out = Output(UInt(1.W))
    val arf_copy_in = Input(UInt(1.W))
    val s_or_r_out = Output(UInt(2.W))
    val s_or_r_in = Input(UInt(2.W))
    val gtimer_reset_out = Output(UInt(1.W))
    val gtimer_reset_in = Input(UInt(1.W))
    val fi_sel_out = Output(UInt(8.W))
    val fi_sel_in  = Input(UInt(8.W))
    val fi_latency = Input(UInt(64.W))
    val rsu_status_in = Input(UInt(2.W))
    //===== GuardianCouncil Function: End   ====//
  }

  val cmd = Queue(io.in)
  val cmdReadys = io.out.zip(opcodes).map { case (out, opcode) =>
    val me = opcode.matches(cmd.bits.inst.opcode)
    out.valid := cmd.valid && me
    out.bits := cmd.bits
    out.ready && me
  }
  cmd.ready := cmdReadys.reduce(_ || _)
  io.busy := cmd.valid
  //===== GuardianCouncil Function: Start ====//
  io.ghe_event_out := io.ghe_event_in
  io.ght_mask_out := io.ght_mask_in
  io.ght_status_out := io.ght_status_in
  io.ght_cfg_out := io.ght_cfg_in
  io.ght_cfg_valid := io.ght_cfg_valid_in
  io.debug_bp_reset := io.debug_bp_reset_in

  io.agg_packet_out := io.agg_packet_in
  io.agg_core_status_out := io.agg_core_status_in
  io.ght_sch_na_out := io.ght_sch_na_in
  io.ght_sch_dorefresh_out := io.ght_sch_dorefresh_in
  io.if_correct_process_out := io.if_correct_process_in

  /* R Features */
  io.icctrl_out := io.icctrl_in
  io.gtimer_reset_out := io.gtimer_reset_in
  io.fi_sel_out := io.fi_sel_in
  io.t_value_out := io.t_value_in
  io.s_or_r_out := io.s_or_r_in
  io.arf_copy_out := io.arf_copy_in
  //===== GuardianCouncil Function: End   ====//

  assert(PopCount(cmdReadys) <= 1.U,
    "Custom opcode matched for more than one accelerator")
}
