import Foundation
import NitroModules
import cvie64

class NitroFrameProcessor: HybridNitroFrameProcessorSpec {
  private var isEnabled: Bool = true
  private var setting: Int32 = 0
  private var numThreads: Int32 = 1
  private var parameterFilePath: String = ""
  private var cvieHandle: HCVIE? = nil
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
    _ = deviceId.withCString { deviceIdCString in
      CVLMSetParameterValue(LM_INIT, deviceIdCString)
    }

    var productIds: UnsafeMutablePointer<UnsafePointer<CChar>?>?
    guard CVLMGetPossibleModules(nil, &productIds) == CVIE_E_OK, let productIds else {
      return false
    }

    var index: Int32 = 0
    while true {
      guard let productId = productIds[Int(index)] else { break }
      if productId.pointee == 0 { break }

      let result = activationKey.withCString { activationKeyCString in
        CVLMSetKey(nil, index, activationKeyCString)
      }
      if result == CVIE_E_OK {
        isLicenseActivated = true
        return true
      }
      index += 1
    }

    return false
  }

  public func processImage(
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

      let result = CVIEEnhanceNext(
        cvieHandle,
        &inputBytes,
        &outputBytes,
        setting
      )
      if result != CVIE_E_OK {
        return input
      }

      return ArrayBuffer.copy(data: Data(outputBytes))
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

    if var handle = cvieHandle {
      _ = CVIEDestroy(&handle)
      cvieHandle = nil
    }

    var handle: HCVIE? = nil
    guard CVIECreate(&handle, 0) == CVIE_E_OK else {
      throw NSError(domain: "NitroFrameProcessor", code: 1)
    }
    cvieHandle = handle

    guard isLicenseActivated else {
      throw NSError(domain: "NitroFrameProcessor", code: 2)
    }

    _ = CVIESetThreads(cvieHandle, numThreads)
    _ = parameterFilePath.withCString { path in
      CVIESetParameterFile(cvieHandle, path, nil)
    }
    _ = CVIEEnhanceSetup(cvieHandle, width, height, CVIE_DATA_U8, setting, nil)

    previousWidth = width
    previousHeight = height
    previousSetting = setting
    previousParameterFilePath = parameterFilePath
  }
}
