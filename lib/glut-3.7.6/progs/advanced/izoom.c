/**
 **	izoom- 
 **		Magnify or minify a picture with or without filtering.  The 
 **	filtered method is one pass, uses 2-d convolution, and is optimized 
 **	by integer arithmetic and precomputation of filter coeffs.
 **
 **		    		Paul Haeberli - 1988
 **/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "izoom.h"

#ifdef _WIN32
#pragma warning (disable:4244)          /* disable bogus conversion warnings */
#endif

typedef struct filtinteg {
  float rad, min, max;
  float *tab;
} filtinteg;

float flerp(float f0, float f1, float p);

#define GRIDTOFLOAT(pos,n)	(((pos)+0.5)/(n))
#define FLOATTOGRID(pos,n)	((pos)*(n))
#define SHIFT 			12
#define ONE 			(1<<SHIFT)
#define EPSILON			0.0001
#define FILTERRAD		(blurfactor*shape->rad)
#define FILTTABSIZE		250

static void makexmap(short *abuf, short *xmap[], int anx, int bnx);
static void setintrow(int *buf, int val, int n);
static void xscalebuf(short *xmap[], short *bbuf, int bnx);
static void addrow(int *iptr, short *sptr, int w, int n);
static void divrow(int *iptr, short *sptr, int tot, int n);
static FILTER *makefilt(short *abuf, int anx, int bnx, int *maxn);
static void freefilt(FILTER * filt, int n);
static void applyxfilt(short *bbuf, FILTER * xfilt, int bnx);
float filterinteg(float bmin, float bmax, float blurf);
static void mitchellinit(float b, float c);
static void clamprow(short *iptr, short *optr, int n);

float filt_box(float x);
float filt_triangle(float x);
float filt_quadratic(float x);
float filt_mitchell(float x);
float filt_gaussian(float x);

static int (*xfiltfunc) (short *, int);
static float blurfactor;
int izoomdebug;

static filtinteg *shapeBOX;
static filtinteg *shapeTRIANGLE;
static filtinteg *shapeQUADRATIC;
static filtinteg *shapeMITCHELL;
static filtinteg *shapeGAUSSIAN;
static filtinteg *shape;

static filtinteg *
integrate(float (*filtfunc) (float), float rad)
{
  int i;
  float del, x, min, max;
  double tot;
  filtinteg *filt;

  min = -rad;
  max = rad;
  del = 2 * rad;
  tot = 0.0;
  filt = (filtinteg *) malloc(sizeof(filtinteg));
  filt->rad = rad;
  filt->min = min;
  filt->max = max;
  filt->tab = (float *) malloc(FILTTABSIZE * sizeof(float));
  for (i = 0; i < FILTTABSIZE; i++) {
    x = min + (del * i / (FILTTABSIZE - 1.0));
    tot = tot + filtfunc(x);
    filt->tab[i] = tot;
  }
  for (i = 0; i < FILTTABSIZE; i++)
    filt->tab[i] /= tot;
  return filt;
}

float 
filterinteg(float bmin, float bmax, float blurf)
{
  int i1, i2;
  float f1, f2;
  float *tab;
  float mult;

  bmin /= blurf;
  bmax /= blurf;
  tab = shape->tab;
  mult = (FILTTABSIZE - 1.0) / (2.0 * shape->rad);

  f1 = ((bmin - shape->min) * mult);
  i1 = floor(f1);
  f1 = f1 - i1;
  if (i1 < 0)
    f1 = 0.0;
  else if (i1 >= (FILTTABSIZE - 1))
    f1 = 1.0;
  else
    f1 = flerp(tab[i1], tab[i1 + 1], f1);

  f2 = ((bmax - shape->min) * mult);
  i2 = floor(f2);
  f2 = f2 - i2;
  if (i2 < 0)
    f2 = 0.0;
  else if (i2 >= (FILTTABSIZE - 1))
    f2 = 1.0;
  else
    f2 = flerp(tab[i2], tab[i2 + 1], f2);
  return f2 - f1;
}

