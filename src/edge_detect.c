#include "edge_detect.h"

void edge_init(edge_t *e, uint16_t min_swing, uint16_t decay) {
    e->lo = 0xFFFF;
    e->hi = 0;
    e->min_swing = min_swing;
    e->decay = decay;
    e->level = false;
}

bool edge_feed(edge_t *e, uint16_t v, bool decay_now) {
    if (v < e->lo) e->lo = v;
    if (v > e->hi) e->hi = v;

    if (decay_now && e->hi > e->lo + (uint16_t)(e->decay * 2)) {
        e->hi -= e->decay;
        e->lo += e->decay;
    }

    uint16_t swing = edge_swing(e);
    if (swing < e->min_swing) { // nothing convincing patched in
        e->level = false;
        return false;
    }

    uint16_t mid = e->lo + swing / 2;
    uint16_t hyst = swing / 4;
    if (!e->level) {
        if (v > mid + hyst) {
            e->level = true;
            return true;
        }
    } else if (v < mid - hyst) {
        e->level = false;
    }
    return false;
}
