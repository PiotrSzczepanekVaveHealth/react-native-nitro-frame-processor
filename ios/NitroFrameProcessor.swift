import Foundation
import NitroModules

@_silgen_name("CVIEBridgeActivateLicense")
private func CVIEBridgeActivateLicense(
  _ activationKey: UnsafePointer<CChar>?,
  _ deviceId: UnsafePointer<CChar>?
) -> Bool

@_silgen_name("CVIEBridgeCreate")
private func CVIEBridgeCreate(
  _ handleOut: UnsafeMutablePointer<UnsafeMutableRawPointer?>?
) -> Bool

@_silgen_name("CVIEBridgeDestroy")
private func CVIEBridgeDestroy(_ handle: UnsafeMutableRawPointer?)

@_silgen_name("CVIEBridgeConfigure")
private func CVIEBridgeConfigure(
  _ handle: UnsafeMutableRawPointer?,
  _ threads: Int32,
  _ parameterFilePath: UnsafePointer<CChar>?,
  _ width: Int32,
  _ height: Int32,
  _ setting: Int32
) -> Bool

@_silgen_name("CVIEBridgeEnhanceNextU8")
private func CVIEBridgeEnhanceNextU8(
  _ handle: UnsafeMutableRawPointer?,
  _ inputBytes: UnsafePointer<UInt8>?,
  _ outputBytes: UnsafeMutablePointer<UInt8>?,
  _ setting: Int32
) -> Bool

class NitroFrameProcessor: HybridNitroFrameProcessorSpec {
  private var isEnabled: Bool = true
  private var setting: Int32 = 0
  private var numThreads: Int32 = 1
  private var parameterFilePath: String = ""
  private var cvieHandle: UnsafeMutableRawPointer? = nil
  private var isLicenseActivated: Bool = false

  private var previousWidth: Int32 = -1
  private var previousHeight: Int32 = -1
  private var previousSetting: Int32 = -1
  private var previousParameterFilePath: String = ""

  public func setEnabled(value: Bool) throws {
    isEnabled = value
  }

  public func setNumThreads(numThreads: Double) throws {
    self.numThreads = Int32(max(1, Int(numThreads)))
  }

  public func setSetting(setting: Double) throws {
    self.setting = Int32(setting)
  }

  public func setParameterFilePath(path: String) throws {
    self.parameterFilePath = path
  }

  public func activateLicense(activationKey: String, deviceId: String) throws -> Bool {
    let activated = activationKey.withCString { activationKeyCString in
      deviceId.withCString { deviceIdCString in
        CVIEBridgeActivateLicense(activationKeyCString, deviceIdCString)
      }
    }
    isLicenseActivated = activated
    return activated
  }

  public func processFrame(
    width: Double,
    height: Double,
    input: ArrayBuffer
  ) throws -> ArrayBuffer {
    if !isEnabled { return input }

    let imageWidth = Int32(width)
    let imageHeight = Int32(height)
    if imageWidth <= 0 || imageHeight <= 0 { return input }
    if parameterFilePath.isEmpty { return input }

    do {
      try ensureInstanceInitialized(width: imageWidth, height: imageHeight)
      let inputData = input.toData(copyIfNeeded: true)
      var inputBytes = [UInt8](inputData)
      var outputBytes = [UInt8](repeating: 0, count: inputBytes.count)

      let result = CVIEBridgeEnhanceNextU8(
        cvieHandle,
        &inputBytes,
        &outputBytes,
        setting
      )
      if !result {
        return input
      }

      return try ArrayBuffer.copy(data: Data(outputBytes))
    } catch {
      return input
    }
  }

  private func ensureInstanceInitialized(width: Int32, height: Int32) throws {
    let needsRecreate =
      cvieHandle == nil ||
      previousWidth != width ||
      previousHeight != height ||
      previousSetting != setting ||
      previousParameterFilePath != parameterFilePath

    if !needsRecreate { return }

    if let handle = cvieHandle {
      CVIEBridgeDestroy(handle)
      cvieHandle = nil
    }

    var handle: UnsafeMutableRawPointer?
    guard CVIEBridgeCreate(&handle) else {
      throw NSError(domain: "NitroFrameProcessor", code: 1)
    }
    cvieHandle = handle

    guard isLicenseActivated else {
      throw NSError(domain: "NitroFrameProcessor", code: 2)
    }

    let configured = parameterFilePath.withCString { path in
      CVIEBridgeConfigure(
        cvieHandle,
        Int32(numThreads),
        path,
        width,
        height,
        setting
      )
    }
    if !configured {
      throw NSError(domain: "NitroFrameProcessor", code: 3)
    }

    previousWidth = width
    previousHeight = height
    previousSetting = setting
    previousParameterFilePath = parameterFilePath
  }
}
