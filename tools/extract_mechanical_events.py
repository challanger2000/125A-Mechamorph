#!/usr/bin/env python3
"""
Extract short mechanical one-shots from 16-bit mono PCM WAV files.

Standard-library only, for GitHub Actions reproducibility.
"""

from __future__ import annotations

import argparse
import math
import struct
import wave
from pathlib import Path


def read_pcm16_mono(path: Path):
    with wave.open(str(path), "rb") as wf:
        if wf.getnchannels() != 1:
            raise ValueError("expected mono WAV")
        if wf.getsampwidth() != 2:
            raise ValueError("expected 16-bit PCM WAV")
        sr = wf.getframerate()
        frames = wf.getnframes()
        raw = wf.readframes(frames)
    data = struct.unpack("<" + "h" * frames, raw)
    return sr, [v / 32768.0 for v in data]


def write_pcm16_mono(path: Path, sr: int, x):
    path.parent.mkdir(parents=True, exist_ok=True)
    peak = max((abs(v) for v in x), default=0.0)
    scale = 1.0 if peak <= 0.78 or peak == 0 else 0.78 / peak
    samples = []
    for v in x:
        q = max(-1.0, min(1.0, v * scale))
        samples.append(int(round(q * 32767.0)))
    raw = struct.pack("<" + "h" * len(samples), *samples)
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sr)
        wf.writeframes(raw)


def moving_rms(x, window):
    window = max(1, int(window))
    sq = [v * v for v in x]
    out = [0.0] * len(x)
    acc = 0.0
    for i, v in enumerate(sq):
        acc += v
        if i >= window:
            acc -= sq[i - window]
        n = min(i + 1, window)
        out[i] = math.sqrt(max(0.0, acc / n))
    return out


def percentile(values, q):
    if not values:
        return 0.0
    s = sorted(values)
    idx = int(round((len(s) - 1) * q))
    return s[max(0, min(len(s) - 1, idx))]


def fade_edges(x, fade):
    fade = min(fade, len(x) // 2)
    if fade <= 0:
        return
    for i in range(fade):
        g = i / float(fade)
        x[i] *= g
        x[-1 - i] *= g


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input", type=Path)
    ap.add_argument("output_dir", type=Path)
    ap.add_argument("--prefix", default="action")
    ap.add_argument("--max-events", type=int, default=8)
    ap.add_argument("--pre-ms", type=float, default=25.0)
    ap.add_argument("--post-ms", type=float, default=220.0)
    ap.add_argument("--refractory-ms", type=float, default=220.0)
    args = ap.parse_args()

    sr, x = read_pcm16_mono(args.input)
    env = moving_rms(x, int(0.008 * sr))
    max_env = max(env) if env else 0.0
    noise = percentile(env, 0.50)

    threshold = max(noise * 4.0, max_env * 0.18)
    refractory = int(args.refractory_ms * 0.001 * sr)
    pre = int(args.pre_ms * 0.001 * sr)
    post = int(args.post_ms * 0.001 * sr)
    fade = int(0.004 * sr)

    candidates = []
    last = -refractory
    for i in range(1, len(env) - 1):
        if i - last < refractory:
            continue
        if env[i] >= threshold and env[i] >= env[i - 1] and env[i] >= env[i + 1]:
            # Search local peak in the next 30 ms.
            end = min(len(env), i + int(0.030 * sr))
            peak_i = max(range(i, end), key=lambda k: env[k])
            candidates.append((env[peak_i], peak_i))
            last = peak_i

    # Keep strongest events, then restore chronological order.
    strongest = sorted(candidates, reverse=True)[: max(1, args.max_events)]
    strongest.sort(key=lambda item: item[1])

    args.output_dir.mkdir(parents=True, exist_ok=True)
    written = 0
    for _, peak_i in strongest:
        a = max(0, peak_i - pre)
        b = min(len(x), peak_i + post)
        clip = list(x[a:b])
        if len(clip) < int(0.040 * sr):
            continue
        fade_edges(clip, fade)
        path = args.output_dir / f"{args.prefix}__{written+1:02d}.wav"
        write_pcm16_mono(path, sr, clip)
        written += 1

    print(
        f"{args.input}: threshold={threshold:.6f} candidates={len(candidates)} "
        f"written={written}"
    )

    if written == 0:
        raise SystemExit("no usable transients extracted")


if __name__ == "__main__":
    main()
