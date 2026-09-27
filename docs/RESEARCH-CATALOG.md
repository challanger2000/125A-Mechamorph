# Research Catalog — DSP, Models, Samples and References

This catalog collects material that may help 125A Mechamorph. It deliberately separates:

- **directly reusable / commercially friendly**
- **research-only**
- **sample / asset candidates**
- **listening / categorization references**

Nothing enters release code or release assets without an explicit license check.

---

## 1. Commercially friendly code / DSP references

### DaisySP

Repository:
https://github.com/electro-smith/DaisySP

License:
MIT

Why useful:
- modal synthesis
- resonators
- Karplus-Strong / physical modelling components
- white / clocked / particle / fractal noise
- random / probabilistic generators
- filters
- envelopes
- phase / timing utilities
- pitch-shift / decimation / nonlinear effects

Potential Mechamorph use:
- body resonators
- spring-like structures
- metallic resonances
- stochastic rattle excitation
- dust / particle style micro-events
- common DSP building blocks

Important:
Use only modules from the MIT DaisySP repository. DaisySP-LGPL is a separate repository and has LGPL obligations.

Source:
https://github.com/electro-smith/DaisySP

---

### Harmonium Companion

Repository:
https://github.com/ledlaux/harmonium-companion

License:
MIT

Why useful:
- explicit bellows simulation
- air reservoir concept
- manual / automatic pumping
- pressure-driven expression
- practical reference for coupling airflow to musical behaviour

Potential Mechamorph use:
- conceptual reference for AIR/BELLOWS state
- tank pressure
- leak
- note/load-dependent pressure consumption

Do not reuse bundled soundfont/audio assets without separately verifying their licenses.

---

## 2. Research-only code / model references

### Sound Design Toolkit (SDT)

Repository:
https://github.com/SkAT-VG/SDT

License:
GPLv3

Capabilities:
- collisions
- rubbing
- rolling
- scraping
- gas / wind
- engines
- physically informed sound synthesis

Why useful:
This is nearly a virtual Foley toolkit and therefore extremely relevant for understanding procedural mechanical sound design.

Policy:
**Research only. No GPL code copied into proprietary 125A implementation.**

---

### Clatter

Repository:
https://github.com/alters-mit/clatter

License:
Hippocratic License 3.0

Capabilities:
- rigid-body impact synthesis
- scraping
- rolling
- perceptually inspired material/object models

Why useful:
Provides a strong conceptual basis for replacing large sample banks with parameterized events.

Policy:
Research only unless the license is separately reviewed and approved.

---

### modal-synth

Repository:
https://github.com/crispinha/modal-synth

License:
GPLv3-or-later

Capabilities:
- modal resonator synthesis
- parametrically controlled modal spectra

Why useful:
Reference for body/material resonator structures.

Policy:
Research only. No code copied.

---

### OpenWurli

Repository:
https://github.com/hal0zer0/openwurli

License:
GPLv3

Why useful despite different target:
- 7-mode reed oscillator
- beam-theory-inspired inharmonicity
- hammer impact/noise model
- nonlinear pickup / electronics
- cabinet/speaker body model

Potential conceptual lessons:
- mechanical exciter + resonating element + nonlinear transduction + cabinet
- per-register physical variation
- impact-noise coupling

Policy:
Research only. No GPL code copied.

---

## 3. CC0 sample / Foley collections

### OpenGameArt — 100 CC0 metal and wood SFX

URL:
https://opengameart.org/content/100-cc0-metal-and-wood-sfx

License:
CC0

Contains:
- metal hits
- metal sheets
- keys
- locks
- springs
- tools
- squeaks
- wood hits
- wood cracks
- wood squeaks
- doors

Why useful:
Excellent low-risk raw-material pool for:
- impact analysis
- material-body comparison
- thin-sheet chatter
- spring / key / lock events
- wood-vs-metal resonance studies

---

### OpenGameArt — Chain winch sounds

URL:
https://opengameart.org/content/chain-winch-sounds

License:
CC0

Why useful:
- chain drive
- winch cadence
- background mechanical motion
- load-dependent rhythmic texture reference

---

### OpenGameArt — Mechanical Sounds

URL:
https://opengameart.org/content/mechanical-sounds

License:
CC0

Contains:
- clanks
- light clunks
- rattle
- squeaky clicks
- generic mechanisms
- typewriter-like attacks

Why useful:
Isolated micro-events suitable for transient/body studies.

---

### OpenGameArt — Clock Wind Sounds

URL:
https://opengameart.org/content/clock-wind-sounds

License:
CC0

Why useful:
- winding
- spring drive
- ticks
- clockwork texture

---

### OpenGameArt — Spring Sounds

URL:
https://opengameart.org/content/spring-sounds

License:
CC0

Why useful:
Real spring excitation and decay references.

---

### OpenGameArt — CC0 Sounds Library

URL:
https://opengameart.org/content/cc0-sounds-library

License:
Collection of CC0 entries, but verify each linked asset independently.

Relevant categories surfaced:
- mechanical clicks / buzzes
- chain winch
- ticking clock
- door / latch
- wood / metal
- button clicks

---

### RPG Foley Sounds

URL:
https://warsong.pages.dev/sounds/rpg/

