# Science / Evidence Map

Purpose: map Mechamorph design ideas to published engineering evidence, permissive implementation references, and explicit license boundaries.

## Evidence labels used here

- **PUBLISHED / PEER-REVIEWED** — published paper or proceedings article
- **PHYSICS DERIVED** — direct physical relation suitable for independent implementation
- **DOCUMENTED IMPLEMENTATION** — open implementation with documented behaviour
- **MEASURED TARGET** — real recording used as calibration/analysis target
- **EMPIRICALLY TUNED** — final production tuning after measurement/listening

---

## 1. Rigid-body impacts and material identity

### Evidence

**Traer, Cusimano, McDermott — A perceptually inspired generative model of rigid-body contact sounds (DAFx 2019)**

Finding:
- impact/contact sounds can be synthesized efficiently using lower-dimensional statistical/material descriptions rather than complete expensive simulation;
- listeners can still recover perceptually relevant properties such as material, mass and motion;
- resonant impulse-response statistics are central to perceived object/material identity.

Evidence class:
- PUBLISHED / PEER-REVIEWED

Mechamorph implication:
- use **excitation + material/body resonator** rather than storing every possible mechanical event as a sample;
- allow controlled variation in modal frequencies, decay and excitation;
- calibrate "WOOD / METAL / THIN METAL / HARD / SOFT" behaviour from real recordings.

Primary reference:
https://www.dafx.de/paper-archive/details/MitGzTkeKmKIhQw6HybCxg

---

## 2. Scraping and rolling

### Evidence

**Agarwal, Cusimano, Traer, McDermott — Object-Based Synthesis of Scraping and Rolling Sounds Based on Non-Linear Physical Constraints (DAFx 2021)**

Key findings:
- realistic scraping benefits from a source-filter model;
- contact force must be physically constrained rather than simple noise;
- normal-force variation matters;
- location-dependent response can be approximated by morphing object impulse responses;
- rolling can include periodic forcing from center-of-mass / geometric-center mismatch.

Evidence class:
- PUBLISHED / PEER-REVIEWED

Mechamorph implication:
- FRICTION should not be "filtered white noise";
- use motion speed, pressure and surface roughness as coupled controls;
- body response may evolve with contact position;
- rolling/eccentric machinery can create periodic-but-imperfect forces.

Primary reference:
https://www.dafx.de/paper-archive/2021/proceedings/papers/DAFx20in21_paper_33.pdf

License note:
The MIT repository copy of the manuscript is CC BY-NC-SA; the **ideas/equations may be independently implemented**, but no protected text/assets/code should be copied.

---

## 3. Repeated mechanical impacts / after-bounce

### Evidence

**Oksanen, Parker, Välimäki — Physically Informed Synthesis of Jackhammer Tool Impact Sounds (DAFx 2013)**

Key findings:
- impact sequences can be generated from repeated pulses with slightly randomized amplitude;
- realism improves when a primary impact is followed by variable secondary impacts;
- longitudinal/transversal resonances can be split into separate resonant structures.

Evidence class:
- PUBLISHED / PEER-REVIEWED

Mechamorph implication:
- CLATTER should use **clustered secondary impacts**, not one repeated sample;
- a primary input transient can create:
  - main mechanical strike
  - 1..N rebounds
  - decaying energy
  - variable spacing
- different resonant families can represent body vs. spring/rod vibration.

Primary reference:
https://www.dafx.de/paper-archive/details/tToUh5dFycviheiDn-Cn8g

---

## 4. Non-linear friction / stick-slip

### Evidence

**Thoret et al. — Controlling a Non Linear Friction Model for Evocative Sound Synthesis Applications (DAFx 2013)**

Key findings:
- creaky doors, singing glass and squeaking surfaces can be generated from nonlinear friction behaviour;
- source/resonance separation remains computationally practical;
- physically meaningful descriptors can drive nonlinear synthesis behaviour.

Evidence class:
- PUBLISHED / PEER-REVIEWED

Mechamorph implication:
- HINGE / BEARING / SQUEAK should use a **stick-slip style state**, not merely random pitch-modulated noise;
- useful controls:
  - motion speed
  - normal force / pressure
  - friction threshold
  - slip intensity
  - object resonance

Primary reference:
https://dafx.de/paper-archive/details/KDEntfnKPqcanh-POaz2Yw

---

## 5. Modal source/resonator architecture

### Evidence

The Sound Design Toolkit literature models interacting solid objects with:
- modal resonators
- contact/interaction mechanisms
- stochastic force/pressure processes

Evidence class:
- PUBLISHED / PEER-REVIEWED

Mechamorph implication:
Use the generic architecture:

```
ACTION / EXCITER -> CONTACT MODEL -> RESONANT OBJECT(S)
```

Examples:
- hammer -> metal plate
- pawl -> ratchet wheel
- loose washer -> cabinet
- scrape force -> wooden body
- valve pulse -> hollow box

