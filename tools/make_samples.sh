#!/bin/sh
# Regenerate the 909-style kit and rebuild src/samples.h.
# The kit is synthesized (tools/gen909.py) rather than sampled from hardware,
# so it carries no licensing baggage. Edit gen909.py to reshape the sounds,
# or point wav2h.py at your own WAVs instead.
set -e
cd "$(dirname "$0")/.."
python3 tools/gen909.py samples/
python3 tools/wav2h.py samples/kick909.wav:620 samples/snare909.wav:300 \
    samples/hat909.wav:140 > src/samples.h
echo "wrote src/samples.h"
