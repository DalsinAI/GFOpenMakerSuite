/* OpenMakeWorkshop, from the shell: a project drawer and its project.om.
 *   openmake-workshop new <drawer> [name=]
 *   openmake-workshop list <drawer>
 *   openmake-workshop add <drawer> kind= file= [name=] [from=] [settings=]
 * new makes the drawer, the kind subdrawers and a project.om. list prints
 * the name, the notes and the items. add copies or references the file
 * and writes the manifest. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
#include <dos/dos.h>
#include <proto/dos.h>
#else
#include <unistd.h>
#endif

/* file= is a path the user names (usually from the current directory).
 * A relative path that can be opened here is made absolute so the
 * library does not treat it as inside the project drawer. */
static int file_abs(char *out, int n, const char *file)
{
    if (!file || !*file || !out || n <= 0) return -1;
    if (file[0] == '/') {
        if ((int)strlen(file) >= n) return -1;
        memcpy(out, file, strlen(file) + 1);
        return 0;
    }
    if (strchr(file, ':') && (strchr(file, '/') == NULL || strchr(file, ':') < strchr(file, '/'))) {
        if ((int)strlen(file) >= n) return -1;
        memcpy(out, file, strlen(file) + 1);
        return 0;
    }
    {
        FILE *f = fopen(file, "rb");
        if (!f) {
            if ((int)strlen(file) >= n) return -1;
            memcpy(out, file, strlen(file) + 1);
            return 0;
        }
        fclose(f);
    }
#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
    {
        BPTR l = Lock((STRPTR)file, ACCESS_READ);
        if (!l || !NameFromLock(l, (STRPTR)out, n)) {
            if (l) UnLock(l);
            return -1;
        }
        UnLock(l);
        return 0;
    }
#else
    {
        char cwd[GPR_PATH_MAX + 1];
        int wr;
        if (!getcwd(cwd, sizeof cwd)) return -1;
        wr = snprintf(out, (size_t)n, "%s/%s", cwd, file);
        return wr < 0 || wr >= n ? -1 : 0;
    }
#endif
}

static void usage(void)
{
    fprintf(stderr,
            "usage: openmake-workshop new <drawer> [name=]\n"
            "       openmake-workshop list <drawer>\n"
            "       openmake-workshop add <drawer> kind= file= [name=] [from=] [settings=]\n"
            "new makes the drawer and a project.om (name= is the product; otherwise\n"
            "the drawer's name is used). list prints the name, the notes and the items.\n"
            "add copies a suite file into the project, or references a KiCad project\n"
            "in place. kind is cad, print, laser, cnc or electronics, or another token.\n"
            "from= is the 1-based number of an item this one was made from.\n");
}

static const char *arg(int argc, char **argv, int from, const char *name)
{
    size_t n = strlen(name);
    int i;
    for (i = from; i < argc; i++)
        if (strncmp(argv[i], name, n) == 0 && argv[i][n] == '=') return argv[i] + n + 1;
    return NULL;
}

static int known_key(const char *cmd, const char *s)
{
    static const char *const newk[] = {"name", "NAME", NULL};
    static const char *const addk[] = {"kind", "KIND", "file", "FILE", "name", "NAME",
                                       "from", "FROM", "settings", "SETTINGS", NULL};
    const char *const *keys = !strcmp(cmd, "new") ? newk : !strcmp(cmd, "add") ? addk : NULL;
    const char *eq = strchr(s, '=');
    size_t n;
    int i;
    if (!eq) return 0;
    if (!keys) return 0;
    n = (size_t)(eq - s);
    for (i = 0; keys[i]; i++)
        if (strlen(keys[i]) == n && strncmp(s, keys[i], n) == 0) return 1;
    return 0;
}