Reference:
https://dafx10.iem.at/papers/AdilogluDrioliPolottiRocchessoDelleMonache_DAFx10_P62.pdf

License boundary:
The SDT software itself is GPLv3 and therefore remains research-only for proprietary 125A code.

---

## 6. More detailed friction simulation

### Evidence

**Morishima & Nakatsuka — Simulating the Friction Sounds Using a Friction-Based Adhesion Theory Model (DAFx 2017)**

Key idea:
- sliding friction can be interpreted as large numbers of microscopic contacts/deformations rather than discrete isolated collisions;
- continuous friction requires a different model from rolling/discrete contacts.

Evidence class:
- PUBLISHED / PEER-REVIEWED

Mechamorph implication:
Maintain two distinct families:

1. **DISCRETE CONTACT**
   - clicks
   - impacts
   - gear teeth
   - rattles

2. **CONTINUOUS CONTACT**
   - scrape
   - squeal
   - rubbing
   - bearing friction

Do not try to generate both with one generic event generator.

Reference:
https://dafx.de/paper-archive/2017/papers/DAFx17_paper_58.pdf

---

## 7. Permissive implementation references

### STK — Synthesis ToolKit in C++

Repository:
https://github.com/thestk/stk

License:
MIT

Evidence class:
- DOCUMENTED IMPLEMENTATION

Relevant concepts/classes to study:
- modal synthesis
- banded waveguides
- resonant objects
- physical instrument models
- shaker/noise-driven resonators
- delay / allpass structures

Policy:
Direct code reuse is legally more permissive than GPL research projects, but any reuse still requires:
- attribution/license preservation;
- code review for realtime suitability;
- independent validation against Mechamorph requirements;
- no blind transplantation.

---

### Gamma

Repository:
https://github.com/LancePutnam/Gamma

License:
MIT

Evidence class:
- DOCUMENTED IMPLEMENTATION

Potential use:
- oscillators
- filters
- envelopes
- noise/random utilities
- delay / interpolation building blocks

Policy:
Treat as optional utility/reference code only. 125A should avoid dependency bloat when a small independently implemented primitive is clearer and easier to QA.

---

### DaisySP

Repository:
https://github.com/electro-smith/DaisySP

License:
MIT

Evidence class:
- DOCUMENTED IMPLEMENTATION

Relevant areas:
- resonators
- modal synthesis
- particle / clocked noise
- random generators
- nonlinear/physical-model blocks

Policy:
Use only the MIT DaisySP repository; distinguish it from separately licensed DaisySP-LGPL material.

---

### Harmonium Companion

Repository:
https://github.com/ledlaux/harmonium-companion

License:
MIT

Evidence class:
- DOCUMENTED IMPLEMENTATION

Useful concept:
- bellows / reservoir / leak / load-dependent air-pressure state

Mechamorph implication:
AIR should be a **state variable with memory**, not a free-running LFO.

---

## 8. Proposed independent Mechamorph equations / state

The following is an implementation direction, not a claim of exact historical-machine simulation.

### Mechanical drive

```
phase[n+1] = wrap(phase[n] + 2*pi*speed[n]/Fs)

speed[n] =
    targetSpeed
    + slowDrift
    + eccentricity * sin(phase)
    + loadResponse
```

Evidence:
- PHYSICS DERIVED for rotating phase
- EMPIRICALLY TUNED for drift/load mapping unless later measured

### Gear mesh event rate

```
meshFrequency = rotationFrequency * toothCount
```

Evidence:
- PHYSICS DERIVED

Add:
- eccentricity
- backlash
- tooth irregularity
- wear jitter

These modifiers must be measured/tuned rather than presented as exact hardware facts.

### Pressure reservoir

```
P[n+1] = P[n]
       + pumpInflow[n]
       - leak(P[n])
       - loadConsumption[n]
```

Evidence:
- PHYSICS DERIVED qualitative state model
- coefficients initially ESTIMATED / APPROXIMATED
- later calibrated against real bellows recordings

### Rattle event cluster

```
onPrimaryImpact:
    generate N secondary impacts
    with decreasing energy
    and variable inter-impact intervals
```

Evidence:
- PUBLISHED / PEER-REVIEWED concept
- detailed distributions to be MEASURED / EMPIRICALLY TUNED

---

## 9. Mandatory prototype measurements

Before judging the first prototype:

### Mechanical event statistics
Measure:
- event interval distribution
- level distribution
- cluster length
- repeat correlation

### Modal bodies
Measure:
- modal peak frequencies
- Q / decay
- inharmonicity
- spectral centroid vs. time
- response to different excitation spectra

### Friction
Measure:
- modulation period vs. drive speed
- spectral centroid vs. speed/pressure
- onset/offset transient behaviour
- repeatability / stochastic variation

