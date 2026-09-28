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
    "friction": 6,
}

def profile_mask(name: str, role: str) -> int:
    n = name.lower()
    profiles = {
        "tiny": 1 << 0,
        "intricate": 1 << 1,
        "heavy": 1 << 2,
        "colossal": 1 << 3,
        "pneumatic": 1 << 4,
        "broken": 1 << 5,
    }

    mask = 0
    for token, bit in profiles.items():
        if f"__{token}__" in f"__{n}__":
            mask |= bit
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

    profile_names = ("tiny","intricate","heavy","colossal","pneumatic","broken")
    required_roles = {0:"START",1:"RUN",2:"ACTION",5:"STOP"}
    print("PROFILE POOL MATRIX")
    for profile_index, profile_name in enumerate(profile_names):
        bit = 1 << profile_index
        counts = {role: 0 for role in ROLE_MAP.values()}
        for _, _, role, mask, _ in rows:
            if mask & bit:
                counts[role] += 1
        summary = " ".join(
            f"{name.upper()}={counts[ROLE_MAP[name]]}"
            for name in ("start","run","action","load","release","stop","friction")
        )
        print(profile_name.upper(), summary)
        for role_id, role_name in required_roles.items():
            if counts[role_id] == 0:
                raise SystemExit(
                    f"profile {profile_name} missing required role {role_name}"
                )

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
