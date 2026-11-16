/* The OpenMake libraries' files, over dos.library (libshell.c opens it):
 * fopen, fclose, fread, fwrite, fgets, fseek and ftell, as much as the core
 * uses. A FILE here is our own; nothing but these calls looks inside it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <stdlib.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <stdio.h>

extern struct DosLibrary *DOSBase;

typedef struct {
    BPTR fh;
    int err;
} om_file;

FILE *fopen(const char *path, const char *mode)
{
    om_file *f = (om_file *)malloc(sizeof *f);
    LONG how = mode[0] == 'w' ? MODE_NEWFILE : mode[0] == 'a' ? MODE_READWRITE : MODE_OLDFILE;
    if (!f) return NULL;
    f->err = 0;
    f->fh = Open((CONST_STRPTR)path, how);
    if (!f->fh) {
        free(f);
        return NULL;
    }
    if (mode[0] == 'a') Seek(f->fh, 0, OFFSET_END);
    return (FILE *)f;
}

int fclose(FILE *fp)
{
    om_file *f = (om_file *)fp;
    int ok;
    if (!f) return -1;
    ok = Close(f->fh) ? 0 : -1;
    free(f);
    return ok;
}

size_t fread(void *buf, size_t size, size_t n, FILE *fp)
{
    om_file *f = (om_file *)fp;
    LONG got;
    if (!size || !n) return 0;
    got = Read(f->fh, buf, (LONG)(size * n));
    if (got < 0) {
        f->err = 1;
        return 0;
    }
    return (size_t)got / size;
}

size_t fwrite(const void *buf, size_t size, size_t n, FILE *fp)
{
    om_file *f = (om_file *)fp;
    LONG put;
    if (!size || !n) return 0;
    put = Write(f->fh, (APTR)buf, (LONG)(size * n));
    if (put < 0) {
        f->err = 1;
        return 0;
    }
    return (size_t)put / size;
}

/* A byte at a time through dos.library's buffer (FGetC). */
char *fgets(char *s, int len, FILE *fp)
{
    om_file *f = (om_file *)fp;
    int n = 0;
    while (n + 1 < len) {
        LONG c = FGetC(f->fh);
        if (c < 0) break;
        s[n++] = (char)c;
        if (c == '\n') break;
    }
    if (!n) return NULL;
    s[n] = 0;
    return s;
}

int fseek(FILE *fp, long off, int whence)
{
    om_file *f = (om_file *)fp;
    LONG mode = whence == SEEK_END ? OFFSET_END : whence == SEEK_CUR ? OFFSET_CURRENT : OFFSET_BEGINNING;
    Flush(f->fh);
    return Seek(f->fh, off, mode) < 0 ? -1 : 0;
}

long ftell(FILE *fp)
{
    om_file *f = (om_file *)fp;
    Flush(f->fh);
    return Seek(f->fh, 0, OFFSET_CURRENT);
}
