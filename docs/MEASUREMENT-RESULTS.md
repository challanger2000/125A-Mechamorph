# Measurement Results Register

This file contains **no fabricated measurements**.

Populate only after audio bytes have been analyzed with a recorded tool version and source provenance.

## Result entry template

### <source internal ID>

- Original asset:
- Provenance record:
- SHA-256:
- Analyzer commit:
- Analyzer schema version:
- Sample rate:
- Channels:
- Duration:
- Evidence class: **MEASURED**

#### Level
- Peak:
- RMS:
- Crest factor:

#### Spectrum
- Mean centroid:
- Median centroid:
- 85% rolloff:
- Dominant peaks:

#### Events
- Count:
- Events/s:
- Median inter-event interval:
- p10/p90 inter-event interval:

#### Periodicity
- Dominant low-rate modulation:
- Periodicity strength:

#### Interpretation
- Relevant Mechamorph module:
- Parameters informed:
- Caveats:
- Sections excluded from analysis:

---

## Aggregation rule

Never tune a core material or mechanism from one recording when multiple sources are available.

For each mechanism class, prefer:
- median across sources for central tendency;
- percentile range for variation;
- source-by-source inspection for outliers.

Do not normalize away meaningful dynamics before measuring event force distributions.
