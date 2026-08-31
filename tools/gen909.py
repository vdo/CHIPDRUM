#!/usr/bin/env python3
"""Synthesize a 909-style drum kit as WAV files.

Every widely circulated "TR-909 sample pack" is an unlicensed rip of Roland
hardware, so instead of shipping one we model the voices. The result is
909-flavoured and unambiguously royalty-free.

    python3 tools/gen909.py samples/

Writes kick909.wav, snare909.wav, hat909.wav at 44.1 kHz / 16-bit mono,
ready for tools/wav2h.py.
"""
import math
import os
import struct
import sys
import wave

SR = 44100

# Deterministic noise, so regenerating the kit gives byte-identical output
_rng = 0x1234567


def noise():
    """xorshift32 -> -1..1"""
    global _rng
    x = _rng
    x ^= (x << 13) & 0xFFFFFFFF
    x ^= x >> 17
    x ^= (x << 5) & 0xFFFFFFFF
    _rng = x & 0xFFFFFFFF
    return (_rng / 2147483648.0) - 1.0


def highpass(xs, fc):
    """One-pole high-pass; strips body from metallic sounds."""
    rc = 1.0 / (2 * math.pi * fc)
    a = rc / (rc + 1.0 / SR)
    out, prev_x, prev_y = [], 0.0, 0.0
    for x in xs:
        y = a * (prev_y + x - prev_x)
        out.append(y)
        prev_x, prev_y = x, y
    return out


def saturate(x, drive=1.6):
    """Soft clip. The 909's punch comes largely from being driven hard."""
    x *= drive
    return math.tanh(x) if abs(x) < 8 else math.copysign(1.0, x)


def kick(dur=0.62, f_start=210.0, f_end=47.0, pitch_tau=0.024, amp_tau=0.20):
    """909 kick: fast exponential pitch drop into a low sine, plus the
    characteristic click transient that makes it cut through a mix."""
    n = int(SR * dur)
    out, ph = [], 0.0
    for i in range(n):
        t = i / SR
        f = f_end + (f_start - f_end) * math.exp(-t / pitch_tau)
        ph += 2 * math.pi * f / SR
        s = math.sin(ph) * math.exp(-t / amp_tau)
        if t < 0.006:  # click: sharp impulse + a sliver of noise
            k = math.exp(-t / 0.0009)
            s += 0.85 * k + 0.35 * k * noise()
        out.append(saturate(s, 1.5))
    return out


def snare(dur=0.30, tone_tau=0.032, noise_tau=0.115):
    """909-style snare, pushed industrial: two inharmonic tone partials for
    the shell, a long noise tail for the wires, ring modulation and hard
    saturation for a metallic, clanging edge."""
    n = int(SR * dur)
    out, p1, p2, p3 = [], 0.0, 0.0, 0.0
    f1, f2, f3 = 186.0, 331.0, 467.0  # inharmonic: rings rather than pitches
    for i in range(n):
        t = i / SR
        p1 += 2 * math.pi * f1 / SR
        p2 += 2 * math.pi * f2 / SR
        p3 += 2 * math.pi * f3 / SR
        env_t = math.exp(-t / tone_tau)
        env_n = math.exp(-t / noise_tau)
        body = (math.sin(p1) * 0.9 + math.sin(p2) * 0.7) * env_t
        metal = math.sin(p2) * math.sin(p3) * env_t * 0.8  # ring mod = clang
        wires = noise() * env_n
        out.append(saturate(body * 0.7 + metal * 0.6 + wires * 0.9, 2.2))
    return highpass(out, 180.0)


def hat(dur=0.14, tau=0.030, fc=6800.0):
    """Closed hat: filtered white noise.

    The 909's hats are noise-based, unlike the 808's ringing bank of
    inharmonic square oscillators - so this is noise through a steep
    high-pass, which gives a tight non-metallic tick rather than a clang.
    """
    n = int(SR * dur)
    out = [noise() * math.exp(-t / tau) for t in (i / SR for i in range(n))]
    out = highpass(out, fc)
    out = highpass(out, fc)  # 12 dB/oct: one pole leaves too much body
    return [saturate(x, 1.2) for x in out]


def write_wav(path, samples):
    peak = max(abs(s) for s in samples) or 1.0
    data = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s / peak * 0.97)) * 32767))
                    for s in samples)
    w = wave.open(path, "wb")
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(SR)
    w.writeframes(data)
    w.close()
    print("%-22s %6.3f s  %d frames" % (os.path.basename(path),
                                        len(samples) / float(SR), len(samples)))


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else "samples"
    os.makedirs(out_dir, exist_ok=True)
    write_wav(os.path.join(out_dir, "kick909.wav"), kick())
    write_wav(os.path.join(out_dir, "snare909.wav"), snare())
    write_wav(os.path.join(out_dir, "hat909.wav"), hat())


if __name__ == "__main__":
    main()
