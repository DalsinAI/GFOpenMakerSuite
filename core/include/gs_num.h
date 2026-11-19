/* A number whose decimal point is always '.'.
 * strtof follows the locale. On the Amiga a comma is a decimal point,
 * so "2)" was read as a second number and "1.5 2 3" was not three.
 * Returns 1 and *end just after the number, or 0 and *end at s.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_NUM_H
#define GS_NUM_H

static int gs_number(const char *s, float *out, const char **end)
{
    const char *p = s;
    int neg = 0, any = 0, exp = 0, digits = 0, k;
    double m = 0;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v') p++;
    if (*p == '+' || *p == '-') neg = *p++ == '-';
    for (; *p >= '0' && *p <= '9'; p++) {
        any = 1;
        if (digits < 9) {
            m = m * 10.0 + (double)(*p - '0');
            digits++;
        } else {
            exp++;
        }
    }
    if (*p == '.') {
        p++;
        for (; *p >= '0' && *p <= '9'; p++) {
            any = 1;
            if (digits < 9) {
                m = m * 10.0 + (double)(*p - '0');
                digits++;
                exp--;
            }
        }
    }
    if (!any) {
        *end = s;
        return 0;
    }
    if (*p == 'e' || *p == 'E') {
        const char *q = p + 1;
        int en = 0, eneg = 0, ed = 0;
        if (*q == '+' || *q == '-') eneg = *q++ == '-';
        for (; *q >= '0' && *q <= '9'; q++) {
            ed = 1;
            if (en < 1000) en = en * 10 + (*q - '0');
        }
        if (ed) {
            exp += eneg ? -en : en;
            p = q;
        }
    }
    if (neg) m = -m;
    if (exp > 0)
        for (k = 0; k < exp; k++) m *= 10.0;
    else
        for (k = 0; k < -exp; k++) m /= 10.0;
    *out = (float)m;
    *end = p;
    return 1;
}

#endif
