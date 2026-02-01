# TECLA Build Guide

This guide covers building and deploying TECLA on macOS and Linux.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Hardware Requirements](#hardware-requirements)
3. [Installing CircuitPython](#installing-circuitpython)
4. [Installing Libraries](#installing-libraries)
5. [Deploying TECLA](#deploying-tecla)
6. [Verification](#verification)
7. [Development Workflow](#development-workflow)
8. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required tools
brew install wget curl

# Optional: Install screen for serial console
brew install screen
```

### Linux (Debian/Ubuntu)

```bash
# Update package list
sudo apt update

# Install required tools
sudo apt install -y wget curl python3 python3-pip

# Install screen for serial console
sudo apt install -y screen

# Add user to dialout group for serial port access
sudo usermod -a -G dialout $USER
# Log out and back in for group change to take effect
```

### Linux (Fedora/RHEL)

```bash
# Install required tools
sudo dnf install -y wget curl python3 python3-pip screen

# Add user to dialout group
sudo usermod -a -G dialout $USER
```

### Linux (Arch)

```bash
# Install required tools
sudo pacman -S wget curl python python-pip screen

# Add user to uucp group (Arch uses uucp instead of dialout)
sudo usermod -a -G uucp $USER
```

---

## Hardware Requirements

### Components Needed

| Component | Quantity | Notes |
|-----------|----------|-------|
| Raspberry Pi Pico | 1 | RP2040-based |
| SSD1306 OLED Display | 1 | 128x64, I2C (0x3C) |
| Potentiometer | 2 | 10kΩ linear |
| LDR (Light Dependent Resistor) | 1 | With voltage divider |
| Slide Potentiometer | 1 | Linear, for Parameter 1 |
| Tactile Buttons | 6 | D-pad (4) + Extra (2) |
| LEDs | 7 | With appropriate resistors |
| 3.5mm Audio Jack | 3 | PWM outputs |
| 3.5mm CV Jack | 1 | Clock/Tempo input |
| Resistors | Various | LED current limiting, voltage dividers |
| USB Micro-B Cable | 1 | For programming and power |

### Pin Connections

See `MANUAL.md` Section 2 for complete pin assignments.

---

## Installing CircuitPython

### Step 1: Download CircuitPython

Download the latest CircuitPython UF2 file for Raspberry Pi Pico:

```bash
# Create downloads directory
mkdir -p ~/Downloads/circuitpython

# Download CircuitPython 10.x for Raspberry Pi Pico
wget -O ~/Downloads/circuitpython/circuitpython-pico.uf2 \
  "https://downloads.circuitpython.org/bin/raspberry_pi_pico/en_US/adafruit-circuitpython-raspberry_pi_pico-en_US-10.0.0.uf2"
```

Or visit: https://circuitpython.org/board/raspberry_pi_pico/

### Step 2: Enter Bootloader Mode

1. **Disconnect** the Pico from USB
2. **Hold** the BOOTSEL button on the Pico
3. **While holding** BOOTSEL, connect the USB cable
4. **Release** BOOTSEL after connecting

The Pico will appear as a USB drive named `RPI-RP2`.

### Step 3: Flash CircuitPython

**macOS:**
```bash
# Copy UF2 file to Pico
cp ~/Downloads/circuitpython/circuitpython-pico.uf2 /Volumes/RPI-RP2/

# Wait for Pico to reboot (drive will disconnect and reconnect)
```

**Linux:**
```bash
# Find the mount point (usually /media/$USER/RPI-RP2 or /run/media/$USER/RPI-RP2)
PICO_MOUNT=$(findmnt -rno TARGET -S LABEL=RPI-RP2)

# Copy UF2 file
cp ~/Downloads/circuitpython/circuitpython-pico.uf2 "$PICO_MOUNT/"

# Wait for Pico to reboot
```

After flashing, the Pico will reboot and appear as a drive named `CIRCUITPY`.

---

## Installing Libraries

TECLA requires several Adafruit CircuitPython libraries.

### Step 1: Download Adafruit Bundle

```bash
# Download the library bundle
wget -O ~/Downloads/adafruit-bundle.zip \
  "https://github.com/adafruit/Adafruit_CircuitPython_Bundle/releases/download/20250130/adafruit-circuitpython-bundle-10.x-mpy-20250130.zip"

# Extract
cd ~/Downloads
unzip adafruit-bundle.zip
```

### Step 2: Copy Required Libraries

**macOS:**
```bash
CIRCUITPY="/Volumes/CIRCUITPY"
BUNDLE=~/Downloads/adafruit-circuitpython-bundle-10.x-mpy-*

# Create lib directory if it doesn't exist
mkdir -p "$CIRCUITPY/lib"

# Copy required libraries
cp -r "$BUNDLE/lib/adafruit_midi" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_ssd1306.mpy" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_framebuf.mpy" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_bitmap_font" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_display_text" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_displayio_ssd1306.mpy" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_hid" "$CIRCUITPY/lib/"
```

**Linux:**
```bash
CIRCUITPY=$(findmnt -rno TARGET -S LABEL=CIRCUITPY)
BUNDLE=~/Downloads/adafruit-circuitpython-bundle-10.x-mpy-*

# Create lib directory
mkdir -p "$CIRCUITPY/lib"

# Copy required libraries
cp -r "$BUNDLE/lib/adafruit_midi" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_ssd1306.mpy" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_framebuf.mpy" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_bitmap_font" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_display_text" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_displayio_ssd1306.mpy" "$CIRCUITPY/lib/"
cp -r "$BUNDLE/lib/adafruit_hid" "$CIRCUITPY/lib/"
```

### Required Libraries Summary

| Library | Purpose |
|---------|---------|
| `adafruit_midi` | USB MIDI output |
| `adafruit_ssd1306` | OLED display driver |
| `adafruit_framebuf` | Frame buffer for display |
| `adafruit_bitmap_font` | Font rendering |
| `adafruit_display_text` | Text display utilities |
| `adafruit_displayio_ssd1306` | Display I/O |
| `adafruit_hid` | USB HID support |

---

## Deploying TECLA

### Step 1: Clone or Download TECLA

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/CHIPTUNE.git
cd CHIPTUNE

# Or if you already have it
cd /path/to/CHIPTUNE
```

### Step 2: Deploy to Pico

**macOS:**
```bash
CIRCUITPY="/Volumes/CIRCUITPY"
TECLA_SRC="$(pwd)"

# Copy main code
cp "$TECLA_SRC/main.py" "$CIRCUITPY/"
cp "$TECLA_SRC/reset.py" "$CIRCUITPY/"
cp "$TECLA_SRC/settings.toml" "$CIRCUITPY/"
cp "$TECLA_SRC/font5x8.bin" "$CIRCUITPY/"

# Copy modules
cp -r "$TECLA_SRC/core" "$CIRCUITPY/"
cp -r "$TECLA_SRC/modes" "$CIRCUITPY/"
cp -r "$TECLA_SRC/music" "$CIRCUITPY/"
cp -r "$TECLA_SRC/display" "$CIRCUITPY/"
cp -r "$TECLA_SRC/config" "$CIRCUITPY/"
cp -r "$TECLA_SRC/fonts" "$CIRCUITPY/"

# Sync to ensure all files are written
sync
```

**Linux:**
```bash
CIRCUITPY=$(findmnt -rno TARGET -S LABEL=CIRCUITPY)
TECLA_SRC="$(pwd)"

# Copy main code
cp "$TECLA_SRC/main.py" "$CIRCUITPY/"
cp "$TECLA_SRC/reset.py" "$CIRCUITPY/"
cp "$TECLA_SRC/settings.toml" "$CIRCUITPY/"
cp "$TECLA_SRC/font5x8.bin" "$CIRCUITPY/"

# Copy modules
cp -r "$TECLA_SRC/core" "$CIRCUITPY/"
cp -r "$TECLA_SRC/modes" "$CIRCUITPY/"
cp -r "$TECLA_SRC/music" "$CIRCUITPY/"
cp -r "$TECLA_SRC/display" "$CIRCUITPY/"
cp -r "$TECLA_SRC/config" "$CIRCUITPY/"
cp -r "$TECLA_SRC/fonts" "$CIRCUITPY/"

# Sync filesystem
sync
```

### Step 3: Quick Deploy Script

Create a deploy script for convenience:

```bash
#!/bin/bash
# deploy.sh - Quick deploy TECLA to Pico

set -e

# Detect OS and set mount point
if [[ "$OSTYPE" == "darwin"* ]]; then
    CIRCUITPY="/Volumes/CIRCUITPY"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    CIRCUITPY=$(findmnt -rno TARGET -S LABEL=CIRCUITPY 2>/dev/null || echo "")
fi

if [ -z "$CIRCUITPY" ] || [ ! -d "$CIRCUITPY" ]; then
    echo "Error: CIRCUITPY not found. Is the Pico connected?"
    exit 1
fi

echo "Deploying to $CIRCUITPY..."

# Copy files
cp main.py "$CIRCUITPY/"
cp -r core "$CIRCUITPY/"
cp -r modes "$CIRCUITPY/"
cp -r music "$CIRCUITPY/"
cp -r display "$CIRCUITPY/"
cp -r config "$CIRCUITPY/"

sync
echo "Deploy complete! TECLA will restart automatically."
```

Save as `deploy.sh` and run with `chmod +x deploy.sh && ./deploy.sh`

---

## Verification

### Check Serial Console

**macOS:**
```bash
# Find the serial port
ls /dev/tty.usbmodem*

# Connect (replace with your port)
screen /dev/tty.usbmodem14201 115200
```

**Linux:**
```bash
# Find the serial port
ls /dev/ttyACM*

# Connect (replace with your port)
screen /dev/ttyACM0 115200
```

### Expected Output

On successful boot, you should see:
```
🚀 TECLA Professional - Iniciant...
✅ Mòduls importats
✅ Hardware inicialitzat
✅ Gestors creats
🔦 Prova LEDs: Encenent tots els LEDs...
✅ Prova LEDs completada
✅ Sistema preparat - Arquitectura Modular Activa
🔄 Bucle principal actiu
✅ 2000 it | Mode:1 Oct:5 BPM:120 Gate:50.0ms Nota:C5
```

### Exit Screen

Press `Ctrl+A` then `K`, then `Y` to exit screen.

---

## Development Workflow

### Live Reload

CircuitPython supports auto-reload when files change. Simply save your modified Python file to the CIRCUITPY drive, and the Pico will restart automatically.

### REPL Access

For interactive debugging:

1. Connect via screen (see above)
2. Press `Ctrl+C` to interrupt the running program
3. You'll get a Python REPL (`>>>`)
4. Press `Ctrl+D` to soft-reboot and restart main.py

### Recommended Editor Setup

**VS Code:**
```bash
# Install CircuitPython extension
code --install-extension joedevivo.vscode-circuitpython
```

**Vim/Neovim:**
```bash
# Add to your config for Python syntax
autocmd BufRead,BufNewFile *.py set filetype=python
```

---

## Troubleshooting

### CIRCUITPY Drive Not Appearing

**Problem:** After flashing CircuitPython, no CIRCUITPY drive appears.

**Solutions:**
1. Try a different USB cable (some are power-only)
2. Try a different USB port
3. Re-flash CircuitPython:
   - Hold BOOTSEL, connect USB
   - Copy UF2 file again

### Import Errors

**Problem:** `ImportError: no module named 'adafruit_midi'`

**Solution:** Ensure libraries are in the `lib/` folder:
```bash
ls /Volumes/CIRCUITPY/lib/  # macOS
ls $(findmnt -rno TARGET -S LABEL=CIRCUITPY)/lib/  # Linux
```

### Display Not Working

**Problem:** OLED display shows nothing.

**Solutions:**
1. Check I2C wiring (SDA=GP20, SCL=GP21)
2. Verify display I2C address is 0x3C
3. Check power to display (3.3V)

### No Audio Output

**Problem:** PWM outputs produce no sound.

**Solutions:**
1. Check PWM pin connections (GP22, GP2, GP0)
2. Verify mode is not 0 (Pausa)
3. Check slider/pot position
4. Connect headphones or amplifier with proper impedance

### Permission Denied (Linux)

**Problem:** Cannot write to CIRCUITPY.

**Solution:**
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Log out and back in, or:
newgrp dialout
```

### Serial Port Busy

**Problem:** `screen` says device is busy.

**Solution:**
```bash
# Kill existing screen sessions
screen -ls
screen -X -S [session_name] quit

# Or find and kill the process
fuser /dev/ttyACM0  # Linux
lsof /dev/tty.usbmodem*  # macOS
```

---

## Project Structure

```
CHIPTUNE/
├── main.py              # Main entry point
├── reset.py             # Factory reset script
├── settings.toml        # CircuitPython settings
├── font5x8.bin          # Display font
├── core/                # Core system modules
│   ├── config.py        # Global configuration
│   ├── hardware.py      # Hardware abstraction
│   ├── clock.py         # Tempo/clock system
│   ├── rtos.py          # Real-time task manager
│   ├── midi_handler.py  # MIDI output
│   ├── button_handler.py # Button input
│   └── calibration.py   # CV calibration
├── modes/               # Musical modes
│   └── loader.py        # Mode implementations
├── music/               # Music utilities
│   ├── algorithms.py    # Generative algorithms
│   └── converters.py    # Value conversions
├── display/             # Display modules
│   ├── screens.py       # Screen rendering
│   └── animations.py    # Visual effects
├── config/              # Configuration files
│   └── settings.py      # User settings
├── lib/                 # Adafruit libraries (on device)
├── MANUAL.md            # User manual
├── BUILD.md             # This file
└── GUIA_TECNICA.md      # Technical guide (Catalan)
```

---

## Version Compatibility

| Component | Tested Version |
|-----------|----------------|
| CircuitPython | 10.0.1 |
| adafruit_midi | 1.6.x |
| adafruit_ssd1306 | 2.12.x |
| Python | 3.x (CircuitPython) |

---

*TECLA Build Guide - Last updated: 2025*
