/* G-code from a toolpath (gs_toolpath.h): for a printer, the machine's own
 * start and end code, a speed for each kind of move held to what the
 * machine can do, filament worked out from each line's width and height,
 * retraction over long travels, and how long and how much it will take.
 * The Amiga build is to be openmakegcode.library; this is its C API.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_GCODE_H
#define GS_GCODE_H

#include "gs_toolpath.h"
#include "om_machine.h"

typedef struct {
    float hotend, bed;          /* degrees, for the start code's {hotend} and {bed} */
    float filament;             /* mm across */
    float layer_h;              /* mm: each line's height */
    float flow;                 /* 1 is as worked out */
    float speed[GT_KINDS];      /* mm/s for each kind; GT_TRAVEL's for travels */
    float retract;              /* mm of filament pulled back over a travel; 0: none */
    float retract_speed;        /* mm/s */
    float retract_min;          /* travels shorter than this, mm, do not retract */
} gc_opts;

typedef struct {
    float seconds;              /* moving: with the machine's accel and jerk when its profile gives them, else distance over speed */
    float filament;             /* mm of filament */
    int moves;                  /* G0 and G1 lines */
} gc_stats;

void gc_defaults(gc_opts *o);

/* How long and how much, as gc_write would write it for m (m may be NULL:
 * the speeds are not held to a machine's). */
void gc_estimate(const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st);

/* How a time was got: */
#define GC_AT_SPEEDS 0          /* distance over speed: no acceleration given (or no machine) */
#define GC_WITH_ACCEL 1         /* with the machine's acceleration and jerk */
#define GC_FELL_BACK 2          /* distance over speed: no memory to plan with, so short */

/* gc_estimate, and how its time was got (GC_AT_SPEEDS and so on). */
int gc_estimate_how(const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st);

/* The job p is for om_check: where every move goes (travels too), the
 * fastest of each axis as gc_write would ask it of m (held to m's most,
 * so never too fast for m itself; m may be NULL), o's temperatures, one
 * extruder, and a heated bed when o heats one. */
void gc_job(const gt_path *p, const om_machine *m, const gc_opts *o, om_job *job);

/* p's G-code for the printer m to out, st (may be NULL) filled in. 0, or
 * -1 with why in err: m is not a printer, its code does not fit, or out
 * could not be written. */
int gc_write(FILE *out, const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st, char *err, int errlen);

/* Where gc_write_to puts the G-code: n bytes at s, ud as given. It returns
 * how many it took; fewer than n and the writing stops. */
typedef long (*gc_put)(const char *s, long n, void *ud);

/* gc_write to put instead of a file: the same text, the same answers. A
 * program uses this one through openmakegcode.library, whose files are
 * not the program's. */
int gc_write_to(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st, char *err,
                int errlen);

/* More than gc_opts says, for the _more calls: o itself, and the first
 * layer's speed. A program sets size to sizeof (gc_more) as it was
 * built: fields past it are as gc_more_defaults gives them. */
typedef struct {
    int size;
    const gc_opts *o;
    float first;                /* mm/s: no line on the first layer faster (travels are not held); 0: none */
    const char *start;          /* the user's own lines, after the machine's start; NULL: none */
    const char *end;            /* the user's own lines, before the machine's end; NULL: none */
} gc_more;

void gc_more_defaults(gc_more *x, const gc_opts *o);

/* gc_estimate_how and gc_write_to with x's options. */
int gc_estimate_more(const gt_path *p, const om_machine *m, const gc_more *x, gc_stats *st);
int gc_write_more(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_more *x, gc_stats *st, char *err,
                  int errlen);

/* Laser G-code for a Grbl machine in laser mode (public Grbl 1.1 laser
 * documentation): M4 dynamic power by default, or M3 to hold power
 * constant; S scaled to s100 (Grbl's $30, often 1000). A move's w is the
 * power, 0 to 100. Travels (GT_TRAVEL, and the first move, which only
 * places the head) are G0 with the laser off. A program sets size to
 * sizeof (gc_laser) as it was built. */
typedef struct {
    int size;
    int constant; /* 0: M4, not 0: M3 */
    float s100;   /* S at 100% power; 0: 1000 */
    float speed;  /* mm/s for a cut; 0: 20 */
    float travel; /* mm/s for a travel; 0: the machine's most X, or 150 */
    float z;      /* unused: each move has its own z */
} gc_laser;

void gc_laser_defaults(gc_laser *l);

/* More than gc_laser says: l itself, and a speed for each move. A
 * program sets size to sizeof (gc_laser_more) as it was built. */
typedef struct {
    int size;
    const gc_laser *l;
    const float *speed; /* p->n speeds, mm/s; 0 or NULL: l->speed */
} gc_laser_more;

void gc_laser_more_defaults(gc_laser_more *x, const gc_laser *l);

/* How long, as gc_laser_write_to would write it (m may be NULL: the
 * speeds are not held to a machine's). */
int gc_laser_estimate_how(const gt_path *p, const om_machine *m, const gc_laser *l, gc_stats *st);
int gc_laser_estimate_more(const gt_path *p, const om_machine *m, const gc_laser_more *x, gc_stats *st);

/* The job p is for om_check: where every move goes, the fastest of each
 * axis, and the most laser power the cuts ask for. */
void gc_laser_job(const gt_path *p, const om_machine *m, const gc_laser *l, om_job *job);
void gc_laser_job_more(const gt_path *p, const om_machine *m, const gc_laser_more *x, om_job *job);

/* p's Grbl laser G-code for m to put. 0, or -1 with why in err: m is not
 * a laser, its code does not fit, or put could not be written. */
int gc_laser_write_to(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_laser *l, gc_stats *st,
                      char *err, int errlen);
int gc_laser_write_more(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_laser_more *x,
                        gc_stats *st, char *err, int errlen);

#endif
