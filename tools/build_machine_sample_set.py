#!/usr/bin/env python3
"""
Build a curated machine sample set from a CSV manifest.

Research/offline tool only.
- Reads mono/stereo WAV source files.
- Extracts manifest time ranges.
- Converts to mono float internally.
- Applies very short boundary fades.
- Writes PCM16 WAV assets plus a JSON metadata file.
- Performs no EQ, compression, reverb or creative processing.
"""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly


def read_mono(path: Path) -> tuple[int, np.ndarray]:
    sr, raw = wavfile.read(path)
    x = raw.astype(np.float64)

    if np.issubdtype(raw.dtype, np.integer):
        x /= float(max(abs(np.iinfo(raw.dtype).min), np.iinfo(raw.dtype).max))

    if x.ndim > 1:
        x = np.mean(x, axis=1)

    return int(sr), x


def fade_boundaries(x: np.ndarray, sr: int, fade_ms: float = 6.0) -> np.ndarray:
    y = x.copy()
    n = min(len(y) // 2, max(1, int(sr * fade_ms * 0.001)))
    if n > 1:
        y[:n] *= np.linspace(0.0, 1.0, n)
        y[-n:] *= np.linspace(1.0, 0.0, n)
    return y


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("manifest", type=Path)
    ap.add_argument("source_dir", type=Path)
    ap.add_argument("output_dir", type=Path)
    ap.add_argument("--target-rate", type=int, default=48000)
    args = ap.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)
    entries = []

    with args.manifest.open(newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))

    for row in rows:
        src = args.source_dir / row["source_file"]
        if not src.exists():
            print(f"SKIP missing: {src}")
            continue

        sr, x = read_mono(src)
        a = max(0, int(float(row["start_s"]) * sr))
        b = min(len(x), int(float(row["end_s"]) * sr))
        if b <= a:
            print(f"SKIP invalid range: {row['internal_id']}")
            continue

        clip = x[a:b]

        if sr != args.target_rate:
            clip = resample_poly(clip, args.target_rate, sr)
            sr = args.target_rate

        clip = fade_boundaries(clip, sr)

        peak = float(np.max(np.abs(clip))) if len(clip) else 0.0
        # Safety only: preserve relative dynamics unless clipping would occur.
        if peak > 0.98:
            clip *= 0.98 / peak
            peak = 0.98

        out_name = f"{row['role'].lower()}__{row['internal_id']}.wav"
        out_path = args.output_dir / out_name
        wavfile.write(out_path, sr, (clip * 32767.0).astype(np.int16))

        entries.append({
            "id": row["internal_id"],
            "role": row["role"],
            "source": row["source_file"],
            "source_start_s": float(row["start_s"]),
            "source_end_s": float(row["end_s"]),
            "priority": row["priority"],
            "notes": row["notes"],
            "file": out_name,
            "sample_rate": sr,
            "frames": int(len(clip)),
            "duration_s": float(len(clip) / sr),
            "peak": peak,
        })

    meta = {
        "schema": 1,
        "target_sample_rate": args.target_rate,
        "clips": entries,
    }
    (args.output_dir / "machine_sample_set.json").write_text(
        json.dumps(meta, indent=2) + "\n", encoding="utf-8"
    )

    print(f"Built {len(entries)} machine clips in {args.output_dir}")


if __name__ == "__main__":
    main()
