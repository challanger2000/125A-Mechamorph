# Research Audio

Third-party source audio is **not committed here by default**.

Local convention:

```
research-audio/
  originals/       # untouched downloaded/original files
  working/         # trimmed/converted research copies
  measurements/    # JSON analyzer output
```

Before any third-party file is added to a shipping package, follow `docs/PROVENANCE.md`.

Files in `originals/` should retain:
- original filename
- source URL
- author
- license
- retrieval date
- checksum

The Git repository should contain measurements and provenance metadata, not an uncontrolled mirror of external sound libraries.
