# Machine Sample Lab — CC0 Prototype Manifest

This manifest is for **prototype sound-design validation**.

The current automated lab uses BigSoundBank / La Sonothèque MP3 preview files because they are directly retrievable without authentication. The underlying source pages explicitly mark the recordings as **CC0 / public-domain equivalent** and permit editing, redistribution and commercial use.

For a shipping plugin, prefer the original lossless WAV downloads where available.

## Sources

### Heidelberg printing press #4
- Role: RUN BED / industrial mechanical body
- Source page: https://bigsoundbank.com/heidelberg-printing-press-4-s3407.html
- Direct prototype audio: https://bigsoundbank.com/UPLOAD/mp3/3407.mp3
- License: CC0 / public-domain equivalent
- Source metadata: 48 kHz / 24-bit / stereo
- Prototype use:
  - middle stable section -> RUN
  - stronger section -> LOAD candidate

### 35mm cinema projector #7
- Role: START / STOP state gestures
- Source page: https://bigsoundbank.com/35mm-cinema-projector-7-s0071.html
- Direct prototype audio: https://bigsoundbank.com/UPLOAD/mp3/0071.mp3
- License: CC0 / public-domain equivalent
- Source metadata: 48 kHz / 24-bit / mono
- Description explicitly includes start, running and stop.
- Prototype use:
  - beginning -> START
  - ending -> STOP

### Old switch
- Role: ACTION / contact / lever-like gesture source
- Source page: https://bigsoundbank.com/old-switch-s0540.html
- Direct prototype audio: https://bigsoundbank.com/UPLOAD/mp3/0540.mp3
- License: CC0 / public-domain equivalent
- Source metadata: 48 kHz / 16-bit / mono
- Prototype use:
  - automatic transient extraction -> ACTION pool

### Calculator-printer #1
- Role: ACTION / indexing / compact retro mechanism
- Source page: https://bigsoundbank.com/calculator-printer-1-s3553.html
- Direct prototype audio: https://bigsoundbank.com/UPLOAD/mp3/3553.mp3
- License: CC0 / public-domain equivalent
- Source metadata: 48 kHz / 24-bit / mono
- Prototype use:
  - automatic transient extraction -> ACTION / RELEASE candidates

## Prototype machine recipe

```
START:
  projector beginning

RUN:
  Heidelberg press stable middle section

ACTION:
  old switch one-shots
  calculator-printer one-shots

LOAD:
  alternate Heidelberg segment / same run bed slowed by engine load coupling

RELEASE:
  selected calculator-printer return/index event if suitable

STOP:
  projector ending
```

## Acceptance

A 30–40 second render must sound like **one coherent physical machine**, not like four unrelated recordings collaged together.

If source identity remains too obvious:
- shorten events
- use smaller micro-events
- increase shared timing/state coupling
- reduce full-length recognizable source phrases
- diversify action pools
- add shared body later
