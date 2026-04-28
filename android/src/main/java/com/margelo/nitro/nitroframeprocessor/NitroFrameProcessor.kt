package com.margelo.nitro.nitroframeprocessor
  
import com.facebook.proguard.annotations.DoNotStrip

@DoNotStrip
class NitroFrameProcessor : HybridNitroFrameProcessorSpec() {
  override fun multiply(a: Double, b: Double): Double {
    return a * b
  }
}
