/* Host checks for a GFOpenMakerWorkshop project (gs_project.h): the
 * manifest, create / open / save, add / remove / rename, unknown kinds,
 * recent projects, and the app for an item.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
#include <dos/dos.h>
#include <proto/dos.h>
#else
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#endif

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static int write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb");
    size_t n = strlen(text);
    int ok = f && fwrite(text, 1, n, f) == n;
    if (f && fclose(f)) ok = 0;
    return ok ? 0 : -1;
}

static int mkdir_p(const char *path)
{
#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
    BPTR l = Lock((STRPTR)path, ACCESS_READ);
    if (l) { UnLock(l); return 0; }
    l = CreateDir((STRPTR)path);
    if (!l) return -1;
    UnLock(l);
    return 0;
#else
    return mkdir(path, 0777) == 0 || errno == EEXIST ? 0 : -1;
#endif
}

static int readable(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static void test_dir(char *dir, int n)
{
#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
    snprintf(dir, (size_t)n, "RAM:gpr-test");
#else
    snprintf(dir, (size_t)n, "/tmp/gpr-test-%d", (int)getpid());
#endif
}

int main(void)
{
    gpr_project p, q;
    gpr_add_opts o;
    gpr_recent rec;
    char err[192], buf[4096], dir[128], path[512], pcb[512], sch[512], resolved[512];
    int n, i;
    const char *sample =
        "# a widget\n"
        "format = 1\n"
        "name = Widget\n"
        "notes = A small desktop widget\n"
        "later-key = kept\n"
        "\n"
        "item.1.kind = cad\n"
        "item.1.name = Body\n"
        "item.1.file = cad/body.stl\n"
        "item.3.kind = render\n"
        "item.3.name = Shot\n"
        "item.3.file = render/shot.exr\n"
        "item.3.engine = later\n"
        "item.3.from = 1\n"
        "item.2.kind = print\n"
        "item.2.name = Body for the printer\n"
        "item.2.file = print/body.gcode\n"
        "item.2.from = 1\n"
        "item.2.settings = settings/standard\n";

    err[0] = 0;
    gpr_init(&p);
    expect(p.size == (int)sizeof p && p.n == 0 && p.dirty == 0, "gpr_init");
    expect(gpr_parse(&p, "", err, (int)sizeof err) == 0 && p.n == 0, "empty text is an empty project");
    expect(gpr_parse(&p, "name Widget\n", err, (int)sizeof err) == -1 && strstr(err, "key = value") != NULL,
           "a line without = says why");
    expect(gpr_parse(&p, "item.0.kind = cad\nitem.0.file = a.stl\n", err, (int)sizeof err) == -1,
           "item.0 is refused");

    expect(gpr_parse(&p, sample, err, (int)sizeof err) == 0, "a sample project parses");
    expect(!strcmp(p.name, "Widget") && !strcmp(p.notes, "A small desktop widget"), "name and notes");
    expect(p.n == 3 && p.nextra == 1 && !strcmp(p.extra[0].key, "later-key") &&
               !strcmp(p.extra[0].value, "kept"),
           "an unknown project key is kept");
    expect(!strcmp(p.items[0].kind, "cad") && !strcmp(p.items[0].name, "Body") &&
               !strcmp(p.items[0].file, "cad/body.stl"),
           "item 1 is the CAD body, in number order");
    expect(!strcmp(p.items[1].kind, "print") && p.items[1].from == 1 &&
               !strcmp(p.items[1].settings, "settings/standard"),
           "item 2 is the print job, from the body");
    expect(!strcmp(p.items[2].kind, "render") && !strcmp(p.items[2].name, "Shot") &&
               p.items[2].from == 1 && p.items[2].nextra == 1 && !strcmp(p.items[2].extra[0].key, "engine") &&
               !strcmp(p.items[2].extra[0].value, "later"),
           "an unknown Render kind and its extra key are kept, and from=3 remaps");
    expect(!gpr_kind_known("render") && !strcmp(gpr_kind_group("render"), "render") &&
               gpr_kind_app("render") == NULL && gpr_kind_port("render") == NULL,
           "Render is not a built kind, but the group is the token");

    n = gpr_format(&p, buf, (int)sizeof buf);
    expect(n > 0 && strstr(buf, "format = 1") && strstr(buf, "later-key = kept") &&
               strstr(buf, "item.3.kind = render") && strstr(buf, "item.3.engine = later") &&
               strstr(buf, "item.3.from = 1") && strstr(buf, "item.2.settings = settings/standard"),
           "format rewrites 1..N and keeps unknown keys");
    /* Gaps are gone: the render item is now item.3, still kind render. */
    expect(strstr(buf, "item.3.kind = render") != NULL && strstr(buf, "item.4.") == NULL,
           "numbers are compact on save");
    gpr_init(&q);
    expect(gpr_parse(&q, buf, err, (int)sizeof err) == 0 && q.n == 3 && !strcmp(q.items[2].kind, "render") &&
               q.items[2].from == 1 && q.nextra == 1 && q.items[2].nextra == 1,
           "a written manifest round-trips, including an unknown kind");
    gpr_free(&q);

    expect(gpr_kind_known("CAD") && gpr_kind_known("electronics") && !gpr_kind_known(""),
           "known kinds, case does not matter");
    expect(!strcmp(gpr_kind_group("cad"), "CAD") && !strcmp(gpr_kind_group("print"), "Print") &&
               !strcmp(gpr_kind_group("laser"), "Laser") && !strcmp(gpr_kind_group("cnc"), "CNC") &&
               !strcmp(gpr_kind_group("electronics"), "Electronics"),
           "group titles");
    expect(!strcmp(gpr_kind_app("cad"), "GFOpenMake") && !strcmp(gpr_kind_port("cad"), "GFOPENMAKE") &&
               !strcmp(gpr_kind_app("print"), "GFOpenSlicer") &&
               !strcmp(gpr_kind_port("print"), "OPENMAKESLICER") && !strcmp(gpr_kind_app("laser"), "GFOpenBurn") &&
               !strcmp(gpr_kind_port("laser"), "OPENMAKEBURN") && !strcmp(gpr_kind_app("electronics"), "gfamigakicad") &&
               gpr_kind_port("electronics") == NULL && !strcmp(gpr_kind_app("cnc"), "GFOpenCNC") &&
               !strcmp(gpr_kind_port("cnc"), "GFOPENCNC"),
           "the app and port for each known kind");
    expect(!strcmp(gpr_kind_suggest("body.STL"), "cad") && !strcmp(gpr_kind_suggest("a.svg"), "laser") &&
               !strcmp(gpr_kind_suggest("a.dxf"), "laser") && !strcmp(gpr_kind_suggest("a.pgm"), "laser") &&
               !strcmp(gpr_kind_suggest("board.kicad_pro"), "electronics") && gpr_kind_suggest("job.gcode") == NULL &&
               gpr_kind_suggest("notes.txt") == NULL,
           "a suffix may suggest a kind; .gcode is not guessed");

    test_dir(dir, (int)sizeof dir);
    expect(mkdir_p(dir) == 0, "a temp drawer");
    /* Already-a-project is tested after the first create. */
    gpr_free(&p);
    expect(gpr_create(&p, dir, "Widget", err, (int)sizeof err) == 0 && !strcmp(p.name, "Widget") && p.n == 0 &&
               !p.dirty,
           "create a project");
    snprintf(path, sizeof path, "%s/project.om", dir);
    expect(gpr_create(&q, dir, "Other", err, (int)sizeof err) == -1 && strstr(err, "already") != NULL,
           "create refuses a drawer that already has a project");
    gpr_free(&q);

    snprintf(path, sizeof path, "%s/body.stl", dir);
    expect(write_file(path, "solid test\nendsolid test\n") == 0, "a dummy STL beside the project");
    gpr_add_defaults(&o);
    o.kind = "cad";
    o.file = path;
    o.name = "Body";
    i = gpr_add(&p, &o, err, (int)sizeof err);
    expect(i == 0 && p.n == 1 && p.dirty && !strcmp(p.items[0].kind, "cad") && !strcmp(p.items[0].name, "Body") &&
               !strcmp(p.items[0].file, "cad/body.stl"),
           "adding an STL copies it into cad/ and stores a relative path");
    snprintf(path, sizeof path, "%s/cad/body.stl", dir);
    expect(readable(path), "the copy is there");
    expect(gpr_resolve(&p, 0, resolved, (int)sizeof resolved) == 0 && !strcmp(resolved, path),
           "resolve is the copy in the drawer");

    /* A file already in the drawer is referenced, not copied again. */
    gpr_add_defaults(&o);
    o.kind = "cad";
    o.file = "cad/body.stl";
    o.name = "Body copy";
    i = gpr_add(&p, &o, err, (int)sizeof err);
    expect(i == 1 && !strcmp(p.items[1].file, "cad/body.stl"), "an in-drawer file stays relative");

    snprintf(path, sizeof path, "%s/outside.gcode", dir);
    expect(write_file(path, "; gcode\nG28\n") == 0, "a dummy G-code");
    gpr_add_defaults(&o);
    o.kind = "print";
    o.file = path;
    o.from = 1;
    o.settings = "settings/standard";
    i = gpr_add(&p, &o, err, (int)sizeof err);
    expect(i == 2 && !strcmp(p.items[2].kind, "print") && p.items[2].from == 1 &&
               !strcmp(p.items[2].file, "print/outside.gcode") &&
               !strcmp(p.items[2].settings, "settings/standard"),
           "a print item is copied and can point at a CAD item");

    /* KiCad: referenced in place, siblings listed by name. */
    snprintf(path, sizeof path, "%s-kicad", dir);
    expect(mkdir_p(path) == 0, "a KiCad drawer outside the project");
    {
        char pro[1024], pcbpath[1024], schpath[1024];
        snprintf(pro, sizeof pro, "%s/controller.kicad_pro", path);
        snprintf(pcbpath, sizeof pcbpath, "%s/controller.kicad_pcb", path);
        snprintf(schpath, sizeof schpath, "%s/controller.kicad_sch", path);
        expect(write_file(pro, "(kicad_pro)\n") == 0 && write_file(pcbpath, "(kicad_pcb)\n") == 0 &&
                   write_file(schpath, "(kicad_sch)\n") == 0,
               "a dummy KiCad project, not parsed");
        expect(gpr_kicad_files(pro, pcb, sch, (int)sizeof pcb) == 2 && !strcmp(pcb, pcbpath) &&
                   !strcmp(sch, schpath),
               "siblings are found by name, without reading the files");
        gpr_add_defaults(&o);
        o.kind = "electronics";
        o.file = pro;
        o.name = "Controller";
        i = gpr_add(&p, &o, err, (int)sizeof err);
        expect(i == 3 && !strcmp(p.items[3].kind, "electronics") && !strcmp(p.items[3].file, pro),
               "a KiCad project is referenced in place, not copied");
        snprintf(path, sizeof path, "%s/electronics/controller.kicad_pro", dir);
        expect(!readable(path), "no copy was made under electronics/");
    }

    expect(gpr_find(&p, "1") == 0 && gpr_find(&p, "Body") == 0 && gpr_find(&p, "Controller") == 3 &&
               gpr_find(&p, "no-such") == -1 && gpr_find(&p, "99") == -1,
           "find by number or name");
    expect(gpr_rename(&p, 2, "Body slice") == 0 && !strcmp(p.items[2].name, "Body slice"), "rename");
    expect(gpr_set_notes(&p, "A small desktop widget") == 0, "set notes");
    expect(gpr_save(&p, err, (int)sizeof err) == 0 && !p.dirty, "save");

    gpr_init(&q);
    snprintf(path, sizeof path, "%s/project.om", dir);
    expect(gpr_open(&q, path, err, (int)sizeof err) == 0, "open by project.om");
    expect(!strcmp(q.name, "Widget") && !strcmp(q.notes, "A small desktop widget") && q.n == 4 &&
               !strcmp(q.items[3].kind, "electronics") && !q.dirty,
           "open restores the saved project");
    gpr_free(&q);
    gpr_init(&q);
    expect(gpr_open(&q, dir, err, (int)sizeof err) == 0 && q.n == 4, "open by the drawer");

    expect(gpr_remove(&q, 1) == 0 && q.n == 3 && !strcmp(q.items[1].name, "Body slice") &&
               q.items[1].from == 1,
           "remove compact the list and remaps from=");
    /* items were Body, Body copy, Body slice (from 1), Controller. After
     * removing Body copy, Body slice is index 1 and still from 1 (Body). */
    expect(gpr_remove(&q, 0) == 0 && q.items[0].from == 0,
           "removing the target of from= clears it");
    gpr_free(&q);

    /* A short add-opts still works (copy defaults, no name). */
    {
        struct {
            int size;
            const char *kind;
            const char *file;
        } short_opts;
        char src[512];
        gpr_free(&p);
        expect(gpr_open(&p, dir, err, (int)sizeof err) == 0, "reopen for a short opts add");
        snprintf(src, sizeof src, "%s/cad/body.stl", dir);
        memset(&short_opts, 0, sizeof short_opts);
        short_opts.size = (int)sizeof short_opts;
        short_opts.kind = "cad";
        short_opts.file = src;
        i = gpr_add(&p, (const gpr_add_opts *)&short_opts, err, (int)sizeof err);
        expect(i >= 0 && !strcmp(p.items[i].name, "body") && !strcmp(p.items[i].file, "cad/body.stl"),
               "a shorter add-opts uses the defaults and the file's stem");
    }

    expect(gpr_add(&p, NULL, err, (int)sizeof err) == -1 && err[0], "add with no options says why");
    gpr_add_defaults(&o);
    o.kind = "cad";
    o.file = "/tmp/gpr-no-such-file.stl";
    expect(gpr_add(&p, &o, err, (int)sizeof err) == -1 && err[0], "add of a missing file says why");

    gpr_recent_init(&rec);
    expect(rec.size == (int)sizeof rec && rec.n == 0, "gpr_recent_init");
    expect(gpr_recent_touch(&rec, dir) == 0 && rec.n == 1 && !strcmp(rec.path[0], dir), "touch the first path");
    expect(gpr_recent_touch(&rec, "/tmp/other-project") == 0 && rec.n == 2 &&
               !strcmp(rec.path[0], "/tmp/other-project") && !strcmp(rec.path[1], dir),
           "a new path goes to the front");
    expect(gpr_recent_touch(&rec, dir) == 0 && rec.n == 2 && !strcmp(rec.path[0], dir) &&
               !strcmp(rec.path[1], "/tmp/other-project"),
           "touching again moves it to the front, unique");
    {
        int k;
        char many[64];
        for (k = 0; k < 10; k++) {
            snprintf(many, sizeof many, "/tmp/p%d", k);
            gpr_recent_touch(&rec, many);
        }
        expect(rec.n == GPR_RECENT_MAX && !strcmp(rec.path[0], "/tmp/p9"), "the recent list is capped");
    }
    snprintf(path, sizeof path, "%s/recent.om", dir);
    expect(gpr_recent_save(&rec, path, err, (int)sizeof err) == 0, "save recent");
    {
        gpr_recent rec2;
        gpr_recent_init(&rec2);
        expect(gpr_recent_load(&rec2, path, err, (int)sizeof err) == 0 && rec2.n == rec.n &&
                   !strcmp(rec2.path[0], rec.path[0]) && !strcmp(rec2.path[rec.n - 1], rec.path[rec.n - 1]),
               "recent load/save round-trips");
        snprintf(path, sizeof path, "%s/no-such-recent.om", dir);
        expect(gpr_recent_load(&rec2, path, err, (int)sizeof err) == 0 && rec2.n == 0,
               "a missing recent file is an empty list");
    }

    expect(gpr_open(&q, "gpr-no-such-project-xyz", err, (int)sizeof err) == -1 && strstr(err, "cannot open") != NULL,
           "open of a missing project says why");

    gpr_free(&p);
    expect(p.n == 0 && p.items == NULL && p.size == (int)sizeof p, "gpr_free");
    if (fails) printf("%d failed\n", fails);
    else printf("ok\n");
    return fails ? 1 : 0;
}
