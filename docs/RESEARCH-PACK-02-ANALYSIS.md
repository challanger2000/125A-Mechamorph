# Research Pack 02 — Machine Roles

## Main findings

Pack 02 fills the most important gaps from Pack 01.

### Manual winch
Strong candidate for:
- CRANK / DRIVE
- hand-operated speed
- engagement and load feel

### Sewing machine slow / fast
Especially valuable because the **same machine family is represented at two real operating speeds**.

Use:
- low-speed RUN
- high-speed RUN
- speed crossfade / state transition reference

Measured low-rate envelope periodicity is clearly different between the slow and fast recordings, supporting real state-based speed switching rather than simply pitch-shifting one loop.

### Large ratchet
Strong:
- pawl identity
- indexed motion
- action event pool
- speed-dependent ratchet source

### Large spring
Contains several distinct spring gestures across a long recording.

Use:
- RETURN
- RELEASE
- spring recoil
- worn-machine accent

### Medium chain
Useful for:
- LOAD
- chain slack
- loose hardware
- drive/linkage movement

### Air burst
Use only as a causal pressure/release event.
Not as a constant air/noise layer.

### Stapler
Not used literally as "stapler".
Provides compact:
- spring-loaded hard contact
- latch-like action
- return click

### Roller shutter
Hand-cranked mechanism with roller/load behaviour.
Useful as an alternate drive family.

### Ratchet/rattle
Dense ratchet texture useful as:
- high-speed pawl bed
- worn/loose alternate state

## Hybrid machine conclusion

Packs 01+02 are already sufficient to test a real state machine:

```
START
  projector / winch

RUN SLOW
  winch + sewing slow

RUN FAST
  sewing fast + press

ACTION
  ratchet / old switch / calculator / stapler

LOAD
  chain + press

RELEASE
  spring + optional air burst

STOP
  projector run-down + final spring/contact
```

This is the first architecture that can plausibly sound like a coherent machine without relying on synthetic "mechanical" effects.
