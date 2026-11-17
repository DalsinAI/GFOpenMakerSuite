/* openmakerexx: the command parser, dispatch, HELP, and the host stub
 * for a port and a client (find, send, timeout).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_rexx.h"

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

static int verb_open(void *cmd, void *app)
{
    const char *file = omr_arg(cmd, "FILE");
    int *hits = app;
    if (hits) (*hits)++;
    omr_set_result(cmd, file ? file : "");
    return OMR_OK;
}

static int verb_quit(void *cmd, void *app)
{
    int *hits = app;
    if (hits) *hits += 10;
    return omr_switch(cmd, "FORCE") ? OMR_OK : OMR_ERROR;
}

static int verb_status(void *cmd, void *app)
{
    (void)app;
    omr_set_result(cmd, "TEST.1 file.stl 0");
    omr_set_stem(cmd, "ST", "PORT", "TEST.1");
    omr_set_stem(cmd, "ST", "FILE", "file.stl");
    omr_set_stem(cmd, "ST", "DIRTY", "0");
    omr_set_var(cmd, "MYVAR", "ok");
    return OMR_OK;
}

static int verb_add(void *cmd, void *app)
{
    char buf[80];
    (void)app;
    snprintf(buf, sizeof buf, "%s %s %s", omr_arg(cmd, "SHAPE") ? omr_arg(cmd, "SHAPE") : "",
             omr_arg(cmd, "X") ? omr_arg(cmd, "X") : "-", omr_arg(cmd, "W") ? omr_arg(cmd, "W") : "-");
    omr_set_result(cmd, buf);
    return OMR_OK;
}

static const omr_verb verbs[] = {
    {sizeof(omr_verb), "OPEN", "FILE/A", "open a file", verb_open},
    {sizeof(omr_verb), "QUIT", "FORCE/S", "close the window", verb_quit},
    {sizeof(omr_verb), "STATUS", "STEM/K VAR/K", "the port and the file", verb_status},
    {sizeof(omr_verb), "ADD", "SHAPE/A X/K Y/K Z/K W/K D/K H/K", "add a part", verb_add},
    {sizeof(omr_verb), "SELECT", "PART/N/A NAME/K", "pick a part", verb_open}, /* FILE unused */
};

static void io_init(omr_io *io, char *buf, int len, int timeout)
{
    memset(io, 0, sizeof *io);
    io->size = (int)sizeof *io;
    io->timeout = timeout;
    io->result = buf;
    io->resultlen = len;
}

static void test_parse(void)
{
    omr_io io;
    char buf[256];
    int hits = 0;

    io_init(&io, buf, (int)sizeof buf, 100);
    expect(omr_do_line("OPEN file.stl", verbs, 5, &hits, &io) == 0 && io.rc == OMR_OK &&
               strcmp(buf, "file.stl") == 0 && hits == 1,
           "OPEN file.stl, positional /A");
    expect(omr_do_line("open FILE=other.stl", verbs, 5, &hits, &io) == 0 && io.rc == OMR_OK &&
               strcmp(buf, "other.stl") == 0,
           "OPEN FILE=other.stl, case-insensitive verb");
    expect(omr_do_line("OPEN FILE/third.stl", verbs, 5, &hits, &io) == 0 && io.rc == OMR_OK &&
               strcmp(buf, "third.stl") == 0,
           "OPEN FILE/third.stl");
    expect(omr_do_line("OPEN FILE \"two words.stl\"", verbs, 5, &hits, &io) == 0 && io.rc == OMR_OK &&
               strcmp(buf, "two words.stl") == 0,
           "quoted argument");
    expect(omr_do_line("OPEN", verbs, 5, &hits, &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "required"),
           "OPEN without FILE is RC 10");
    expect(omr_do_line("OPEN a.stl leftover", verbs, 5, &hits, &io) == 0 && io.rc == OMR_ERROR &&
               strstr(buf, "unexpected"),
           "unexpected argument");
    expect(omr_do_line("NOPE", verbs, 5, &hits, &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "unknown command"),
           "unknown command");
    expect(omr_do_line("", verbs, 5, &hits, &io) == 0 && io.rc == OMR_ERROR,
           "empty command");
    expect(omr_do_line("QUIT", verbs, 5, &hits, &io) == 0 && io.rc == OMR_ERROR, "QUIT without FORCE");
    expect(omr_do_line("QUIT FORCE", verbs, 5, &hits, &io) == 0 && io.rc == OMR_OK, "QUIT FORCE");
    expect(omr_do_line("ADD box X 10 W 20", verbs, 5, &hits, &io) == 0 && io.rc == OMR_OK &&
               strcmp(buf, "box 10 20") == 0,
           "ADD box with keywords");
    expect(omr_do_line("ADD cylinder", verbs, 5, &hits, &io) == 0 && io.rc == OMR_OK &&
               strcmp(buf, "cylinder - -") == 0,
           "ADD with only the required shape");
    expect(omr_do_line("SELECT 3", verbs, 5, &hits, &io) == 0 && io.rc == OMR_OK, "SELECT 3, /N positional");
    expect(omr_do_line("SELECT PART/x", verbs, 5, &hits, &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "number"),
           "SELECT PART/x is not a number");
}

