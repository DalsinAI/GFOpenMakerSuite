/* Netpbm load (burn_image.h). The scan path is built in image.c.
 * Public Netpbm format only. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_image.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void burn_image_init(burn_image *im)
{
    if (!im) return;
    memset(im, 0, sizeof *im);
    im->size = (int)sizeof *im;
}

void burn_image_free(burn_image *im)
{
    if (!im) return;
    free(im->grey);
    memset(im, 0, sizeof *im);
}

static int suffix_is(const char *path, const char *suf)
{
    size_t n, m;
    if (!path || !suf) return 0;
    n = strlen(path);
    m = strlen(suf);
    if (n < m) return 0;
    return strcmp(path + n - m, suf) == 0 ||
           (m >= 4 && path[n - m] == '.' && (path[n - m + 1] | 32) == (suf[1] | 32) &&
            (path[n - m + 2] | 32) == (suf[2] | 32) && (path[n - m + 3] | 32) == (suf[3] | 32));
}

static int peek_magic(FILE *f)
{
    int c, p;
    for (;;) {
        c = fgetc(f);
        if (c == '#') {
            while (c != EOF && c != '\n') c = fgetc(f);
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        break;
    }
    if (c != 'P') return 0;
    p = fgetc(f);
    if (p < '1' || p > '6') return 0;
    return p - '0';
}

int burn_is_image(const char *path)
{
    FILE *f;
    int m;
    if (!path || !path[0]) return 0;
    if (suffix_is(path, ".pbm") || suffix_is(path, ".pgm") || suffix_is(path, ".ppm")) return 1;
    f = fopen(path, "rb");
    if (!f) return 0;
    m = peek_magic(f);
    fclose(f);
    return m >= 1 && m <= 6;
}

static int skip_space(FILE *f, int *c)
{
    for (;;) {
        if (*c == '#') {
            while (*c != EOF && *c != '\n') *c = fgetc(f);
        }
        if (*c != ' ' && *c != '\t' && *c != '\r' && *c != '\n') return *c != EOF;
        *c = fgetc(f);
    }
}

static int read_int(FILE *f, int *c, int *out)
{
    int n = 0, seen = 0;
    if (!skip_space(f, c)) return -1;
    if (*c < '0' || *c > '9') return -1;
    while (*c >= '0' && *c <= '9') {
        if (n > 100000000) return -1;
        n = n * 10 + (*c - '0');
        seen = 1;
        *c = fgetc(f);
    }
    if (!seen) return -1;
    *out = n;
    return 0;
}

static float luma(int r, int g, int b, int maxv)
{
    /* Rec. 601 coefficients, as 299/587/114, public. */
    if (maxv < 1) return 0.f;
    return (float)(299 * r + 587 * g + 114 * b) / (float)(1000 * maxv);
}