static int check_keys(const char *cmd, int argc, char **argv, int from)
{
    int i;
    for (i = from; i < argc; i++) {
        if (!strchr(argv[i], '=')) {
            fprintf(stderr, "openmake-workshop: %s: not key=value\n", argv[i]);
            return -1;
        }
        if (!known_key(cmd, argv[i])) {
            fprintf(stderr, "openmake-workshop: %s: no such setting\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

static int cmd_new(int argc, char **argv)
{
    gpr_project p;
    char err[192];
    const char *drawer, *name;
    if (argc < 3) {
        usage();
        return 1;
    }
    drawer = argv[2];
    if (check_keys("new", argc, argv, 3)) return 1;
    name = arg(argc, argv, 3, "name");
    if (!name) name = arg(argc, argv, 3, "NAME");
    gpr_init(&p);
    if (gpr_create(&p, drawer, name && *name ? name : NULL, err, (int)sizeof err)) {
        fprintf(stderr, "openmake-workshop: %s\n", err);
        gpr_free(&p);
        return 1;
    }
    gpr_free(&p);
    return 0;
}

static int cmd_list(int argc, char **argv)
{
    gpr_project p;
    char err[192];
    int i;
    if (argc < 3) {
        usage();
        return 1;
    }
    if (check_keys("list", argc, argv, 3)) return 1;
    gpr_init(&p);
    if (gpr_open(&p, argv[2], err, (int)sizeof err)) {
        fprintf(stderr, "openmake-workshop: %s\n", err);
        gpr_free(&p);
        return 1;
    }
    printf("%s\n", p.name);
    if (p.notes[0]) printf("%s\n", p.notes);
    for (i = 0; i < p.n; i++) {
        const gpr_item *it = &p.items[i];
        printf("%d %s %s %s", i + 1, it->kind, it->name, it->file);
        if (it->from > 0) printf(" from=%d", it->from);
        if (it->settings[0]) printf(" settings=%s", it->settings);
        printf("\n");
    }
    gpr_free(&p);
    return 0;
}

static int cmd_add(int argc, char **argv)
{
    gpr_project p;
    gpr_add_opts o;
    char err[192];
    const char *from;
    int i;
    if (argc < 3) {
        usage();
        return 1;
    }
    if (check_keys("add", argc, argv, 3)) return 1;
    gpr_add_defaults(&o);
    o.kind = arg(argc, argv, 3, "kind");
    if (!o.kind) o.kind = arg(argc, argv, 3, "KIND");
    o.file = arg(argc, argv, 3, "file");
    if (!o.file) o.file = arg(argc, argv, 3, "FILE");
    if (o.file) {
        static char file[GPR_PATH_MAX + 1];
        if (file_abs(file, (int)sizeof file, o.file)) {
            fprintf(stderr, "openmake-workshop: %s: path is too long\n", o.file);
            return 1;
        }
        o.file = file;
    }
    o.name = arg(argc, argv, 3, "name");
    if (!o.name) o.name = arg(argc, argv, 3, "NAME");
    o.settings = arg(argc, argv, 3, "settings");
    if (!o.settings) o.settings = arg(argc, argv, 3, "SETTINGS");
    from = arg(argc, argv, 3, "from");
    if (!from) from = arg(argc, argv, 3, "FROM");
    if (from) {
        char *end = NULL;
        long n = strtol(from, &end, 10);
        if (!from[0] || (end && *end) || n < 1 || n > 100000) {
            fprintf(stderr, "openmake-workshop: from=%s: not a 1-based item number\n", from);
            return 1;
        }
        o.from = (int)n;
    }
    gpr_init(&p);
    if (gpr_open(&p, argv[2], err, (int)sizeof err)) {
        fprintf(stderr, "openmake-workshop: %s\n", err);
        gpr_free(&p);
        return 1;
    }
    i = gpr_add(&p, &o, err, (int)sizeof err);
    if (i < 0) {
        fprintf(stderr, "openmake-workshop: %s\n", err);
        gpr_free(&p);
        return 1;
    }
    if (gpr_save(&p, err, (int)sizeof err)) {
        fprintf(stderr, "openmake-workshop: %s\n", err);
        gpr_free(&p);
        return 1;
    }
    gpr_free(&p);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return 1;
    }
    if (!strcmp(argv[1], "new")) return cmd_new(argc, argv);
    if (!strcmp(argv[1], "list")) return cmd_list(argc, argv);
    if (!strcmp(argv[1], "add")) return cmd_add(argc, argv);
    fprintf(stderr, "openmake-workshop: %s: not new, list or add\n", argv[1]);
    usage();
    return 1;
}
