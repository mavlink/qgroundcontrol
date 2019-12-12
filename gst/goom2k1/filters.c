/* filter.c version 0.7
 * contient les filtres applicable a un buffer
 * creation : 01/10/2000
 *  -ajout de sinFilter()
 *  -ajout de zoomFilter()
 *  -copie de zoomFilter() en zoomFilterRGB(), gérant les 3 couleurs
 *  -optimisation de sinFilter (utilisant une table de sin)
 *      -asm
 *      -optimisation de la procedure de génération du buffer de transformation
 *              la vitesse est maintenant comprise dans [0..128] au lieu de [0..100]
*/

/*#define _DEBUG_PIXEL; */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters.h"
#include "graphic.h"
#include "goom_tools.h"
#include "goom_core.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifdef MMX
#define USE_ASM
#endif
#ifdef POWERPC
#define USE_ASM
#endif

#ifdef USE_ASM
#define EFFECT_DISTORS 4
#else
#define EFFECT_DISTORS 10
#endif


#ifdef USE_ASM

#ifdef MMX
int mmx_zoom ();
guint32 mmx_zoom_size;
#endif /* MMX */

#ifdef POWERPC
extern unsigned int useAltivec;
extern void ppc_zoom (void);
extern void ppc_zoom_altivec (void);
unsigned int ppcsize4;
#endif /* PowerPC */


unsigned int *coeffs = 0, *freecoeffs = 0;
guint32 *expix1 = 0;            /* pointeur exporte vers p1 */
guint32 *expix2 = 0;            /* pointeur exporte vers p2 */
guint32 zoom_width;
#endif /* ASM */


static int firstTime = 1;
static int sintable[0xffff];

ZoomFilterData *
zoomFilterNew (void)
{
  ZoomFilterData *zf = malloc (sizeof (ZoomFilterData));

  zf->vitesse = 128;
  zf->pertedec = 8;
  zf->sqrtperte = 16;
  zf->middleX = 1;
  zf->middleY = 1;
  zf->reverse = 0;
  zf->mode = WAVE_MODE;
  zf->hPlaneEffect = 0;
  zf->vPlaneEffect = 0;
  zf->noisify = 0;
  zf->buffsize = 0;
  zf->res_x = 0;
  zf->res_y = 0;

  zf->buffer = NULL;
  zf->firedec = NULL;

  zf->wave = 0;
  zf->wavesp = 0;

  return zf;
}

/* retourne x>>s , en testant le signe de x */
static inline int
ShiftRight (int x, const unsigned char s)
{
  if (x < 0)
    return -(-x >> s);
  else
    return x >> s;
}