void
setfiltertype(int filttype)
{
  switch (filttype) {
  case IMPULSE:
    shape = 0;
    break;
  case BOX:
    if (!shapeBOX)
      shapeBOX = integrate(filt_box, 0.5);
    shape = shapeBOX;
    break;
  case TRIANGLE:
    if (!shapeTRIANGLE)
      shapeTRIANGLE = integrate(filt_triangle, 1.0);
    shape = shapeTRIANGLE;
    break;
  case QUADRATIC:
    if (!shapeQUADRATIC)
      shapeQUADRATIC = integrate(filt_quadratic, 1.5);
    shape = shapeQUADRATIC;
    break;
  case MITCHELL:
    if (!shapeMITCHELL)
      shapeMITCHELL = integrate(filt_mitchell, 2.0);
    shape = shapeMITCHELL;
    break;
  case GAUSSIAN:
    if (!shapeGAUSSIAN)
      shapeGAUSSIAN = integrate(filt_gaussian, 1.5);
    shape = shapeGAUSSIAN;
    break;
  }
}

void
copyimage(getfunc_t getfunc, getfunc_t putfunc, int nx, int ny)
{
  int y;
  short *abuf;

  abuf = (short *) malloc(nx * sizeof(short));
  for (y = 0; y < ny; y++) {
    getfunc(abuf, y);
    putfunc(abuf, y);
  }
  free(abuf);
}

/* general zoom follows */
zoom *
newzoom(getfunc_t getfunc, int anx, int any, int bnx, int bny, int filttype, float blur)
{
  zoom *z;
  int i;

  setfiltertype(filttype);
  z = (zoom *) malloc(sizeof(zoom));
  z->getfunc = getfunc;
  z->abuf = (short *) malloc(anx * sizeof(short));
  z->bbuf = (short *) malloc(bnx * sizeof(short));
  z->anx = anx;
  z->any = any;
  z->bnx = bnx;
  z->bny = bny;
  z->curay = -1;
  z->y = 0;
  z->type = filttype;
  if (filttype == IMPULSE) {
    if (z->anx != z->bnx) {
      z->xmap = (short **) malloc(z->bnx * sizeof(short *));
      makexmap(z->abuf, z->xmap, z->anx, z->bnx);
    }
  } else {
    blurfactor = blur;
    if (filttype == MITCHELL)
      z->clamp = 1;
    else
      z->clamp = 0;
    z->tbuf = (short *) malloc(bnx * sizeof(short));
    z->xfilt = makefilt(z->abuf, anx, bnx, &z->nrows);
    z->yfilt = makefilt(0, any, bny, &z->nrows);
    z->filtrows = (short **) malloc(z->nrows * sizeof(short *));
    for (i = 0; i < z->nrows; i++)
      z->filtrows[i] = (short *) malloc(z->bnx * sizeof(short));
    z->accrow = (int *) malloc(z->bnx * sizeof(int));
    z->ay = 0;
  }
  return z;
}

