#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sample_dir", type=Path)
    args = ap.parse_args()

    wavs = sorted(args.sample_dir.glob("*.wav"))
    seen = {}
    duplicates = []

    for path in wavs:
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest in seen:
            duplicates.append((seen[digest], path))
        else:
            seen[digest] = path

    print(f"WAV files checked: {len(wavs)}")
    print(f"Exact duplicates: {len(duplicates)}")

    for a, b in duplicates:
        print(f"DUPLICATE: {a.name} == {b.name}")

    if duplicates:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
