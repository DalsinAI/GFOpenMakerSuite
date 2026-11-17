/* Every printer profile under machines/printer, loaded over its driver:
 * the numbers its maker publishes, its dialect, and start and end G-code
 * that heat, home and let go. The Makefile slices a part with each.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "om_machine.h"

#include <dirent.h>
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

static const struct {
    const char *driver, *profile, *name;
    float x, y, z, hotend, bed;
    unsigned features, send;
} known[] = {
    {"marlin", "generic-220", "Generic Marlin 220", 220, 220, 250, 260, 100, OM_HEATED_BED | OM_HOMING,
     OM_SEND_FILE | OM_SEND_SERIAL},
    {"marlin", "creality-ender-3-v2", "Creality Ender-3 V2", 220, 220, 250, 255, 100, OM_HEATED_BED | OM_HOMING,
     OM_SEND_FILE | OM_SEND_SERIAL},
    {"marlin", "prusa-i3-mk3s-plus", "Original Prusa i3 MK3S+", 250, 210, 210, 300, 120,
     OM_HEATED_BED | OM_HOMING | OM_BED_LEVELLING | OM_PROBING, OM_SEND_FILE | OM_SEND_SERIAL},
    {"klipper", "voron-2.4-300", "Voron 2.4 300", 300, 300, 280, 300, 110,
     OM_HEATED_BED | OM_HOMING | OM_BED_LEVELLING | OM_PROBING, OM_SEND_FILE | OM_SEND_NETWORK},
    {"reprap", "generic-300", "Generic RepRapFirmware 300", 300, 300, 300, 280, 110, OM_HEATED_BED | OM_HOMING,
     OM_SEND_FILE | OM_SEND_NETWORK},
};
#define KNOWN ((int)(sizeof known / sizeof known[0]))

/* Each profile file in the tree is one of the known. */
static int count_files(void)
{
    static const char *const drivers[] = {"marlin", "klipper", "reprap"};
    int d, n = 0, k;
    char dir[128], what[160];
    for (d = 0; d < 3; d++) {
        DIR *h;
        struct dirent *e;
        snprintf(dir, sizeof dir, "machines/printer/%s/profiles", drivers[d]);
        h = opendir(dir);
        if (!h) continue;
        while ((e = readdir(h)) != NULL) {
            size_t len = strlen(e->d_name);
            int found = 0;
            if (len < 4 || strcmp(e->d_name + len - 3, ".om")) continue;
            n++;
            for (k = 0; k < KNOWN; k++)
                if (!strcmp(known[k].driver, drivers[d]) && !strncmp(known[k].profile, e->d_name, len - 3) &&
                    strlen(known[k].profile) == len - 3)
                    found = 1;
            snprintf(what, sizeof what, "%s/%.64s is tested here", dir, e->d_name);
            expect(found, what);
        }
        closedir(h);
    }
    return n;
}

int main(void)
{
    int k;
    for (k = 0; k < KNOWN; k++) {
        om_machine m;
        char path[160], err[160], code[OM_CODE], what[200];
        om_clear(&m);
        err[0] = 0;
        snprintf(path, sizeof path, "machines/printer/%s/driver.om", known[k].driver);
        expect(om_load(&m, path, err, (int)sizeof err) == 0, path);
        snprintf(path, sizeof path, "machines/printer/%s/profiles/%s.om", known[k].driver, known[k].profile);
        snprintf(what, sizeof what, "%s loads over its driver", path);
        expect(om_load(&m, path, err, (int)sizeof err) == 0, what);
        if (err[0]) printf("%s\n", err);
        snprintf(what, sizeof what, "%s: its name, a printer, the %s dialect", known[k].profile, known[k].driver);
        expect(!strcmp(m.name, known[k].name) && m.family == OM_PRINTER && !strcmp(m.dialect, known[k].driver), what);
        snprintf(what, sizeof what, "%s: %g by %g by %g mm", known[k].profile, (double)known[k].x, (double)known[k].y,
                 (double)known[k].z);
        expect(m.area[0] == known[k].x && m.area[1] == known[k].y && m.area[2] == known[k].z, what);
        snprintf(what, sizeof what, "%s: hotend %g, bed %g, one extruder", known[k].profile, (double)known[k].hotend,
                 (double)known[k].bed);
        expect(m.hotend == known[k].hotend && m.bed == known[k].bed && m.extruders == 1, what);
        snprintf(what, sizeof what, "%s: its features and how a job is sent", known[k].profile);
        expect(m.features == known[k].features && m.send == known[k].send, what);
        snprintf(what, sizeof what, "%s: print and travel speeds (50, 150) within its most", known[k].profile);
        expect((m.speed[OM_X] <= 0.f || m.speed[OM_X] >= 150.f) && (m.speed[OM_Y] <= 0.f || m.speed[OM_Y] >= 150.f),
               what);
        snprintf(what, sizeof what, "%s: the start heats the bed and hotend, homes, waits", known[k].profile);
        expect(om_code(m.start, 210.f, 60.f, code, (int)sizeof code) == 0 && strstr(code, "M140 S60") &&
                   strstr(code, "M104 S210") && strstr(code, "G28") && strstr(code, "M190 S60") &&
                   strstr(code, "M109 S210") && strstr(code, "M82") && strstr(code, "G92 E0") &&
                   strstr(code, "G28") < strstr(code, "M109 S210"),
               what);
        snprintf(what, sizeof what, "%s: the end cools and lets the motors go", known[k].profile);
        expect(om_code(m.end, 0.f, 0.f, code, (int)sizeof code) == 0 && strstr(code, "M104 S0") &&
                   strstr(code, "M140 S0") && strstr(code, "M84"),
               what);
    }
    expect(count_files() == KNOWN, "every printer profile in the tree is here");
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("printer profiles ok\n");
    return 0;
}
