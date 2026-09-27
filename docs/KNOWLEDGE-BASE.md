# 125A Mechamorph — Mechanical Sound Knowledge Base

## Core idea

**Modern sound in. Mechanical machine out.**

Mechamorph is conceived as an audio transformation effect, not a sampled barrel-organ instrument and not a generic lo-fi processor.

The target percept is:

> The incoming sound appears to be physically produced, driven or transmitted by an old mechanical music machine.

The mechanism must therefore be part of the signal behaviour, not merely an unrelated noise layer.

---

## Perceptual model

A convincing mechanical system can be reduced to a physically informed chain:

```
DRIVE -> MOTION -> CONTACT -> EXCITATION -> BODY -> AIR -> OUTPUT
```

The audio input may excite or be transformed by the same body and mechanical state that generates the incidental mechanical sounds.

### Shared mechanical state

A central state should control multiple subsystems coherently:

- Speed
- Phase
- Load
- Wear
- Pressure
- Backlash
- Loose parts
- Age / degradation
- Mechanical instability

This coupling is essential. If the virtual crank slows down, several things should react together: pitch drift, gear timing, bellows pressure, rattle probability and possibly amplitude.

---

## Mechanical sound families

### Impact

Real causes:
- hammer
- pin
- valve
- lever
- stop
- key mechanism

Percept:
- click
- clack
- knock
- thunk

DSP candidates:
- short impulse / noise burst
- modal resonator bank
- velocity-dependent excitation
- material-dependent decay

### Rattle / clatter

Real causes:
- loose parts
- multiple secondary collisions
- worn joints
- free metal plates

Percept:
- rattling
- chatter
- after-clacks
- metallic debris

DSP candidates:
- stochastic secondary impact generator
- decaying collision clusters
- randomized contact position
- shared resonator body

### Ratchet

Real causes:
- pawl and tooth wheel
- escapement-like mechanisms

Percept:
- repeated tick / click train

DSP candidates:
- speed-dependent pulse train
- tooth-count parameter
- wear-dependent timing variation
- skipped / doubled contacts

### Gear train

Real causes:
- meshing teeth
- eccentric gears
- backlash

Percept:
- periodic chatter
- whirr
- cyclic roughness

DSP candidates:
- gear-mesh pulse frequency derived from rotation speed and tooth count
- sideband modulation
- eccentricity / backlash modulation
- multiple coupled gear ratios

### Friction / scrape / squeak

Real causes:
- dry bearings
- wood-on-metal contact
- slipping belts
- hinges
- worn surfaces

Percept:
- scrape
- squeal
- creak
- rubbing

DSP candidates:
- filtered stochastic excitation
- stick-slip nonlinearity
- velocity and pressure controls
- contact roughness
- resonant body

### Spring

Real causes:
- tension spring
- leaf spring
- clock spring

Percept:
- boing
- zing
- metallic recoil

DSP candidates:
- dispersive delay / resonator network
- modal bank
- nonlinear excitation

### Bellows / air

Real causes:
- bellows
- reservoir
- valve leakage
- pneumatic channels

Percept:
- breath
- chuff
- hiss
- pressure pulse

DSP candidates:
- filtered turbulent noise
- pressure envelope tied to mechanical phase
- leak amount
- valve transient generator

### Valve / pneumatic trigger

Real causes:
- opening and closing of air valves

Percept:
- pop
- click
- chuff

DSP candidates:
- transient plus air burst
- attack linked to input transient
- velocity / pressure dependency

### Body / enclosure

Real causes:
- wood box
- thin metal
- cast housing
- tube
- cabinet

Percept:
- boxiness
- ringing
- physical size / material

DSP candidates:
- modal resonator bank
- convolution for selected bodies
- shared excitation from both dry input and mechanical events

### Pipes / reeds

Optional, not the core identity.

Possible sources:
- pipe resonance
- free reeds
- harmonium / accordion-like response

DSP candidates:
- digital waveguides
- reed oscillator / nonlinear excitation
- resonant filter bank

---

## Audio-to-machine coupling

The effect must avoid sounding like "music plus foley".

### Dynamic coupling

Input envelope / transient activity drives:
- impact density
- mechanical force
- valve strength
- air pressure
- rattle probability

### Spectral coupling

Input signal excites:
- the same body resonators used by mechanical elements
- shared material coloration
- optional resonant "machine voice"

### Modulation coupling

One central mechanical motion influences:
- pitch drift
- amplitude drift
- timing
- body excitation
- friction intensity
- noise / rattle density

### Optional pitch-aware coupling

Pitch tracking may be useful for monophonic material, but must not be required. The effect should remain useful on:
- chords
- pads
- drums
- loops
- polyphonic synths
- complex sound design

---

## Candidate machine models

### Clockwork
- ratchet
- spring
- small metal
- ticking
- fine gears

### Barrel mechanism
- crank
- wood body
- bellows
- valve events
- rollers / pins

### Tin automaton
- loose metal
- strong rattles
- bright modal body

### Pneumatic machine
- bellows
- valves
- leaks
- chuffs
- pressure instability

### Music-box mechanism
- pins
- comb-like metallic responses
- spring drive

### Orchestrion-inspired machine
- larger cabinet
- mixed pneumatic and mechanical behaviour
- multiple body types

### Broken machine
- wear
- backlash
- friction
- misfires
- drift
- loose parts

---

## Design principle

Prefer **physically informed, perceptually convincing models** over computationally expensive full physical simulation.

Goal: convince the ear, not reproduce a machine-engineering finite-element model.

---

## Sample strategy

Core identity should not depend on a large commercial sample library.

Preferred order:

1. procedural / synthesized mechanical events
2. CC0 or otherwise redistributable micro-textures with documented licenses
3. original 125A recordings
4. commercial libraries only as listening references, never redistributed unless the license explicitly allows plugin embedding

Potential categories for a small internal source library:

- gear
- ratchet
- click / clack
- rattle
- spring
- scrape
- squeak
- air
- valve
- wood
- metal
- chain
- lever
- switch
- lock
- loose screws / hardware

---

## Main technical risk

The primary risk is not VST3 implementation or CPU usage.

It is **sonic credibility and integration**.

If the machine layer sounds generic or detached, the result becomes ordinary lo-fi plus noise.

The success condition is:

> The listener should perceive one coherent physical mechanism that appears to be producing the transformed source.
