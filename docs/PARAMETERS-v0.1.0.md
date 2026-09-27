# Parameter Contract — v0.1.0

These IDs are reserved before VST3 integration so automation/state semantics do not drift casually.

| ID | Symbol | Display | Default | Neutral | Notes |
|---:|---|---|---:|---:|---|
| 1000 | kMechanize | MECHANIZE | 0.35 | 0.0 | Master machine transformation macro |
| 1001 | kCrank | CRANK | 0.35 | n/a | Mechanical drive activity/speed |
| 1002 | kClatter | CLATTER | 0.25 | 0.0 | Secondary impact/rattle density |
| 1003 | kWobble | WOBBLE | 0.15 | 0.0 | Drive-correlated instability |
| 1004 | kAir | AIR | 0.20 | 0.0 | Bellows/air audibility and coupling |
| 1005 | kBody | BODY | 0.35 | 0.0 | Shared resonant body contribution |
| 1006 | kWear | WEAR | 0.20 | 0.0 | Backlash/friction/leak/irregularity macro |
| 1007 | kOutput | OUTPUT | 0.50 | 0.50 | Output trim; center = unity target |

Rules:
- IDs are not to be renumbered after public release.
- 0% is neutral/off for effect-intensity controls.
- CRANK is not an effect amount; its minimum will map to the slowest meaningful machine state once measured.
- OUTPUT mapping is not final until gain range is specified and measured.
- Current defaults are **EMPIRICALLY TUNED placeholders** for prototype audibility, not final product defaults.
