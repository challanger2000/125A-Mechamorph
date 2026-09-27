# 125A Mechamorph

**Long-term goal:** transform modern audio into a believable physical machine.

> Modern sound in. Mechanical machine out.

## Current development pivot

The first FX prototype proved an important point:

**A source does not become mechanically believable by adding resonators, wobble, noise, clacks and other "mechanical" DSP layers.**

The project therefore now develops the problem in the opposite order.

### Current milestone: build the machine first

The `v0.1.0` branch is focused on a standalone **Machine Sampler / Synth Core** that must already sound like a coherent real mechanism without any musical input signal.

The core combines:

- real CC0 / original mechanical recordings for literal sound identity;
- deterministic machine state and motion;
- START / RUN / ACTION / LOAD / RELEASE / STOP behaviour;
- speed-dependent operation;
- load and wear;
- ratchets, levers, spring returns, chains and pressure events;
- bounded variation without random-FX behaviour.

Only after this machine is convincing will incoming musical audio be coupled to it.

## Core development rule

> Samples answer **"what does this physical part actually sound like?"**

> DSP answers **"why, when and how does this part move?"**

This hybrid approach replaces the failed assumption that every old mechanical detail should be synthesized procedurally.

## Current verified engineering status

The existing DSP/VST3 infrastructure remains useful:

- Windows VST3 prototype builds successfully.
- Steinberg Validator: **47/47 PASS** on the verified prototype build.
- Standalone DSP tests pass.
- Standalone Machine Sampler state/scheduler tests pass.
- Research audio provenance and license handling are documented.

The current VST3 sound architecture is **not** considered the final Mechamorph sound concept.

## Machine engine states

```
STOPPED
   |
   v
STARTING
   |
   v
RUNNING <------+
   |           |
   v           |
LOADED         |
   |           |
   v           |
RELEASING -----+
   |
   v
STOPPING
   |
   v
STOPPED
```

## Current real-source research

Two CC0 research packs have already been gathered and measured.

Representative material includes:

- 35mm projector
- Heidelberg printing press
- old calculator-printer
- old switch
- slide projector
- manual winch
- slow/fast sewing machine
- large ratchet
- large spring
- chain
- air/pressure burst
- ratchet/rattle
- hand-cranked roller mechanism
- spring-loaded hard contact

## Important documents

- [Machine Sampler Core](docs/MACHINE-SAMPLER-CORE.md)
- [Machine Identity Priority](docs/MACHINE-IDENTITY-PRIORITY.md)
- [Mechanical Sound Identity](docs/MECHANICAL-SOUND-IDENTITY.md)
- [Architecture Pivot](docs/ARCHITECTURE-PIVOT.md)
- [Control Physics Map](docs/CONTROL-PHYSICS-MAP.md)
- [Deep Internet Source Sweep](docs/DEEP-INTERNET-SOURCE-SWEEP.md)
- [Research Pack 01 Analysis](docs/RESEARCH-PACK-01-ANALYSIS.md)
- [Research Pack 02 Analysis](docs/RESEARCH-PACK-02-ANALYSIS.md)
- [Machine Sample Manifest 01](docs/MACHINE-SAMPLE-MANIFEST-01.csv)
- [Machine Sample Manifest 02](docs/MACHINE-SAMPLE-MANIFEST-02.csv)
- [Provenance Policy](docs/PROVENANCE.md)

## Success gate

Before returning to the audio-FX stage, the standalone engine must pass a simple listening test:

> **With no synth, no musical source and no decorative effects, does it already sound like one believable working machine?**

If not, the machine core is not finished.
