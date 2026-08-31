#!/usr/bin/env python3
"""Convert WAV drum samples into a C header for TECLA BASS.

Handles PCM 8/16/24/32-bit and IEEE-float WAVs, mono or stereo, at any rate.
Each sample is downmixed to mono, resampled to the engine rate, peak
normalised, silence-trimmed, fitted with a short fade-out (so playback never
ends on a step and clicks) and written as unsigned 8-bit data, which is
exactly what the PWM duty register wants.

Each argument may carry a maximum length in milliseconds as "file.wav:140".
That matters for hats: a quietly-recorded sample has a noise floor that
normalising lifts above the silence threshold, so trimming alone leaves a
long tail that smears across fast patterns.

    python3 tools/wav2h.py kick.wav:500 snare.wav:300 hat.wav:140 > src/samples.h
    python3 tools/wav2h.py --prefix BANK1 kick.wav snare.wav hat.wav \
        > src/samples_bank1.h
"""
import struct
import sys

RATE = 25000          # engine sample rate; must match AUDIO_SR in src/config.h
PEAK = 0.97           # normalise target, leaves a little headroom
SILENCE = 0.004       # trim threshold, fraction of peak
FADE_MS = 4.0         # fade-out length


def read_wav(path):
    """Return (samples as floats in -1..1, rate). Minimal RIFF parser."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("%s: not a RIFF/WAVE file" % path)

    pos, fmt, raw = 12, None, None
    while pos + 8 <= len(data):
        cid = data[pos:pos + 4]
        size = struct.unpack("<I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            fmt = struct.unpack("<HHIIHH", body[:16])
        elif cid == b"data":
            raw = body
        pos += 8 + size + (size & 1)  # chunks are word-aligned

    if fmt is None or raw is None:
        raise ValueError("%s: missing fmt or data chunk" % path)

    tag, channels, rate, _, _, bits = fmt
    if tag == 0xFFFE:  # WAVE_FORMAT_EXTENSIBLE: subformat decides
        tag = 3 if bits == 32 else 1

    n = len(raw) * 8 // bits
    if tag == 3:  # IEEE float
        if bits == 32:
            vals = list(struct.unpack("<%df" % n, raw[:n * 4]))
        elif bits == 64:
            vals = list(struct.unpack("<%dd" % n, raw[:n * 8]))
        else:
            raise ValueError("%s: odd float width %d" % (path, bits))
    elif tag == 1:  # PCM
        if bits == 8:  # 8-bit PCM is unsigned
            vals = [(b - 128) / 128.0 for b in raw]
        elif bits == 16:
            vals = [v / 32768.0
                    for v in struct.unpack("<%dh" % n, raw[:n * 2])]
        elif bits == 24:
            vals = []
            for i in range(0, len(raw) - 2, 3):
                v = raw[i] | (raw[i + 1] << 8) | (raw[i + 2] << 16)
                if v & 0x800000:
                    v -= 1 << 24
                vals.append(v / 8388608.0)
        elif bits == 32:
            vals = [v / 2147483648.0
                    for v in struct.unpack("<%di" % n, raw[:n * 4])]
        else:
            raise ValueError("%s: odd PCM width %d" % (path, bits))
    else:
        raise ValueError("%s: unsupported format tag %d" % (path, tag))

    if channels > 1:  # downmix
        vals = [sum(vals[i:i + channels]) / channels
                for i in range(0, len(vals) - channels + 1, channels)]
    return vals, rate


def resample(vals, src_rate, dst_rate):
    """Linear interpolation. Fine here: drums, and we only ever downsample."""
    if src_rate == dst_rate:
        return vals
    out, step, pos = [], src_rate / float(dst_rate), 0.0
    while pos < len(vals) - 1:
        i = int(pos)
        frac = pos - i
        out.append(vals[i] * (1.0 - frac) + vals[i + 1] * frac)
        pos += step
    return out


def process(path, max_ms=None):
    vals, rate = read_wav(path)
    vals = resample(vals, rate, RATE)

    peak = max((abs(v) for v in vals), default=0.0)
    if peak <= 0.0:
        raise ValueError("%s: silent" % path)
    vals = [v * (PEAK / peak) for v in vals]

    # trim trailing silence (keeps tails from eating flash and CPU)
    end = len(vals)
    while end > 1 and abs(vals[end - 1]) < SILENCE:
        end -= 1
    vals = vals[:end]

    if max_ms:
        vals = vals[:int(RATE * max_ms / 1000.0)]

    fade = min(int(RATE * FADE_MS / 1000.0), len(vals))
    for i in range(fade):
        vals[len(vals) - fade + i] *= 1.0 - (i / float(fade))

    # to unsigned 8-bit, centred on 128 (the PWM idle level)
    out = []
    for v in vals:
        s = int(round(v * 127.0)) + 128
        out.append(0 if s < 0 else (255 if s > 255 else s))
    return out, rate


def main():
    args = sys.argv[1:]
    prefix = ""
    if len(args) == 5 and args[0] == "--prefix":
        prefix = args[1].upper()
        args = args[2:]
        if (not prefix or not prefix[0].isalpha() or
                not prefix.replace("_", "").isalnum()):
            sys.exit("prefix must be a C identifier")
    if len(args) != 3:
        sys.exit("usage: wav2h.py [--prefix NAME] "
                 "<kick.wav> <snare.wav> <hat.wav>")

    names = ["KICK", "SNARE", "HAT"]
    blobs = []
    for arg, name in zip(args, names):
        path, _, ms = arg.partition(":")
        pcm, src = process(path, float(ms) if ms else None)
        blobs.append((name, pcm, path, src))

    w = sys.stdout.write
    w("// Generated by tools/wav2h.py - do not edit by hand.\n")
    w("// Unsigned 8-bit mono PCM at %d Hz, written straight to the PWM\n" % RATE)
    w("// duty register during playback.\n")
    w("#pragma once\n#include <stdint.h>\n\n")
    rate_name = "%s_SAMPLE_RATE_HZ" % prefix if prefix else "SAMPLE_RATE_HZ"
    w("#define %s %d\n\n" % (rate_name, RATE))
    total = 0
    for name, pcm, path, src in blobs:
        symbol = "%s_%s" % (prefix, name) if prefix else name
        total += len(pcm)
        w("// %s: %s (%d Hz source, %.3f s)\n"
          % (name.lower(), path.split("/")[-1], src, len(pcm) / float(RATE)))
        w("#define %s_LEN %d\n" % (symbol, len(pcm)))
        w("static const uint8_t %s_PCM[%s_LEN] = {\n" %
          (symbol, symbol))
        for i in range(0, len(pcm), 16):
            w("    " + ",".join("%3d" % v for v in pcm[i:i + 16]) + ",\n")
        w("};\n\n")
    sys.stderr.write("total %d bytes (%.1f KB), %.3f s of audio\n"
                     % (total, total / 1024.0, total / float(RATE)))


if __name__ == "__main__":
    main()