/*
  calculer px et py en fonction de x,y,middleX,middleY et theMode
  px et py indique la nouvelle position (en sqrtperte ieme de pixel)
  (valeur * 16)
*/
static void
calculatePXandPY (GoomData * gd, int x, int y, int *px, int *py)
{
  ZoomFilterData *zf = gd->zfd;
  int middleX, middleY;
  guint32 resoly = zf->res_y;
  int vPlaneEffect = zf->vPlaneEffect;
  int hPlaneEffect = zf->hPlaneEffect;
  int vitesse = zf->vitesse;
  char theMode = zf->mode;

  if (theMode == WATER_MODE) {
    int wavesp = zf->wavesp;
    int wave = zf->wave;
    int yy = y + RAND (gd) % 4 + wave / 10;

    yy -= RAND (gd) % 4;
    if (yy < 0)
      yy = 0;
    if (yy >= resoly)
      yy = resoly - 1;

    *px = (x << 4) + zf->firedec[yy] + (wave / 10);
    *py = (y << 4) + 132 - ((vitesse < 132) ? vitesse : 131);

    wavesp += RAND (gd) % 3;
    wavesp -= RAND (gd) % 3;
    if (wave < -10)
      wavesp += 2;
    if (wave > 10)
      wavesp -= 2;
    wave += (wavesp / 10) + RAND (gd) % 3;
    wave -= RAND (gd) % 3;
    if (wavesp > 100)
      wavesp = (wavesp * 9) / 10;

    zf->wavesp = wavesp;
    zf->wave = wave;
  } else {
    int dist;
    register int vx, vy;
    int fvitesse = vitesse << 4;

    middleX = zf->middleX;
    middleY = zf->middleY;

    if (zf->noisify) {
      x += RAND (gd) % zf->noisify;
      x -= RAND (gd) % zf->noisify;
      y += RAND (gd) % zf->noisify;
      y -= RAND (gd) % zf->noisify;
    }

    if (hPlaneEffect)
      vx = ((x - middleX) << 9) + hPlaneEffect * (y - middleY);
    else
      vx = (x - middleX) << 9;

    if (vPlaneEffect)
      vy = ((y - middleY) << 9) + vPlaneEffect * (x - middleX);
    else
      vy = (y - middleY) << 9;

    switch (theMode) {
      case WAVE_MODE:
        dist =
            ShiftRight (vx, 9) * ShiftRight (vx, 9) + ShiftRight (vy,
            9) * ShiftRight (vy, 9);
        fvitesse *=
            1024 +
            ShiftRight (sintable[(unsigned short) (0xffff * dist *
                    EFFECT_DISTORS)], 6);
        fvitesse /= 1024;
        break;
      case CRYSTAL_BALL_MODE:
        dist =
            ShiftRight (vx, 9) * ShiftRight (vx, 9) + ShiftRight (vy,
            9) * ShiftRight (vy, 9);
        fvitesse += (dist * EFFECT_DISTORS >> 10);
        break;
      case AMULETTE_MODE:
        dist =
            ShiftRight (vx, 9) * ShiftRight (vx, 9) + ShiftRight (vy,
            9) * ShiftRight (vy, 9);
        fvitesse -= (dist * EFFECT_DISTORS >> 4);
        break;
      case SCRUNCH_MODE:
        dist =
            ShiftRight (vx, 9) * ShiftRight (vx, 9) + ShiftRight (vy,
            9) * ShiftRight (vy, 9);
        fvitesse -= (dist * EFFECT_DISTORS >> 9);
        break;
    }
    if (vx < 0)
      *px = (middleX << 4) - (-(vx * fvitesse) >> 16);
    else
      *px = (middleX << 4) + ((vx * fvitesse) >> 16);
    if (vy < 0)
      *py = (middleY << 4) - (-(vy * fvitesse) >> 16);
    else
      *py = (middleY << 4) + ((vy * fvitesse) >> 16);
  }
}

/*#define _DEBUG */

static inline void
setPixelRGB (Uint * buffer, Uint x, Uint y, Color c,
    guint32 resolx, guint32 resoly)
{
/*              buffer[ y*WIDTH + x ] = (c.r<<16)|(c.v<<8)|c.b */
#ifdef _DEBUG_PIXEL
  if (x + y * resolx >= resolx * resoly) {
    fprintf (stderr, "setPixel ERROR : hors du tableau... %i, %i\n", x, y);
    /*exit (1) ; */
  }
#endif

#ifdef USE_DGA
  buffer[y * resolx + x] = (c.b << 16) | (c.v << 8) | c.r;
#else
  buffer[y * resolx + x] = (c.r << 16) | (c.v << 8) | c.b;
#endif
}


static inline void
setPixelRGB_ (Uint * buffer, Uint x, Color c, guint32 resolx, guint32 resoly)
{
#ifdef _DEBUG
  if (x >= resolx * resoly) {
    printf ("setPixel ERROR : hors du tableau... %i >= %i*%i (%i)\n", x, resolx,
        resoly, resolx * resoly);
    exit (1);
  }
#endif

#ifdef USE_DGA
  buffer[x] = (c.b << 16) | (c.v << 8) | c.r;
#else
  buffer[x] = (c.r << 16) | (c.v << 8) | c.b;
#endif
}

static inline void
getPixelRGB_ (Uint * buffer, Uint x, Color * c, guint32 resolx, guint32 resoly)
{
  register unsigned char *tmp8;

#ifdef _DEBUG
  if (x >= resolx * resoly) {
    printf ("getPixel ERROR : hors du tableau... %i\n", x);
    exit (1);
  }
#endif

#ifdef __BIG_ENDIAN__
  c->b = *(unsigned char *) (tmp8 = (unsigned char *) (buffer + x));
  c->r = *(unsigned char *) (++tmp8);
  c->v = *(unsigned char *) (++tmp8);
  c->b = *(unsigned char *) (++tmp8);

#else
  /* ATTENTION AU PETIT INDIEN  */
  tmp8 = (unsigned char *) (buffer + x);
  c->b = *(unsigned char *) (tmp8++);
  c->v = *(unsigned char *) (tmp8++);
  c->r = *(unsigned char *) (tmp8);
/*      *c = (Color) buffer[x+y*WIDTH] ; */
#endif
}

