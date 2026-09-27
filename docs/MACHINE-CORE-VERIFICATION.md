# Machine Sampler Core Verification

## Verified state

Date: 2026-09-27

The standalone machine sampler core has now been exercised with **real curated CC0 mechanical recordings**, not synthetic placeholder tones.

## Engineering verification

### Machine core tests

Result:
- **100% PASS**
- machine_sampler_tests: PASS

Coverage includes:
- START audibility
- START -> RUN lifecycle
- autonomous cam/action triggering
- LOAD / RELEASE transitions
- STOP lifecycle
- residual STOP audio tail after drive reaches Stopped
- deterministic random behaviour
- sample-rate conversion
- restart after stop
- bounded pool/voice architecture

### Real sample pool

Loaded by the verified renderer:

- START: 2
- RUN: 7
- ACTION: 18
- LOAD: 3
- RELEASE: 9
- STOP: 1

Total curated clips in active pools:
- 40

Source families include:
- 35mm projector
- Heidelberg printing press
- calculator-printer
- old switch
- slide projector
- manual winch
- sewing machine slow/fast
- large ratchet
- large spring
- chain
- air/pressure burst
- ratchet/rattle
- crank roller mechanism
- spring-loaded hard contact

## Actual C++ render

Output:
`Mechamorph_MachineSamplerCore_CC0_v0.1.wav`

Render sequence:
- START
- slow RUN
- faster RUN
- LOAD
- RELEASE
- STOP + residual mechanical tail

Measured render:
- 48 kHz mono
- 40 seconds
- peak approximately 0.9003
- RMS approximately 0.07925
- final machine drive state: **Stopped (0)**

Important:
The STOP audio tail may continue after the logical drive reaches Stopped. This is intentional and models a machine whose drive has stopped while physical run-down/contact noise continues.

## Resulting architecture

The verified core is now:

```
REAL MECHANICAL SAMPLES
        |
        v
ROLE POOLS
START / RUN / ACTION / LOAD / RELEASE / STOP
        |
        v
DETERMINISTIC MACHINE STATE
speed / phase / load / wear / action / clatter
        |
        v
CAUSAL EVENT SCHEDULING
        |
        v
BOUNDED SAMPLE VOICES
        |
        v
MACHINE OUTPUT
```

## What is not yet verified

The next gate is perceptual, not technical:

> Does this render actually sound like one believable machine rather than a collage of mechanical recordings?

That requires listening judgement.

If YES:
- proceed to VST3 sampler/synth wrapper and controls.

If NO:
- improve sample-role selection, run-bed coherence and state transitions before any UI or FX work.
