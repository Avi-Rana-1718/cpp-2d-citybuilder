#!/usr/bin/env python3
"""
Synthesised sound effects and ambience (no recordings needed).

Writes assets/sounds/<name>.wav, 22050 Hz mono 16-bit:
  click      UI click
  build      a building going up (thud + clink)
  road       laying road / lines / zones (gravel scrape)
  bulldoze   demolition crunch
  error      low double buzz (can't build here)
  alert      three short beeps (fires, shortages)
  milestone  rising arpeggio (city reached a new rank)
  fire       2 s crackle loop (plays while something burns)
  ambient    12 s city hum loop: low rumble, passing cars, birds
Deterministic (fixed random seed).
"""
import math
import os
import random
import struct
import wave

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "sounds")
RATE = 22050
rng = random.Random(1234)


def write(name, samples):
    os.makedirs(OUT, exist_ok=True)
    peak = max(1e-6, max(abs(s) for s in samples))
    scale = 0.85 / peak
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s * scale)) * 32767)) for s in samples))


def n(seconds):
    return int(seconds * RATE)


def env(i, total, attack=0.005, release=None):
    t = i / RATE
    a = min(1.0, t / attack) if attack > 0 else 1.0
    if release is None:
        return a
    rem = (total - i) / RATE
    return a * min(1.0, rem / release)


def lowpass(samples, k):
    out, y = [], 0.0
    for s in samples:
        y += k * (s - y)
        out.append(y)
    return out


def tone(freq, seconds, decay=8.0, harmonics=(1.0,)):
    total = n(seconds)
    return [sum(a * math.sin(2 * math.pi * freq * (h + 1) * i / RATE) for h, a in enumerate(harmonics)) *
            math.exp(-decay * i / RATE) * env(i, total, 0.002) for i in range(total)]


def noise(seconds):
    return [rng.uniform(-1, 1) for _ in range(n(seconds))]


def mix(*tracks):
    length = max(len(t) for t in tracks)
    return [sum(t[i] for t in tracks if i < len(t)) for i in range(length)]


def click():
    return tone(1800, 0.04, decay=120, harmonics=(1.0, 0.3))


def build():
    thud = [s * math.exp(-18 * i / RATE) for i, s in enumerate(lowpass(noise(0.25), 0.08))]
    body = tone(150, 0.25, decay=14)
    clink = [0.0] * n(0.07) + tone(1320, 0.12, decay=40, harmonics=(1.0, 0.5))
    return mix([s * 2.0 for s in thud], body, [s * 0.35 for s in clink])


def road():
    grit = lowpass(noise(0.16), 0.35)
    return [s * math.exp(-14 * i / RATE) * env(i, len(grit), 0.01) for i, s in enumerate(grit)]


def bulldoze():
    total = n(0.45)
    raw = lowpass(noise(0.45), 0.2)
    return [s * (0.6 + 0.4 * math.sin(2 * math.pi * 22 * i / RATE)) * math.exp(-4 * i / RATE) * env(i, total, 0.01)
            for i, s in enumerate(raw)]


def error():
    def buzz(f, d):
        total = n(d)
        return [(1.0 if math.sin(2 * math.pi * f * i / RATE) > 0 else -1.0) * 0.4 * env(i, total, 0.005, 0.03)
                for i in range(total)]
    return lowpass(buzz(180, 0.12) + [0.0] * n(0.04) + buzz(140, 0.16), 0.3)


def alert():
    out = []
    for _ in range(3):
        out += tone(940, 0.09, decay=6, harmonics=(1.0, 0.25)) + [0.0] * n(0.06)
    return out


def milestone():
    out = []
    for k, f in enumerate((523.25, 659.25, 783.99, 1046.5)):
        out += tone(f, 0.16 if k < 3 else 0.5, decay=5 if k < 3 else 3, harmonics=(1.0, 0.4, 0.15))
    return out


def fire():
    total = n(2.0)
    base = lowpass(noise(2.0), 0.05)
    out = [s * 0.5 for s in base]
    for _ in range(140):                       # crackles
        at = rng.randrange(total - n(0.02))
        amp = rng.uniform(0.3, 1.0)
        for i in range(n(0.008)):
            out[at + i] += amp * rng.uniform(-1, 1) * math.exp(-600 * i / RATE)
    return crossfade_loop(out, n(0.2))


def ambient():
    seconds = 12.0
    total = n(seconds)
    rumble = lowpass(lowpass(noise(seconds), 0.01), 0.02)
    out = [s * 3.0 for s in rumble]
    for _ in range(5):                          # passing cars: filtered noise swelling in and out
        start, length = rng.uniform(0, seconds - 3), rng.uniform(2.0, 3.0)
        car = lowpass(noise(length), 0.06)
        for i, s in enumerate(car):
            k = n(start) + i
            out[k] += s * 0.9 * math.sin(math.pi * i / len(car)) ** 2
    for _ in range(7):                          # birds
        start, f = rng.uniform(0, seconds - 1), rng.uniform(2500, 3800)
        for c in range(rng.randint(2, 4)):
            chirp_len = n(0.07)
            for i in range(chirp_len):
                k = n(start + c * 0.12) + i
                if k < total:
                    out[k] += 0.12 * math.sin(2 * math.pi * (f + 900 * i / chirp_len) * i / RATE) * math.sin(math.pi * i / chirp_len)
    return crossfade_loop(out, n(0.6))


def crossfade_loop(samples, fade):
    body = samples[:len(samples) - fade]
    tail = samples[len(samples) - fade:]
    for i in range(fade):
        w = i / fade
        body[i] = body[i] * w + tail[i] * (1 - w)
    return body


SOUNDS = {"click": click, "build": build, "road": road, "bulldoze": bulldoze, "error": error,
          "alert": alert, "milestone": milestone, "fire": fire, "ambient": ambient}

if __name__ == "__main__":
    for name, fn in SOUNDS.items():
        write(name, fn())
    print(f"Generated {len(SOUNDS)} sounds -> {os.path.abspath(OUT)}")
