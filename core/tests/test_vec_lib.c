/* openmakevec.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function. Every answer must be the C function's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakevec.h>

#include "../library/openmakevec_vectors.h"

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

static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKEVEC_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakeVecBase;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake && bias % 6 == 0 && bias >= 30 && bias <= OPENMAKEVEC_LAST_BIAS, "a call in the table");
    calls++;
    return table[bias / 6 - 1];
}

static int same_doc(const gv_doc *a, const gv_doc *b)
{
    int i, k;
    if (a->n != b->n || a->nlayer != b->nlayer) return 0;
    if (a->width != b->width || a->height != b->height) return 0;
    for (i = 0; i < a->nlayer; i++)
        if (a->layer[i].colour != b->layer[i].colour || a->layer[i].n != b->layer[i].n) return 0;
    for (i = 0; i < a->n; i++) {
        const gv_path *p = &a->p[i], *q = &b->p[i];
        if (p->n != q->n || p->closed != q->closed || p->stroke != q->stroke || p->fill != q->fill ||
            p->width != q->width)
            return 0;
        for (k = 0; k < p->n * 2; k++)
            if (p->xy[k] != q->xy[k]) return 0;
    }
    return 1;
}

int main(void)
{
    static const char *svg =
        "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
        "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"none\" stroke=\"#ff0000\"/>"
        "</svg>";
    gv_doc a, b;
    char err[128], errb[128];
    float ax0, ay0, ax1, ay1, bx0, by0, bx1, by1;
    OpenMakeVecBase = fake;
    expect(sizeof table / sizeof table[0] == OPENMAKEVEC_LAST_BIAS / 6, "every call has its slot");

    memset(&a, 0xff, sizeof a);
    GV_DocInit(&a);
    gv_doc_init(&b);
    expect(a.size == b.size && a.n == 0 && a.p == NULL, "GV_DocInit");

    err[0] = errb[0] = 0;
    expect(gv_load_svg_text(&b, svg, -1, errb, (int)sizeof errb) == 0, "the C load");
    expect(GV_LoadSvgText(&a, svg, -1, err, (int)sizeof err) == 0 && same_doc(&a, &b),
           "GV_LoadSvgText matches the C function");
    expect(GV_PathColour(&a.p[0]) == gv_path_colour(&b.p[0]) && GV_PathColour(&a.p[0]) == 0xff0000,
           "GV_PathColour");
    gv_path_box(&b.p[0], &bx0, &by0, &bx1, &by1);
    GV_PathBox(&a.p[0], &ax0, &ay0, &ax1, &ay1);
    expect(ax0 == bx0 && ay0 == by0 && ax1 == bx1 && ay1 == by1, "GV_PathBox");

    GV_DocFree(&a);
    expect(a.n == 0 && a.p == NULL && a.layer == NULL, "GV_DocFree");
    expect(GV_LoadSvg(&a, "core/tests/data/rect-10mm.svg", err, (int)sizeof err) == 0 &&
               gv_load_svg(&b, "core/tests/data/rect-10mm.svg", errb, (int)sizeof errb) == 0 &&
               same_doc(&a, &b),
           "GV_LoadSvg matches the C function");
    err[0] = 0;
    expect(GV_LoadSvg(&a, "core/tests/data/no-such.svg", err, (int)sizeof err) == -1 && err[0],
           "GV_LoadSvg says why it failed");
    GV_DocFree(&a);
    gv_doc_free(&b);
    expect(calls == 9, "every call went through the table");
    if (calls != 9)
        printf("calls: %d\n", calls);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("vec library ok\n");
    return 0;
}
