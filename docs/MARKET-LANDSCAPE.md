# Market / Reference Landscape

Purpose: define what Mechamorph is *not*, identify adjacent products, and protect the core differentiation during development.

## Core differentiation

Mechamorph is intended to transform arbitrary audio into the percept of a **coherent mechanical music machine**.

The target is not merely:
- resonator FX
- lo-fi
- mechanical noise layering
- clockwork samples
- barrel-organ emulation
- steampunk SFX playback

The defining architecture is:

**shared drive/state + mechanical events + friction/air + shared body + input coupling**

---

## Adjacent products

### Audio Damage — Tessera

Type:
- resonator FX

Known behaviour:
- incoming audio excites parallel resonator voices
- multiple physical-model resonator types
- strings, bars, glass, bells, bowls, membranes, plates, tubes
- tempo-synced motion

Overlap with Mechamorph:
- external audio excites physical models
- resonant-body transformation

Difference:
- Tessera is fundamentally a tuned resonator effect.
- Mechamorph should add a coherent mechanical system:
  - drive
  - gears
  - ratchets
  - friction
  - bellows
  - valves
  - wear
  - loose hardware
  - shared machine state

Conclusion:
Do not compete on "many resonator types". Use resonators as one subsystem.

Reference:
https://www.synthtopia.com/content/2026/07/20/audio-damage-intros-tessera-tuned-resonator-effect/

---

### Sympiano

Type:
- physical-model resonator FX

Known behaviour:
- arbitrary audio excites a physically modelled piano-string system
- hundreds of tuned resonators
- bridge coupling
- damper/felt behaviour

Overlap:
- physical object responds to incoming audio

Difference:
- one coherent resonant instrument/object
- no general mechanical drive/contact/air/wear engine

Lesson:
A convincing effect can be based on the idea that the input **physically excites an object** rather than simply receiving EQ/convolution.

Reference:
https://sympiano.com/

---

### Orbiter Sound Bowl

Type:
- physical model instrument + effect

Known behaviour:
- stick-slip friction model
- strike / rub excitation
- external audio can excite bowl resonators

Overlap:
- nonlinear friction
- external audio excitation

Difference:
- target object is a singing bowl
- Mechamorph target is a complete mechanical apparatus

Lesson:
Friction can be a musically controllable generator, not only background noise.

Reference:
https://orbiter.audio/instruments/singing-bowl/

---

### AudioThing — Gong Amp

Type:
- resonator / physical-system effect

Known behaviour:
- convolution + physical modelling + feedback
- input drives a resonant physical system

Overlap:
- physical coloration / coupled resonance

Difference:
- narrow historical resonator concept
- not a general machine-behaviour transformer

Lesson:
Hybrid modelling (measured response + model + feedback) can be more effective than insisting on one synthesis method.

Reference:
https://www.audiothing.net/effects/gong-amp/

---

### Arturia — Tape MELLO-FI

Type:
- lo-fi / tape character effect

Known behaviour:
- wow/flutter
- mechanical noise
- tape coloration

Overlap:
- instability
- mechanical noise
- age / imperfection

Difference:
- tape transport character, not mechanical object generation

Danger:
If Mechamorph is reduced to wow/flutter + noise + filter, it enters this category and loses the core concept.

Reference:
https://www.arturia.com/products/software-effects/tape-mello-fi/overview

---

### Heavyocity — Machina

Type:
- Kontakt instrument / mechanical sound library

Known behaviour:
- 700+ mechanical source sounds
- clocks, gears, engines
- loops and playable designed content

Overlap:
- mechanical vocabulary
- cinematic use
- rhythmic machinery

Difference:
- sample instrument, not arbitrary-input transformation effect

Lesson:
The mechanical sound palette itself is well established and commercially useful.
Our differentiation must come from **transforming the user's own sound**.

Reference:
https://www.native-instruments.com/products/machina

---

### MechanOdd

Type:
- physical-modelling synthesizer

Known behaviour:
- exciters
- strings / plates / membranes / beams
- feedback matrix
- effects and modulation

Overlap:
- excitation + resonator
- physical modelling
- mechanical-material timbres

Difference:
- synth/instrument architecture
- not primarily an input-transform effect
- lacks Mechamorph's proposed drive/gear/bellows/wear semantics

Reference:
https://github.com/odoare/MechanOdd

---

## SFX libraries as design references

### Escapement — Clockwork, Brass and the Machine Room

Current pack includes:
- escapement
- gears
- springs
- steam
- levers/switches
- latches/bolts/locks
- chimes/bells
- machinery loops
- pneumatics
- failure
- pendulum
- gear train
- mainspring
- detent
- governor
- bellows
- bearing squeal

Why relevant:
This category list is an excellent **taxonomy sanity check** for the breadth of a believable machine.

Important:
Commercial license is for use as SFX in projects, not automatically for redistribution in a plugin. Treat as reference unless explicit plugin-embedding permission is obtained.

Reference:
https://csaf.itch.io/escapement-clockwork-sfx

---

## Positioning guardrails

Mechamorph should remain:

### YES
- input transformation
- physically informed
- highly characterful
- mechanically coherent
- cinematic-capable
- usable on synths, piano, vocals, drums, pads and loops
- able to range from subtle antique mechanism to broken industrial machine

### NO
- another lo-fi tape plugin
- a barrel-organ sample instrument
- a generic resonator bank
- a random Foley layer generator
- a steampunk sample player
- a one-trick clock tick effect

---

## Product thesis

**Modern audio goes in. A believable physical music machine appears to be playing it.**

The key perceptual claim is stronger than "mechanical sounding":

> The user should hear causal relationships between motion, impacts, air, friction, resonance and the transformed source.

That causal coherence is the product.
