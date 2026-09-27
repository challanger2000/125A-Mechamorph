#!/usr/bin/env python3
"""
125A Mechamorph reference-audio analyzer.

Purpose:
- reproducible measurement of mechanical reference recordings
- no subjective guessing when measurable quantities are available
- produces JSON suitable for later aggregation / regression

Dependencies:
    numpy
    scipy

Supported:
    PCM WAV readable by scipy.io.wavfile

This is a research tool, not plugin DSP.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
from typing import Any

import numpy as np
from scipy.io import wavfile
from scipy.signal import find_peaks, get_window


def _to_float32(x: np.ndarray) -> np.ndarray:
    if np.issubdtype(x.dtype, np.floating):
        return x.astype(np.float32, copy=False)
    if x.dtype == np.int16:
        return (x.astype(np.float32) / 32768.0)
    if x.dtype == np.int32:
        return (x.astype(np.float32) / 2147483648.0)
    if x.dtype == np.uint8:
        return ((x.astype(np.float32) - 128.0) / 128.0)
    raise TypeError(f"Unsupported WAV dtype: {x.dtype}")


def _mono(x: np.ndarray) -> np.ndarray:
    if x.ndim == 1:
        return x
    return np.mean(x, axis=1, dtype=np.float64).astype(np.float32)


def _db(v: float, floor: float = 1e-15) -> float:
    return 20.0 * math.log10(max(abs(v), floor))


def _frame_signal(x: np.ndarray, frame: int, hop: int) -> np.ndarray:
    if len(x) < frame:
        pad = np.zeros(frame, dtype=np.float32)
        pad[: len(x)] = x
        x = pad
    n = 1 + max(0, (len(x) - frame) // hop)
    shape = (n, frame)
    strides = (x.strides[0] * hop, x.strides[0])
    return np.lib.stride_tricks.as_strided(x, shape=shape, strides=strides)


def _spectral_stats(x: np.ndarray, sr: int) -> dict[str, Any]:
    frame = 4096
    hop = 1024
    frames = _frame_signal(x, frame, hop).astype(np.float64, copy=True)
    win = get_window("hann", frame, fftbins=True).astype(np.float64)
    frames *= win[None, :]
    mag = np.abs(np.fft.rfft(frames, axis=1))
    freqs = np.fft.rfftfreq(frame, 1.0 / sr)

    power = mag * mag
    denom = np.sum(power, axis=1) + 1e-30
    centroid = np.sum(power * freqs[None, :], axis=1) / denom

    cumsum = np.cumsum(power, axis=1)
    targets = 0.85 * cumsum[:, -1]
    rolloff_idx = np.array([
        int(np.searchsorted(cumsum[i], targets[i]))
        for i in range(len(cumsum))
    ])
    rolloff = freqs[np.clip(rolloff_idx, 0, len(freqs) - 1)]

    avg = np.mean(power, axis=0)
    avg_db = 10.0 * np.log10(avg + 1e-30)

    # Peak finding on average spectrum. Use moderate prominence to avoid
    # dumping hundreds of insignificant bins.
    peaks, props = find_peaks(avg_db, prominence=6.0, distance=4)
    order = np.argsort(avg_db[peaks])[::-1][:16] if len(peaks) else np.array([], dtype=int)
    top = [
        {
            "frequency_hz": float(freqs[peaks[i]]),
            "level_db_relative": float(avg_db[peaks[i]] - np.max(avg_db)),
            "prominence_db": float(props["prominences"][i]),
        }
        for i in order
    ]

    return {
        "spectral_centroid_hz_mean": float(np.mean(centroid)),
        "spectral_centroid_hz_median": float(np.median(centroid)),
        "spectral_rolloff85_hz_mean": float(np.mean(rolloff)),
        "dominant_spectral_peaks": top,
    }


def _envelope(x: np.ndarray, sr: int, time_ms: float) -> np.ndarray:
    # Simple one-pole absolute-value envelope.
    tau = max(time_ms / 1000.0, 1.0 / sr)
    a = math.exp(-1.0 / (tau * sr))
    y = np.empty_like(x, dtype=np.float64)
    s = 0.0
    for i, v in enumerate(np.abs(x).astype(np.float64)):
        s = a * s + (1.0 - a) * v
        y[i] = s
    return y


def _transient_stats(x: np.ndarray, sr: int) -> dict[str, Any]:
    fast = _envelope(x, sr, 2.0)
    slow = _envelope(x, sr, 30.0)
    novelty = np.maximum(0.0, fast - slow)

    if len(novelty) == 0 or np.max(novelty) <= 1e-12:
        return {
            "event_count": 0,
            "events_per_second": 0.0,
            "median_inter_event_ms": None,
            "p10_inter_event_ms": None,
            "p90_inter_event_ms": None,
        }

    # Robust adaptive threshold.
    med = float(np.median(novelty))
    mad = float(np.median(np.abs(novelty - med))) + 1e-15
    threshold = med + 8.0 * mad

    min_distance = max(1, int(sr * 0.008))
    peaks, _ = find_peaks(novelty, height=threshold, distance=min_distance)

    duration = len(x) / sr
    intervals_ms = np.diff(peaks) * 1000.0 / sr if len(peaks) > 1 else np.array([])

    return {
        "event_count": int(len(peaks)),
        "events_per_second": float(len(peaks) / duration) if duration > 0 else 0.0,
        "median_inter_event_ms": float(np.median(intervals_ms)) if len(intervals_ms) else None,
        "p10_inter_event_ms": float(np.percentile(intervals_ms, 10)) if len(intervals_ms) else None,
        "p90_inter_event_ms": float(np.percentile(intervals_ms, 90)) if len(intervals_ms) else None,
    }


def _periodicity(x: np.ndarray, sr: int) -> dict[str, Any]:
    # Downsample envelope to ~200 Hz for robust low-rate periodicity search.
    env = _envelope(x, sr, 10.0)
    target = 200.0
    step = max(1, int(round(sr / target)))
    e = env[::step]
    effective_sr = sr / step

    if len(e) < 20:
        return {"dominant_modulation_hz": None, "periodicity_strength": 0.0}

    e = e - np.mean(e)
    n = min(len(e), int(effective_sr * 30.0))
    e = e[:n]
    ac = np.correlate(e, e, mode="full")[n - 1 :]
    if ac[0] <= 1e-20:
        return {"dominant_modulation_hz": None, "periodicity_strength": 0.0}
    ac /= ac[0]

    min_hz, max_hz = 0.2, 20.0
    min_lag = max(1, int(effective_sr / max_hz))
    max_lag = min(len(ac) - 1, int(effective_sr / min_hz))
    if max_lag <= min_lag:
        return {"dominant_modulation_hz": None, "periodicity_strength": 0.0}

    segment = ac[min_lag : max_lag + 1]
    lag = min_lag + int(np.argmax(segment))
    strength = float(ac[lag])
    hz = float(effective_sr / lag) if lag > 0 else None

    return {
        "dominant_modulation_hz": hz,
        "periodicity_strength": strength,
    }


def analyze(path: Path) -> dict[str, Any]:
    sr, raw = wavfile.read(path)
    x = _to_float32(raw)
    channels = 1 if x.ndim == 1 else x.shape[1]
    mono = _mono(x)

    if len(mono) == 0:
        raise ValueError("Empty WAV")

    peak = float(np.max(np.abs(mono)))
    rms = float(np.sqrt(np.mean(np.square(mono, dtype=np.float64))))
    crest = peak / max(rms, 1e-15)
    zcr = float(np.mean(np.abs(np.diff(np.signbit(mono))).astype(np.float64))) if len(mono) > 1 else 0.0

    result: dict[str, Any] = {
        "file": str(path),
        "sample_rate_hz": int(sr),
        "channels": int(channels),
        "duration_s": float(len(mono) / sr),
        "peak_dbfs": _db(peak),
        "rms_dbfs": _db(rms),
        "crest_factor_db": 20.0 * math.log10(max(crest, 1e-15)),
        "zero_crossing_rate": zcr,
    }
    result.update(_spectral_stats(mono, sr))
    result["transients"] = _transient_stats(mono, sr)
    result["periodicity"] = _periodicity(mono, sr)
    return result


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("inputs", nargs="+", type=Path)
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()

    results = []
    for path in args.inputs:
        try:
            results.append({"status": "ok", **analyze(path)})
        except Exception as exc:
            results.append({"status": "error", "file": str(path), "error": str(exc)})

    payload = {
        "tool": "125A Mechamorph reference-audio analyzer",
        "schema_version": 1,
        "results": results,
    }

    text = json.dumps(payload, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text + "\n", encoding="utf-8")
    else:
        print(text)


if __name__ == "__main__":
    main()