void
getzoomrow(zoom * z, short *buf, int y)
{
  float fy;
  int ay;
  FILTER *f;
  int i, max;
  short *row;

  if (y == 0) {
    z->curay = -1;
    z->y = 0;
    z->ay = 0;
  }
  if (z->type == IMPULSE) {
    fy = GRIDTOFLOAT(z->y, z->bny);
    ay = FLOATTOGRID(fy, z->any);
    if (z->anx == z->bnx) {
      if (z->curay != ay) {
        z->getfunc(z->abuf, ay);
        z->curay = ay;
        if (xfiltfunc)
          xfiltfunc(z->abuf, z->bnx);
      }
      memcpy(buf, z->abuf, z->bnx * sizeof(short));
    } else {
      if (z->curay != ay) {
        z->getfunc(z->abuf, ay);
        xscalebuf(z->xmap, z->bbuf, z->bnx);
        z->curay = ay;
        if (xfiltfunc)
          xfiltfunc(z->bbuf, z->bnx);
      }
      memcpy(buf, z->bbuf, z->bnx * sizeof(short));
    }
  } else if (z->any == 1 && z->bny == 1) {
    z->getfunc(z->abuf, z->ay++);
    applyxfilt(z->filtrows[0], z->xfilt, z->bnx);
    if (xfiltfunc)
      xfiltfunc(z->filtrows[0], z->bnx);
    if (z->clamp) {
      clamprow(z->filtrows[0], z->tbuf, z->bnx);
      memcpy(buf, z->tbuf, z->bnx * sizeof(short));
    } else {
      memcpy(buf, z->filtrows[0], z->bnx * sizeof(short));
    }
  } else {
    f = z->yfilt + z->y;
    max = (int) (sizeof(f->dat) / sizeof(short) + (f->n - 1));
    while (z->ay <= max) {
      z->getfunc(z->abuf, z->ay++);
      row = z->filtrows[0];
      for (i = 0; i < (z->nrows - 1); i++)
        z->filtrows[i] = z->filtrows[i + 1];
      z->filtrows[z->nrows - 1] = row;
      applyxfilt(z->filtrows[z->nrows - 1], z->xfilt, z->bnx);
      if (xfiltfunc)
        xfiltfunc(z->filtrows[z->nrows - 1], z->bnx);
    }
    if (f->n == 1) {
      if (z->clamp) {
        clamprow(z->filtrows[z->nrows - 1], z->tbuf, z->bnx);
        memcpy(buf, z->tbuf, z->bnx * sizeof(short));
      } else {
        memcpy(buf, z->filtrows[z->nrows - 1], z->bnx * sizeof(short));
      }
    } else {
      setintrow(z->accrow, f->halftotw, z->bnx);
      for (i = 0; i < f->n; i++)
        addrow(z->accrow, z->filtrows[i + (z->nrows - 1) - (f->n - 1)],
          f->w[i], z->bnx);
      divrow(z->accrow, z->bbuf, f->totw, z->bnx);
      if (z->clamp) {
        clamprow(z->bbuf, z->tbuf, z->bnx);
        memcpy(buf, z->tbuf, z->bnx * sizeof(short));
      } else {
        memcpy(buf, z->bbuf, z->bnx * sizeof(short));
      }
    }
  }
  z->y++;
}

static void
setintrow(int *buf, int val, int n)
{
  while (n >= 8) {
    buf[0] = val;
    buf[1] = val;
    buf[2] = val;
    buf[3] = val;
    buf[4] = val;
    buf[5] = val;
    buf[6] = val;
    buf[7] = val;
    buf += 8;
    n -= 8;
  }
  while (n--)
    *buf++ = val;
}

void
freezoom(zoom * z)
{
  int i;

  if (z->type == IMPULSE) {
    if (z->anx != z->bnx)
      free(z->xmap);
  } else {
    freefilt(z->xfilt, z->bnx);
    freefilt(z->yfilt, z->bny);
    free(z->tbuf);
    for (i = 0; i < z->nrows; i++)
      free(z->filtrows[i]);
    free(z->filtrows);
    free(z->accrow);
  }
  free(z->abuf);
  free(z->bbuf);
  free(z);

}

void
filterzoom(getfunc_t getfunc, getfunc_t putfunc, int anx, int any, int bnx, int bny, int filttype, float blur)
{
  zoom *z;
  int y;
  short *buf;

  buf = (short *) malloc(bnx * sizeof(short));
  z = newzoom(getfunc, anx, any, bnx, bny, filttype, blur);
  for (y = 0; y < bny; y++) {
    getzoomrow(z, buf, y);
    putfunc(buf, y);
  }
  freezoom(z);
  free(buf);
}

/* impulse zoom utilities */
static void
makexmap(short *abuf, short *xmap[], int anx, int bnx)
{
  int x, ax;
  float fx;

  for (x = 0; x < bnx; x++) {
    fx = GRIDTOFLOAT(x, bnx);
    ax = FLOATTOGRID(fx, anx);
    xmap[x] = abuf + ax;
  }
}

static void
xscalebuf(short *xmap[], short *bbuf, int bnx)
{
  while (bnx >= 8) {
    bbuf[0] = *(xmap[0]);
    bbuf[1] = *(xmap[1]);
    bbuf[2] = *(xmap[2]);
    bbuf[3] = *(xmap[3]);
    bbuf[4] = *(xmap[4]);
    bbuf[5] = *(xmap[5]);
    bbuf[6] = *(xmap[6]);
    bbuf[7] = *(xmap[7]);
    bbuf += 8;
    xmap += 8;
    bnx -= 8;
  }
  while (bnx--)
    *bbuf++ = *(*xmap++);
}

void
zoomxfilt(int (*filtfunc) (short *, int))
{
  xfiltfunc = filtfunc;
}

