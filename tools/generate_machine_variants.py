#!/usr/bin/env python3
"""
Generate transformed mechanical materials from the curated CC0 machine set.

Offline/research step only.
Goal: create new, less recognizable machine materials while preserving the
mechanical gesture. No random processing at runtime; all transforms are
deterministic and reproducible.
"""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt, resample_poly


PROFILES = ("tiny", "clockwork", "heavy", "colossal", "pneumatic", "broken")


def read_mono(path: Path):
    sr, raw = wavfile.read(path)
    x = raw.astype(np.float64)
    if np.issubdtype(raw.dtype, np.integer):
        x /= float(max(abs(np.iinfo(raw.dtype).min), np.iinfo(raw.dtype).max))
    if x.ndim > 1:
        x = np.mean(x, axis=1)
    return int(sr), x


def normalize_safe(x: np.ndarray, ceiling: float = 0.92) -> np.ndarray:
    if len(x) == 0:
        return x
    peak = float(np.max(np.abs(x)))
    if peak > ceiling and peak > 0.0:
        x = x * (ceiling / peak)
    return x


def fade(x: np.ndarray, sr: int, ms: float = 5.0) -> np.ndarray:
    y = x.copy()
    n = min(len(y)//2, max(1, int(sr*ms*0.001)))
    if n > 1:
        y[:n] *= np.linspace(0.0,1.0,n)
        y[-n:] *= np.linspace(1.0,0.0,n)
    return y


def rate_change(x: np.ndarray, factor: float) -> np.ndarray:
    # Physical tape/sample-rate style transformation: pitch and time change
    # together, which preserves mechanical gesture better than arbitrary pitch.
    if abs(factor - 1.0) < 1e-6:
        return x.copy()
    up = max(1, int(round(factor * 1000)))
    down = 1000
    # resample_poly changes sample count inversely to perceived playback rate
    # when we retain the original file sample rate.
    return resample_poly(x, down, up)


def lowpass(x: np.ndarray, sr: int, hz: float) -> np.ndarray:
    hz = min(hz, 0.45*sr)
    sos = butter(2, hz/(0.5*sr), btype="low", output="sos")
    return sosfilt(sos, x)


def highpass(x: np.ndarray, sr: int, hz: float) -> np.ndarray:
    hz = min(hz, 0.45*sr)
    sos = butter(2, hz/(0.5*sr), btype="high", output="sos")
    return sosfilt(sos, x)


def transient_emphasis(x: np.ndarray, amount: float) -> np.ndarray:
    if len(x) < 2:
        return x.copy()
    d = np.concatenate([[0.0], np.diff(x)])
    return x + amount * d


def soft_clip(x: np.ndarray, drive: float) -> np.ndarray:
    if drive <= 0.0:
        return x.copy()
    return np.tanh((1.0 + drive) * x) / np.tanh(1.0 + drive)


def delayed_layer(x: np.ndarray, sr: int, delay_ms: float, gain: float) -> np.ndarray:
    n = max(1, int(sr * delay_ms * 0.001))
    y = np.zeros(len(x) + n, dtype=np.float64)
    y[:len(x)] += x
    y[n:n+len(x)] += gain*x
    return y


def reverse_tail(x: np.ndarray, fraction: float, gain: float) -> np.ndarray:
    n = max(1, int(len(x)*fraction))
    y = x.copy()
    tail = x[-n:][::-1]
    y[-n:] += gain*tail
    return y


def profile_transform(x: np.ndarray, sr: int, profile: str, role: str) -> np.ndarray:
    y = x.copy()

    if profile == "tiny":
        y = rate_change(y, 1.45 if role != "run" else 1.30)
        y = highpass(y, sr, 180.0)
        y = transient_emphasis(y, 0.20)

    elif profile == "clockwork":
        y = rate_change(y, 1.18)
        y = highpass(y, sr, 120.0)
        y = lowpass(y, sr, 8500.0)
        y = transient_emphasis(y, 0.35)

    elif profile == "heavy":
        y = rate_change(y, 0.78)
        y = lowpass(y, sr, 6500.0)
        y = soft_clip(y, 0.28)
        if role in {"action","load","stop"}:
            y = delayed_layer(y, sr, 18.0, 0.20)

    elif profile == "colossal":
        y = rate_change(y, 0.58)
        y = lowpass(y, sr, 4200.0)
        y = soft_clip(y, 0.34)
        if role in {"action","load","stop","release"}:
            y = delayed_layer(y, sr, 42.0, 0.27)
            y = delayed_layer(y, sr, 95.0, 0.14)

    elif profile == "pneumatic":
        y = rate_change(y, 0.92)
        y = highpass(y, sr, 70.0)
        y = lowpass(y, sr, 9000.0)
        if role in {"release","load","action"}:
            y = delayed_layer(y, sr, 11.0, 0.16)

    elif profile == "broken":
        y = rate_change(y, 0.86 if role == "run" else 1.06)
        y = soft_clip(y, 0.20)
        y = transient_emphasis(y, 0.22)
        if role in {"action","release","stop"}:
            y = reverse_tail(y, 0.22, 0.18)

    return normalize_safe(fade(y, sr))


def eligible_profiles(stem: str, role: str):
    n = stem.lower()

    # Deliberately mix unlike real devices into each fictional machine.
    if role == "run":
        mapping = {
            "tiny": ("sewing", "projector", "rattle"),
            "clockwork": ("winch", "projector", "sewing", "rattle"),
            "heavy": ("press", "winch", "roller"),
            "colossal": ("press", "roller", "winch"),
            "pneumatic": ("press", "sewing", "projector"),
            "broken": ("sewing", "rattle", "roller", "projector"),
        }
    elif role == "action":
        mapping = {
            "tiny": ("switch", "stapler", "calc"),
            "clockwork": ("ratchet", "switch", "slide", "calc"),
            "heavy": ("ratchet", "stapler", "calc"),
            "colossal": ("ratchet", "stapler"),
            "pneumatic": ("switch", "calc", "stapler"),
            "broken": ("ratchet", "slide", "switch", "calc"),
        }
    elif role == "load":
        mapping = {
            "tiny": ("chain",),
            "clockwork": ("chain",),
            "heavy": ("chain", "press"),
            "colossal": ("chain", "press"),
            "pneumatic": ("press", "chain"),
            "broken": ("chain", "press"),
        }
    elif role == "release":
        mapping = {
            "tiny": ("spring", "switch"),
            "clockwork": ("spring", "winch"),
            "heavy": ("spring", "winch", "air"),
            "colossal": ("spring", "winch", "air"),
            "pneumatic": ("air", "spring", "switch"),
            "broken": ("spring", "winch", "switch"),
        }
    elif role == "start":
        mapping = {p: ("projector","winch") for p in PROFILES}
    elif role == "stop":
        mapping = {p: ("projector","winch") for p in PROFILES}
    else:
        return []

    return [p for p in PROFILES if any(token in n for token in mapping[p])]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("sample_dir", type=Path)
    ap.add_argument("output_dir", type=Path)
    args = ap.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)

    generated = 0
    for src in sorted(args.sample_dir.glob("*.wav")):
        stem = src.stem
        role = stem.split("__",1)[0].lower()
        if role not in {"start","run","action","load","release","stop"}:
            continue

        sr, x = read_mono(src)
        for profile in eligible_profiles(stem, role):
            y = profile_transform(x, sr, profile, role)
            # deterministic compact identity in case names later collide
            digest = hashlib.sha1(stem.encode("utf-8")).hexdigest()[:6]
            out = args.output_dir / f"{role}__{profile}__{stem}__{digest}.wav"
            wavfile.write(out, sr, (np.clip(y,-1.0,1.0)*32767.0).astype(np.int16))
            generated += 1

    print(f"Generated {generated} transformed machine materials")


if __name__ == "__main__":
    main()
