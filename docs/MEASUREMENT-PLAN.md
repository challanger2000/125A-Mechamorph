# Reference Audio Measurement Plan

## Objective

Convert real mechanical recordings into reproducible design targets before tuning DSP.

This follows the 125A sequence:

**understand -> derive -> measure -> implement -> measure again**

## Tool

`tools/analyze_reference_audio.py`

Dependencies:
- Python 3
- NumPy
- SciPy

The tool currently measures:

- sample rate / channels / duration
- peak and RMS
- crest factor
- zero-crossing rate
- mean/median spectral centroid
- 85% spectral rolloff
- dominant average spectral peaks
- transient/event count
- event rate
- inter-event interval statistics
- low-rate envelope periodicity

## Measurement source policy

Use only recordings with verified provenance.

Preferred:
1. original 125A recordings
2. CC0/Public Domain
3. CC-BY only for research where attribution is recorded

A file being downloadable does not establish release redistribution rights.

## Research folder convention

Local working layout:

```
research-audio/
  originals/
  working/
  measurements/
```

Do not commit third-party audio into the repository unless redistribution rights have been verified and the asset has passed the provenance process.

## First target set

### WOOD / GEAR

Candidate:
- Wooden Gear LQ 9 Clacking
- Wooden Gear LQ 5 Sprocket Rattling
- Wooden Gear shaft friction variants

Measure:
- event rate
- modal/spectral peaks
- event timing variation
- low-rate rotation periodicity
- centroid during friction vs impact sections

Goal:
derive initial:
- gear tooth/event density
- wooden-body resonator ranges
- clack hardness variation

### BELLOWS / AIR

Candidate:
- 20180327_Bellows.wav
- accordion/bellows CC0 references

Measure:
- envelope cycle
- dominant pump periodicity
- airflow spectral centroid
- pressure-release decay proxy
- transient chuff statistics

Goal:
derive initial:
- pump cycle
- pressure rise/fall time
- noise filter region
- leak decay range

### METAL RATTLE

Candidate:
- Metal Rattle FX.wav
- thin metal rattle recordings
- washer/bolt and loose hardware recordings

Measure:
- collision density
- inter-event distribution
- dominant modal peaks
- modal decay spread
- spectral centroid evolution

Goal:
derive:
- bounded secondary-event counts
- rattle cluster duration
- thin-metal body modes
- looseness macro mapping

### CRANK / CLOCKWORK

Candidate:
- toy crank
- wind-up
- clock spring / clock wind

Measure:
- periodicity
- impact density vs cycle
- timing jitter
- spectral stability across cycles

Goal:
derive:
- drive-speed range
- ratchet tooth-density model
- microvariation bounds

### FRICTION / HINGE / BEARING

Candidate:
- leather creak
- door hinge
- squeaky bearing

Measure:
- sustained spectral peaks
- modulation rate
- intermittency
- centroid vs apparent speed
- onset/offset statistics

Goal:
derive:
- initial friction-state transitions
- squeal resonator ranges
- speed-to-friction mapping

## Evidence tagging

Every value copied into DSP documentation must be tagged:

- MEASURED
- PHYSICS DERIVED
- PUBLISHED-PARAMETER DERIVED
- EMPIRICALLY TUNED
- ESTIMATED / APPROXIMATED

Do not call a value MEASURED unless the source audio was actually analyzed.

## Important limitation

Public Freesound metadata confirms several candidate files and CC0 status, but direct binary downloads may require user authentication.

Until the actual audio bytes are present locally:
- metadata may be documented
- source categories may be selected
- DSP target values must remain ESTIMATED / APPROXIMATED
- no fabricated measurements are allowed

## Initial acceptance target for measurement bank

Before parameter tuning, obtain at least:

- 3 wood/gear recordings
- 3 metal/rattle recordings
- 2 bellows/air recordings
- 2 crank/clockwork recordings
- 3 friction/hinge/bearing recordings

Prefer multiple recordings per mechanism to avoid fitting the plugin to one source.