static void
zoomFilterSetResolution (GoomData * gd, ZoomFilterData * zf)
{
  unsigned short us;

  if (zf->buffsize >= gd->buffsize) {
    zf->res_x = gd->resolx;
    zf->res_y = gd->resoly;
    zf->middleX = gd->resolx / 2;
    zf->middleY = gd->resoly - 1;

    return;
  }
#ifndef USE_ASM
  if (zf->buffer)
    free (zf->buffer);
  zf->buffer = 0;
#else
  if (coeffs)
    free (freecoeffs);
  coeffs = 0;
#endif
  zf->middleX = gd->resolx / 2;
  zf->middleY = gd->resoly - 1;
  zf->res_x = gd->resolx;
  zf->res_y = gd->resoly;

  if (zf->firedec)
    free (zf->firedec);
  zf->firedec = 0;

  zf->buffsize = gd->resolx * gd->resoly * sizeof (unsigned int);

#ifdef USE_ASM
  freecoeffs = (unsigned int *)
      malloc (resx * resy * 2 * sizeof (unsigned int) + 128);
  coeffs = (guint32 *) ((1 + ((unsigned int) (freecoeffs)) / 128) * 128);

#else
  zf->buffer = calloc (sizeof (guint32), zf->buffsize * 5);
  zf->pos10 = zf->buffer;
  zf->c[0] = zf->pos10 + zf->buffsize;
  zf->c[1] = zf->c[0] + zf->buffsize;
  zf->c[2] = zf->c[1] + zf->buffsize;
  zf->c[3] = zf->c[2] + zf->buffsize;
#endif
  zf->firedec = (int *) malloc (zf->res_y * sizeof (int));

  if (firstTime) {
    firstTime = 0;

    /* generation d'une table de sinus */
    for (us = 0; us < 0xffff; us++) {
      sintable[us] = (int) (1024.0f * sin (us * 2 * 3.31415f / 0xffff));
    }
  }
}

void
zoomFilterDestroy (ZoomFilterData * zf)
{
  if (zf) {
    if (zf->firedec)
      free (zf->firedec);
    if (zf->buffer)
      free (zf->buffer);
    free (zf);
  }
}