static void test_help(void)
{
    omr_io io;
    char buf[256];
    io_init(&io, buf, (int)sizeof buf, 100);
    expect(omr_do_line("HELP", verbs, 5, NULL, &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPEN") &&
               strstr(buf, "HELP") && strstr(buf, "ADD"),
           "HELP lists the commands");
    expect(omr_do_line("HELP COMMAND OPEN", verbs, 5, NULL, &io) == 0 && io.rc == OMR_OK && strstr(buf, "FILE/A") &&
               strstr(buf, "open a file"),
           "HELP COMMAND OPEN");
    expect(omr_do_line("HELP COMMAND no-such", verbs, 5, NULL, &io) == 0 && io.rc == OMR_ERROR,
           "HELP of an unknown command");
    expect(omr_do_line("VERSION", verbs, 5, NULL, &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "no version"),
           "VERSION without a port is RC 10");
}

static void test_stem(void)
{
    omr_io io;
    char buf[256];
    io_init(&io, buf, (int)sizeof buf, 100);
    expect(omr_do_line("STATUS STEM ST VAR MYVAR", verbs, 5, NULL, &io) == 0 && io.rc == OMR_OK &&
               strcmp(buf, "TEST.1 file.stl 0") == 0,
           "STATUS RESULT");
    expect(omr_host_var("ST.PORT") && strcmp(omr_host_var("ST.PORT"), "TEST.1") == 0, "STEM ST.PORT");
    expect(omr_host_var("ST.FILE") && strcmp(omr_host_var("ST.FILE"), "file.stl") == 0, "STEM ST.FILE");
    expect(omr_host_var("ST.DIRTY") && strcmp(omr_host_var("ST.DIRTY"), "0") == 0, "STEM ST.DIRTY");
    expect(omr_host_var("MYVAR") && strcmp(omr_host_var("MYVAR"), "ok") == 0, "VAR MYVAR");
}

static void test_port_client(void)
{
    omr_port *a, *b;
    omr_client *c;
    omr_opt opt;
    omr_io io;
    char buf[256], found[40];
    int hits = 0;

    a = omr_open("TEST", "TestApp 1.0");
    expect(a && omr_name(a) && strcmp(omr_name(a), "TEST.1") == 0, "first port is TEST.1");
    b = omr_open("TEST", NULL);
    expect(b && strcmp(omr_name(b), "TEST.2") == 0, "second port is TEST.2");
    expect(omr_find("TEST", found, (int)sizeof found) == 0 && strcmp(found, "TEST.1") == 0,
           "find TEST gets .1");
    expect(omr_find("TEST.2", found, (int)sizeof found) == 0 && strcmp(found, "TEST.2") == 0,
           "find TEST.2");
    expect(omr_find("NO-SUCH-PORT-XYZ", found, (int)sizeof found) != 0, "find missing");
    expect(omr_mask(a) == 0, "host mask is 0");
    expect(omr_attach(a, verbs, 5, &hits) == 0, "attach");

    memset(&opt, 0, sizeof opt);
    opt.size = (int)sizeof opt;
    c = omr_connect("TEST", &opt);
    expect(c && strcmp(opt.name, "TEST.1") == 0, "connect finds TEST.1");
    io_init(&io, buf, (int)sizeof buf, 200);
    expect(omr_send(c, "VERSION", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "TestApp 1.0") == 0,
           "send VERSION to the attached port");
    expect(omr_send(c, "HELP", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPEN"), "send HELP");
    expect(omr_send(c, "OPEN part.stl", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "part.stl") == 0 && hits == 1,
           "send OPEN through the client");
    omr_disconnect(c);

    /* Unattached port: send times out. */
    io_init(&io, buf, (int)sizeof buf, 80);
    c = omr_connect("TEST.2", NULL);
    expect(c != NULL, "connect TEST.2");
    expect(omr_send(c, "OPEN z.stl", &io) == -1 && io.rc == OMR_FAIL && strstr(buf, "timeout"),
           "send to an unattached port times out");
    omr_disconnect(c);

    /* Connect to a missing port with no program: fails at once. */
    expect(omr_connect("MISSING", NULL) == NULL, "connect missing, find only");

#if !defined(AMIGA) && !defined(__amigaos__)
    /* Connect to a missing port, start a program that will not open one. */
    memset(&opt, 0, sizeof opt);
    opt.size = (int)sizeof opt;
    opt.timeout = 80;
    opt.program = "true";
    expect(omr_connect("MISSING", &opt) == NULL, "connect start+timeout");
#endif

    io_init(&io, buf, (int)sizeof buf, 100);
    expect(omr_do_line(NULL, verbs, 5, NULL, &io) == 0 && io.rc == OMR_ERROR, "do_line NULL line");

    omr_close(a);
    omr_close(b);
    expect(omr_find("TEST", found, (int)sizeof found) != 0, "ports closed");
    omr_close(NULL);
    omr_disconnect(NULL);
}

int main(void)
{
    omr_io bad;
    memset(&bad, 0, sizeof bad);
    expect(omr_do_line("OPEN x", verbs, 5, NULL, &bad) == -1, "short io is refused");
    test_parse();
    test_help();
    test_stem();
    test_port_client();
    printf(fails ? "rexx: %d failed\n" : "rexx ok\n", fails);
    return fails != 0;
}
