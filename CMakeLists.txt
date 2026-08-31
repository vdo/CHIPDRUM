cmake_minimum_required(VERSION 3.13)

# Pico SDK (set PICO_SDK_PATH, or let it fetch from git)
include(pico_sdk_import.cmake)

project(tecla_bass C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(tecla_bass
    src/main.c
    src/hardware.c
    src/voice.c
    src/sampler.c
    src/drumsynth.c
    src/truchet_patterns.c
    src/clockgen.c
    src/buttons.c
    src/scales.c
    src/edge_detect.c
    src/persist.c
    src/ssd1306.c
    src/gfx.c
    src/ui.c
    src/modes/modes.c
    src/modes/mode_bassloop.c
    src/modes/mode_droner.c
    src/modes/mode_organ.c
    src/modes/mode_grinder.c
    src/modes/chop.c
    src/modes/mode_drums.c
    src/modes/mode_syndrum.c
)

target_include_directories(tecla_bass PRIVATE src)

target_link_libraries(tecla_bass
    pico_stdlib
    pico_multicore
    pico_flash
    hardware_pwm
    hardware_timer
    hardware_adc
    hardware_i2c
    hardware_flash
)

# USB serial: gives debug printf AND lets picotool reboot the module into the
# bootloader remotely ("picotool reboot -f -u"), so updating it in a rack does
# not need physical access to the BOOTSEL button.
# UART stdio is off: GP0/GP1 are the voice-3 and clock-out jacks.
pico_enable_stdio_usb(tecla_bass 1)
pico_enable_stdio_uart(tecla_bass 0)

pico_add_extra_outputs(tecla_bass) # produces .uf2
