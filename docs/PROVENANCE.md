# Third-Party Asset / Code Provenance Policy

## Purpose

Mechamorph may use external research, permissive source code, and CC0 micro-textures.

Every external item must remain traceable.

---

## Asset record template

For every audio asset considered for inclusion:

```
Internal ID:
Original filename:
Internal filename:
Category:
Author:
Source URL:
Source platform:
Retrieved date:
Original license:
License URL/text:
Attribution required: yes/no
Modification allowed: yes/no
Commercial use allowed: yes/no
Redistribution inside commercial plugin verified: yes/no/unclear
Original technical format:
Checksum SHA-256:
Edits performed:
Intended role:
Release approved by:
Notes:
```

### Rule

**Free to use in music/video is not enough.**

For embedding an audio file in a distributable plugin, redistribution rights must be explicit or the asset must be CC0/Public Domain/original.

---

## Source-code record template

```
Project:
Repository:
Exact commit/tag:
Files/concepts used:
License:
Copyright holder(s):
Direct code copied: yes/no
Modified code: yes/no
Required notices:
Realtime review:
125A tests added:
Notes:
```

### GPL / incompatible projects

Research concepts may be studied.

No source copied into proprietary 125A code.

Prefer primary papers/equations when independently implementing a published technique.

---

## Research-paper record

```
Title:
Authors:
Venue/year:
DOI/URL:
Relevant concept:
Evidence classification:
Implementation notes:
Unresolved assumptions:
```

---

## Naming

Third-party assets should receive internal names only after provenance is recorded.

Do not erase provenance by renaming a file and losing its original identity.

---

## Processing third-party CC0 samples

Allowed workflow after verification:
- trim
- denoise if necessary
- normalize only with documented method
- resample
- extract micro-events
- derive analysis data
- create transformed derivative assets

Keep:
- original untouched source outside release package or in controlled research archive
- checksum
- processed-file checksum
- processing recipe

---

## Release packaging

A release asset manifest must list every embedded non-125A source.

Prefer a release containing:
- mostly procedural synthesis
- original 125A assets
- only a small set of essential verified CC0 micro-textures

This reduces:
- legal ambiguity
- package size
- repetition
- dependence on third-party recordings
