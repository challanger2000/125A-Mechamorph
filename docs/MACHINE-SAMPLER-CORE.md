# Machine Sampler / Synth Core — v0.1

## Goal

Build a standalone **machine generator** that sounds and behaves like a coherent mechanism before it is used as an audio effect.

The first machine core is a **state-driven sampler/synth hybrid**.

Real CC0/original recordings provide component identity.
DSP provides:
- timing
- state
- load
- wear
- speed
- causal triggering
- variation
- shared body / glue

## Machine states

```
Stopped
  -> Starting
  -> Running
  -> Loaded
  -> Releasing
  -> Stopping
  -> Stopped
```

Not every transition must be exposed to the user.

## Sample roles

### START
Examples:
- motor/crank engagement
- first gear catch
- initial clunk
- first spring tension

### RUN
Examples:
- crank/gear loop
- projector transport
- sewing-machine mechanism
- print-press cycle
- freewheel/chain

### ACTION
Examples:
- lever
- ratchet
- latch
- shutter
- carriage return
- selector detent

### LOAD
Examples:
- strained bearing
- slower gear mesh
- stronger clatter
- housing vibration
- hydraulic/pressure strain

### RELEASE
Examples:
- spring return
- pressure dump
- latch release
- backlash re-contact

### STOP
Examples:
- deceleration
- final clunk
- spring unwind
- motor/gear run-down

## Core parameters

- SPEED
- LOAD
- WEAR
- ACTION
- CLATTER
- BODY
- PRESSURE (optional)
- OUTPUT

No "mechanize" control is needed in the standalone machine generator.

## Causal rules

- RUN follows the shared machine phase/speed.
- ACTION events may only occur while the machine is active.
- LOAD changes playback behaviour and event choice, not just gain.
- WEAR changes:
  - timing tolerance
  - event choice
  - backlash/re-contact
  - friction contribution
  - stop/start cleanliness
- RELEASE follows ACTION/LOAD changes.
- STOP must finish the machine rather than hard-muting it.

## Sample handling

All release samples must have provenance.

Realtime engine:
- receives preloaded mono/stereo float clips;
- no file I/O on audio thread;
- no allocations while processing;
- bounded number of active voices;
- deterministic random selection with stored seed.

## Playback variation

Allowed:
- small start-offset variation
- small gain variation
- small playback-rate variation
- sample-pool selection
- phase-aligned run-loop switching
- wear-dependent alternate samples

Not allowed:
- large random pitch shifts
- unrelated random event times
- sample roulette that destroys machine identity

## First machine preset concept

**Lost Workshop Machine**

Source families:
- Heidelberg printing press
- calculator-printer
- 35mm projector
- turnstile / ratchet
- camera lever/shutter
- sewing machine
- spring return
- steam/pressure release (optional)

Goal:
sound like one believable impossible historical machine, not a collage.

## Acceptance

Before any FX integration:

1. START must sound like a machine engaging.
2. RUN must maintain a coherent mechanical cycle.
3. ACTION must sound physically connected to RUN.
4. LOAD must audibly strain/change the same machine.
5. STOP must sound like that same machine disengaging.
6. Ten consecutive triggers must not reveal obvious sample repetition.
7. SPEED changes must preserve identity.
8. WEAR=100% must sound like the same machine in bad condition.

If the machine is only convincing because of reverb, distortion or music underneath, the core fails.
