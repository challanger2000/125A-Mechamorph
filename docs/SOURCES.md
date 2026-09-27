# Research Sources and Asset Policy

This file tracks research references and potential asset sources for 125A Mechamorph.

## License policy

Every external sample or code reference must be classified before use.

### Allowed for direct plugin inclusion

Preferred:
- CC0 / Public Domain
- original 125A recordings
- assets with an explicit license allowing redistribution inside commercial software/plugins

### Research/reference only unless separately cleared

- GPL code: may be studied conceptually, but **no code is copied into proprietary 125A implementation**
- royalty-free stock audio without explicit redistribution rights
- commercial SFX libraries
- Kontakt / sampler libraries
- unclear licenses

---

## Mechanical sample sources

### Freesound

Useful search target for isolated:
- gears
- ratchets
- springs
- wood mechanisms
- friction
- clicks / clacks
- rattles

Important: license is file-specific. Only assets with a clearly compatible license should be considered for redistribution.

Candidate reference:
- Freesound wooden gear / mechanical recordings
- Freesound ratchet / gear / switch recordings

Source:
https://freesound.org/

### OpenGameArt

Potential CC0 mechanical sound packs.

Source:
https://opengameart.org/

### itch.io mechanical SFX packs

Useful for listening research. Redistribution rights must be checked pack-by-pack.

Source:
https://itch.io/

### Pixabay sound effects

Useful as listening references and prototype audition material. Do not embed in a shipped plugin without verifying the current license specifically permits redistribution as a plugin asset.

Source:
https://pixabay.com/sound-effects/

### BOOM Library — Mechanicals

Professional reference for how isolated mechanical construction-kit elements are categorized and recorded.

Reference only unless a separate license explicitly permits embedding.

Source:
https://www.boomlibrary.com/sound-effects/mechanicals/

### Sonniss GDC bundles

High-quality reference material. Treat as production-use sound libraries, not as redistributable plugin assets unless explicitly licensed for that purpose.

Source:
https://gdc.sonniss.com/

---

## Open-source / research references

### Clatter

Research-oriented procedural / physically informed impact and scraping sound synthesis.

Source:
https://github.com/alters-mit/clatter

Use:
- conceptual research
- understand reduced physical parameterization
- impact / scrape event modelling

Do not copy incompatible code into the 125A implementation.

### MechanOdd

Mechanical/resonator sound synthesis reference using resonant structures.

Source:
https://github.com/odoare/MechanOdd

Use:
- conceptual study of resonators
- strings / plates / membranes / beams
- feedback and excitation strategies

### Modal synthesis references

Search target:
- modal resonator banks
- material-dependent resonance
- collision / impact synthesis
- wood / metal / glass perceptual modelling

### Digital waveguides

Useful for:
- pipes
- tubes
- strings
- air-column behaviour

Background:
https://ccrma.stanford.edu/~jos/waveguide/

---

## Scientific / technical topics to research further

- modal synthesis of impact sounds
- stochastic collision models
- friction / stick-slip models
- scraping and rolling sound synthesis
- gear mesh frequency and sidebands
- digital waveguides for acoustic tubes
- free-reed physical modelling
- bellows / pneumatic pressure modelling
- perceptual material recognition
- resonator coupling
- transient detection
- envelope-driven event generation
- parameterized mechanical wear / backlash

---

## Asset provenance rule

For every asset eventually placed in the repository, record:

- filename
- source URL
- author
- original license
- date retrieved
- whether modification is allowed
- whether redistribution in commercial software is allowed
- attribution requirement
- internal processing performed

No third-party audio asset should enter a release package without this record.
