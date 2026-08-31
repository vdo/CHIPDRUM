#include "persist.h"

#include <stddef.h>
#include <string.h>

#include "hardware/flash.h"
#include "pico/flash.h"

#include "mode.h"
#include "state.h"

// Last 4 KB sector of the 2 MB flash
#define PERSIST_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define PERSIST_MAGIC 0x54454331u // "TEC1"
#define PERSIST_VERSION 2
#define MODE_SAVE_DELAY_US 1000000u

typedef struct {
    uint32_t magic;
    uint16_t version;
    int16_t mode_idx;
    int16_t root;
    int16_t octave;
    int16_t gate_pct;
    uint32_t checksum;
} persist_t;

static bool mode_dirty;
static uint64_t mode_save_at;

static uint32_t checksum(const persist_t *p) {
    const uint8_t *b = (const uint8_t *)p;
    uint32_t c = 0xA5A5A5A5u;
    for (size_t i = 0; i < offsetof(persist_t, checksum); i++)
        c = (c << 1 | c >> 31) ^ b[i];
    return c;
}

void persist_load(void) {
    const persist_t *p = (const persist_t *)(XIP_BASE + PERSIST_OFFSET);
    if (p->magic != PERSIST_MAGIC ||
        (p->version != 1 && p->version != PERSIST_VERSION))
        return;
    if (p->checksum != checksum(p)) return;
    // Version 1 used the old seven-mode ordering. Preserve its musical
    // globals, but deliberately ignore that numeric mode index now that the
    // Euclidean mode is gone and DRUMS owns slot 0.
    if (p->version == PERSIST_VERSION && p->mode_idx >= 0 &&
        p->mode_idx < NUM_MODES)
        g.mode_idx = p->mode_idx;
    if (p->root >= 0 && p->root <= 11) g.root = p->root;
    if (p->octave >= 0 && p->octave <= 3) g.octave = p->octave;
    if (p->gate_pct >= 10 && p->gate_pct <= 90) g.gate_pct = p->gate_pct;
}

static void do_write(void *param) {
    const uint8_t *page = (const uint8_t *)param;
    flash_range_erase(PERSIST_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(PERSIST_OFFSET, page, FLASH_PAGE_SIZE);
}

bool persist_save(void) {
    static uint8_t page[FLASH_PAGE_SIZE]; // static: keep off the stack
    memset(page, 0xFF, sizeof(page));
    persist_t p = {
        .magic = PERSIST_MAGIC,
        .version = PERSIST_VERSION,
        .mode_idx = g.mode_idx,
        .root = g.root,
        .octave = g.octave,
        .gate_pct = g.gate_pct,
    };
    p.checksum = checksum(&p);
    memcpy(page, &p, sizeof(p));
    // flash_safe_execute pauses core 1 (which called
    // flash_safe_execute_core_init) and disables IRQs during the write
    bool ok = flash_safe_execute(do_write, page, 200) == PICO_OK;
    if (ok) mode_dirty = false;
    return ok;
}

void persist_mode_changed(uint64_t now) {
    mode_dirty = true;
    mode_save_at = now + MODE_SAVE_DELAY_US;
}

void persist_task(uint64_t now) {
    if (!mode_dirty || now < mode_save_at) return;
    // Retry later on the unlikely chance that flash-safe execution is busy.
    if (!persist_save()) mode_save_at = now + 5000000u;
}