/* filter zoom utilities */
static void
addrow(int *iptr, short *sptr, int w, int n)
{
  while (n >= 8) {
    iptr[0] += (w * sptr[0]);
    iptr[1] += (w * sptr[1]);
    iptr[2] += (w * sptr[2]);
    iptr[3] += (w * sptr[3]);
    iptr[4] += (w * sptr[4]);
    iptr[5] += (w * sptr[5]);
    iptr[6] += (w * sptr[6]);
    iptr[7] += (w * sptr[7]);
    iptr += 8;
    sptr += 8;
    n -= 8;
  }
  while (n--)
    *iptr++ += (w * *sptr++);
}

static void
divrow(int *iptr, short *sptr, int tot, int n)
{
  while (n >= 8) {
    sptr[0] = iptr[0] / tot;
    sptr[1] = iptr[1] / tot;
    sptr[2] = iptr[2] / tot;
    sptr[3] = iptr[3] / tot;
    sptr[4] = iptr[4] / tot;
    sptr[5] = iptr[5] / tot;
    sptr[6] = iptr[6] / tot;
    sptr[7] = iptr[7] / tot;
    sptr += 8;
    iptr += 8;
    n -= 8;
  }
  while (n--)
    *sptr++ = (*iptr++) / tot;
}

static FILTER *
makefilt(short *abuf, int anx, int bnx, int *maxn)
{
  FILTER *f, *filter;
  int x, n;
  float bmin, bmax, bcent, brad;
  float fmin, fmax, acent, arad;
  int amin, amax;
  float coverscale;

  if (izoomdebug)
    fprintf(stderr, "makefilt\n");
  f = filter = (FILTER *) malloc(bnx * sizeof(FILTER));
  *maxn = 0;
  if (bnx < anx) {
    coverscale = ((float) anx / bnx * ONE) / 2.0;
    brad = FILTERRAD / bnx;
    for (x = 0; x < bnx; x++) {
      bcent = ((float) x + 0.5) / bnx;
      amin = floor((bcent - brad) * anx + EPSILON);
      amax = floor((bcent + brad) * anx - EPSILON);
      if (amin < 0)
        amin = 0;
      if (amax >= anx)
        amax = anx - 1;
      f->n = 1 + amax - amin;
      f->dat = abuf + amin;
      f->w = (short *) malloc(f->n * sizeof(short));
      f->totw = 0;
      if (izoomdebug)
        fprintf(stderr, "| ");
      for (n = 0; n < f->n; n++) {
        bmin = bnx * ((((float) amin + n) / anx) - bcent);
        bmax = bnx * ((((float) amin + n + 1) / anx) - bcent);
        f->w[n] = floor((coverscale * filterinteg(bmin, bmax, blurfactor)) + 0.5);
        if (izoomdebug)
          fprintf(stderr, "%d ", f->w[n]);
        f->totw += f->w[n];
      }
      f->halftotw = f->totw / 2;
      if (f->n > *maxn)
        *maxn = f->n;
      f++;
    }
  } else {
    coverscale = ((float) bnx / anx * ONE) / 2.0;
    arad = FILTERRAD / anx;
    for (x = 0; x < bnx; x++) {
      bmin = ((float) x) / bnx;
      bmax = ((float) x + 1.0) / bnx;
      amin = floor((bmin - arad) * anx + (0.5 + EPSILON));
      amax = floor((bmax + arad) * anx - (0.5 + EPSILON));
      if (amin < 0)
        amin = 0;
      if (amax >= anx)
        amax = anx - 1;
      f->n = 1 + amax - amin;
      f->dat = abuf + amin;
      f->w = (short *) malloc(f->n * sizeof(short));
      f->totw = 0;
      if (izoomdebug)
        fprintf(stderr, "| ");
      for (n = 0; n < f->n; n++) {
        acent = (amin + n + 0.5) / anx;
        fmin = anx * (bmin - acent);
        fmax = anx * (bmax - acent);
        f->w[n] = floor((coverscale * filterinteg(fmin, fmax, blurfactor)) + 0.5);
        if (izoomdebug)
          fprintf(stderr, "%d ", f->w[n]);
        f->totw += f->w[n];
      }
      f->halftotw = f->totw / 2;
      if (f->n > *maxn)
        *maxn = f->n;
      f++;
    }
  }
  if (izoomdebug)
    fprintf(stderr, "|\n");
  return filter;
}

