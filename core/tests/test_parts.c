/* gs_parts: the bench's parts list. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_model.h"
#include "gs_parts.h"

#include <stdio.h>
#include <string.h>

static int failed;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        failed++;
    }
}

int main(void)
{
    gs_parts *ps = gs_parts_new(), *copy;
    gs_mesh m = {0}, all = {0};
    int i, n;
    char name[80];
    expect(ps && gs_parts_count(ps) == 0 && gs_parts_picked(ps) == -1, "a new list is empty, nothing picked");
    gm_cube(&m, 0, 0, 0, 10, 10, 10);
    n = m.n;
    expect(gs_parts_add(ps, &m, "  Block  ") == 0 && m.n == 0 && !m.t && gs_parts_mesh(ps, 0)->n == n,
           "add takes the mesh's triangles");
    expect(!strcmp(gs_parts_name(ps, 0), "Block") && gs_parts_shown(ps, 0) == 1 && gs_parts_picked(ps) == 0 &&
               gs_parts_colour(ps, 0) == 0,
           "trimmed name, shown, picked, the first metal");
    gm_cylinder(&m, 30, 0, 0, 5, 10, 16);
    expect(gs_parts_add(ps, &m, NULL) == 1 && !strcmp(gs_parts_name(ps, 1), "Part 2") && gs_parts_colour(ps, 1) == 1 &&
               gs_parts_picked(ps) == 1,
           "no name is Part n; the next metal; the new part picked");
    expect(gs_parts_add(ps, &m, "empty") == -1, "a part needs triangles");
    expect(gs_parts_rename(ps, 1, " Peg ") == 0 && !strcmp(gs_parts_name(ps, 1), "Peg") &&
               gs_parts_rename(ps, 1, "   ") == -1 && !strcmp(gs_parts_name(ps, 1), "Peg"),
           "rename trims; a blank name is refused");
    memset(name, 'x', sizeof name - 1);
    name[sizeof name - 1] = 0;
    expect(gs_parts_rename(ps, 1, name) == 0 && strlen(gs_parts_name(ps, 1)) == GS_PARTS_NAME, "names keep to 48");
    expect(gs_parts_set_colour(ps, 0, 4) == 0 && gs_parts_colour(ps, 0) == 4 && gs_parts_set_colour(ps, 0, 6) == -1 &&
               gs_parts_rgb(0) == 0xc4a15aUL && gs_parts_rgb(4) == 0x3e8f96UL,
           "six metals, the bench's");
    expect(gs_parts_scene(ps, &all) == 0 && all.n == gs_parts_mesh(ps, 0)->n + gs_parts_mesh(ps, 1)->n,
           "the scene is both parts");
    expect(gs_parts_show(ps, 0, 0) == 0 && gs_parts_shown(ps, 0) == 0 && gs_parts_scene(ps, &all) == 0 &&
               all.n == gs_parts_mesh(ps, 1)->n && all.min.x > 24.f,
           "a hidden part is not in the scene");
    copy = gs_parts_copy(ps);
    expect(copy && gs_parts_count(copy) == 2 && gs_parts_mesh(copy, 0)->t != gs_parts_mesh(ps, 0)->t &&
               gs_parts_mesh(copy, 0)->n == n && gs_parts_shown(copy, 0) == 0 && gs_parts_colour(copy, 0) == 4 &&
               gs_parts_picked(copy) == 1,
           "a copy for undo owns its meshes and keeps the rest");
    expect(gs_parts_pick(ps, 0) == 0 && gs_parts_picked(ps) == 0 && gs_parts_pick(ps, 5) == -1 &&
               gs_parts_pick(ps, -1) == 0 && gs_parts_picked(ps) == -1,
           "pick a part, or none");
    expect(gs_parts_remove(ps, 0) == 0 && gs_parts_count(ps) == 1 && !strncmp(gs_parts_name(ps, 0), "xxx", 3) &&
               gs_parts_picked(ps) == 0 && gs_parts_remove(ps, 3) == -1,
           "remove: the rest move up, the last is picked");
    for (i = 1; i < GS_PARTS_MAX; i++) {
        gm_cube(&m, 0, 0, 0, 1, 1, 1);
        gs_parts_add(ps, &m, NULL);
    }
    gm_cube(&m, 0, 0, 0, 1, 1, 1);
    expect(gs_parts_count(ps) == GS_PARTS_MAX && gs_parts_add(ps, &m, NULL) == -1 && m.n == 12,
           "24 parts is the limit; the mesh stays the caller's");
    expect(gs_parts_mesh(ps, 24) == NULL && gs_parts_name(ps, -1) == NULL && gs_parts_shown(ps, 30) == -1,
           "no such part");
    gs_parts_free(copy);
    gs_parts_free(ps);
    gs_parts_free(NULL);
    gs_mesh_free(&m);
    gs_mesh_free(&all);
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("parts ok\n");
    return 0;
}
