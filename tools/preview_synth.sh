#!/bin/sh
# Render the synthesized drums to samples/preview_*.wav for auditioning.
# Compiles the real src/drumsynth.c, so the previews match the module exactly.
set -e
cd "$(dirname "$0")/.."
mkdir -p samples
cc -O2 -Wall -DPICO_NO_HARDWARE -Isrc -o /tmp/chipdrum_preview \
    tools/preview_synth.c src/drumsynth.c -lm
exec /tmp/chipdrum_preview