static void
freefilt(FILTER * filt, int n)
{
  FILTER *f;

  f = filt;
  while (n--) {
    free(f->w);
    f++;
  }
  free(filt);
}

static void
applyxfilt(short *bbuf, FILTER * xfilt, int bnx)
{
  short *w;
  short *dptr;
  int n, val;

  while (bnx--) {
    if ((n = xfilt->n) == 1) {
      *bbuf++ = *xfilt->dat;
    } else {
      w = xfilt->w;
      dptr = xfilt->dat;
      val = xfilt->halftotw;
      n = xfilt->n;
      while (n--)
        val += *w++ * *dptr++;
      *bbuf++ = val / xfilt->totw;
    }
    xfilt++;
  }
}

/* filter shape functions follow */
float 
filt_box(float x)
{
  if (x < -0.5)
    return 0.0;
  if (x < 0.5)
    return 1.0;
  return 0.0;
}

float 
filt_triangle(float x)
{
  if (x < -1.0)
    return 0.0;
  if (x < 0.0)
    return 1.0 + x;
  if (x < 1.0)
    return 1.0 - x;
  return 0.0;
}

float 
filt_quadratic(float x)
{
  if (x < -1.5)
    return 0.0;
  if (x < -0.5)
    return 0.5 * (x + 1.5) * (x + 1.5);
  if (x < 0.5)
    return 0.75 - (x * x);
  if (x < 1.5)
    return 0.5 * (x - 1.5) * (x - 1.5);
  return 0.0;
}

static float p0, p2, p3, q0, q1, q2, q3;

/* see Mitchell&Netravali, "Reconstruction Filters in Computer Graphics",
   SIGGRAPH 88.  Mitchell code provided by Paul Heckbert.  */

float 
filt_mitchell(float x)
{                       /* Mitchell & Netravali's two-param cubic */
  static int mitfirsted;

  if (!mitfirsted) {
    mitchellinit(1.0f / 3.0f, 1.0f / 3.0f);
    mitfirsted = 1;
  }
  if (x < -2.0)
    return 0.0;
  if (x < -1.0)
    return (q0 - x * (q1 - x * (q2 - x * q3)));
  if (x < 0.0)
    return (p0 + x * x * (p2 - x * p3));
  if (x < 1.0)
    return (p0 + x * x * (p2 + x * p3));
  if (x < 2.0)
    return (q0 + x * (q1 + x * (q2 + x * q3)));
  return 0.0;
}

static void
mitchellinit(float b, float c)
{
  p0 = (6.0 - 2.0 * b) / 6.0;
  p2 = (-18.0 + 12.0 * b + 6.0 * c) / 6.0;
  p3 = (12.0 - 9.0 * b - 6.0 * c) / 6.0;
  q0 = (8.0 * b + 24.0 * c) / 6.0;
  q1 = (-12.0 * b - 48.0 * c) / 6.0;
  q2 = (6.0 * b + 30.0 * c) / 6.0;
  q3 = (-b - 6.0 * c) / 6.0;
}

#define NARROWNESS	1.5

float 
filt_gaussian(float x)
{
  x = x * NARROWNESS;
  return (1.0 / exp(x * x) - 1.0 / exp(1.5 * NARROWNESS * 1.5 * NARROWNESS));
}

float 
flerp(float f0, float f1, float p)
{
  return ((f0 * (1.0 - p)) + (f1 * p));
}

#define DOCLAMP(iptr,optr)	*(optr) = ((*(iptr)<0) ? 0 : (*(iptr)>255) ? 255 : *(iptr))

static void
clamprow(short *iptr, short *optr, int n)
{
  while (n >= 8) {
    DOCLAMP(iptr + 0, optr + 0);
    DOCLAMP(iptr + 1, optr + 1);
    DOCLAMP(iptr + 2, optr + 2);
    DOCLAMP(iptr + 3, optr + 3);
    DOCLAMP(iptr + 4, optr + 4);
    DOCLAMP(iptr + 5, optr + 5);
    DOCLAMP(iptr + 6, optr + 6);
    DOCLAMP(iptr + 7, optr + 7);
    iptr += 8;
    optr += 8;
    n -= 8;
  }
  while (n--) {
    DOCLAMP(iptr, optr);
    iptr++;
    optr++;
  }
}
