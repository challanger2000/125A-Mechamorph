# Control Physics Map — Mechamorph

## Design rule

Every user control must answer four questions:

1. **What physical mechanism does this control represent?**
2. **What audible consequence would that mechanism create in a real machine?**
3. **What DSP structure reproduces that consequence?**
4. **How do we verify that the result actually behaves like the mechanism?**

No control may exist merely because "EQ / compression / saturation / modulation sounds old."

---

# What a real mechanical music machine actually does

Historical barrel organs and related automatic instruments are coupled systems.

A hand crank can simultaneously:

- rotate the pinned cylinder/barrel;
- operate bellows;
- drive gears;
- actuate valves or trackers;
- trigger pipes, reeds, piano actions or other sounding elements.

The same mechanical motion therefore affects:

- timing;
- pressure;
- attack;
- duration;
- mechanical noise;
- pitch/timbre stability;
- physical resonance.

This shared causality is central.

A barrel organ, for example, is not:

> pipe sound + crank noise + old EQ

It is:

> crank motion -> barrel pins -> lever/valve action -> bellows pressure -> pipe excitation -> cabinet/body radiation

Sources:
- The Metropolitan Museum of Art barrel organ documentation
- Association of Musical Box Collectors chamber barrel organ mechanism
- Princeton EPICS player-piano pneumatic mechanism

---

# Core perceptual target

The user should hear that the **same machine movement** is responsible for:

- when a sound starts;
- how hard it starts;
- how long it sustains;
- how pressure builds and falls;
- how the tone wavers;
- when clicks/clacks happen;
- how the body resonates.

Mechanical noises must therefore be consequences of articulation, not independent decoration.

---

# Proposed control set — physical meaning first

## 1. MECHANIZE

### Physical meaning

How completely the incoming audio is forced through the mechanical articulation system.

This does **not** mean wet/dry in the conventional sense.

### Real-world analogy

At 0%:
- source bypasses the machine.

At 20–50%:
- source is clearly being actuated by the mechanism but remains musically recognizable.

At 50–75%:
- articulation, body and pressure increasingly dominate.

At 75–100%:
- the original source behaves mainly as excitation/content for the virtual machine.

### Audible consequences

Increasing MECHANIZE should cause:

- attacks to become mechanically shaped;
- sustains to follow valve/cam pressure behaviour;
- timing to obey machine motion;
- source envelope to become less electronically smooth;
- machine body to become part of the sound;
- mechanical incidental events to become causally attached.

### DSP

Macro over:

- source re-articulation depth;
- cam/valve envelope depth;
- shared-body excitation;
- pressure coupling;
- event coupling;
- source retention.

### Wrong implementation

- simple wet/dry;
- EQ tilt;
- compressor amount;
- saturation amount;
- louder Foley layer.

### Verification

At 50%, with incidental mechanics muted:

> Is the source itself already audibly mechanically actuated?

If not, MECHANIZE is wrong.

---

## 2. CRANK

### Physical meaning

Mechanical drive speed / energy.

### Real-world mechanism

In a barrel organ the crank can simultaneously:

- turn the barrel through worm/gearing;
- pump bellows;
- advance the complete mechanism.

Therefore CRANK affects much more than repetition rate.

### Audible consequences

Increasing CRANK should change:

- articulation cadence;
- cam/valve timing;
- gear/rachet rate;
- pumping frequency;
- pressure replenishment;
- friction regime;
- slight timing regularity/instability;
- possibly attack sharpness under higher mechanical energy.

### DSP

One shared phase/speed state drives:

- cam apertures;
- ratchet tooth crossings;
- gear mesh events;
- bellows pump cycle;
- periodic load;
- friction speed;
- optional actuator timing.

### Wrong implementation

- tremolo rate only;
- LFO speed only;
- delay time;
- sample playback speed alone.

### Verification

Changing CRANK should audibly change several related behaviours **together**.

---

## 3. CLATTER

### Physical meaning

Mechanical looseness / secondary contact energy.

### Real-world mechanism

Loose linkages, pawls, washers, worn joints and rebound create additional collisions after a primary actuation.

### Audible consequences

Increasing CLATTER should create:

- more secondary contacts;
- stronger rebound;
- longer collision clusters;
- more audible cabinet excitation;
- slightly less precise articulation.

The primary event must remain identifiable.

### DSP

Primary actuator event -> bounded cluster:

- 0..N secondary collisions;
- decaying energy;
- mechanically plausible spacing;
- same body resonator;
- density related to force/load.

### Wrong implementation

- random clicks unrelated to note/transient;
- noise bursts independent of machine state;
- generic bitcrush.

### Verification

No primary actuation -> no clatter cluster.

---

## 4. WOBBLE

### Physical meaning

Rotational irregularity / eccentric drive / uneven transmission.

### Real-world mechanism

Possible causes:

- eccentric gear/barrel;
- uneven crank motion;
- changing load;
- worn bearings;
- compliant belt/linkage;
- pressure load feeding back into motion.

### Audible consequences

WOBBLE should affect:

- timing between actuator events;
- pressure cycle timing;
- slight amplitude instability;
- possibly pitch/timbre of pressure-driven sounding elements;
- mechanical event spacing.

It should not simply chorus the final output.

### DSP

Modulate **machine state before articulation**:

```
drive speed -> actuator timing -> pressure -> source envelope
```

Optional later:
small pitch influence for reed/pipe machine types.

