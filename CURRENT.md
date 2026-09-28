# Current authoritative state

Date: 2026-09-28

## Development authority

- Repository: `challanger2000/125A-Mechamorph`
- Active development branch: `v0.1.0`
- Current verified HEAD: `c6e6f164bc09ec4b2fb2276f5ec00e2bf8c43d09`
- Current verified GitHub Actions run: `36398235410`
- Current Windows artifact: `125A-Mechamorph-Machine-v0.1.0-Windows-x64-PROTOTYPE`

This file is the navigation authority for the current Mechamorph Machine development state. It does not by itself declare a public release.

## Verified in run 36398235410

- Windows x64 VST3 build: PASS
- Machine engine regression: PASS
- Editor lifecycle: PASS over 5 open/close cycles
- UI/DPI zoom: PASS at 100 / 125 / 150 / 200%
- Ctrl+click default-reset matrix: PASS
- State save/restore and legacy migration: PASS
- VST3 process contract: PASS
  - realtime and offline
  - 44.1 / 48 / 96 / 192 kHz
  - block sizes 1 / 16 / 64 / 257 / 1024
  - MIDI note on/off
  - parameter automation
  - NaN parameter robustness
  - repeated activate/deactivate
- Steinberg Validator: 47/47 PASS
- BODY / SPACE measurement lab: PASS
- Validated Windows artifact upload: PASS

## Important fixed defect

The process-contract probe exposed that the processor inherited Steinberg `AudioEffect::setProcessing()`, which returns `kNotImplemented`.

Commit `af429573c7bf348780496d4cf32d8fd060f700c1` added the required processing-state implementation. The complete matrix then passed in run `36398235410`.

## Not yet claimed here

The current evidence does not by itself claim:

- 125A Plugin Tester PASS for this exact artifact
- manual loading/operation in the user's production DAW for this exact artifact
- final perceptual acceptance of all machine personalities
- public release / Gumroad readiness
- macOS validation

Until those product-level gates are explicitly completed, keep the artifact labelled development/prototype rather than silently promoting it to a final release.