/*===============================================================*/
void
zoomFilterFastRGB (GoomData * goomdata, ZoomFilterData * zf, int zfd_update)
{
  guint32 prevX = goomdata->resolx;
  guint32 prevY = goomdata->resoly;

  guint32 *pix1 = goomdata->p1;
  guint32 *pix2 = goomdata->p2;
  unsigned int *pos10;
  unsigned int **c;

  Uint x, y;

/*  static unsigned int prevX = 0, prevY = 0; */

#ifdef USE_ASM
  expix1 = pix1;
  expix2 = pix2;
#else
  Color couleur;
  Color col1, col2, col3, col4;
  Uint position;
#endif

  if ((goomdata->resolx != zf->res_x) || (goomdata->resoly != zf->res_y)) {
    zoomFilterSetResolution (goomdata, zf);
  }

  pos10 = zf->pos10;
  c = zf->c;

  if (zfd_update) {
    guchar sqrtperte = zf->sqrtperte;
    gint start_y = 0;

    if (zf->reverse)
      zf->vitesse = 256 - zf->vitesse;

    /* generation du buffer */
    for (y = 0; y < zf->res_y; y++) {
      gint y_16 = y << 4;
      gint max_px = (prevX - 1) * sqrtperte;
      gint max_py = (prevY - 1) * sqrtperte;

      for (x = 0; x < zf->res_x; x++) {
        gint px, py;
        guchar coefv, coefh;

        /* calculer px et py en fonction de */
        /*   x,y,middleX,middleY et theMode */
        calculatePXandPY (goomdata, x, y, &px, &py);

        if ((px == x << 4) && (py == y_16))
          py += 8;

        if ((py < 0) || (px < 0) || (py >= max_py) || (px >= max_px)) {
#ifdef USE_ASM
          coeffs[(y * prevX + x) * 2] = 0;
          coeffs[(y * prevX + x) * 2 + 1] = 0;
#else
          pos10[start_y + x] = 0;
          c[0][start_y + x] = 0;
          c[1][start_y + x] = 0;
          c[2][start_y + x] = 0;
          c[3][start_y + x] = 0;
#endif
        } else {
          int npx10;
          int npy10;
          int pos;

          npx10 = (px / sqrtperte);
          npy10 = (py / sqrtperte);

/*                        if (npx10 >= prevX) fprintf(stderr,"error npx:%d",npx10);
                          if (npy10 >= prevY) fprintf(stderr,"error npy:%d",npy10);
*/
          coefh = px % sqrtperte;
          coefv = py % sqrtperte;
#ifdef USE_ASM
          pos = (y * prevX + x) * 2;
          coeffs[pos] = (npx10 + prevX * npy10) * 4;

          if (!(coefh || coefv))
            coeffs[pos + 1] = (sqrtperte * sqrtperte - 1);
          else
            coeffs[pos + 1] = ((sqrtperte - coefh) * (sqrtperte - coefv));

          coeffs[pos + 1] |= (coefh * (sqrtperte - coefv)) << 8;
          coeffs[pos + 1] |= ((sqrtperte - coefh) * coefv) << 16;
          coeffs[pos + 1] |= (coefh * coefv) << 24;
#else
          pos = start_y + x;
          pos10[pos] = npx10 + prevX * npy10;

          if (!(coefh || coefv))
            c[0][pos] = sqrtperte * sqrtperte - 1;
          else
            c[0][pos] = (sqrtperte - coefh) * (sqrtperte - coefv);

          c[1][pos] = coefh * (sqrtperte - coefv);
          c[2][pos] = (sqrtperte - coefh) * coefv;
          c[3][pos] = coefh * coefv;
#endif
        }
      }
      /* Advance start of line index */
      start_y += prevX;
    }
  }
#ifdef USE_ASM
#ifdef MMX
  zoom_width = prevX;
  mmx_zoom_size = prevX * prevY;
  mmx_zoom ();
#endif

#ifdef POWERPC
  zoom_width = prevX;
  if (useAltivec) {
    ppcsize4 = ((unsigned int) (prevX * prevY)) / 4;
    ppc_zoom_altivec ();
  } else {
    ppcsize4 = ((unsigned int) (prevX * prevY));
    ppc_zoom ();
  }
#endif
#else
  for (position = 0; position < prevX * prevY; position++) {
    getPixelRGB_ (pix1, pos10[position], &col1, goomdata->resolx,
        goomdata->resoly);
    getPixelRGB_ (pix1, pos10[position] + 1, &col2, goomdata->resolx,
        goomdata->resoly);
    getPixelRGB_ (pix1, pos10[position] + prevX, &col3, goomdata->resolx,
        goomdata->resoly);
    getPixelRGB_ (pix1, pos10[position] + prevX + 1, &col4, goomdata->resolx,
        goomdata->resoly);

    couleur.r = col1.r * c[0][position]
        + col2.r * c[1][position]
        + col3.r * c[2][position]
        + col4.r * c[3][position];
    couleur.r >>= zf->pertedec;

    couleur.v = col1.v * c[0][position]
        + col2.v * c[1][position]
        + col3.v * c[2][position]
        + col4.v * c[3][position];
    couleur.v >>= zf->pertedec;

    couleur.b = col1.b * c[0][position]
        + col2.b * c[1][position]
        + col3.b * c[2][position]
        + col4.b * c[3][position];
    couleur.b >>= zf->pertedec;

    setPixelRGB_ (pix2, position, couleur, goomdata->resolx, goomdata->resoly);
  }
#endif
}


void
pointFilter (GoomData * goomdata, Color c,
    float t1, float t2, float t3, float t4, Uint cycle)
{
  Uint *pix1 = goomdata->p1;
  ZoomFilterData *zf = goomdata->zfd;
  Uint x = (Uint) (zf->middleX + (int) (t1 * cos ((float) cycle / t3)));
  Uint y = (Uint) (zf->middleY + (int) (t2 * sin ((float) cycle / t4)));

  if ((x > 1) && (y > 1) && (x < goomdata->resolx - 2)
      && (y < goomdata->resoly - 2)) {
    setPixelRGB (pix1, x + 1, y, c, goomdata->resolx, goomdata->resoly);
    setPixelRGB (pix1, x, y + 1, c, goomdata->resolx, goomdata->resoly);
    setPixelRGB (pix1, x + 1, y + 1, WHITE, goomdata->resolx, goomdata->resoly);
    setPixelRGB (pix1, x + 2, y + 1, c, goomdata->resolx, goomdata->resoly);
    setPixelRGB (pix1, x + 1, y + 2, c, goomdata->resolx, goomdata->resoly);
  }
}
