# Mechanical Sound Identity — Literal Component Requirement

## Core rule

A mechanical component must sound recognizably like the component it represents.

This is stricter than "mechanically flavored DSP".

Examples:

- a **clack** must read as a physical clack/contact;
- a **valve** must read as valve actuation + air event;
- a **pipe** must read as an air-column / pipe-like sounding element;
- a **reed** must read as a pressure-driven reed;
- a **spring** must read as spring resonance/recoil;
- a **gear** must read as tooth/mesh/contact motion;
- a **bellows** must read as pumping leather/air pressure;
- a **wooden cabinet** must read as wood-body resonance.

If the listener cannot identify the mechanical cause, the model is not specific enough.

---

# Consequence for Mechamorph

The plugin cannot be built from generic abstractions such as:

- "short transient"
- "modal resonance"
- "colored noise"
- "wobble"
- "distortion"

Those are implementation tools, not the sound target.

Every module needs a literal target sound first.

---

# Required component identities

## CLACK / LEVER CONTACT

Real percept:
- hard contact onset;
- very short broadband transient;
- material-dependent resonance;
- often one stronger hit plus a smaller rebound;
- body coupling.

Needed model:
- contact impulse with hardness;
- wood/metal contact resonator;
- optional rebound;
- same cabinet/body coupling.

Acceptance:
Soloed module must be recognizable as a small physical lever/pawl/contact.

---

## VALVE

Real percept:
- mechanical click/opening;
- short pressure release/chuff;
- different closing event;
- pressure-dependent strength.

Needed model:
1. valve opening contact;
2. airflow transient;
3. sustained flow while open;
4. closing contact/release.

Acceptance:
With pitch/sounding element muted, the event should still read as a valve/pneumatic mechanism rather than generic noise.

---

## PIPE

Real percept:
- pressure-driven onset;
- air-column resonance;
- stable pitched body;
- breath/air component;
- attack depends on pressure;
- release follows valve/pressure closure.

Needed model:
- air-column/waveguide or equivalent resonant source;
- pressure-controlled excitation;
- attack transient;
- optional chiff;
- finite valve release.

Acceptance:
Solo output should clearly evoke a small organ/barrel-organ pipe.

---

## REED

Real percept:
- nonlinear pressure threshold;
- buzzy/harmonic excitation;
- pressure controls amplitude and timbre;
- possible hysteresis / onset delay;
- small instability.

Needed model:
- pressure-driven nonlinear oscillator or reed transfer;
- resonant tract/body;
- pressure-dependent spectral tilt/harmonic content.

Acceptance:
Must read as a reed/harmonium/accordion-family sound, not as a saw wave with EQ.

---

## BELLOWS

Real percept:
- pump cycle;
- leather/fold creak;
- airflow;
- reservoir pressure;
- slight lag between pumping and sounding result.

Needed model:
- pump state;
- reservoir state;
- pressure-dependent flow;
- optional leather/fold microevents.

Acceptance:
Muted sounding element should still reveal pumping behaviour rather than continuous white noise.

---

## GEAR

Real percept:
- tooth mesh periodicity;
- repetitive small contacts;
- speed determines density;
- housing/body resonance;
- wear/backlash changes contact pattern.

Needed model:
- phase-derived tooth crossings;
- material-specific tooth contact;
- mesh whirr component;
- backlash/re-contact under wear.

Acceptance:
Soloed output must sound like a gear train rather than a click sequencer.

---

## RATCHET / PAWL

Real percept:
- distinct directional click sequence;
- click density follows rotation;
- pawl rebound;
- harder than generic rattle.

Needed model:
- phase-driven discrete pawl strikes;
- asymmetric attack/rebound;
- material resonator.

Acceptance:
Must read as ratchet/escapement-like mechanism.

---

## SPRING

Real percept:
- metallic recoil;
- dispersive ringing;
- pitch glide/settling;
- characteristic boing/zing.

Needed model:
- dispersive resonator or calibrated sample-derived model;
- mechanical trigger;
- nonlinear decay optional.

Acceptance:
Must read as spring, not generic metallic reverb.

---

## FRICTION / BEARING

Real percept:
- continuous or intermittent scrape/squeal;
- speed dependent;
- stick-slip;
- often tonal squeal plus noisy component.

Needed model:
- nonlinear friction state;
- speed/pressure dependence;
- resonant squeal/body coupling.

Acceptance:
Must read as physical rubbing/bearing/hinge motion.

---

## WOOD CABINET

Real percept:
- short, uneven modal response;
- low/mid woody knocks;
- constrained high-frequency decay;
- cavity/body contribution.

Needed model:
- measured/calibrated modal set;
- shared excitation from tone and mechanics.

Acceptance:
Impulse response must evoke a physical wooden enclosure, not generic resonator EQ.

---

# Architecture implication

Each component gets two things:

1. **Identity model**
   - what makes this object recognizably itself;

2. **Causal coupling**
   - when and why the object is excited.

Example:

```
CRANK
  -> gear tooth
      -> real gear contact identity
      -> shared wood body

BARREL PIN
  -> lever
      -> real clack identity
      -> valve opens
          -> real valve/chuff identity
          -> pressure drives pipe
              -> real pipe identity
```

That causal chain is the product.

---

# Testing rule

Every mechanical component must first pass a **solo identity test**.

Question:

> If this module is heard alone, does it sound like the named physical thing?

Only after passing that test may it be integrated into the full machine.

This prevents vague DSP layers from hiding behind the full mix.

---

# Development order

1. Reference recordings for each component.
2. Measure the recognizable features.
3. Build one component model.
4. Solo A/B against references.
5. Correct until identity is convincing.
6. Integrate into causal machine chain.
7. Re-test in context.

No more "add generic transient/noise/resonance and hope it reads as mechanical."
