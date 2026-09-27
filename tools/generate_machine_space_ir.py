#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import numpy as np
from scipy.io import wavfile

def read_float(path: Path):
    sr, x = wavfile.read(path)
    if x.ndim == 1:
        x = np.stack([x, x], axis=1)
    elif x.shape[1] == 1:
        x = np.repeat(x, 2, axis=1)

    if np.issubdtype(x.dtype, np.integer):
        info = np.iinfo(x.dtype)
        scale = max(abs(info.min), info.max)
        x = x.astype(np.float64) / float(scale)
    else:
        x = x.astype(np.float64)

    return int(sr), x[:, :2]

def resample_linear(x: np.ndarray, sr: int, target: int = 48000):
    if sr == target:
        return x
    n = int(round(len(x) * target / sr))
    old = np.arange(len(x), dtype=np.float64)
    new = np.linspace(0.0, max(0.0, len(x)-1), n)
    out = np.empty((n, 2), dtype=np.float64)
    for ch in range(2):
        out[:, ch] = np.interp(new, old, x[:, ch])
    return out

def extract_taps(path: Path, window_ms: float, tap_count: int, min_sep_ms: float):
    sr, x = read_float(path)
    x = resample_linear(x, sr, 48000)

    mag = np.max(np.abs(x), axis=1)
    peak = int(np.argmax(mag))
    end = min(len(x), peak + int(window_ms * 0.001 * 48000))
    seg = x[peak:end].copy()

    if len(seg) == 0:
        raise RuntimeError(f"empty IR: {path}")

    # Relative to the direct peak. Do not let one giant direct impulse turn
    # BODY/SPACE into gain-only processing.
    norm = max(1e-12, np.max(np.abs(seg)))
    seg /= norm

    env = np.max(np.abs(seg), axis=1)
    min_sep = max(1, int(min_sep_ms * 0.001 * 48000))
    order = np.argsort(env)[::-1]

    chosen = []
    for idx in order:
        idx = int(idx)
        if env[idx] < 0.002:
            break
        if all(abs(idx-j) >= min_sep for j in chosen):
            chosen.append(idx)
            if len(chosen) >= tap_count:
                break

    if 0 not in chosen:
        chosen.append(0)

    chosen = sorted(set(chosen))
    taps=[]
    for idx in chosen:
        l=float(seg[idx,0])
        r=float(seg[idx,1])
        # keep direct component conservative; BODY/SPACE are parallel paths
        if idx == 0:
            l *= 0.35
            r *= 0.35
        taps.append((idx,l,r))

    # Energy normalize sparse set to stable output.
    energy=sum(l*l+r*r for _,l,r in taps)
    gain=0.9/np.sqrt(max(energy,1e-12))
    taps=[(d,l*gain,r*gain) for d,l,r in taps]
    return taps

def emit_array(f, name: str, taps):
    f.write(f"inline constexpr SparseIrTap {name}[] = {{\n")
    for d,l,r in taps:
        f.write(f"    {{{d}u, {l:.9f}f, {r:.9f}f}},\n")
    f.write("};\n")
    f.write(f"inline constexpr std::size_t {name}Count = sizeof({name}) / sizeof({name}[0]);\n\n")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--body", type=Path, required=True)
    ap.add_argument("--warehouse", type=Path, required=True)
    ap.add_argument("--reactor", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    args=ap.parse_args()

    body=extract_taps(args.body, 140.0, 24, 1.5)
    warehouse=extract_taps(args.warehouse, 650.0, 40, 6.0)
    reactor=extract_taps(args.reactor, 1000.0, 48, 8.0)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w",encoding="utf-8") as f:
        f.write("#pragma once\n#include <cstddef>\n#include <cstdint>\n\n")
        f.write("namespace MechamorphMachine {\n")
        f.write("struct SparseIrTap { std::uint32_t delay48k; float left; float right; };\n\n")
        emit_array(f,"kBodyTaps",body)
        emit_array(f,"kWarehouseTaps",warehouse)
        emit_array(f,"kReactorTaps",reactor)
        f.write("}\n")

    print("BODY taps",len(body))
    print("WAREHOUSE taps",len(warehouse))
    print("REACTOR taps",len(reactor))

if __name__ == "__main__":
    main()
