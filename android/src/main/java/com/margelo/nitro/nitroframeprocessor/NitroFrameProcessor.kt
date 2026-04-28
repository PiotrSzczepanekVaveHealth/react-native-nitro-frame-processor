package com.margelo.nitro.nitroframeprocessor
  
import com.facebook.proguard.annotations.DoNotStrip
import com.margelo.nitro.core.ArrayBuffer
import java.nio.ByteBuffer
import java.util.concurrent.atomic.AtomicBoolean
import se.contextvision.cvie.Cvie
import se.contextvision.cvie.CvieDataFlag
import se.contextvision.cvie.CvieLibrary
import se.contextvision.cvie.Cvlm

@DoNotStrip
class NitroFrameProcessor : HybridNitroFrameProcessorSpec() {
  private var isEnabled = true
  private var setting = 0
  private var numThreads = 1
  private var parameterFilePath = ""
  private var cvieHandle: CvieLibrary.HCVIE? = null
  private val isLicenseActivated = AtomicBoolean(false)

  private var previousWidth = -1
  private var previousHeight = -1
  private var previousSetting = -1
  private var previousParameterFilePath = ""

  override fun setEnabled(value: Boolean) {
    isEnabled = value
  }

  override fun setNumThreads(numThreads: Double) {
    this.numThreads = numThreads.toInt().coerceAtLeast(1)
  }

  override fun setSetting(setting: Double) {
    this.setting = setting.toInt()
  }

  override fun setParameterFilePath(path: String) {
    this.parameterFilePath = path
  }

  override fun activateLicense(activationKey: String, deviceId: String): Boolean {
    return try {
      Cvlm.SetParameterValue(Cvlm.LM_INIT, deviceId)
      val modules = Cvlm.GetPossibleModules()
      for (index in modules.indices) {
        if (Cvlm.SetKey(index, activationKey)) {
          isLicenseActivated.set(true)
          return true
        }
      }
      false
    } catch (_: Throwable) {
      false
    }
  }

  override fun processFrame(width: Double, height: Double, input: ArrayBuffer): ArrayBuffer {
    if (!isEnabled) return input

    val imageWidth = width.toInt()
    val imageHeight = height.toInt()
    if (imageWidth <= 0 || imageHeight <= 0) return input
    if (parameterFilePath.isEmpty()) return input

    return try {
      ensureInstanceInitialized(imageWidth, imageHeight)
      val inBuffer = input.getBuffer(copyIfNeeded = true)
      val outBuffer = ByteBuffer.allocateDirect(inBuffer.capacity())
      Cvie.CVIEEnhanceNext(requireNotNull(cvieHandle), inBuffer, outBuffer, setting)
      outBuffer.rewind()
      ArrayBuffer.copy(outBuffer)
    } catch (_: Throwable) {
      input
    }
  }

  private fun ensureInstanceInitialized(width: Int, height: Int) {
    val needsRecreate =
      cvieHandle == null ||
        previousWidth != width ||
        previousHeight != height ||
        previousSetting != setting ||
        previousParameterFilePath != parameterFilePath

    if (!needsRecreate) return

    cvieHandle?.let { Cvie.CVIEDestroy(it) }
    cvieHandle = Cvie.CVIECreate(0)

    if (!isLicenseActivated.get()) {
      throw IllegalStateException("CVIE license is not activated.")
    }

    val handle = requireNotNull(cvieHandle)
    Cvie.CVIESetThreads(handle, numThreads)
    Cvie.CVIESetParameterFile(handle, parameterFilePath)
    Cvie.CVIEEnhanceSetup(handle, width, height, CvieDataFlag.U8.code, setting)

    previousWidth = width
    previousHeight = height
    previousSetting = setting
    previousParameterFilePath = parameterFilePath
  }

}
