# react-native-nitro-frame-processor

Native React Native frame processor built with [Nitro Modules](https://nitro.margelo.com/) and ContextVision CVIE.

The library accepts ultrasound frame payloads as `ArrayBuffer`s, enhances the raw U8 image segment with CVIE, optionally applies needle enhancement, and returns a frame with the original header/trailer layout preserved.

## Installation

```sh
npm install react-native-nitro-frame-processor react-native-nitro-modules
```

> `react-native-nitro-modules` is required as this library relies on [Nitro Modules](https://nitro.margelo.com/).

For iOS, install pods after adding the package:

```sh
cd ios && pod install
```

This is a native module, so Expo Go is not supported. Use a native React Native build or an Expo dev client/prebuild setup.

## Development

This repository uses Bun for dependency management:

```sh
bun install
bun run typecheck
```

`bun.lock` is committed. Do not add `yarn.lock` back.

## What It Does

- Creates a native Nitro HybridObject named `NitroFrameProcessor` on iOS and Android.
- Activates a ContextVision license through the bundled CVIE SDK.
- Configures CVIE with a parameter file, frame dimensions, setting index, and a fixed native thread count.
- Processes U8 frame data while preserving the incoming binary message layout.
- Optionally detects and enhances needle-like structures before CVIE processing.
- Uses the bundled reference Needle Enhancement C implementation, including multi-frame confidence tracking, motion-tip correction, and EMA background differencing.

If the native module is unavailable, disabled, unlicensed, not configured, or receives an unsupported frame layout, `processFrame` returns the original input buffer unchanged.

## Frame Format

`processFrame(input)` expects an `ArrayBuffer` containing the full binary frame message, not just the image bytes. The native implementation reads:

- header length from byte `8`
- trailer length from bytes `9..10`
- image depth/sample count from bytes `13..14`
- beam count from byte `24` when the header length is `33`, otherwise from the message length

Only the raw U8 image segment is enhanced. Header and trailer bytes are copied through unchanged.

## Usage

```ts
import {
  activateLicense,
  processFrame,
  setEnabled,
  setNeedleEnhancementAngle,
  setNeedleEnhancementAngleRange,
  setNeedleEnhancementDepthMask,
  setNeedleEnhancementEnabled,
  setNeedleEnhancementNeedleLength,
  setNeedleEnhancementPipParams,
  setParameterFilePath,
  setSetting,
} from 'react-native-nitro-frame-processor';

const activated = activateLicense('<activation-key>', '<device-id>');
const parameterFilePath = 'US2D-7_default.us2d7'; // Use an absolute bundle/file path on iOS.

if (activated) {
  setEnabled(true);
  setParameterFilePath(parameterFilePath);
  setSetting(0);

  setNeedleEnhancementEnabled(true);
  setNeedleEnhancementAngle(30);
  setNeedleEnhancementNeedleLength(100);
}

// `inputFrame` should be a full frame message in the format described above.
const enhancedFrame = processFrame(inputFrame);
```

## Parameter Files

Parameter files are passed with `setParameterFilePath(path)`.

- On Android, `.cov`, `.us2d4`, `.us2d5`, `.us2d6`, `.us2d7`, and `.us2d9` files from app assets or `assets/context_vision` are copied into the app files directory. Relative parameter names are resolved against that directory.
- On iOS, the package bundles the CVIE framework and parameter resources in the pod. Pass an absolute path to the parameter file available in the app bundle or filesystem.

## API

### `activateLicense(activationKey, deviceId): boolean`

Activates the ContextVision license. Returns `false` when the native module is unavailable or activation fails.

### `setParameterFilePath(path): void`

Sets the CVIE parameter file used when the processor is configured.

### `setSetting(setting): void`

Sets the CVIE setting index passed to `CVIEEnhanceSetup` and `CVIEEnhanceNext`.

### `setEnabled(value): void`

Enables or disables frame processing. Disabled processing returns the original input buffer.

### `setNumThreads(numThreads): void`

Public API for thread configuration. The current native implementation uses a fixed value of `2`.

### `processFrame(input): ArrayBuffer`

Processes a frame and returns a new enhanced buffer when processing succeeds. Returns the original buffer unchanged when processing cannot run.

### Needle Enhancement

```ts
setNeedleEnhancementEnabled(true);
setNeedleEnhancementAngle(30);
```

Use `setNeedleEnhancementAngleRange(min, max, step)` instead when the UI should run a sweep across an angle range, for example `setNeedleEnhancementAngleRange(20, 40, 1)`.

The public input surface supports:

- enabling/disabling Needle Enhancement with `setNeedleEnhancementEnabled(value)`
- selecting one angle with `setNeedleEnhancementAngle(degrees)`
- selecting an angle sweep range with `setNeedleEnhancementAngleRange(minDegrees, maxDegrees, stepDegrees)`
- overriding the expected needle length in pixels with `setNeedleEnhancementNeedleLength(needleLengthPx)`
- configuring depth masking with `setNeedleEnhancementDepthMask(maskSkinLayer, depthMaskThicknessPx)`
- configuring the full PIP search params with `setNeedleEnhancementPipParams(thetaStepDeg, thetaRangeMinDeg, thetaRangeMaxDeg, resizeFactor, normalize)`

These map to the configurable inputs passed into `needle_enhance_process()`: `needle_len_px`, `DepthMaskParams`, and `NeedlePipParams`. Frame size, header offset, frame number, and line output are managed internally by the adapter.

Default UI values are intentionally owned by the host application. This package exposes setters only and does not export a Needle Enhancement defaults object.

When both a selected angle and an angle sweep UI exist, use one native mode at a time: `setNeedleEnhancementAngle()` for a single angle or `setNeedleEnhancementPipParams()` / `setNeedleEnhancementAngleRange()` for sweep-based detection.

The native algorithm lives under `cpp/needle-reference/algorithm/`:

- `needle_enhancement.h`
- `needle_enhancement.c`
- `needle_confidence_score.c`

These files are copied from the upstream Needle Enhancement reference implementation and are used directly by the mobile adapter in `cpp/NeedleEnhancement.cpp`. Compatibility shims for firmware-specific globals and headers live in `cpp/debug.h`, `cpp/probe/`, `cpp/frame/`, and `cpp/needle-reference/cmd_central.h`.

## Contributing

- [Development workflow](CONTRIBUTING.md#development-workflow)
- [Sending a pull request](CONTRIBUTING.md#sending-a-pull-request)
- [Code of conduct](CODE_OF_CONDUCT.md)

## License

MIT

---

Made with [create-react-native-library](https://github.com/callstack/react-native-builder-bob)
