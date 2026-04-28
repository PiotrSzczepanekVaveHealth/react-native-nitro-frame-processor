export function setEnabled(_value: boolean): void {}

export function setNumThreads(_numThreads: number): void {}

export function setSetting(_setting: number): void {}

export function setParameterFilePath(_path: string): void {}

export function activateLicense(
  _activationKey: string,
  _deviceId: string
): boolean {
  return false;
}

export function processImage(
  _width: number,
  _height: number,
  input: ArrayBuffer
): ArrayBuffer {
  return input;
}
