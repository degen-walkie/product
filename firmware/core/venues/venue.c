#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dw_hal.h"
#include "dw_venue.h"

const dw_venue_t *dw_venue(int v)
{
    switch (v) {
    case V_HL: return &dw_hl;
    case V_PAC: return &dw_pac;
    case V_LIT: return &dw_lit;
    default: return NULL;
    }
}

uint64_t dw_now_ms(void) { return (uint64_t)hal_unix_ms(); }

int dw_interval_ms(const char *iv)
{
    int n = atoi(iv);
    if (n <= 0) n = 1;
    switch (iv[strlen(iv) - 1]) {
    case 'm': return n * 60 * 1000;
    case 'h': return n * 3600 * 1000;
    case 'd': return n * 86400 * 1000;
    default: return 15 * 60 * 1000;
    }
}

static void strip_zeros(char *s)
{
    if (!strchr(s, '.')) return;
    size_t n = strlen(s);
    while (n > 0 && s[n - 1] == '0') s[--n] = 0;
    if (n > 0 && s[n - 1] == '.') s[--n] = 0;
    if (!strcmp(s, "-0")) strcpy(s, "0");
}

void dw_num_str(double v, int dec, char *out, size_t cap)
{
    if (dec < 0) dec = 0;
    snprintf(out, cap, "%.*f", dec, v);
    strip_zeros(out);
}

void dw_px_str(double px, int sig, int max_dec, char *out, size_t cap)
{
    char tmp[40];
    snprintf(tmp, sizeof tmp, "%.*g", sig, px);
    double r = atof(tmp);
    if (fabs(r - round(r)) < 1e-12 || fabs(px) >= pow(10, sig)) r = fabs(px) >= pow(10, sig) ? round(px) : r;
    dw_num_str(r, max_dec, out, cap);
}