int burn_image_load(burn_image *im, const char *path, char *err, int errlen)
{
    FILE *f;
    int c, magic, cols = 0, rows = 0, maxv = 1, i, n, raw;
    float *grey = NULL;
    if (!im || !path || !path[0]) {
        snprintf(err, (size_t)errlen, "no picture");
        return -1;
    }
    burn_image_free(im);
    burn_image_init(im);
    f = fopen(path, "rb");
    if (!f) {
        snprintf(err, (size_t)errlen, "%s: cannot open", path);
        return -1;
    }
    c = fgetc(f);
    if (!skip_space(f, &c) || c != 'P') {
        snprintf(err, (size_t)errlen, "%s: not a Netpbm picture", path);
        fclose(f);
        return -1;
    }
    magic = fgetc(f);
    c = fgetc(f);
    if (magic < '1' || magic > '6') {
        snprintf(err, (size_t)errlen, "%s: not a Netpbm picture", path);
        fclose(f);
        return -1;
    }
    magic -= '0';
    if (read_int(f, &c, &cols) || read_int(f, &c, &rows)) {
        snprintf(err, (size_t)errlen, "%s: wants width and height", path);
        fclose(f);
        return -1;
    }
    if (magic != 1 && magic != 4) {
        if (read_int(f, &c, &maxv)) {
            snprintf(err, (size_t)errlen, "%s: wants a max value", path);
            fclose(f);
            return -1;
        }
    } else
        maxv = 1;
    if (cols < 1 || rows < 1 || cols > BURN_IMAGE_MAX || rows > BURN_IMAGE_MAX) {
        snprintf(err, (size_t)errlen, "%s: the picture is too large (up to %d)", path, BURN_IMAGE_MAX);
        fclose(f);
        return -1;
    }
    if (maxv < 1 || maxv > 255) {
        snprintf(err, (size_t)errlen, "%s: max value must be 1 to 255", path);
        fclose(f);
        return -1;
    }
    n = cols * rows;
    grey = (float *)malloc((size_t)n * sizeof *grey);
    if (!grey) {
        snprintf(err, (size_t)errlen, "out of memory");
        fclose(f);
        return -1;
    }
    raw = (magic == 4 || magic == 5 || magic == 6);
    /* One whitespace separates the header from raw samples. */
    if (raw) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            snprintf(err, (size_t)errlen, "%s: wants a blank after the header", path);
            free(grey);
            fclose(f);
            return -1;
        }
        if (c == '\r') {
            int n2 = fgetc(f);
            if (n2 != '\n') ungetc(n2, f);
        }
    }
    if (magic == 4) {
        int row, col, bit, b = 0;
        for (row = 0; row < rows; row++) {
            bit = 0;
            for (col = 0; col < cols; col++, bit--) {
                if (bit <= 0) {
                    b = fgetc(f);
                    if (b == EOF) {
                        snprintf(err, (size_t)errlen, "%s: unexpected end", path);
                        free(grey);
                        fclose(f);
                        return -1;
                    }
                    bit = 8;
                }
                /* PBM: 1 is black. */
                grey[row * cols + col] = (b & 0x80) ? 1.f : 0.f;
                b <<= 1;
            }
        }
    } else if (magic == 5 || magic == 6) {
        for (i = 0; i < n; i++) {
            int r, g, b;
            r = fgetc(f);
            if (r == EOF) {
                snprintf(err, (size_t)errlen, "%s: unexpected end", path);
                free(grey);
                fclose(f);
                return -1;
            }
            if (magic == 5)
                grey[i] = 1.f - (float)r / (float)maxv;
            else {
                g = fgetc(f);
                b = fgetc(f);
                if (g == EOF || b == EOF) {
                    snprintf(err, (size_t)errlen, "%s: unexpected end", path);
                    free(grey);
                    fclose(f);
                    return -1;
                }
                grey[i] = 1.f - luma(r, g, b, maxv);
            }
        }
    } else {
        /* Plain P1 / P2 / P3. c already holds the next character. */
        for (i = 0; i < n; i++) {
            int r, g, b;
            if (magic == 1) {
                if (read_int(f, &c, &r) || (r != 0 && r != 1)) {
                    snprintf(err, (size_t)errlen, "%s: a PBM sample is 0 or 1", path);
                    free(grey);
                    fclose(f);
                    return -1;
                }
                grey[i] = r ? 1.f : 0.f;
            } else if (magic == 2) {
                if (read_int(f, &c, &r) || r < 0 || r > maxv) {
                    snprintf(err, (size_t)errlen, "%s: a sample is out of range", path);
                    free(grey);
                    fclose(f);
                    return -1;
                }
                grey[i] = 1.f - (float)r / (float)maxv;
            } else {
                if (read_int(f, &c, &r) || read_int(f, &c, &g) || read_int(f, &c, &b) || r < 0 || r > maxv || g < 0 ||
                    g > maxv || b < 0 || b > maxv) {
                    snprintf(err, (size_t)errlen, "%s: a sample is out of range", path);
                    free(grey);
                    fclose(f);
                    return -1;
                }
                grey[i] = 1.f - luma(r, g, b, maxv);
            }
        }
    }
    fclose(f);
    im->cols = cols;
    im->rows = rows;
    im->grey = grey;
    return 0;
}
