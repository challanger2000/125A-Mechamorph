# Core Verification Log

## Scope

This log records only tests that were actually executed against a materialized copy of the `v0.1.0` DSP core.

It is **not** a VST3 release qualification record.

## Environment

- OS: Linux container
- Compiler: GCC 14.2.0
- CMake build type: Release
- C++ standard: C++17
- Test framework: CTest + assert-based standalone test executable

## Verified standalone core

### Build / test pass

The standalone core compiled successfully with:

- `-Wall`
- `-Wextra`
- `-Wpedantic`

CTest result:

- 1/1 standalone core test target PASS

Current test coverage includes:

- exact neutral path at MECHANIZE=0 / unity output
- finite output on silence at strong settings
- impulse stress
- deterministic reset / PRNG behaviour
- sample-rate safety: 44.1 / 48 / 88.2 / 96 / 192 kHz
- modal gain runaway regression
- finite mechanical tail / return toward silence

## Smoke render finding

A deterministic 8-second synthetic fixture was rendered through the prototype.

### Before modal normalization

Measured:
- peak: 5.71028
- RMS: 1.45109

Interpretation:
- modal body accumulated excessive energy under periodic material
- not acceptable

Action:
- changed modal excitation from direct raw gain to energy-normalized excitation based on `sqrt(1-r^2)`

### After modal normalization

Measured:
- peak: approximately 0.659
- RMS: approximately 0.0765

After input-driven machine activity gating:
- peak: approximately 0.653
- RMS: approximately 0.0760

Interpretation:
- runaway removed
- activity gating did not reintroduce level instability
- values are test-fixture measurements, not product loudness targets

Evidence class:
- MEASURED

## Important limitations

Not yet verified:

- Windows compiler
- Steinberg VST3 build
- Steinberg Validator
- 125A Plugin Tester
- host loading
- automation correctness
- state recall in host
- realtime p95/p99/max
- sound quality against real mechanical reference recordings
- GUI lifecycle

No VST3 PASS claim is allowed until those are actually tested.


## 2026-09-27 — Current core regression after finite-tail changes

Materialized current standalone core after:
- input-driven machine activity
- dedicated GearEngine
- modal energy normalization
- bounded bellows reservoir discharge
- 8-second conservative VST3 tail target

Build:
- GCC 14.2
- Release
- C++17

CTest:
- **1/1 PASS**
- elapsed approximately 0.15 s

Additional regression now covers:
- 4 seconds sustained sine excitation at AIR=100%, WEAR=0%
- 8 seconds subsequent tail window
- final 1-second peak below 1e-4

Evidence class:
- MEASURED / VERIFIED STANDALONE CORE

This still does not constitute VST3/host PASS.


## 2026-09-27 — Friction, true morph and stereo-preservation regression

Changes covered:
- reduced speed/load/activity/wear-coupled FrictionEngine
- MECHANIZE changed from additive layer behaviour to a true source-to-machine morph
- separate L/R body resonators with one shared mechanical state
- stereo analysis uses channel energy rather than phase-cancellable mono sum
- anti-phase stereo regression fixture added

Standalone verification:
- GCC 14.2 / Release / C++17
- CTest: **1/1 PASS**
- elapsed approximately 0.19 s

Deterministic smoke render after morph/stereo changes:
- peak: approximately **0.5106**
- RMS: approximately **0.0738**

Anti-phase fixture:
- L = -R source
- machine remains active
- output remains finite
- measurable non-zero delta from dry source

Evidence class:
- MEASURED / VERIFIED STANDALONE CORE

VST3 wrapper remains unverified until a current Windows/Steinberg build is run.
