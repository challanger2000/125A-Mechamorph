# Machine Engine v0.1 — Sampler/Synth Foundation

## Goal

Build an engine that already sounds and behaves like a machine **before any audio input is transformed**.

The engine is intentionally separate from the old FX core.

## State model

```
STOPPED
   |
   v
STARTING
   |
   v
RUNNING <----> LOADED
   |
   v
STOPPING
   |
   v
STOPPED
```

## Sample roles

### START
Spin-up / first engagement / first clunk.

### RUN BED
Continuous authentic machine motion:
- press
- projector
- sewing machine
- crank mechanism
- freewheel / chain

### ACTION
Drive-synchronous events:
- pawl
- ratchet
- lever
- switch
- cam contact
- clack

### LOAD
Mechanical strain / heavier body / slower motion.

### RELEASE
Unload / spring return / pressure release.

### STOP
Deceleration / final clunk / end-stop.

## Core rule

The engine chooses *when* samples happen from machine state.

Samples provide identity.
DSP/state provides causality.

No free-running random Foley.

## First technical implementation

`MachineEngine` provides:
- deterministic sample-role pools
- fixed maximum voices
- no realtime allocation
- start/run/load/stop state machine
- speed-dependent sample playback
- drive-phase-synchronous action triggers
- deterministic variation
- load-dependent speed reduction
- wear/backlash/friction state variables

Current tests use synthetic placeholder arrays only.

No third-party sample has been embedded in code or repository.

## Next milestone

Curate the first real CC0 sample set:

- 1–2 START samples
- 2 RUN beds at different speeds
- 4–8 ACTION samples
- 2 LOAD/strain samples
- 2 RELEASE samples
- 1–2 STOP samples

Then render a complete 10–20 second machine performance:

```
start -> idle/run -> actions -> load -> unload -> stop
```

Success criterion:

> Without synth input and without explanation, the render should sound like one coherent physical machine.
