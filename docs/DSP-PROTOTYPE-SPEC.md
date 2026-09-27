# DSP Prototype Specification — v0.1.0

## Objective

Build the smallest realtime-safe proof of concept that can establish whether arbitrary audio can be transformed into the percept of a coherent old mechanical music machine.

The first prototype is not a release candidate. It is an engineering experiment.

---

## 1. Fixed scope

### Included
- shared mechanical drive
- gear/ratchet event generation
- rattle cluster generator
- air/bellows state
- friction proxy
- shared body resonator
- input-derived excitation
- dry/wet and output control

### Excluded for v0.1.0
- GUI polish
- presets beyond test states
- pitch tracking
- historical machine emulation claims
- convolution body IRs
- sample-library dependency
- oversampling unless a nonlinear block measurably requires it
- MIDI
- tempo sync
- multiple machine models

---

## 2. Signal architecture

```
INPUT
  |
  +--> envelope / transient detector --------------------+
  |                                                     |
  +--> source transform --------------------------------+----> SHARED BODY ---> MIX ---> OUTPUT
                                                        ^
MECHANICAL DRIVE -> gear / ratchet / rattle / air / friction
```

The important design rule:

**input-derived energy and generated mechanical events must excite the same physical/body stage.**

---

## 3. Shared mechanical state

Realtime state:

- drivePhase
- driveSpeed
- targetSpeed
- load
- wear
- backlash
- looseness
- pressure
- leak
- frictionState
- deterministic random state

All random processes must:
- be deterministic for a given stored seed/state where practical;
- avoid heap allocation;
- use bounded work;
- not create pathological event storms.

---

## 4. Input analysis

### Envelope follower
Need:
- fast attack path
- slower energy path

Purpose:
- determine mechanical force
- drive air/load response
- scale generated events

### Transient detector
Initial method:
- smoothed fast/slow energy ratio or high-pass energy delta

Purpose:
- trigger:
  - primary clacks
  - valve events
  - rattle clusters

Acceptance:
- should work on drums and melodic attacks;
- should not retrigger continuously on steady pads;
- should remain stable across sample rates.

No FFT is required for the first prototype.

---

## 5. Mechanical drive

### Phase
Use one central phase accumulator.

### Speed behaviour
Components:
- target rotation speed
- low-rate bounded drift
- eccentricity-linked periodic variation
- load-dependent slowdown

Initial constants must be labelled **EMPIRICALLY TUNED** until measured.

### Why one drive
Every mechanical subsystem must derive timing from one drive state or a rationally related sub-state.

This creates causal coherence.

---

## 6. Gear / ratchet engine

### Gear
Inputs:
- drive speed
- tooth count
- eccentricity
- wear
- load

Output:
- mesh/excitation pulses
- low-level continuous chatter

### Ratchet
Inputs:
- phase crossings
- wear
- backlash

Output:
- discrete pawl/tooth impacts
- optional rebound

### Variation
Allowed:
- amplitude variation
- tiny timing variation
- contact hardness variation

Not allowed:
- independent unconstrained random rhythm unrelated to drive.

---

## 7. Rattle / loose-part engine

Triggered by:
- strong input transient
- strong gear/ratchet impact
- high looseness state

Each cluster:
- bounded maximum event count
- decaying energy
- variable but bounded time gaps
- shared material/body resonator

No unbounded stochastic process.

---

## 8. Air / bellows engine

State model:

```
pressure += pumpInflow
pressure -= leakLoss
pressure -= signalLoad
pressure = clamp(pressure)
```

Pump inflow:
- periodic component linked to mechanical phase
- optionally asymmetric up/down stroke

Air output:
- filtered noise scaled by pressure/flow
- transient chuff on valve-like activity

Pressure may modulate:
- overall mechanized level
- brightness
- slight pitch instability in selected modes later

For v0.1.0 keep pressure coupling conservative.

---

## 9. Friction engine

First version is intentionally reduced.

Inputs:
- drive speed
- load
- wear

Generate:
- colored noise excitation
- nonlinear threshold / stick-slip inspired gating
- resonant squeak mode

This is a placeholder research implementation, not a claim of exact friction physics.

Acceptance:
- must audibly follow speed/load;
- must not become generic static noise.

---

## 10. Shared body

First body:
**small old wooden cabinet with light metal hardware**

