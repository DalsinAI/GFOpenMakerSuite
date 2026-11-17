/* openmakemesh.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function. Every answer must be the C function's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakemesh.h>

#include "../library/openmakemesh_vectors.h"

#include <stdio.h>
#include <string.h>

static int fails, calls;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKEMESH_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakeMeshBase;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake && bias % 6 == 0 && bias >= 30 && bias <= OPENMAKEMESH_LAST_BIAS, "a call in the table");
    calls++;
    return table[bias / 6 - 1];
}

/* Two meshes, triangle for triangle and box for box the same. */
static int same(const gs_mesh *a, const gs_mesh *b)
{
    return a->n == b->n && memcmp(&a->min, &b->min, sizeof a->min) == 0 && memcmp(&a->max, &b->max, sizeof a->max) == 0 &&
           (!a->n || memcmp(a->t, b->t, (size_t)a->n * sizeof *a->t) == 0);
}

int main(void)
{
    gs_mesh c[2], r[2], u[2], s[2], x[2], l[2];
    char err[96];
    int i;
    OpenMakeMeshBase = fake;
    memset(c, 0, sizeof c);
    memset(r, 0, sizeof r);
    memset(u, 0, sizeof u);
    memset(s, 0, sizeof s);
    memset(x, 0, sizeof x);
    memset(l, 0, sizeof l);
    expect(sizeof table / sizeof table[0] == OPENMAKEMESH_LAST_BIAS / 6, "every call has its slot");

    expect(gm_cube(&c[0], 1.5f, -2, 0.25f, 20, 10, 5) == 0 && GM_Cube(&c[1], 1.5f, -2, 0.25f, 20, 10, 5) == 0 &&
               same(&c[0], &c[1]) && c[1].min.x == 1.5f && c[1].max.z == 5.25f,
           "GM_Cube, six floats");
    expect(gm_cylinder(&r[0], 10, 5, -1, 3, 8, 24) == 0 && GM_Cylinder(&r[1], 10, 5, -1, 3, 8, 24) == 0 &&
               same(&r[0], &r[1]),
           "GM_Cylinder, five floats and the sides");
    expect(GM_Cylinder(&r[1], 0, 0, 0, 1, 1, 2) == gm_cylinder(&r[0], 0, 0, 0, 1, 1, 2), "GM_Cylinder refuses two sides");
    expect(GM_Cylinder(&r[1], 10, 5, -1, 3, 8, 24) == 0 && gm_cylinder(&r[0], 10, 5, -1, 3, 8, 24) == 0, "the rod again");

    /* 1.1: the bench's other solids, the same through the library. */
    expect(gm_sphere(&x[0], 1, 2, 3, 6, 20, 10) == 0 && GM_Sphere(&x[1], 1, 2, 3, 6, 20, 10) == 0 && same(&x[0], &x[1]),
           "GM_Sphere, four floats, the sides and rings");
    expect(gm_cone(&x[0], 1, 2, 3, 6, 9, 20) == 0 && GM_Cone(&x[1], 1, 2, 3, 6, 9, 20) == 0 && same(&x[0], &x[1]),
           "GM_Cone");
    expect(gm_torus(&x[0], 1, 2, 3, 8, 2.5f, 20, 12) == 0 && GM_Torus(&x[1], 1, 2, 3, 8, 2.5f, 20, 12) == 0 &&
               same(&x[0], &x[1]),
           "GM_Torus, five floats and two counts");
    expect(gm_wedge(&x[0], -1, 2, 0.5f, 20, 10, 7) == 0 && GM_Wedge(&x[1], -1, 2, 0.5f, 20, 10, 7) == 0 &&
               same(&x[0], &x[1]),
           "GM_Wedge, six floats");
    expect(gm_slot(&x[0], -1, 2, 0.5f, 30, 8, 6, 16) == 0 && GM_Slot(&x[1], -1, 2, 0.5f, 30, 8, 6, 16) == 0 &&
               same(&x[0], &x[1]),
           "GM_Slot, six floats and the sides");
    expect(GM_Torus(&x[1], 0, 0, 0, 2, 3, 20, 12) == -1, "GM_Torus refuses a tube wider than its ring");
    GS_MeshFree(&x[0]);
    GS_MeshFree(&x[1]);

    expect(gm_union(&u[0], &c[0], &r[0]) == 0 && GM_Union(&u[1], &c[1], &r[1]) == 0 && same(&u[0], &u[1]), "GM_Union");
    expect(gm_subtract(&s[0], &c[0], &r[0]) == 0 && GM_Subtract(&s[1], &c[1], &r[1]) == 0 && same(&s[0], &s[1]),
           "GM_Subtract");
    expect(gm_intersect(&x[0], &c[0], &r[0]) == 0 && GM_Intersect(&x[1], &c[1], &r[1]) == 0 && same(&x[0], &x[1]),
           "GM_Intersect");
    expect(GM_OpenEdges(&s[1]) == gm_open_edges(&s[0]) && GM_OpenEdges(&s[1]) == 0, "GM_OpenEdges, a closed solid");

    expect(GM_StlWrite(&s[1], "build/mesh_lib.stl") == 0, "GM_StlWrite");
    expect(gs_stl_load(&l[0], "build/mesh_lib.stl", err, (int)sizeof err) == 0 &&
               GS_StlLoad(&l[1], "build/mesh_lib.stl", err, (int)sizeof err) == 0 && same(&l[0], &l[1]) &&
               l[1].n == s[1].n,
           "GS_StlLoad reads back what GM_StlWrite wrote");
    GS_MeshFree(&l[1]); /* a load starts from an empty mesh */
    err[0] = 0;
    expect(GS_StlLoad(&l[1], "build/no-such.stl", err, (int)sizeof err) == -1 && err[0], "GS_StlLoad says why it failed");
    expect(GS_StlLoad(&l[1], "build/mesh_lib.stl", err, (int)sizeof err) == 0, "and loads again");
    gs_mesh_repair(&l[0]);
    GS_MeshRepair(&l[1]);
    expect(same(&l[0], &l[1]), "GS_MeshRepair");

    for (i = 0; i < 2; i++) {
        gs_mesh *all[] = {&c[i], &r[i], &u[i], &s[i], &x[i], &l[i]};
        unsigned k;
        for (k = 0; k < sizeof all / sizeof all[0]; k++) {
            if (i)
                GS_MeshFree(all[k]);
            else
                gs_mesh_free(all[k]);
            expect(all[k]->n == 0 && all[k]->t == NULL, "GS_MeshFree");
        }
    }
    expect(calls == 29, "every call went through the table");
    if (calls != 29)
        printf("calls: %d\n", calls);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("mesh library ok\n");
    return 0;
}
