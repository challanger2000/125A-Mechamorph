# Asset Candidates — Verified Mechanical Sources

Status: initial verified research pass.

This file is a candidate database, not a release manifest. Nothing here is automatically approved for shipping until it has been downloaded, archived, and provenance-recorded.

## CC0 candidates

| Category | Asset | Author | Format / Quality | License | Intended use | Source |
|---|---|---|---|---|---|---|
| ratchet / gear / metal | Turnstile_RX.wav | strikingtwice | WAV, 48 kHz, 24-bit, mono | CC0 | ratchet, clunk, metal mechanism reference / micro-texture | https://freesound.org/people/strikingtwice/sounds/260208/ |
| wood gear / rattle | Wooden Gear LQ 5 Sprocket Rattling.wav | hisoul | WAV, 96 kHz, 24-bit, stereo | CC0 | wooden sprocket, rattling, friction | https://freesound.org/people/hisoul/sounds/461166/ |
| wood gear / friction | Wooden Gear LQ 8 Cog Friction Rattling.wav | hisoul | WAV, 96 kHz, 24-bit, stereo | CC0 | cog friction + rattle | https://freesound.org/people/hisoul/sounds/461163/ |
| wood shaft / friction | Wooden Gear LQ 2 Shaft Friction.wav | hisoul | WAV, 96 kHz, 24-bit, stereo | CC0 | wood-on-wood friction / shaft motion | https://freesound.org/people/hisoul/sounds/461161/ |
| wood gear / clack | Wooden Gear LQ 9 Clacking.wav | hisoul | WAV, 96 kHz, 24-bit, stereo | CC0 | dry clacking, gear attack layer | https://freesound.org/people/hisoul/sounds/461167/ |
| spring mechanism | Spring Mechanism | alegemaate | WAV, 44.1 kHz, 32-bit, stereo | CC0 | spring-open / spring-close source | https://freesound.org/people/alegemaate/sounds/667279/ |
| compact metal mechanism | Mechanism Activation Sequence | qubodup | FLAC, 48 kHz, 16-bit, mono | CC0 | lever / spring / activation micro-sequence | https://freesound.org/people/qubodup/sounds/752067/ |
| metal click | Metallic_Click | BlondPanda | WAV, 48 kHz, 24-bit, mono | CC0 | switch / trigger transient | https://freesound.org/people/BlondPanda/sounds/778444/ |
| metal spring resonance | Clang single long swing.WAV | Fionnsty | WAV, 48 kHz, 24-bit, stereo | CC0 | spring/body resonance study | https://freesound.org/people/Fionnsty/sounds/273967/ |
| large wooden gear | Wooden Gear inside of old Watermill | tombinambur | WAV, 96 kHz, 16-bit, stereo | CC0 | full mechanical ambience / rotation reference | https://freesound.org/people/tombinambur/sounds/347628/ |
| wood clicks / rattle | Board Game Pieces.WAV | taure | WAV, 44.1 kHz, 16-bit, stereo | CC0 | small wood impact / rattle extraction | https://freesound.org/people/taure/sounds/555190/ |
| mixed mechanical impacts | Mechanical Sounds | BMacZero | WAV collection | CC0 | clank, light clunk, rattle, squeaky click, mechanical impacts | https://opengameart.org/content/mechanical-sounds |

## Why these are useful

The first set deliberately focuses on **isolated mechanical primitives**, not finished cinematic sound effects.

That makes them more useful for Mechamorph because we can:

- cut them into micro-events
- extract envelopes and spectra
- use them as listening references for our procedural generators
- build variation pools
- combine them with a shared resonator/body instead of playing whole stock effects
- compare synthesized clacks, rattles and friction against real sources

## Research-code references

### Clatter

Repository:
https://github.com/alters-mit/clatter

Purpose:
- impact synthesis
- scrape synthesis
- perceptually informed physical variables
- material / mass / motion relationships

Important license note:
Clatter currently uses the **Hippocratic License 3.0**, not MIT/BSD/ISC. Therefore treat it as a research reference only unless a deliberate license review later says otherwise. No code should be copied into Mechamorph.

### MechanOdd

Repository:
https://github.com/odoare/MechanOdd

Purpose:
- excitation + resonator architecture
- strings
- plates
- membranes
- beams
- feedback coupling
- global resonator concepts

Current policy:
The repository is valuable as a conceptual/DSP reference. No explicit license was surfaced in the repository search during this pass, so its code is **not approved for reuse**. Treat it as research-only unless the license is later verified.

## Next candidate searches

Still missing good verified CC0 sources for:

- bellows / hand-pumped air
- valve chuffs
- leather creaks
- old hinges
- chains
- clock escapements
- clock springs
- wind-up mechanisms
- loose screws / washers
- thin sheet-metal chatter
- cast metal body knocks
- small wooden cabinet resonance
- dry bearing squeak
- pulley / belt friction
- cams / followers
- manual crank rotation

These should be researched next before deciding whether we need original recordings.
