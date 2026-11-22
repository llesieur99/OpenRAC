/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (c) 2026 the OpenRAC contributors */
/* Optional Greatest Hits corrections for the two earlier discs. The original
 * functions remain callable when a fix is disabled. See differences.json. */
#include <stdio.h>
#include <string.h>
#include "game_protos.h"
#include "openrac/game_host.h"
#include "openrac/guest.h"
#include "rac2.h"
#include "gh_fixes.h"

static int greatest_hits(void) {
    return strcmp(openrac_game.id, "rac2-ntsc2") == 0;
}

static const unsigned char* reverse_fix;
static rac2_mining_thrust* mining_thrust;

void func_L08_0041EBF8(gaddr moby) {
    if (greatest_hits()) {
        func_L08_0041EBF8__game(moby);
        return;
    }
    if (!reverse_fix) {
        reverse_fix = openrac_game_fix("fix_mining_reverse");
    }
    rac2_mining_thrust phase = {moby + 0xC0u, 0, 0.0f};
    rac2_mining_thrust* previous = mining_thrust;
    mining_thrust = *reverse_fix ? &phase : NULL;
    func_L08_0041EBF8__game(moby);
    mining_thrust = previous;
}

void func_00282F20(gaddr destination, gaddr source, float scale) {
    func_00282F20__game(destination, source,
                      rac2_mining_thrust_scale(mining_thrust, destination, source, scale));
}

#define PLANET_INFO_COUNT_WIDGET RAC2_K(PLANET_INFO_COUNT_WIDGET, 0x410, func_0033ED88)
#define PLANET_INFO_COUNT_BUFFER RAC2_K(PLANET_INFO_COUNT_BUFFER, 0x600, func_0033ED88)
#define PLANET_INFO_COUNT_LABEL RAC2_K(PLANET_INFO_COUNT_LABEL, 0x2DBB, func_0033ED88)
#define PLANET_INFO_TEXT_POINTER RAC2_K(PLANET_INFO_TEXT_POINTER, 0x40, func_00336A38)
#define PLANET_INFO_COUNT_TITLE RAC2_K(PLANET_INFO_COUNT_TITLE, 0x4E8, func_0033ED88)

void func_0033ED88(gaddr object) {
    static const unsigned char* counter_fix;
    if (greatest_hits()) {
        func_0033ED88__game(object);
        return;
    }
    if (!counter_fix) {
        counter_fix = openrac_game_fix("fix_planet_platinum_count");
    }
    if (!*counter_fix) {
        func_0033ED88__game(object);
        return;
    }
    const int planet = GREF(int, OPENRAC_DATA(RAC2_ADDR(0x001C4FA0u)) + 0x230u);
    /* GH shows this row for the selectable planets 1..20, not special maps. */
    if (planet < 1 || planet > 20) {
        return;
    }
    const gaddr widget = object + PLANET_INFO_COUNT_WIDGET;
    const gaddr buffer = object + PLANET_INFO_COUNT_BUFFER;
    const gaddr text = GREF(gaddr, widget + PLANET_INFO_TEXT_POINTER);
    const gaddr position = func_00335E10(widget);
    const float x = GREF(float, position), y = GREF(float, position + 4u);
    const int gap = GREF(int, OPENRAC_DATA(RAC2_ADDR(0x001ADF90u)));
    const uint8_t (*flags)[4] = G(RAC2_ADDR(0x0019B278u));

    /* Use the disc's localized label directly instead of copying it into the
     * original 24-byte numeric buffer (some translations can be longer). */
    func_00336A38(widget, func_00288FB8(PLANET_INFO_COUNT_LABEL));
    func_00336A30(widget, 0);
    func_00336A78(widget);
    snprintf(G(buffer), 24, "%u/%u", rac2_planet_bolt_count(flags, (unsigned)planet),
             rac2_planet_bolt_total((unsigned)planet));
    func_00336A38(widget, buffer);
    func_00335E38(widget, x + (float)gap, y, 0.0f, 0.0f);
    func_00336A30(widget, 2);
    func_00336A78(widget);
    func_00335E38(widget, x, y, 0.0f, 0.0f);
    func_00336A38(widget, text);
    func_00336A30(widget, 0);
    /* The original heading is already the disc's localized "Platinum Bolt". */
    func_00336A78(object + PLANET_INFO_COUNT_TITLE);
}

/* Borrow the revised geometry only for this screen's update/draw. Do not
 * change saved state or repeatedly shrink the markers in the loaded program. */
typedef struct galaxy_layout {
    rac2_galaxy_marker markers[21];
    float bar_x, map_x, select_y, exit_y;
} galaxy_layout;

#define GALAXY_MARKERS OPENRAC_DATA(RAC2_ADDR(0x00265140u))
#define GALAXY_BAR_X OPENRAC_DATA(RAC2_ADDR(0x001ADD98u))
#define GALAXY_MAP_X OPENRAC_DATA(RAC2_ADDR(0x001ADDB8u))
#define GALAXY_SELECT_Y OPENRAC_DATA(RAC2_ADDR(0x001ADDACu))
#define GALAXY_EXIT_Y OPENRAC_DATA(RAC2_ADDR(0x001ADDB4u))

static int revised_galaxy(void) {
    static const unsigned char* fix;
    if (strcmp(openrac_game.id, "rac2-ntsc") != 0) {
        return 0; /* PAL already has the narrower map and revised marker table. */
    }
    if (!fix) {
        fix = openrac_game_fix("fix_galactic_map_layout");
    }
    return *fix != 0;
}