License:
CC0

Contains:
- small chain
- small metal
- clanking
- door / latch
- metal-on-metal clank / slide
- tumbling small wooden objects

Why useful:
A compact multi-material Foley pool with useful small mechanical primitives.

---

### Signature Sounds — Old Wooden Door Foley

URL:
https://signaturesounds.org/store/p/old-wooden-door-foley

License:
CC0

Contains:
- creaks
- knocks
- hinges
- rattles
- latches
- aged wood movement

Why useful:
Good source for:
- cabinet/wood-body character
- dry friction / creak study
- old mechanical enclosure behaviour

---

### BigSoundBank / La Sonothèque

Reference:
https://tmhsdigital.github.io/Free-Game-Dev-Assets/entry/bigsoundbank/

License:
Many items marked "Free and Royalty Free" are described as CC0/public-domain dedicated.

Important:
Verify the live asset page before using any individual recording.

Why useful:
Large field/Foley pool that may fill gaps such as:
- pulleys
- bearings
- old tools
- locks
- wood
- metal
- doors
- machinery

---

## 4. CC-BY but useful if attribution is acceptable

### JC Sounds — Mechanical Pack Vol 1

URL:
https://opengameart.org/comment/111099

License:
CC-BY 4.0

Contains 50 files across:
- clockwork & gears
- wind-up mechanisms
- locks
- steampunk mechanisms
- start / run / stop sequences
- steam-release variants

Why useful:
Very targeted reference pack for exactly the mechanical vocabulary Mechamorph needs.

Release decision:
Prefer CC0 sources first. Consider CC-BY only if the contribution materially improves the plugin and attribution handling is acceptable.

---

## 5. Impulse-response / body modelling strategy

Dedicated CC0 "wooden-box / metal-cabinet" impulse responses are much less common than generic room IRs.

For Mechamorph, the more robust approach is likely:

1. **modal synthesis for the core body**
2. optional short captured IRs from original 125A recordings
3. only use third-party IRs when redistribution is explicitly allowed

Why:
A room IR models acoustic space, but we need **object/body resonances**.

The desired bodies are:
- small wooden cabinet
- hollow wooden box
- thin sheet metal
- brass plate
- cast-metal housing
- metal tube
- spring housing

These are better represented by:
- modal filter banks
- resonant comb structures
- short measured body responses
- convolution only where an original capture adds real value

---

## 6. Bellows / pneumatic modelling

Useful simplified state:

```
pumpVelocity -> inflow
pressure += inflow
pressure -= leak
pressure -= load
pressure -> airflow / amplitude / timbre / noise
```

Possible additions:
- reservoir smoothing
- valve open/close impulses
- pressure overshoot
- nonlinear leakage
- pressure-dependent brightness
- pump-cycle modulation

The key principle:
**pressure is stateful**; it should not be another independent LFO.

This lets the machine "breathe" and makes the crank/drive physically meaningful.

---

## 7. Candidate procedural primitives

### Material impact
- excitation impulse
- modal frequency set
- modal decay set
- contact hardness
- impact force
- body size
- damping

### Rattle
- Poisson / clustered event process
- velocity-dependent density
- decaying collision energy
- shared resonator

### Ratchet
- tooth-count pulse generator
- direction
- speed
- tooth irregularity
- pawl bounce
- wear / missed contact

### Gear
- rotational phase
- gear ratio
- tooth count
- mesh-frequency excitation
- eccentricity
- backlash
- load-dependent chatter

### Friction
- roughness noise
- stick-slip nonlinearity
- pressure
- speed
- material resonator

### Hinge / bearing
- low-speed friction
- resonant squeal
- sudden slip events
- age / dryness

### Spring
- dispersive resonator
- short impulse
- pitch glide / settling
- coupled rattle

### Air / valve
- turbulent noise
- pressure reservoir
- short chuff
- leakage
- valve click

### Loose hardware
- several small high-Q impact bodies
- random collision clusters
- gravity/load dependent density

---

## 8. Useful analysis targets

For every real reference sample we should extract or measure:

- transient shape
- peak / RMS relation
- event spacing
- spectral centroid
- modal peak frequencies
- decay constants
- inharmonicity
- modulation / cycle period
- noise floor
- stereo width
- pitch drift
- repetition variability

Purpose:
Use the recordings as **measurement targets**, not only as playable layers.

---

## 9. What should be synthesized vs sampled

### Strong candidates for synthesis
- gear pulse trains
- ratchets
- clicks
- simple impacts
- rattles
- modal bodies
- springs
- airflow
- drive instability
- wear / backlash
- probabilistic micro-events

### Strong candidates for original or CC0 micro-samples
- distinctive leather creaks
- complex old hinges
- irregular compound mechanisms
- special chain movement
- rare metal scrapes
- difficult-to-model squeals

### Should generally remain reference only
- finished cinematic mechanism effects
- long stock-library sequences
- commercial Foley
- heavily designed steampunk effects

---

## 10. Recommended development principle

Build a **procedural engine first** and use recordings to:

- calibrate
- validate
- enrich
- add rare details

Do not build Mechamorph as a sample player disguised as a DSP effect.

The differentiator is the **shared mechanical state and coupling to the incoming audio**.