### Wrong implementation

- post-effect vibrato;
- stereo chorus;
- arbitrary pitch LFO on the entire output.

### Verification

With incidental noise muted, WOBBLE must still change the physical articulation.

---

## 5. AIR

### Physical meaning

Pneumatic/bellows contribution and pressure authority.

### Real-world mechanism

Barrel organs use bellows/feeders/reservoirs to supply air. Springs/weights regulate reservoir pressure; valves admit air to individual pipes. Leaks and pressure variations alter behaviour.

AMBC describes:
- crank-driven feeder bellows;
- reservoir;
- spring/weight pressure regulation;
- leather-faced valves/pallets;
- spill valve for excess air.

### Audible consequences

AIR should control:

- pressure build-up;
- valve/chuff character;
- sustain authority;
- attack softness/hardness;
- release behaviour;
- subtle turbulent air;
- pressure sag under load.

At zero:
- pneumatic component absent.

At high values:
- audible breathing/pumping;
- stronger pressure envelope;
- more obvious valve behaviour.

### DSP

Stateful pressure reservoir:

```
pump -> reservoir -> valve demand -> sounding element
```

Not just noise level.

### Wrong implementation

- filtered white noise underneath;
- static high shelf;
- compressor pump.

### Verification

AIR must still change the source envelope when air noise itself is muted.

---

## 6. BODY

### Physical meaning

How strongly the machine housing/soundboard/windchest participates.

### Real-world mechanism

Mechanical energy and acoustic source energy are transmitted into:

- wooden cabinet;
- wind chest;
- metal hardware;
- soundboard;
- internal cavities.

### Audible consequences

Increasing BODY should create:

- modal coloration;
- finite resonant decay;
- stronger coupling between clacks and tone;
- sense of object size/material;
- less "direct electronic source".

### DSP

Shared modal system excited by:

- re-articulated source;
- actuator impacts;
- gear/clatter;
- air/valve impulses.

Potential body families later:
- wood cabinet;
- thin tin;
- brass plate;
- iron housing;
- mixed wooden/metal machine.

### Wrong implementation

- static EQ;
- generic room reverb;
- resonator only on the noise layer.

### Verification

Impulse response must show physically plausible modes/decays, and all subsystems must excite the same body.

---

## 7. WEAR

### Physical meaning

Mechanical degradation.

### Real-world causes

- increased backlash;
- leakage;
- dried bearings;
- weakened/irregular springs;
- loose fittings;
- worn valve surfaces;
- uneven contact;
- detuning/instability.

### Audible consequences

WEAR should alter several linked systems:

- backlash/re-contact;
- pressure leakage;
- friction;
- timing precision;
- secondary collisions;
- valve closure;
- possibly resonator damping.

### DSP

Macro controlling physically related fault parameters.

Example mapping:

```
WEAR
 -> backlash up
 -> leak up
 -> friction threshold irregularity up
 -> rattle probability up
 -> timing tolerance up
 -> damping/material irregularity up
```

### Wrong implementation

- distortion amount;
- noise amount;
- random modulation amount.

### Verification

At 100% it should sound like **the same machine in bad condition**, not a different effect preset.

---

# Missing control candidate: ACTUATOR

This may be more fundamental than some existing controls.

## Physical meaning

What kind of mechanism converts the machine movement into sound.

Possible families:

- VALVE / PIPE
- REED
- HAMMER
- TINE / COMB
- PLUCK
- MIXED AUTOMATON

This could become either:

- one selector;
- machine-model presets;
- an internal architecture choice rather than a knob.

It should be tested before product UI is frozen.

---

# The real signal chain

The new core should conceptually be:

```
INPUT
  |
  v
MUSICAL ENERGY / SPECTRAL CONTENT
  |
  v
MECHANICAL ACTUATOR
  |        ^
  |        |
  |    DRIVE / CRANK
  |
  +--> VALVE / HAMMER / CAM / REED
  |
  v
RE-ARTICULATED SOURCE
  |
  +------> SHARED BODY
  |             ^
  |             |
  +-- causal mechanical contacts
  |
  v
OUTPUT
```

AIR, WOBBLE, WEAR and CLATTER alter this chain at physically appropriate locations.

---

# Machine archetype chosen for the next proof

The first proof should be based on a **small crank-driven pneumatic barrel organ / serinette-type mechanism**, because it contains nearly everything we need:

- crank;
- gearing;
- pinned barrel / mechanical sequencing;
- bellows;
- reservoir;
- valves;
- pipes/reeds;
- wooden case.

The Met documents that one crank can simultaneously rotate the barrel and pump the bellows, while barrel pins/levers open valves that admit air to the pipes.

This makes it an excellent reference model for causally linked mechanical sound.

It is better for the proof than an abstract "steampunk machine."

---

# Next research questions before DSP rewrite

1. What does a real crank-driven barrel organ sound like **with close mechanical noise audible**?
2. How fast are:
   - crank cycles;
   - bellows cycles;
   - valve attacks/releases;
   - pin/lever contacts?
3. How much timing variation is normal?
4. What spectral/temporal characteristics distinguish:
   - pipe/reed tone;
   - valve chuff;
   - lever click;
   - gear/barrel noise;
   - cabinet resonance?
5. How much of the perceived "mechanical" character remains if incidental noises are removed?
6. Which source properties can be preserved without pitch tracking?

Only after these are answered should the re-articulation DSP be finalized.
