#!/bin/sh
# Build and flash CHIPDRUM to a connected Pico (macOS).
#
# The module does not need its BOOTSEL button: once CHIPDRUM is running it
# exposes a USB serial port, and a 1200-baud "touch" on that port reboots it
# into the UF2 bootloader. CircuitPython boards are rebooted via their REPL.
# If neither applies (blank board), hold BOOTSEL while plugging in USB.
set -e
cd "$(dirname "$0")"

UF2=build/chipdrum.uf2
SDK=${PICO_SDK_PATH:-$HOME/pico-sdk}

# --- build ---
if [ ! -d build ]; then
    cmake -S . -B build -DPICO_SDK_PATH="$SDK" -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=Release
fi
cmake --build build -j8
[ -f "$UF2" ] || { echo "no $UF2 - build failed"; exit 1; }

# Find the serial port belonging to a Raspberry Pi Pico specifically.
# Never guess: touching an arbitrary usbmodem port would poke unrelated gear
# (audio interfaces, keyboards) that also enumerate as usbmodem*.
pico_port() {
    ioreg -r -c IOUSBHostDevice -l 2>/dev/null | awk '
        /"USB Product Name"/ { split($0, a, "\""); name = a[4] }
        /"IOCalloutDevice"/  { split($0, b, "\"")
                               if (name ~ /Pico|RP2/) { print b[4]; exit } }'
}

# --- get the board into the bootloader ---
if [ ! -d /Volumes/RPI-RP2 ]; then
    PORT=$(pico_port)
    if [ -z "$PORT" ]; then
        echo "No Raspberry Pi Pico found on USB."
        echo "  - plug the module into USB, or"
        echo "  - hold BOOTSEL while plugging it in, then run this again."
        exit 1
    fi
    echo "Found Pico on $PORT"
    if [ -d /Volumes/CIRCUITPY ]; then
        echo "CircuitPython detected - rebooting into bootloader via REPL"
        diskutil unmount /Volumes/CIRCUITPY >/dev/null 2>&1 || true
        stty -f "$PORT" raw 115200 -echo 2>/dev/null || true
        exec 3<>"$PORT"
        printf '\003' >&3; sleep 1
        printf 'import microcontroller\r\n' >&3; sleep 0.3
        printf 'microcontroller.on_next_reset(microcontroller.RunMode.UF2)\r\n' >&3; sleep 0.3
        printf 'microcontroller.reset()\r\n' >&3; sleep 0.3
        exec 3>&-
    else
        echo "Rebooting into bootloader (1200-baud touch on $PORT)"
        stty -f "$PORT" 1200 2>/dev/null || true
    fi

    printf "waiting for RPI-RP2"
    i=0
    while [ ! -d /Volumes/RPI-RP2 ] && [ $i -lt 20 ]; do
        printf "."; sleep 1; i=$((i + 1))
    done
    echo
    [ -d /Volumes/RPI-RP2 ] || { echo "bootloader did not appear"; exit 1; }
fi

# --- flash ---
cp "$UF2" /Volumes/RPI-RP2/
echo "flashed - the module reboots and runs CHIPDRUM"