Implementation:
- parallel modal resonator bank
- 6–16 modes initially
- stable biquad/resonator implementation
- input and mechanical events share the same resonator stage

Parameters:
- modal frequency
- Q/decay
- gain
- optional tiny stereo variation

Modes must be:
- stable at all supported sample rates;
- bounded under repeated excitation;
- reset correctly.

No convolution in first prototype.

---

## 11. User-facing prototype controls

- MECHANIZE
- CRANK
- CLATTER
- WOBBLE
- AIR
- BODY
- WEAR
- OUTPUT

125A scaling:
- 0% neutral/off
- 20–50% useful musical range
- 50–75% clearly audible
- 75–100% strong/creative
- 100% meaningful maximum

### Neutrality
MECHANIZE = 0% must produce a defined neutral/dry result.

Any remaining generated noise at 0% is a defect unless intentionally changed and documented.

---

## 12. Parameter mapping

### MECHANIZE
Macro controlling:
- transformed-source mix
- mechanical event contribution
- shared-body excitation amount

Must not merely crossfade dry to noise.

### CRANK
Controls target mechanical drive speed / activity.

Possible future modes:
- free Hz
- tempo sync

v0.1.0:
- free-running only

### CLATTER
Controls:
- rattle probability
- secondary impacts
- mechanical transient density

### WOBBLE
Controls:
- drive speed variation
- related signal instability

### AIR
Controls:
- bellows/airflow audibility
- pressure coupling

### BODY
Controls:
- shared resonator contribution

### WEAR
Controls multiple causally related faults:
- backlash
- contact irregularity
- friction
- leak
- looseness

This must be a coherent macro, not several unrelated effects.

---

## 13. Realtime constraints

Audio callback:
- no allocation
- no locks
- no file/network I/O
- no logging
- bounded event count
- denormal protection
- preallocated buffers/state

Maximum secondary rattle events per input transient must be explicitly capped.

---

## 14. Determinism

Offline regression requires repeatability.

Use:
- explicit PRNG state
- stable seed in saved plugin state if random variation is audible

On state restore:
- parameters restore exactly
- deterministic generator state policy must be defined

Two acceptable policies:
1. save/restore PRNG state exactly;
2. reset deterministically from saved seed.

Choose one before release.

---

## 15. Test fixtures

### Synthetic
- silence
- impulse
- single sine
- logarithmic sweep
- white noise
- repeated impulse train
- DC rejection test
- very low-level decay

### Real audio
- mono saw synth
- poly pad
- piano
- vocal
- drum loop
- metallic percussion
- full mix fragment

Use fixed files for regression.

---

## 16. Measurements

### Neutral
At MECHANIZE=0:
- output delta vs input
- level
- phase/latency
- noise floor

### Body
- impulse response
- modal frequencies
- decay constants
- peak gain
- stability

### Mechanical event engine
- event density
- max event count
- interval distribution
- repeat correlation

### Noise/air
- RMS
- spectrum
- modulation correlation with pressure/drive phase

### Whole processor
- peak/RMS change
- DC offset
- crest factor
- spectral centroid
- transient timing
- stereo correlation
- latency/tail
- CPU mean/p95/p99/max
- deadline overruns

---

## 17. Listening evaluation

Use level-matched A/B.

Questions:
1. Does the source still read musically?
2. Does the result sound physically mechanical?
3. Do source and mechanism sound causally connected?
4. Is it more than generic lo-fi?
5. Is repetition obvious within 30–60 seconds?
6. Does WEAR increase believable degradation rather than random chaos?
7. Are 20–50% settings genuinely useful?

Do not call success from novelty alone.

---

## 18. Stop conditions

Do not expand scope if any of these remain unsolved:

- mechanical noises sound layered-on
- shared body rings uncontrollably
- drive state does not perceptually unify the system
- 0% is not neutral
- event repetition is obvious
- CPU tails are unacceptable
- steady sounds create uncontrolled event spam
- strong settings destroy output level unpredictably

---

## 19. Next version only after proof

After v0.1.0 passes its proof:
- additional bodies/materials
- proper nonlinear friction model
- spring/beam resonator
- dedicated pneumatic valve
- multiple machine archetypes
- optional CC0/original microtextures
- tempo synchronization
- static product-design phase