void func_002CBC58(int mode) {
    /* The list is a separate panel, laid out before the map frame is drawn. */
    if (!revised_galaxy()) {
        func_002CBC58__game(mode);
        return;
    }
    const gaddr page = GREF(gaddr, OPENRAC_DATA(0x001F2840u) + 0x14u);
    const gaddr frame = page ? GREF(gaddr, page + 0xECu) : 0;
    /* Resolve the callback, not a boot-only page address: levels relocate it. */
    if (!frame || GREF(gaddr, frame + 4u) != OPENRAC_CODE(0x002D5920u)) {
        func_002CBC58__game(mode);
        return;
    }
    int32_t saved[4];
    const int32_t bounds[4] = {125, 166, 186, 186};
    memcpy(saved, G(page + 0x10u), sizeof(saved));
    memcpy(G(page + 0x10u), bounds, sizeof(bounds));
    func_002CBC58__game(mode);
    memcpy(G(page + 0x10u), saved, sizeof(saved));
}

static int galaxy_line_right;

void func_0028FAB8(int x0, int y0, int x1, int y1, int color, int flag) {
    /* The original 304-pixel crosshair must stop at the narrowed map edge. */
    if (galaxy_line_right && y0 == y1 && x1 > galaxy_line_right) {
        x1 = galaxy_line_right;
    }
    func_0028FAB8__game(x0, y0, x1, y1, color, flag);
}

typedef struct galaxy_frame {
    gaddr vertices;
    uint32_t saved[121];
    unsigned count;
} galaxy_frame;

static void borrow_frame(gaddr widget, unsigned layer, galaxy_frame* saved) {
    saved->count = 0;
    const gaddr model = GREF(gaddr, widget + 0x40u);
    if (!model) return;
    const unsigned expected = layer == 1 ? 12 : 121;
    if (GREF(uint16_t, model) != expected || GREF(int32_t, model + 0x24u) != 1 ||
        GREF(uint16_t, model + 6u) != 0 || GREF(uint16_t, model + 8u) != 0) return;
    /* Model loading scales this field by 0.0032116016; use its authored scale. */
    const float scale = GREF(float, model + 0xCu) / 0.0032116016f;
    saved->vertices = GREF(gaddr, model + 0x10u);
    if (!saved->vertices) return;
    saved->count = expected;
    memcpy(saved->saved, G(saved->vertices), expected * sizeof(uint32_t));
    for (unsigned i = 0; i < expected; ++i) {
        GREF(uint32_t, saved->vertices + i * 4u) =
            rac2_galaxy_frame_vertex(saved->saved[i], layer, i, scale);
    }
}

static void restore_frame(const galaxy_frame* saved) {
    if (saved->count) memcpy(G(saved->vertices), saved->saved, saved->count * sizeof(uint32_t));
}

static void borrow_galaxy(galaxy_layout* saved) {
    memcpy(saved->markers, G(GALAXY_MARKERS), sizeof(saved->markers));
    saved->bar_x = GREF(float, GALAXY_BAR_X);
    saved->map_x = GREF(float, GALAXY_MAP_X);
    saved->select_y = GREF(float, GALAXY_SELECT_Y);
    saved->exit_y = GREF(float, GALAXY_EXIT_Y);
    rac2_gh_galaxy_markers(G(GALAXY_MARKERS));
    GREF(float, GALAXY_BAR_X) = -25.0f;
    GREF(float, GALAXY_MAP_X) = -20.0f;
    GREF(float, GALAXY_SELECT_Y) = 101.0f;
    GREF(float, GALAXY_EXIT_Y) = 124.0f;
}

static void restore_galaxy(const galaxy_layout* saved) {
    memcpy(G(GALAXY_MARKERS), saved->markers, sizeof(saved->markers));
    GREF(float, GALAXY_BAR_X) = saved->bar_x;
    GREF(float, GALAXY_MAP_X) = saved->map_x;
    GREF(float, GALAXY_SELECT_Y) = saved->select_y;
    GREF(float, GALAXY_EXIT_Y) = saved->exit_y;
}

int func_0033B540(gaddr object, int buttons) {
    if (strcmp(openrac_game.id, "rac2-ntsc") != 0) {
        return func_0033B540__game(object, buttons);
    }
    const int enabled = revised_galaxy();
    /* This branch only runs for v1.01: its map sprite is object + 0x2B0. */
    func_00335F88(object + 0x2B0u, enabled ? 240.0f : 302.0f, 194.0f, 0.0f, 0.0f);
    if (!enabled) {
        return func_0033B540__game(object, buttons);
    }
    galaxy_layout saved;
    borrow_galaxy(&saved);
    const int result = func_0033B540__game(object, buttons);
    restore_galaxy(&saved);
    return result;
}

void func_0033B910(gaddr object) {
    if (!revised_galaxy()) {
        func_0033B910__game(object);
        return;
    }
    galaxy_layout saved;
    galaxy_frame fill, border;
    borrow_frame(object + 0x4Cu, 1, &fill);
    borrow_frame(object + 0x98u, 2, &border);
    const int previous_right = galaxy_line_right;
    const gaddr position = func_00335E10(object + 0x2B0u);
    galaxy_line_right = (int)(GREF(float, position) + 240.0f);
    borrow_galaxy(&saved);
    func_0033B910__game(object);
    restore_galaxy(&saved);
    galaxy_line_right = previous_right;
    restore_frame(&border);
    restore_frame(&fill);
}
