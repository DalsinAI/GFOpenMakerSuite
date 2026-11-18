/* The shared machine picker (core/ui/omview_machines.h) on the host: the
 * test machines under core/tests/data/machines, and the suite's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "omview_machines.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static int measure(void *userdata, const char *utf8, size_t length, const ol_style *style, ol_text_metrics *metrics)
{
    (void)userdata;
    (void)utf8;
    metrics->width = (ol_unit)(length * (size_t)style->font_size * 60 / 100);
    metrics->ascent = style->font_size * 80 / 100;
    metrics->descent = style->font_size * 22 / 100;
    return 1;
}

int main(void)
{
    static const char markup[] = "<select id=\"machine\"></select><div id=\"status\">ready</div>";
    omv_machines l, none, own;
    om_machine m;
    char err[160];
    olui *ui = olui_new();
    olui_el *sel;
    memset(&l, 0, sizeof l);
    memset(&none, 0, sizeof none);
    memset(&own, 0, sizeof own);

    /* Printers: Marlin's three .om (not notes.txt), Klipper's that loads,
     * nothing from a directory with no driver.om; the suite's generic
     * starting points first, then by name. */
    expect(omv_machines_scan(&l, "core/tests/data/machines", "printer") == 4, "four printers");
    expect(l.n == 4 && !strcmp(l.at[0].name, "Generic Marlin 220") && !strcmp(l.at[1].name, "anon-180") &&
               !strcmp(l.at[2].name, "Big Marlin 300") && !strcmp(l.at[3].name, "Fast Klipper 250"),
           "the generic one first, then by name; a profile with none named by its file");
    expect(l.skipped == 1 && strstr(l.why, "broken.om") && strstr(l.why, "area"), "the broken one skipped, and why");
    expect(omv_machines_load(&l, 3, &m, err, (int)sizeof err) == 0 && !strcmp(m.name, "Fast Klipper 250") &&
               !strcmp(m.dialect, "klipper") && m.area[0] == 250.f && m.family == OM_PRINTER,
           "the Klipper printer loaded over its driver");
    expect(omv_machines_load(&l, 4, &m, err, (int)sizeof err) == -1 && err[0], "no machine 4");

    expect(omv_machines_scan(&none, "core/tests/data/machines", "laser") == 1 &&
               !strcmp(none.at[0].name, "Diode 400"), "one laser, not among the printers");
    omv_machines_free(&none);
    expect(omv_machines_scan(&none, "core/tests/data/machines", "cnc") == 0 && none.n == 0, "no CNC machines");
    expect(omv_machines_scan(&none, "core/tests/data/nowhere", "printer") == 0, "no machines directory: none");
    expect(omv_machines_scan(&own, "machines", "printer") >= 5 && !strcmp(own.at[0].name, "Generic Marlin 220") &&
               !strcmp(own.at[1].name, "Generic RepRapFirmware 300"),
           "the suite's own printers: the generic Marlin first, so the windows start on it");
    {
        int n = own.n;
        expect(omv_machines_scan(&own, "machines/", "printer") == n && own.n == n, "scanned again: the same, not twice");
    }

    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, markup, "", err, sizeof err), "the window loads");
    sel = olui_find(ui, "machine");
    expect(omv_machines_fill(ui, "machine", &l, "No printers") == 4 && olui_select_count(sel) == 4 &&
               !strcmp(olui_select_option_text(sel, 2), "Big Marlin 300") && olui_select_index(sel) == 0,
           "the select filled, the first chosen");
    olui_select_set(ui, sel, 3);
    expect(omv_machines_chosen(ui, "machine", &l) == 3, "and the one chosen");
    expect(omv_machines_show(ui, "machine", &l, "Big Marlin 300") == 1 && omv_machines_chosen(ui, "machine", &l) == 2,
           "a named machine is selected");
    expect(omv_machines_show(ui, "machine", &l, "Big Marlin 300") == 0 && omv_machines_chosen(ui, "machine", &l) == 2,
           "the same name again does not move the list");
    expect(omv_machines_show(ui, "machine", &l, "No such printer") == 0 && omv_machines_chosen(ui, "machine", &l) == 2,
           "an unknown name leaves the list");
    expect(omv_machines_fill(ui, "machine", &none, "No printers") == 0 && olui_select_count(sel) == 1 &&
               !strcmp(olui_select_option_text(sel, 0), "No printers") && omv_machines_chosen(ui, "machine", &none) == -1,
           "none: one option saying so, nothing chosen");
    expect(omv_machines_chosen(ui, "nothing", &l) == -1, "no such select");

    omv_machines_free(&l);
    omv_machines_free(&none);
    omv_machines_free(&own);
    expect(l.n == 0 && !l.at, "freed");
    olui_free(ui);
    printf(fails ? "machine picker: %d failed\n" : "machine picker ok\n", fails);
    return fails != 0;
}
