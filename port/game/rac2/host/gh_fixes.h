/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
#ifndef OPENRAC_RAC2_GH_FIXES_H
#define OPENRAC_RAC2_GH_FIXES_H

#include <math.h>
#include <stdint.h>

typedef struct rac2_galaxy_marker {
    int32_t x, y, icon;
    float diameter;
} rac2_galaxy_marker;

/* Move the existing v1.01 frame's divider and button border. These are
 * transformations of the player's HUD model, not replacement asset vertices.
 * Packed HUD vertices contain signed 14-bit X/Z and a four-bit bone index. */
static inline uint32_t rac2_galaxy_frame_vertex(
    uint32_t vertex, unsigned layer, unsigned index, float scale
) {
    int move_x = 0, move_z = 0;
    if (layer == 1) {
        move_x = index == 4 || index == 5;
        move_z = index == 2 || index == 3 || index == 5 || index == 6 || index == 7 || index == 10;
    } else if (layer == 2) {
        move_x = (index >= 46 && index <= 54) || (index >= 58 && index <= 60) || index == 80
                 || index == 81 || (index >= 83 && index <= 86) || (index >= 89 && index <= 92);
        move_z = (index >= 61 && index <= 63) || (index >= 66 && index <= 68) || index == 73
                 || index == 74 || index == 78 || index == 79 || (index >= 83 && index <= 88)
                 || index == 91 || index == 92;
    }
    if (!(scale >= 0.05f && scale <= 0.2f)) {
        return vertex;
    }
    int x = (int)(vertex & 0x3fffu), z = (int)((vertex >> 14) & 0x3fffu);
    if (x & 0x2000) {
        x -= 0x4000;
    }
    if (z & 0x2000) {
        z -= 0x4000;
    }
    if (move_x) {
        x += (int)lroundf(190.7f / scale);
    }
    if (move_z) {
        z += (int)lroundf(84.3f / scale);
    }
    /* Two corners follow the raised junction's bevel. */
    if (layer == 2 && (index == 51 || index == 52)) {
        z -= (int)lroundf(1.43f / scale);
    }
    if (x < -8192 || x > 8191 || z < -8192 || z > 8191) {
        return vertex;
    }
    return (vertex & 0xf0000000u) | ((uint32_t)x & 0x3fffu) | (((uint32_t)z & 0x3fffu) << 14);
}

/* The revised map is 240 pixels wide rather than 302. PAL and GH have the
 * same 21 marker records; their Y coordinates and icon choices match v1.01.
 * Only v1.01 needs these horizontal positions and smaller markers. */
static inline void rac2_gh_galaxy_markers(rac2_galaxy_marker markers[21]) {
    static const int16_t x[21] = {-99, 15,  58, 104, 137, 170, 163, 185, 148, 114, 62,
                                  6,   -10, 10, 35,  61,  103, 92,  92,  45,  22};

    for (unsigned i = 0; i < 21; ++i) {
        markers[i].x = x[i];
        markers[i].diameter -= i == 20 ? 3.0f : 2.0f;
    }
}

/* Greatest Hits' planet totals (func_US2_002B1D18). Planet 26's bolts belong to
 * Maktar's counter, as in func_US2_002B1C90. These are counts, not asset data. */
static inline unsigned rac2_planet_bolt_total(unsigned planet) {
    static const uint8_t totals[] = {0, 2, 2, 2, 2, 1, 3, 2, 3, 2, 1, 2, 3, 3, 1, 1, 1, 2, 2, 3, 2};
    return planet < sizeof(totals) ? totals[planet] : 0;
}

static inline unsigned rac2_planet_bolt_count(const uint8_t flags[28][4], unsigned planet) {
    if (planet >= 28) {
        return 0;
    }

    unsigned count = 0;

    for (unsigned i = 0; i < 4; ++i) {
        count += flags[planet][i] != 0;

        if (planet == 2) {
            count += flags[26][i] != 0;
        }
    }

    return count;
}

/* The mining update first scales its forward basis into thrust, then scales
 * that same temporary in place after limiting its vertical component. GH uses
 * |acceleration| for the second scale, so reverse thrust keeps its sign.
 * Track that pair, not every negative vector scale in the vehicle's helpers. */
typedef struct rac2_mining_thrust {
    uint32_t forward;
    uint32_t pending;
    float acceleration;
} rac2_mining_thrust;

static inline float rac2_mining_thrust_scale(
    rac2_mining_thrust* phase, uint32_t destination, uint32_t source, float scale
) {
    if (phase == 0) {
        return scale;
    }

    const uint32_t pending = phase->pending;
    const float acceleration = phase->acceleration;
    phase->pending = 0;

    if (source == phase->forward && destination != source) {
        phase->pending = destination;
        phase->acceleration = scale;
    } else if (
        pending != 0 && source == pending && destination == source && scale == acceleration
    ) {
        return fabsf(scale);
    }

    return scale;
}

#endif
