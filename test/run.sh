#!/bin/sh
# Host-side tests of the firmware's musical algorithms.
# Compiles the real src/scales.c natively (no Pico hardware needed) and checks
# Truchets banks, the Turing register and clock math.
set -e
cd "$(dirname "$0")"
cc -O2 -Wall -Wextra -DPICO_NO_HARDWARE -I../src -o /tmp/chipdrum_host_test host_test.c ../src/scales.c ../src/truchet_patterns.c ../src/edge_detect.c ../src/drumsynth.c -lm
exec /tmp/chipdrum_host_test
