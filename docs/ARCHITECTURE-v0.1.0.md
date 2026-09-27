# Internal Architecture Draft — v0.1.0

## Goal

Define clear realtime-safe responsibilities before implementation.

## Top-level processor

`MechamorphProcessorCore`

Owns:
- sample rate / max block size
- parameter smoothing
- shared MechanicalState
- input analysis
- procedural engines
- shared body
- final mix/output
- deterministic random state

No GUI dependency.

---

## MechanicalState

Suggested fields:

```cpp
struct MechanicalState {
    double phase = 0.0;
    float speedHz = 0.0f;
    float targetSpeedHz = 0.0f;

    float load = 0.0f;
    float wear = 0.0f;
    float backlash = 0.0f;
    float looseness = 0.0f;

    float pressure = 0.0f;
    float leak = 0.0f;

    float inputEnvelope = 0.0f;
    float transientStrength = 0.0f;
};
```

This is an architectural sketch, not final code/API.

---

## Module responsibilities

### InputAnalyzer
Outputs:
- fast envelope
- slow envelope
- transient strength

Requirements:
- no allocation
- sample-rate invariant tuning
- deterministic

### MechanicalDrive
Owns:
- phase accumulator
- target speed smoothing
- eccentricity
- load response
- bounded drift source

Outputs:
- phase
- effective speed
- stroke phase if needed

### GearEngine
Inputs:
- MechanicalState
- parameter state

Outputs:
- bounded excitation events / sample stream

Never owns an unrelated LFO.

### RatchetEngine
Detects tooth/pawl phase crossings.

Requirements:
- robust across speed automation
- no double firing at block boundaries
- reverse/zero-speed policy explicitly defined later

### RattleEngine
Event cluster scheduler.

Implementation requirement:
- fixed-size event array/ring buffer
- hard cap on active events
- no allocation

### AirEngine
Owns:
- pressure integrator
- flow/noise filter state
- valve/chuff state

Pressure update must remain bounded.

### FrictionEngine
Owns:
- friction state
- colored excitation state
- optional resonant squeal state

Initial model may be reduced but must respond to speed/load.

### BodyResonator
Owns fixed number of modal resonators.

Inputs:
- transformed dry/input excitation
- gear/ratchet/rattle/air/friction excitation

Output:
- shared body response

### SourceTransform
Purpose:
Make the original audio participate in the machine.

Candidate operations for prototype:
- very small drive-correlated gain movement
- shared-body excitation
- optional bounded pitch/time micro-instability later

Do not start with elaborate pitch shifting.

### OutputMixer
Owns:
- dry/mechanized mix
- safe output gain
- optional DC blocker if measurement justifies it

---

## Event representation

Possible fixed-size event type:

```cpp
enum class EventType : uint8_t {
    Gear,
    Ratchet,
    Rattle,
    Valve
};

struct MechanicalEvent {
    EventType type;
    int32_t sampleOffset;
    float force;
    float hardness;
    float variation;
};
```

For v0.1.0:
- events are generated inside the audio block;
- maximum count per block is fixed;
- overflow policy is deterministic and measured.

---

## Randomness

Requirements:
- small deterministic PRNG
- no std::random_device on audio thread
- no dynamic distributions
- transform PRNG output with simple bounded math

Use randomness only for plausible microvariation.

Do not randomize:
- global machine identity every block
- timing independently of drive
- unrelated parameters

---

## Modal body implementation

First version:
- fixed parallel resonators
- coefficients recalculated only when parameters/sample rate require it
- smoothed or crossfaded coefficient changes if audible discontinuities occur

Potential implementation:
- complex one-pole/modal form
- stable 2nd-order bandpass resonators

Selection must be based on:
- stability
- cost
- parameter update behaviour
- impulse-response accuracy

---

## Parameter smoothing classes

Separate:
- sample-accurate/fast smoothing
- slow macro smoothing

Fast:
- output
- mechanize
- possible drive level

Slow:
- wear
- body morph
- mechanical identity

Do not smooth parameters blindly; define audible/discontinuity reason.

---

## State serialization plan

Store:
- all public parameters
- machine mode once introduced
- deterministic seed or PRNG state policy
- version marker

Do not serialize:
- transient detector instantaneous state unless required for exact live continuation
- temporary event buffers

State restore must never produce invalid resonator coefficients.

---

## Tail / latency expectation

Prototype target:
- **0 samples algorithmic latency** unless later analysis introduces lookahead
- nonzero audio tail from body resonators

Tail duration should be computed/reported conservatively once modal decay range is fixed.

---

## Failure containment

Any coefficient/state validation failure:
- clamp/revert to last valid coefficient set
- clear invalid module state
- never propagate NaN/Inf

Debug-only assertions are allowed outside release audio-path behaviour.

---

## Dependency policy

Prefer:
1. standard C++ + Steinberg VST3
2. small independently written DSP primitives
3. carefully reviewed permissive code only when it provides clear value

Avoid introducing large frameworks/libraries for one small DSP primitive.

Every external code contribution requires:
- license record
- exact file/version/commit
- retained copyright notice where required
- realtime/code review
- regression tests
