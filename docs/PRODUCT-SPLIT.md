# Mechamorph Product Split

## Shared foundation

Both products use the same core assets and machine behavior:

- MachineSamplerEngine
- machine state logic
- CC0 sample library
- machine profiles
- SPEED / LOAD / ACTION / WEAR behavior
- start/run/load/release/stop states
- sample provenance and licensing
- deterministic scheduling / QA

The shared foundation is not itself a user-facing product.

---

# Product A — 125A Mechamorph Machine

## Type
VST3 instrument / sampler-synth

## Input
MIDI / trigger events

## Output
Generated machine audio

## Core behavior
Every MIDI Note-On retriggers the complete machine cycle.

Note-Off does not gate or prematurely stop the machine.

DAW transport stop resets/stops the machine.

## Initial controls
- MACHINE
- SPEED
- LOAD
- ACTION
- WEAR
- OUTPUT

## Initial machine profiles
- PROJECTOR
- HANDCRANK
- INDUSTRIAL

## Goal
A playable cinematic/mechanical instrument that already sounds convincingly like a real working machine without requiring external audio.

---

# Product B — 125A Mechamorph FX

## Type
VST3 audio effect

## Input
External audio

## Output
Mechanically re-articulated/transformed audio

## Core rule
Do not return to the rejected architecture:

> source + generic EQ/modulation/noise/resonators

Instead, reuse the real machine engine as the physical/mechanical layer.

The input signal should drive or excite the same machine states and event structure used by the instrument.

Potential coupling:

- source transients -> ACTION events
- source envelope -> machine energy / LOAD
- source density -> action density
- source spectrum -> machine-body/excitation shaping
- source timing -> optional trigger/re-articulation
- machine SPEED / WEAR / LOAD remain true machine controls

## Goal
Make external audio behave as if it is being operated by, transmitted through, or re-articulated by the same believable machine engine.

---

# Architectural rule

The two products must share:

```
samples/
machine profiles/
MachineSamplerEngine/
machine state + event logic/
QA fixtures/
```

They must NOT share product-specific assumptions.

Instrument-specific:
- MIDI trigger behavior
- instrument bus layout
- note-trigger semantics

FX-specific:
- audio input bus
- transient/envelope analysis
- input-to-machine coupling
- dry/wet or mechanize behavior

This separation prevents instrument requirements from distorting the FX design and vice versa.

---

# Naming

Working product names:

- **125A Mechamorph Machine**
- **125A Mechamorph FX**

Repo remains:

- `125A-Mechamorph`

Shared development branch:

- `v0.1.0`

Until product maturity justifies separate release branches.
