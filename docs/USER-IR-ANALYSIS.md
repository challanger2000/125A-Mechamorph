# User IR Analysis — Mechamorph BODY / SPACE Research

These files were supplied by the user for research/testing.

Do **not** assume redistribution rights from the file name alone.
Until provenance/license is documented, use them only as private reference/test material.

## Factory Hall(1).wav

Measured:
- 44.1 kHz
- stereo
- duration: ~5.272 s
- mono peak: ~0.412
- mono RMS: ~0.00291
- spectral centroid: ~747 Hz
- energy decay:
  - -20 dB: ~0.873 s
  - -40 dB: ~5.164 s
  - -60 dB: ~5.272 s

Interpretation:
- very long industrial-space tail
- comparatively dark/low-mid weighted
- useful reference for large HALL / SPACE behavior
- likely best at medium/high SPACE and larger SCALE

## PA horn in hall.wav

Measured:
- 44.1 kHz
- stereo
- duration: ~2.903 s
- mono peak: ~0.522
- mono RMS: ~0.00773
- spectral centroid: ~1009 Hz
- energy decay:
  - -20 dB: ~0.896 s
  - -40 dB: ~2.460 s
  - -60 dB: ~2.897 s

Interpretation:
- shorter and more focused than Factory Hall
- strong mid character
- useful reference for compact industrial hall / metallic PA-space

## Retro Machines IR.wav

Measured:
- 96 kHz
- stereo
- duration: ~3.499 s
- mono peak: ~0.983
- mono RMS: ~0.02743
- spectral centroid: ~1139 Hz
- energy decay:
  - -20 dB: ~0.745 s
  - -40 dB: ~1.731 s
  - -60 dB: ~2.541 s

Interpretation:
- strongest direct/body-like impulse of the three
- shorter technical/retro-machine character
- candidate reference for BODY / MACHINE-CABINET resonance
- could be used before/under SPACE rather than as the long room itself

## Proposed architecture

```
MACHINE CORE
   |
   +--> BODY
   |      - short/technical IR or equivalent body response
   |      - machine/material character
   |
   +--> SPACE
          - industrial hall IR / equivalent room response
          - room depth and scale
```

BODY and SPACE must remain perceptually distinct.

### BODY
Should answer:
> What physical cabinet/frame/enclosure is vibrating?

### SPACE
Should answer:
> In what industrial environment is the machine located?

## Control-gate requirement

Before either appears in the final UI:

- BODY 0/50/100 must produce a clear, independent body/material change.
- SPACE 0/50/100 must produce a clear, independent room/depth change.
- Neither control may merely act as a loudness macro.
- BODY and SPACE must not collapse into the same perceptual function.
