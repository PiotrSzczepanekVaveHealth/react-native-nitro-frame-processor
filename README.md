# react-native-nitro-frame-processor

Native React Native frame processor built with [Nitro Modules](https://nitro.margelo.com/) and ContextVision CVIE.

The library accepts ultrasound frame payloads as `ArrayBuffer`s, optionally applies Needle Enhancement to the raw U8 image segment, enhances it with CVIE, and returns a frame with the original header/trailer layout preserved.

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
bun run nitrogen
```

`bun.lock` is committed. Do not add `yarn.lock` back.

## What It Does

- Creates a native Nitro HybridObject named `NitroFrameProcessor` on iOS and Android.
- Activates a ContextVision license through the bundled CVIE SDK.
- Configures CVIE with a parameter file, frame dimensions, setting index, and a fixed native thread count (`2`).
- Processes U8 frame data while preserving the incoming binary message layout.
- Optionally detects and enhances needle-like structures **before** CVIE processing.
- Uses the bundled reference Needle Enhancement C implementation, including multi-frame confidence tracking, motion-tip correction, and an internal EMA background frame for motion-based tip correction (the host app does not need to maintain a separate background buffer).
- Serializes native frame processing and validates frame layout before calling CVIE, returning the original buffer unchanged on failure instead of crashing.

If the native module is unavailable, disabled, unlicensed, not configured, or receives an unsupported frame layout, `processFrame` returns the original input buffer unchanged.

## Frame Format

`processFrame(input)` expects an `ArrayBuffer` containing the full binary frame message, not just the image bytes. The native implementation reads:

- header length from byte `8`
- trailer length from bytes `9..10`
- image depth/sample count from bytes `13..14`
- beam count from byte `24` when the header length is `33`, otherwise from the message length

Only the raw U8 image segment is enhanced. Header and trailer bytes are copied through unchanged. The raw segment length must equal `depth × beamCount`.

## Usage

```ts
import {
  activateLicense,
  processNeedleEnhancementFrame,
  processFrame,
  resetNeedleEnhancementTemporalState,
  setEnabled,
  setNeedleEnhancementAngleRange,
  setNeedleEnhancementDepthMask,
  setNeedleEnhancementEnabled,
  setNeedleEnhancementInsertionSide,
  setNeedleEnhancementNeedleLength,
  setParameterFilePath,
  setSetting,
  setVerbose,
} from 'react-native-nitro-frame-processor';

const activated = activateLicense('<activation-key>', '<device-id>');
const parameterFilePath = 'US2D-7_default.us2d7'; // Use an absolute bundle/file path on iOS.

if (activated) {
  setEnabled(true);
  setParameterFilePath(parameterFilePath);
  setSetting(0);

  setNeedleEnhancementEnabled(true);
  setNeedleEnhancementAngleRange(4, 35, 2);
  setNeedleEnhancementInsertionSide(true);
  setNeedleEnhancementNeedleLength(100);
  setNeedleEnhancementDepthMask(false, 8);

  setVerbose(__DEV__);
}

// `inputFrame` should be a full frame message in the format described above.
const enhancedFrame = processFrame(inputFrame);
```

### Dev/test replay of precaptured frame sequences

The mobile adapter can replay external frame sequences frame-by-frame, similar to the firmware `TEST_USING_EXTERNAL_DATA` path (which remains disabled in the bundled algorithm). Reset temporal state before each new sequence, then feed frames in capture order:

```ts
resetNeedleEnhancementTemporalState();

for (const frame of precapturedFrames) {
  const needleOnlyFrame = processNeedleEnhancementFrame(frame);
}
```

`processNeedleEnhancementFrame` runs Needle Enhancement only (no CVIE). Use `processFrame` for the full CVIE + optional Needle Enhancement pipeline.

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

Processes a frame and returns a new enhanced buffer when processing succeeds. Returns the original input buffer unchanged when processing cannot run.

Native safeguards:

- concurrent `processFrame` / `processNeedleEnhancementFrame` calls are serialized
- frame dimensions and raw buffer size are validated before CVIE runs
- on CVIE failure the internal processor handle is invalidated so the next frame reconfigures cleanly

### `resetNeedleEnhancementTemporalState(): void`

Clears Needle Enhancement temporal state: EMA background, motion tracking, multi-frame confidence buffers, and cached detection/fusion geometry. Call before replaying a new precaptured sequence or when the probe preset / imaging context changes.

### `processNeedleEnhancementFrame(input): ArrayBuffer`

Processes one full binary frame message through Needle Enhancement only. Intended for dev/test replay of precaptured frame sequences. Uses the same input frame format as `processFrame`.

### Needle Enhancement

Needle Enhancement is configured through setters only. Default UI values are owned by the host application (for example vave-mobile). The reference algorithm defaults in `needle_enhancement.c` are:

| Reference default | Value |
|---|---|
| `theta_step_deg` | `2.0` |
| `theta_range_min_deg` | `4.0` |
| `theta_range_max_deg` | `35.0` |
| `needle_insertion_side_right` | `1` (right) |
| `depth_mask_thickness_px` | `8` |
| `mask_skin_layer` | `false` |

#### Tunable UI parameters

| UI parameter | Setter | Native target |
|---|---|---|
| Step angle | `setNeedleEnhancementAngleRange(min, max, step)` or `setNeedleEnhancementPipParams(...)` | `NeedlePipParams.theta_step_deg` |
| Min angle | `setNeedleEnhancementAngleRange(min, max, step)` or `setNeedleEnhancementPipParams(...)` | `NeedlePipParams.theta_range_min_deg` |
| Max angle | `setNeedleEnhancementAngleRange(min, max, step)` or `setNeedleEnhancementPipParams(...)` | `NeedlePipParams.theta_range_max_deg` |
| Needle insertion side (`1` = right) | `setNeedleEnhancementInsertionSide(rightSide)` | `needle_insertion_side_right` |
| Needle length (px) | `setNeedleEnhancementNeedleLength(needleLengthPx)` | `needle_len_px` passed to `needle_enhance_process()` |
| Depth mask | `setNeedleEnhancementDepthMask(maskSkinLayer, depthMaskThicknessPx)` | `DepthMaskParams` |

| Fuse mode (`1` = always fuse, `2` = confidence fusion) | `setNeedleEnhancementFuseMode(mode)` | `needle_confidence_on` via `resetNeedleConfidenceAndMotionVariables()` |

> **Firmware NEF:** keep probe firmware needle enhancement off (`NEF0`). App-side fuse mode is configured with `setNeedleEnhancementFuseMode(1 | 2)`.

#### Angle selection modes

Use one mode at a time:

- **Single angle:** `setNeedleEnhancementAngle(degrees)` — sets min and max to the same value.
- **Angle sweep:** `setNeedleEnhancementAngleRange(minDegrees, maxDegrees, stepDegrees)`.
- **Full PIP control:** `setNeedleEnhancementPipParams(thetaStepDeg, thetaRangeMinDeg, thetaRangeMaxDeg, resizeFactor, normalize)`.

Example:

```ts
setNeedleEnhancementEnabled(true);
setNeedleEnhancementFuseMode(2);
setNeedleEnhancementAngleRange(4, 35, 2);
setNeedleEnhancementInsertionSide(true);
```

#### All Needle Enhancement setters

- `setNeedleEnhancementEnabled(value)` — enable/disable Needle Enhancement in `processFrame`
- `setNeedleEnhancementFuseMode(mode)` — `1` = always fuse detected needle; `2` = fuse based on confidence score
- `setVerbose(value)` — log Needle Enhancement setter calls in the React Native console
- `resetNeedleEnhancementTemporalState()` — reset EMA / motion / confidence state
- `processNeedleEnhancementFrame(input)` — replay external sequences (Needle Enhancement only)
- `setNeedleEnhancementAngle(degrees)` — single-angle detection
- `setNeedleEnhancementAngleRange(minDegrees, maxDegrees, stepDegrees)` — angle sweep
- `setNeedleEnhancementNeedleLength(needleLengthPx)` — expected needle length in pixels
- `setNeedleEnhancementDepthMask(maskSkinLayer, depthMaskThicknessPx)` — skin-layer depth mask
- `setNeedleEnhancementPipParams(thetaStepDeg, thetaRangeMinDeg, thetaRangeMaxDeg, resizeFactor, normalize)` — full PIP search params
- `setNeedleEnhancementInsertionSide(rightSide)` — `true` = right-side insertion, `false` = left-side insertion

These map to the configurable inputs passed into `needle_enhance_process()`: `needle_len_px`, `DepthMaskParams`, and `NeedlePipParams`. Frame size, header offset, frame number, motion EMA background, and line output are managed internally by the adapter.

`setVerbose(true)` logs setter calls from TypeScript before they are forwarded to native code.

#### Native sources

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
