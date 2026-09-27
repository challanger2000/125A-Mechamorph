# Prototype QA Matrix — v0.1.0

## Branch / build identity

Every test record must include:
- repository
- branch
- commit SHA
- plugin version
- compiler/build configuration
- OS
- host/tester
- sample rate
- block size

## Functional matrix

### Sample rates
- 44.1 kHz
- 48 kHz
- 88.2 kHz
- 96 kHz
- 192 kHz where practical

### Block sizes
- 1 if host/test harness supports it
- 16
- 32
- 64
- 128
- 256
- 512
- 1024
- irregular block sizes if harness supports them

### Channels
- mono
- stereo

### Modes
- realtime
- offline render

## Safety

Test:
- silence for extended duration
- NaN input protection policy
- Inf input protection policy
- denormal/subnormal stress
- extreme automation
- all controls min/max
- activate/deactivate loops
- editor lifecycle once GUI exists
- repeated state save/restore
- bypass transitions

## DSP-specific

### Drive
- phase continuity
- sample-rate independence
- bounded speed
- no discontinuity under automation

### Events
- deterministic seed behaviour
- max event count enforced
- no event queue overflow
- no duplicate-trigger explosion

### Body
- impulse stability
- worst-case resonance gain
- decay to silence
- no self-oscillation unless explicitly designed
- reset on activate/deactivate

### Air
- pressure bounded
- leak reaches stable state
- silence does not pump indefinitely
- no DC

### Friction
- bounded output
- no stuck NaN state
- low-speed behaviour
- zero-speed silence policy

## Regression

Store fixed renders for:
- neutral
- 25%
- 50%
- 75%
- 100%

For:
- impulse
- saw synth
- drums
- pad

Compare:
- hashes only where fully deterministic
- otherwise metric tolerances + seeded deterministic render

## Realtime

Record:
- mean
- p95
- p99
- max callback time
- deadline
- overrun count

Test both:
- typical settings
- worst-case settings

Profiler overhead must be measured separately.

## Acceptance

Prototype engineering pass requires:
- no crashes
- no invalid samples
- neutral path verified
- bounded event generation
- stable resonators
- repeatable state
- realtime behaviour documented
- audible proof of the core causal-machine concept

This does not constitute release QA.
