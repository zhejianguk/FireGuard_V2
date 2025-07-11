// See LICENSE.SiFive for license details.

package freechips.rocketchip.tile

import Chisel._

import org.chipsalliance.cde.config._
import freechips.rocketchip.rocket._
import freechips.rocketchip.util._

//===== GuardianCouncil Function: Start ====//
import freechips.rocketchip.guardiancouncil._
//===== GuardianCouncil Function: End   ====//

case object XLen extends Field[Int]
case object MaxHartIdBits extends Field[Int]

// These parameters can be varied per-core
trait CoreParams {
  val bootFreqHz: BigInt
  val useVM: Boolean
  val useHypervisor: Boolean
  val useUser: Boolean
  val useSupervisor: Boolean
  val useDebug: Boolean
  val useAtomics: Boolean
  val useAtomicsOnlyForIO: Boolean
  val useCompressed: Boolean
  val useBitManip: Boolean
  val useBitManipCrypto: Boolean
  val useVector: Boolean = false
  val useSCIE: Boolean
  val useCryptoNIST: Boolean
  val useCryptoSM: Boolean
  val useRVE: Boolean
  val useConditionalZero: Boolean
  val mulDiv: Option[MulDivParams]
  val fpu: Option[FPUParams]
  val fetchWidth: Int
  val decodeWidth: Int
  val retireWidth: Int
  val instBits: Int
  val nLocalInterrupts: Int
  val useNMI: Boolean
  val nPMPs: Int
  val pmpGranularity: Int
  val nBreakpoints: Int
  val useBPWatch: Boolean
  val mcontextWidth: Int
  val scontextWidth: Int
  val nPerfCounters: Int
  val haveBasicCounters: Boolean
  val haveFSDirty: Boolean
  val misaWritable: Boolean
  val haveCFlush: Boolean
  val nL2TLBEntries: Int
  val nL2TLBWays: Int
  val nPTECacheEntries: Int
  val mtvecInit: Option[BigInt]
  val mtvecWritable: Boolean
  val traceHasWdata: Boolean
  def traceCustom: Option[Data] = None
  def customIsaExt: Option[String] = None
  def customCSRs(implicit p: Parameters): CustomCSRs = new CustomCSRs

  def hasSupervisorMode: Boolean = useSupervisor || useVM
  def hasBitManipCrypto: Boolean = useBitManipCrypto || useCryptoNIST || useCryptoSM
  def instBytes: Int = instBits / 8
  def fetchBytes: Int = fetchWidth * instBytes
  def lrscCycles: Int

  def dcacheReqTagBits: Int = 6

  def minFLen: Int = 32
  def vLen: Int = 0
  def sLen: Int = 0
  def eLen(xLen: Int, fLen: Int): Int = xLen max fLen
  def vMemDataBits: Int = 0
}

trait HasCoreParameters extends HasTileParameters {
  val coreParams: CoreParams = tileParams.core

  val minFLen = coreParams.fpu.map(_ => coreParams.minFLen).getOrElse(0)
  val fLen = coreParams.fpu.map(_.fLen).getOrElse(0)

  val usingMulDiv = coreParams.mulDiv.nonEmpty
  val usingFPU = coreParams.fpu.nonEmpty
  val usingAtomics = coreParams.useAtomics
  val usingAtomicsOnlyForIO = coreParams.useAtomicsOnlyForIO
  val usingAtomicsInCache = usingAtomics && !usingAtomicsOnlyForIO
  val usingCompressed = coreParams.useCompressed
  val usingBitManip = coreParams.useBitManip
  val usingBitManipCrypto = coreParams.hasBitManipCrypto
  val usingVector = coreParams.useVector
  val usingSCIE = coreParams.useSCIE
  val usingCryptoNIST = coreParams.useCryptoNIST
  val usingCryptoSM = coreParams.useCryptoSM
  val usingNMI = coreParams.useNMI
  val usingConditionalZero = coreParams.useConditionalZero

  val retireWidth = coreParams.retireWidth
  val fetchWidth = coreParams.fetchWidth
  val decodeWidth = coreParams.decodeWidth

  val fetchBytes = coreParams.fetchBytes
  val coreInstBits = coreParams.instBits
  val coreInstBytes = coreInstBits/8
  val coreDataBits = xLen max fLen max vMemDataBits
  val coreDataBytes = coreDataBits/8
  def coreMaxAddrBits = paddrBits max vaddrBitsExtended

