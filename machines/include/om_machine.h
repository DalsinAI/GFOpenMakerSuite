/* OpenMakeMachine: what a machine can do, read from its driver and
 * profile files, so an app can grey out what the machine lacks, clamp
 * what it asks for, and warn before it writes a job that won't fit.
 * The Amiga build is to be openmakemachine.library; this is its C API.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OM_MACHINE_H
#define OM_MACHINE_H

enum { OM_PRINTER = 1, OM_LASER, OM_CNC };

/* How a job reaches the machine (a mask). */
#define OM_SEND_FILE 1u
#define OM_SEND_SERIAL 2u
#define OM_SEND_NETWORK 4u

/* Features (a mask). */
#define OM_HEATED_BED 1u
#define OM_BED_LEVELLING 2u   /* automatic bed levelling */
#define OM_ROTARY 4u
#define OM_AIR_ASSIST 8u
#define OM_TOOL_CHANGER 16u
#define OM_SPINDLE 32u        /* spindle on, off and speed */
#define OM_HOMING 64u
#define OM_PROBING 128u
#define OM_LASER_POWER 256u   /* a laser whose power is set per move */

enum { OM_X, OM_Y, OM_Z, OM_E, OM_AXES };

#define OM_CODE 1024

typedef struct {
    char name[64];
    char dialect[24];          /* marlin, klipper, grbl ... */
    int family;                /* OM_PRINTER ... */
    unsigned features, send;
    float area[3];             /* the work area, mm, from 0 */
    float speed[OM_AXES];      /* most mm/s on each axis */
    float accel[OM_AXES];      /* most mm/s^2 */
    float hotend, bed;         /* most degrees C; 0: none */
    float laser;               /* most laser power, %; 0: none */
    float rpm;                 /* most spindle speed; 0: none */
    int extruders;
    char start[OM_CODE], end[OM_CODE];   /* G-code; {hotend} and {bed} are filled in */
} om_machine;

/* Reads "key = value" lines over m; "key += value" adds a line to start
 * or end, or a word to features or send. '#' starts a comment. A driver
 * is read first, then a profile over it. 0, or -1 with why in err. */
int om_parse(om_machine *m, const char *text, char *err, int errlen);
int om_load(om_machine *m, const char *path, char *err, int errlen);
void om_clear(om_machine *m);

int om_has(const om_machine *m, unsigned feature);

/* A job, as far as the machine is concerned. A 0 asks for nothing. */
typedef struct {
    float min[3], max[3];      /* where the moves go, mm */
    float speed[OM_AXES];      /* the fastest move on each axis, mm/s */
    float hotend, bed, laser, rpm;
    int extruders;
    unsigned needs;            /* features the job uses */
} om_job;

enum {
    OM_OK = 0,
    OM_TOO_BIG = 1,            /* outside the work area */
    OM_TOO_FAST = 2,
    OM_TOO_HOT = 4,
    OM_TOO_MUCH = 8,           /* laser power or spindle speed */
    OM_MISSING = 16            /* a feature or an extruder the machine lacks */
};

/* What is wrong with job on m: a mask of the above, and the first reason
 * in words in why. */
int om_check(const om_machine *m, const om_job *job, char *why, int whylen);

/* v held to the axis's most; *held set when it was. */
float om_clamp_speed(const om_machine *m, int axis, float v, int *held);

/* start or end with {hotend} and {bed} filled in. 0, or -1 if it does not fit. */
int om_code(const char *tmpl, float hotend, float bed, char *out, int outlen);

#endif
