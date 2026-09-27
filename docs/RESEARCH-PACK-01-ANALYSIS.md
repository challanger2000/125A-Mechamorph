# CC0 Research Pack 01 — Analysis and First Machine Roles

## Sources

All five sources were downloaded from BigSoundBank / La Sonothèque and are marked CC0/public-domain equivalent on their source pages.

### projector35mm_0071
- source: 35mm Kinoton FP30 projector
- mono
- 48 kHz / 24-bit source page
- measured duration after research conversion: ~40.8 s

Observed state structure:
- ~0–5 s: START / engagement / acceleration
- ~5–27 s: stable RUN
- ~27–40 s: RELEASE / STOP / run-down

This is currently the strongest complete state-transition source.

### heidelberg_press_3409
- real Heidelberg printing press
- measured duration: ~39.6 s
- highly stable mechanical RUN bed

Observed:
- relatively constant RMS across most of recording
- dense cyclic mechanical contact
- strong body/roller/gear identity

Suggested role:
- RUN
- LOAD reinforcement
- BODY / heavy mechanism layer

### calculator_printer_3553
- old calculator-printer
- measured duration: ~13.2 s
- repeated discrete bursts separated by quieter gaps

Suggested role:
- ACTION
- INDEX
- CAM/SELECTOR
- RETURN/RESET

Strong event neighborhoods detected around:
- 1.09 s
- 3.19 s
- 5.41 s
- 7.34 s
- 9.30 s
- 11.69 s

### old_switch_0540
- old switch mechanism
- measured duration: ~14.8 s
- isolated mechanical contact gestures with long quiet gaps

Strong event neighborhoods:
- 0.23 s
- 1.72 s
- 3.12 s
- 4.66 s
- 6.29 s
- 7.90 s
- 9.48 s
- 11.00 s
- 12.58 s
- 14.13 s

Suggested roles:
- LEVER
- DETENT
- CONTACT
- ACTION
- RELEASE

### slide_projector_0867
- slide projector
- measured duration: ~95.2 s
- quiet machine bed with repeated strong advance/index mechanisms

Strong actions recur every few seconds.

Suggested roles:
- ACTION
- INDEX
- SLIDE/CARRIAGE movement
- LOAD transition
- occasional larger mechanical gesture

---

# First offline proof — Lost Workshop Machine v0.1

A 24-second render was assembled from the CC0 sources with no synthetic machine sound required.

Structure:

```
0.0–5.2 s
  PROJECTOR START

4.7–17.7 s
  PROJECTOR RUN
  + HEIDELBERG PRESS RUN

10.5–17.5 s
  stronger PRESS layer to simulate LOAD

5.8–17.4 s
  OLD SWITCH / CALCULATOR / SLIDE PROJECTOR action events

17.8 s onward
  PROJECTOR STOP / RUN-DOWN
```

No EQ, reverb or compression is required for the proof.
Only gain placement, fades and safety normalization are used.

## Important conclusion

The source recordings naturally provide far more convincing mechanical identity than the synthetic contact/pipe experiments.

Therefore the main development direction is confirmed:

> **real mechanical sample identity + deterministic machine-state DSP**

The procedural DSP should schedule, vary and couple these sources.
It should not attempt to synthesize every real-world mechanical timbre from scratch.

---

# Next pack requirements

Research Pack 02 should focus on gaps not covered well by Pack 01:

- hand crank / hand wheel
- chain / chain hoist
- freewheel / pawl
- spring return
- worn bearing
- squeak / friction
- loose washers / bolts
- pressure / steam release
- start/stop of a smaller machine
- latch / lock / detent
- slower, heavier mechanism
- faster fine clockwork

Goal:
build multiple machine identities rather than one obvious projector/printing-press collage.
