/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include "../game/rac2/host/gh_fixes.h"

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "%d: %s\n", __LINE__, #c); exit(1); } } while (0)

int main(void) {
    /* Reposition the divider without replacing the model or its bone bits. */
    const uint32_t divider = 0xa0000000u | (3899u << 14) | ((uint32_t)-2781 & 0x3fffu);
    const uint32_t moved = rac2_galaxy_frame_vertex(divider, 1, 4, 0.109687753f);
    CHECK((moved & 0xf0000000u) == (divider & 0xf0000000u));
    CHECK(((moved >> 14) & 0x3fffu) == 3899u);
    CHECK((moved & 0x3fffu) == ((uint32_t)-1042 & 0x3fffu));
    CHECK(rac2_galaxy_frame_vertex(divider, 0, 4, 0.109687753f) == divider);
    CHECK(rac2_galaxy_frame_vertex(divider, 1, 0, 0.109687753f) == divider);
    CHECK(rac2_galaxy_frame_vertex(divider, 1, 4, 0.0f) == divider);
    CHECK(rac2_galaxy_frame_vertex(divider, 1, 4, NAN) == divider);
    const uint32_t button = (uint32_t)-5860 & 0x3fffu;
    CHECK((rac2_galaxy_frame_vertex(button, 1, 2, 0.109687753f) & 0x3fffu) == button);
    CHECK((rac2_galaxy_frame_vertex(button, 1, 2, 0.109687753f) >> 14) == 769u);
    /* A modified model outside the packed coordinate range is left intact. */
    CHECK(rac2_galaxy_frame_vertex(8191, 1, 4, 0.109687753f) == 8191);
    rac2_galaxy_marker markers[21];
    for (unsigned i = 0; i < 21; ++i) {
        markers[i] = (rac2_galaxy_marker){-200, (int32_t)i, (int32_t)(i % 3), 18.0f};
    }
    rac2_gh_galaxy_markers(markers);
    CHECK(markers[0].x == -99 && markers[1].x == 15 && markers[20].x == 22);
    for (unsigned i = 0; i < 21; ++i) {
        CHECK(markers[i].y == (int)i && markers[i].icon == (int)(i % 3));
        CHECK(markers[i].diameter == (i == 20 ? 15.0f : 16.0f));
    }
    uint8_t flags[28][4] = {{0}};
    unsigned total = 0;
    for (unsigned i = 0; i <= 20; ++i) {
        total += rac2_planet_bolt_total(i);
        CHECK(rac2_planet_bolt_count(flags, i) == 0);
    }
    CHECK(total == 40);
    flags[2][0] = 1;
    flags[26][1] = 128;
    flags[1][0] = 1;
    CHECK(rac2_planet_bolt_count(flags, 2) == 2);
    CHECK(rac2_planet_bolt_total(2) == 2);
    CHECK(rac2_planet_bolt_count(flags, 1) == 1);
    CHECK(rac2_planet_bolt_count(flags, 26) == 1);
    CHECK(rac2_planet_bolt_count(flags, UINT32_MAX) == 0);
    CHECK(rac2_planet_bolt_total(21) == 0);
    CHECK(rac2_planet_bolt_total(UINT32_MAX) == 0);

    rac2_mining_thrust phase = {0x10C0, 0, 0};
    /* Backward stays backward after both scales; forward stays forward. */
    for (int direction = -1; direction <= 1; direction += 2) {
        const float acceleration = direction * 0.5f;
        const float first = rac2_mining_thrust_scale(&phase, 0x7000, 0x10C0, acceleration);
        const float second = rac2_mining_thrust_scale(&phase, 0x7000, 0x7000, acceleration);
        CHECK(first * second == direction * 0.25f);
        CHECK(phase.pending == 0);
    }
    CHECK(rac2_mining_thrust_scale(0, 0x7000, 0x7000, -0.5f) == -0.5f);
    CHECK(rac2_mining_thrust_scale(&phase, 0x7000, 0x7000, -0.5f) == -0.5f);
    rac2_mining_thrust_scale(&phase, 0x7000, 0x10C0, -0.5f);
    CHECK(rac2_mining_thrust_scale(&phase, 0x7100, 0x10D0, -0.5f) == -0.5f);
    CHECK(rac2_mining_thrust_scale(&phase, 0x7000, 0x7000, -0.5f) == -0.5f);
    rac2_mining_thrust_scale(&phase, 0x7000, 0x10C0, -0.5f);
    CHECK(rac2_mining_thrust_scale(&phase, 0x7000, 0x7000, -0.25f) == -0.25f);
    CHECK(phase.pending == 0);
    puts("RAC2 map geometry, per-planet counts and mining reverse controls passed");
    return 0;
}