### Air
Measure:
- pressure-cycle envelope
- leak decay
- noise spectrum
- correlation between crank phase and airflow amplitude

### Whole effect
Measure:
- dry/wet level
- frequency-response change
- DC
- noise floor
- transient timing
- stereo correlation
- CPU p95/p99/max
- callback deadline misses

---

## 10. Prototype acceptance question

The first prototype is successful only if level-matched listening and measurable behaviour support:

> The source and mechanical components are perceived as one coherent machine.

Reject or redesign if it sounds like:
- dry source plus Foley
- generic vinyl/lo-fi
- repetitive sample playback
- unrelated random clicks
- uncontrolled resonator ringing

---

## 11. Current evidence-driven architecture

```
                MECHANICAL DRIVE
                       |
        +--------------+--------------+
        |              |              |
      GEAR          RATCHET        BELLOWS
        |              |              |
        +------ CONTACT / EVENTS ------+
                       |
INPUT -> SOURCE TRANSFORM -> SHARED BODY -> OUTPUT
                       ^
                       |
               FRICTION / RATTLE
```

The shared body and mechanical state are mandatory architectural concepts for the proof of concept.


---

## 12. Gear / backlash / rattling evidence

### Idle gear impact and backlash

**Gear impacts and idle gear noise: Experimental study and non-linear dynamic model**  
Mechanical Systems and Signal Processing, 2009.

Key evidence:
- loose/unloaded gear pairs can generate repeated impacts;
- backlash and intermittent contact are central to rattle;
- impact energy depends on excitation kinematics;
- a reduced nonlinear SDOF contact model can capture useful behaviour.

Mechamorph implication:
- WEAR should increase more than random jitter;
- it should alter:
  - backlash window
  - missed contact / re-contact
  - secondary impact probability
  - impact energy distribution
- "rattle" should be mechanically related to load and drive speed.

Reference:
https://www.sciencedirect.com/science/article/pii/S0888327009001459

Evidence class:
- PUBLISHED / PEER-REVIEWED

### Gear mesh excitation

Gear-noise literature identifies:
- tooth engagement/disengagement
- time-varying mesh stiffness
- pitch/runout errors
- structural transmission to the housing

as major contributors to emitted gear noise.

Mechamorph implication:
The gear generator should contain at least two perceptual layers:

1. **mesh periodicity**
2. **impact / irregularity layer**

The BODY stage should receive gear excitation because real emitted sound is strongly shaped by the surrounding structure/housing.

References:
https://www.geartechnology.com/transmission-error-and-noise-emission-of-spur-gears
https://link.springer.com/article/10.1007/s10010-023-00685-4

Evidence class:
- PUBLISHED / DOCUMENTED ENGINEERING

---

## 13. Ratchet as speed-driven impact density

Research on tangible ratcheted-wheel sound synthesis describes a useful perceptual relationship:

- wheel speed controls impact density;
- individual impacts convey pawl/wheel material;
- ratchet sound communicates rotational interaction.

Mechamorph implication:
Ratchet timing should be **phase/speed derived**, while material/hardness affects the individual event spectrum.

This supports our rule that ratchet events must not be random independent clicks.

Reference:
https://www.sciencedirect.com/science/article/pii/S1071581909000901

Evidence class:
- PUBLISHED / PEER-REVIEWED

---

## 14. Bellows pressure as primary expressive state

Roland's documented accordion behaviour modelling links bellows pressure to:

- amplitude
- tone colour
- pitch
- onset/hysteresis behaviour

Academic harmonium modelling likewise uses bellows pressure as a key playing/control variable, with a reed chamber and enclosure response.

Mechamorph implication:
Even if v0.1.0 does not implement a free reed, AIR/PRESSURE can legitimately control several linked perceptual dimensions:

- airflow level
- attack strength
- filtered brightness
- subtle instability
- valve/chuff strength

This is better than using independent random modulators.

References:
https://www.rolandus.com/go/v-accordion/technology.html
https://www.dafx.de/paper-archive/2023/DAFx23_paper_47.pdf

Evidence class:
- DOCUMENTED / PUBLISHED

---

## 15. PRNG candidate

### xoshiro256++ / xoshiro256**

The xoshiro family is designed for very fast deterministic pseudorandom generation with compact state.

Potential Mechamorph use:
- microvariation
- rattle-cluster timing
- hardness variation
- wear irregularity
- deterministic seeded offline regression

Candidate implementation/reference:
https://github.com/nessan/xoshiro

Alternative small MIT wrapper:
https://github.com/Reputeless/Xoshiro-cpp

Policy:
Prefer either:
- independently implement the published xoshiro algorithm with correct attribution to the authors/public reference, or
- use a small MIT implementation with provenance recorded.

Do not use cryptographic randomness; deterministic bounded-cost behaviour is preferred for realtime audio.

Evidence class:
- DOCUMENTED IMPLEMENTATION
