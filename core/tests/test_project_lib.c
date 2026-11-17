/* openmakeproject.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function. Every answer must be the C function's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakeproject.h>

#include "../library/openmakeproject_vectors.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if !defined(__amigaos__) && !defined(AMIGA) && !defined(__AMIGA__)
#include <unistd.h>
#endif

static int fails, calls;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKEPROJECT_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakeProjectBase;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake && bias % 6 == 0 && bias >= 30 && bias <= OPENMAKEPROJECT_LAST_BIAS, "a call in the table");
    calls++;
    return table[bias / 6 - 1];
}

static int write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb");
    size_t n = strlen(text);
    int ok = f && fwrite(text, 1, n, f) == n;
    if (f && fclose(f)) ok = 0;
    return ok ? 0 : -1;
}

int main(void)
{
    static const char *sample =
        "format = 1\n"
        "name = Widget\n"
        "later-key = kept\n"
        "item.1.kind = cad\n"
        "item.1.name = Body\n"
        "item.1.file = cad/body.stl\n"
        "item.2.kind = render\n"
        "item.2.name = Shot\n"
        "item.2.file = render/shot.exr\n"
        "item.2.engine = later\n";
    gpr_project a, b;
    gpr_add_opts o;
    gpr_recent rec, recb;
    char err[128], errb[128], buf[2048], bufb[2048], dir[128], stl[512], resolved[512], pcb[512], sch[512];
    int n, nb, i;
    OpenMakeProjectBase = fake;

    err[0] = errb[0] = 0;
    GPR_Init(&a);
    gpr_init(&b);
    expect(a.size == b.size && a.n == 0, "GPR_Init");

    expect(GPR_Parse(&a, sample, err, (int)sizeof err) == 0 && gpr_parse(&b, sample, errb, (int)sizeof errb) == 0,
           "GPR_Parse");
    expect(a.n == 2 && b.n == 2 && !strcmp(a.items[1].kind, "render") && a.items[1].nextra == 1 &&
               !strcmp(a.extra[0].value, b.extra[0].value),
           "an unknown kind matches the C parse");
    n = GPR_Format(&a, buf, (int)sizeof buf);
    nb = gpr_format(&b, bufb, (int)sizeof bufb);
    expect(n == nb && n > 0 && !strcmp(buf, bufb) && strstr(buf, "item.2.engine = later"), "GPR_Format");

    expect(GPR_KindKnown("cad") == gpr_kind_known("cad") && GPR_KindKnown("render") == 0, "GPR_KindKnown");
    expect(!strcmp(GPR_KindGroup("laser"), gpr_kind_group("laser")), "GPR_KindGroup");
    expect(!strcmp(GPR_KindApp("cad"), gpr_kind_app("cad")) && GPR_KindApp("render") == gpr_kind_app("render"),
           "GPR_KindApp");
    expect(!strcmp(GPR_KindPort("print"), gpr_kind_port("print")), "GPR_KindPort");
    expect(!strcmp(GPR_KindSuggest("a.svg"), "laser") && GPR_KindSuggest("a.gcode") == NULL, "GPR_KindSuggest");

#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
    snprintf(dir, sizeof dir, "RAM:gpr-lib");
#else
    snprintf(dir, sizeof dir, "/tmp/gpr-lib-%d", (int)getpid());
#endif
    GPR_Free(&a);
    gpr_free(&b);
    expect(GPR_Create(&a, dir, "LibWidget", err, (int)sizeof err) == 0, "GPR_Create");
    snprintf(stl, sizeof stl, "%s/part.stl", dir);
    expect(write_file(stl, "solid x\nendsolid x\n") == 0, "a dummy STL");
    GPR_AddDefaults(&o);
    o.kind = "cad";
    o.file = stl;
    i = GPR_Add(&a, &o, err, (int)sizeof err);
    expect(i == 0 && !strcmp(a.items[0].file, "cad/part.stl"), "GPR_Add copies into cad/");
    expect(GPR_SetName(&a, "Renamed") == 0 && !strcmp(a.name, "Renamed"), "GPR_SetName");
    expect(GPR_SetNotes(&a, "notes") == 0 && !strcmp(a.notes, "notes"), "GPR_SetNotes");
    expect(GPR_Rename(&a, 0, "Part") == 0 && GPR_Find(&a, "Part") == 0 && GPR_Find(&a, "1") == 0, "GPR_Rename/Find");
    expect(GPR_Resolve(&a, 0, resolved, (int)sizeof resolved) == 0 && strstr(resolved, "cad/part.stl") != NULL,
           "GPR_Resolve");
    expect(GPR_Save(&a, err, (int)sizeof err) == 0, "GPR_Save");
    GPR_Free(&b);
    expect(GPR_Open(&b, dir, errb, (int)sizeof errb) == 0 && !strcmp(b.name, "Renamed") && b.n == 1, "GPR_Open");
    expect(GPR_Remove(&b, 0) == 0 && b.n == 0, "GPR_Remove");

    {
        char pro[512];
        snprintf(pro, sizeof pro, "%s/board.kicad_pro", dir);
        write_file(pro, "x\n");
        expect(GPR_KicadFiles(pro, pcb, sch, (int)sizeof pcb) == 0 && pcb[0] == 0 && sch[0] == 0,
               "GPR_KicadFiles with no siblings");
        write_file(strcat(strcpy(stl, dir), "/board.kicad_pcb"), "y\n");
        expect(GPR_KicadFiles(pro, pcb, sch, (int)sizeof pcb) == 1 && strstr(pcb, "board.kicad_pcb") != NULL,
               "GPR_KicadFiles finds a pcb by name");
    }

    GPR_RecentInit(&rec);
    gpr_recent_init(&recb);
    expect(rec.n == 0 && recb.size == rec.size, "GPR_RecentInit");
    expect(GPR_RecentTouch(&rec, dir) == 0 && rec.n == 1, "GPR_RecentTouch");
    snprintf(stl, sizeof stl, "%s/recent.om", dir);
    expect(GPR_RecentSave(&rec, stl, err, (int)sizeof err) == 0, "GPR_RecentSave");
    expect(GPR_RecentLoad(&recb, stl, errb, (int)sizeof errb) == 0 && recb.n == 1 && !strcmp(recb.path[0], dir),
           "GPR_RecentLoad");

    GPR_Free(&a);
    GPR_Free(&b);
    expect(calls >= 24, "every call went through the table");
    if (fails) printf("%d failed\n", fails);
    else printf("ok\n");
    return fails ? 1 : 0;
}
