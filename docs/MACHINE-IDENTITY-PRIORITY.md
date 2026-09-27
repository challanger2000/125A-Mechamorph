# Machine Identity Priority — Revised

## Core product identity

Mechamorph should sound like a **mechanical machine**, not like an organ, pipe instrument, or reed instrument.

The machine identity must primarily come from:

- rotating drive
- gears
- ratchets / pawls
- cams
- levers
- springs
- backlash
- friction
- bearings
- loose hardware
- mechanical load
- rebounds
- enclosure/body resonance
- pressure systems only where mechanically relevant
- steam/air release only as optional machine events

A musical pipe/reed is **not** a core requirement.

---

## New priority order

### P0 — must define the sound

1. DRIVE / CRANK
2. GEAR TRAIN
3. CAM / LEVER MOTION
4. CONTACT / CLACK
5. BACKLASH / RE-CONTACT
6. FRICTION / BEARING
7. SPRING / RETURN FORCE
8. RATTLE / LOOSE HARDWARE
9. SHARED BODY / ENCLOSURE
10. LOAD / ENERGY COUPLING

### P1 — optional machine pressure systems

- valve actuation
- compressed air
- pneumatic hiss/chuff
- pressure release
- steam release
- relief valve
- piston/cylinder-like events

These are relevant only if they reinforce the machine identity.

### P2 — optional sounding elements

- pipe
- reed
- tine
- comb
- bell

These are not required for the core product.

---

## Revised architecture

```
INPUT
  |
  v
MECHANICAL RE-ARTICULATION / ENERGY TRANSFER
  |
  v
DRIVE -> GEAR -> CAM -> LEVER -> CONTACT
   |       |       |       |        |
   |       |       |       |        +--> CLACK / REBOUND
   |       |       |       +-----------> SPRING RETURN
   |       |       +-------------------> TIMING / ACTUATION
   |       +---------------------------> MESH / BACKLASH
   +-----------------------------------> SPEED / LOAD

             + FRICTION / BEARING
             + RATTLE / LOOSE PARTS
             + OPTIONAL PRESSURE RELEASE
                         |
                         v
                 SHARED MACHINE BODY
                         |
                         v
                       OUTPUT
```

---

## Key acceptance criterion

Even with:
- no pipe
- no reed
- no musical resonator
- no decorative steam hiss

the system must already sound like:

> a physical machine is operating and transforming the source.

If that does not happen, the mechanical core is still wrong.

---

## Steam / pressure role

Steam/air should be treated as a **machine event**, not a musical voice.

Examples:

- relief valve opens under load
- pressure dump after a cycle
- piston/air release
- short pneumatic chuff
- leaking seal
- overpressure hiss

These events should be caused by machine state:
- load
- pressure
- cycle phase
- wear

They must not be random ambience layers.

---

## Design consequence

The current pipe prototype remains a research experiment only.

Do not make further core development dependent on it.

Next proof of concept should be:

**one mechanically coherent mini-machine with no musical pipe at all.**

Suggested chain:

```
CRANK -> GEAR -> CAM -> LEVER -> CLACK -> SPRING RETURN
             \-> BACKLASH
              \-> FRICTION
               \-> OPTIONAL PRESSURE RELEASE
                       |
                       v
                  SHARED BODY
```

This machine should be audible and convincing by itself before it is used to transform incoming audio.
