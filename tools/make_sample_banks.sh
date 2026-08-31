#!/bin/sh
# Convert the two user sample kits to the Pico's native PCM representation.
set -e
cd "$(dirname "$0")/.."

python3 tools/wav2h.py --prefix BANK1 \
    "banks/1/1 KICK 00.wav" "banks/1/2 SNARE 06.wav" \
    "banks/1/3 HH CLOSE 08.wav" > src/samples_bank1.h

python3 tools/wav2h.py --prefix BANK2 \
    "banks/2/1. Kick.wav" "banks/2/2. Snare.wav" \
    "banks/2/3. CLDHat.wav" > src/samples_bank2.h

echo "wrote src/samples_bank{1,2}.h"
