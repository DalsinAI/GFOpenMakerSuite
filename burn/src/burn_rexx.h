/* OpenMakeBurn's ARexx commands, as docs/arexx.md lists them. The window
 * opens the port OPENMAKEBURN.1 and dispatches through this table. Host
 * tests call the same verbs with omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef BURN_REXX_H
#define BURN_REXX_H

#include "burn_image.h"
#include "burn_job.h"
#include "gs_rexx.h"

typedef struct gb_rexx {
    int size;
    gv_doc *doc;
    burn_image *pic;
    int *have_doc;
    int *have_image;
    gt_path *path;
    int *have_path;
    burn_opts *opts;
    burn_layer *store; /* layer overrides; opts->layer points here */
    int store_cap;
    om_machine machine;
    int have_machine;
    char file[256];
    char save_file[256];
    char start[240]; /* the user's own lines; opts->start points here when set */
    char end[240];
    int invert;   /* 1: white is more power on a picture */
    float width;  /* picture width mm; 0: the gap times the columns */
    int have_travel;
    float travel; /* mm/s; kept because the window has no travel field */
    int have_s100;
    float s100; /* S at 100%; kept because the window has no such field */
    int seq[64]; /* 0-based document layers; the list order */
    int nseq;
    int have_order; /* 1: seq is the list order the port keeps */
    int dirty;
    const char *version;
    const char *port_name; /* "OPENMAKEBURN.1", or NULL */
    int quit;
    void (*activate)(void *win);
    void (*hide)(void *win);
    void *win;
    void *ui;
} gb_rexx;

#define GB_REXX_PORT "OPENMAKEBURN"

void gb_rexx_init(gb_rexx *g);
/* Puts the port's start, end, invert, width, travel, S-at-100% and layer
 * order back onto opts. The window calls this after it rebuilds opts from
 * the fields. When the window has a layer list, it copies that list into
 * seq first, so Up and Dn win over an older SET order. */
void gb_rexx_keep(gb_rexx *g);
const omr_verb *gb_rexx_verbs(void);
int gb_rexx_nverbs(void);

#endif
