#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path

ROLE_MAP = {
    "start": 0,
    "run": 1,
    "action": 2,
    "load": 3,
    "release": 4,
    "stop": 5,
}

def profile_mask(name: str, role: str) -> int:
    n = name.lower()
    mask = 0

    # PROJECTOR = 1
    ok = (
        ("projector" in n and role in {"start", "run", "stop"}) or
        (role == "action" and ("slide" in n or "switch" in n)) or
        (role == "release" and "switch" in n)
    )
    if ok:
        mask |= 1

    # HANDCRANK = 2
    ok = (
        ("winch" in n and role in {"start", "run", "stop", "release"}) or
        (role == "action" and ("ratchet" in n or "switch" in n)) or
        (role == "load" and "chain" in n) or
        (role == "release" and "spring" in n)
    )
    if ok:
        mask |= 2

    # INDUSTRIAL = 4
    ok = (
        ("winch" in n and role in {"start", "stop"}) or
        (role == "run" and "press" in n) or
        (role == "action" and ("calc" in n or "stapler" in n or "ratchet" in n)) or
        (role == "load" and ("press" in n or "chain" in n)) or
        (role == "release" and ("air" in n or "spring" in n))
    )
    if ok:
        mask |= 4

    return mask

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sample_dir", type=Path)
    ap.add_argument("out_dir", type=Path)
    args = ap.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    wavs = sorted(args.sample_dir.glob("*.wav"))

    rows = []
    resource_id = 2000

    for p in wavs:
        stem = p.stem
        prefix = stem.split("__", 1)[0].lower()
        if prefix not in ROLE_MAP:
            continue
        mask = profile_mask(stem, prefix)
        if mask == 0:
            continue
        rows.append((resource_id, stem, ROLE_MAP[prefix], mask, p.resolve()))
        resource_id += 1

    if not rows:
        raise SystemExit("no eligible machine samples")

    header = args.out_dir / "machine_assets_generated.h"
    rc = args.out_dir / "machine_assets_generated.rc"

    with header.open("w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("#include <cstddef>\n\n")
        f.write("namespace MechamorphMachine {\n")
        f.write("struct EmbeddedAssetMeta { int resourceId; const char* name; int role; int profileMask; };\n")
        f.write("inline constexpr EmbeddedAssetMeta kEmbeddedAssets[] = {\n")
        for rid, name, role, mask, _ in rows:
            escaped = name.replace("\\", "\\\\").replace('"', '\\"')
            f.write(f'    {{{rid}, "{escaped}", {role}, {mask}}},\n')
        f.write("};\n")
        f.write("inline constexpr std::size_t kEmbeddedAssetCount = sizeof(kEmbeddedAssets) / sizeof(kEmbeddedAssets[0]);\n")
        f.write("}\n")

    with rc.open("w", encoding="utf-8") as f:
        for rid, _, _, _, path in rows:
            # RC accepts forward slashes in quoted paths.
            rp = str(path).replace("\\", "/")
            f.write(f'{rid} RCDATA "{rp}"\n')

    print(f"embedded assets: {len(rows)}")
    for rid, name, role, mask, _ in rows:
        print(rid, name, "role", role, "mask", mask)

if __name__ == "__main__":
    main()