  val nBreakpoints = coreParams.nBreakpoints
  val nPMPs = coreParams.nPMPs
  val pmpGranularity = coreParams.pmpGranularity
  val nPerfCounters = coreParams.nPerfCounters
  val mtvecInit = coreParams.mtvecInit
  val mtvecWritable = coreParams.mtvecWritable
  val customIsaExt = coreParams.customIsaExt
  val traceHasWdata = coreParams.traceHasWdata

  def vLen = coreParams.vLen
  def sLen = coreParams.sLen
  def eLen = coreParams.eLen(xLen, fLen)
  def vMemDataBits = if (usingVector) coreParams.vMemDataBits else 0
  def maxVLMax = vLen

  if (usingVector) {
    require(isPow2(vLen), s"vLen ($vLen) must be a power of 2")
    require(eLen >= 32 && vLen % eLen == 0, s"eLen must divide vLen ($vLen) and be no less than 32")
    require(vMemDataBits >= eLen && vLen % vMemDataBits == 0, s"vMemDataBits ($vMemDataBits) must divide vLen ($vLen) and be no less than eLen ($eLen)")
  }

  lazy val hartIdLen: Int = p(MaxHartIdBits)
  lazy val resetVectorLen: Int = {
    val externalLen = paddrBits
    require(externalLen <= xLen, s"External reset vector length ($externalLen) must be <= XLEN ($xLen)")
    require(externalLen <= vaddrBitsExtended, s"External reset vector length ($externalLen) must be <= virtual address bit width ($vaddrBitsExtended)")
    externalLen
  }

  // Print out log of committed instructions and their writeback values.
  // Requires post-processing due to out-of-order writebacks.
  val enableCommitLog = false

}

abstract class CoreModule(implicit val p: Parameters) extends Module
  with HasCoreParameters

abstract class CoreBundle(implicit val p: Parameters) extends ParameterizedBundle()(p)
  with HasCoreParameters

class CoreInterrupts(implicit p: Parameters) extends TileInterrupts()(p) {
  val buserror = tileParams.beuAddr.map(a => Bool())
}

// This is a raw commit trace from the core, not the TraceCoreInterface
class TraceBundle(implicit val p: Parameters) extends Bundle with HasCoreParameters {
  val insns = Vec(coreParams.retireWidth, new TracedInstruction)
  val time = UInt(64.W)
  val custom = coreParams.traceCustom
}

trait HasCoreIO extends HasTileParameters {
  implicit val p: Parameters
  def nTotalRoCCCSRs: Int
  val io = new CoreBundle()(p) {
    val hartid = UInt(hartIdLen.W).asInput
    val reset_vector = UInt(resetVectorLen.W).asInput
    val interrupts = new CoreInterrupts().asInput
    val imem  = new FrontendIO
    val dmem = new HellaCacheIO
    val ptw = new DatapathPTWIO().flip
    val fpu = new FPUCoreIO().flip
    val rocc = new RoCCCoreIO(nTotalRoCCCSRs).flip
    val trace = Output(new TraceBundle)
    val bpwatch = Vec(coreParams.nBreakpoints, new BPWatch(coreParams.retireWidth)).asOutput
    val cease = Bool().asOutput
    val wfi = Bool().asOutput
    val traceStall = Bool().asInput
    //===== GuardianCouncil Function: Start ====//
    val arfs_if_CPS = UInt(1.W).asInput
    val record_pc = UInt(1.W).asInput
    val ic_counter = UInt(16.W).asInput
    val clear_ic_status = UInt(1.W).asOutput
    val pc = UInt(vaddrBitsExtended.W).asOutput
    val inst = UInt(32.W).asOutput
    val new_commit = UInt(1.W).asOutput
    val clk_enable_gh = Bool().asInput
    val alu_2cycle_delay = UInt(p(XLen).W).asOutput
    val csr_rw_wdata = UInt(p(XLen).W).asOutput

    val packet_arfs = UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W).asInput
    val packet_lsl = UInt(GH_GlobalParams.GH_WIDITH_PACKETS.W).asInput

    val packet_cdc_ready = UInt(1.W).asOutput
    val arf_copy_in = UInt(1.W).asInput
    val rsu_status = UInt(2.W).asOutput
    val s_or_r = UInt(1.W).asInput
    val lsl_near_full = UInt(1.W).asOutput
    val ght_prv = UInt(2.W).asOutput
    val if_correct_process = UInt(1.W).asInput
    val elu_data = UInt(264.W).asOutput
    val elu_deq = UInt(1.W).asInput
    val elu_sel = UInt(1.W).asInput
    val elu_status = UInt(2.W).asOutput
    val icsl_status = UInt(2.W).asOutput
    //===== GuardianCouncil Function: End ====//
  }
}
