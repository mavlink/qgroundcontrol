#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <glib.h>

#include <stdlib.h>
#include <string.h>
#include "goom_core.h"
#include "goom_tools.h"
#include "filters.h"
#include "lines.h"

/*#define VERBOSE */

#ifdef VERBOSE
#include <stdio.h>
#endif

#define STOP_SPEED 128

void
goom_init (GoomData * goomdata, guint32 resx, guint32 resy)
{
#ifdef VERBOSE
  printf ("GOOM: init (%d, %d);\n", resx, resy);
#endif
  goomdata->resolx = 0;
  goomdata->resoly = 0;
  goomdata->buffsize = 0;

  goomdata->pixel = NULL;
  goomdata->back = NULL;
  goomdata->p1 = NULL;
  goomdata->p2 = NULL;

  goom_set_resolution (goomdata, resx, resy);
  RAND_INIT (goomdata, GPOINTER_TO_INT (goomdata->pixel));
  goomdata->cycle = 0;


  goomdata->goomlimit = 2;      /* sensibilité du goom */
  goomdata->zfd = zoomFilterNew ();
  goomdata->lockvar = 0;        /* pour empecher de nouveaux changements */
  goomdata->goomvar = 0;        /* boucle des gooms */
  goomdata->totalgoom = 0;      /* nombre de gooms par seconds */
  goomdata->agoom = 0;          /* un goom a eu lieu..       */
  goomdata->loopvar = 0;        /* mouvement des points */
  goomdata->speedvar = 0;       /* vitesse des particules */
  goomdata->lineMode = 0;       /* l'effet lineaire a dessiner */
}

void
goom_set_resolution (GoomData * goomdata, guint32 resx, guint32 resy)
{
  guint32 buffsize = resx * resy;

  if ((goomdata->resolx == resx) && (goomdata->resoly == resy))
    return;

  if (goomdata->buffsize < buffsize) {
    if (goomdata->pixel)
      free (goomdata->pixel);
    if (goomdata->back)
      free (goomdata->back);
    goomdata->pixel = (guint32 *) malloc (buffsize * sizeof (guint32) + 128);
    goomdata->back = (guint32 *) malloc (buffsize * sizeof (guint32) + 128);
    goomdata->buffsize = buffsize;

    goomdata->p1 = (void *) (((guintptr) goomdata->pixel + 0x7f) & (~0x7f));
    goomdata->p2 = (void *) (((guintptr) goomdata->back + 0x7f) & (~0x7f));
  }

  goomdata->resolx = resx;
  goomdata->resoly = resy;

  memset (goomdata->pixel, 0, buffsize * sizeof (guint32) + 128);
  memset (goomdata->back, 0, buffsize * sizeof (guint32) + 128);
}

guint32 *
goom_update (GoomData * goomdata, gint16 data[2][512])
{
  guint32 *return_val;
  guint32 pointWidth;
  guint32 pointHeight;
  int incvar;                   /* volume du son */
  int accelvar;                 /* acceleration des particules */
  int i;
  float largfactor;             /* elargissement de l'intervalle d'évolution des points */
  int zfd_update = 0;
  int resolx = goomdata->resolx;
  int resoly = goomdata->resoly;
  ZoomFilterData *pzfd = goomdata->zfd;
  guint32 *tmp;

  /* test if the config has changed, update it if so */

  pointWidth = (resolx * 2) / 5;
  pointHeight = (resoly * 2) / 5;

  /* ! etude du signal ... */
  incvar = 0;
  for (i = 0; i < 512; i++) {
    if (incvar < data[0][i])
      incvar = data[0][i];
  }

  accelvar = incvar / 5000;
  if (goomdata->speedvar > 5) {
    accelvar--;
    if (goomdata->speedvar > 20)
      accelvar--;
    if (goomdata->speedvar > 40)
      goomdata->speedvar = 40;
  }
  accelvar--;
  goomdata->speedvar += accelvar;

  if (goomdata->speedvar < 0)
    goomdata->speedvar = 0;
  if (goomdata->speedvar > 40)
    goomdata->speedvar = 40;


  /* ! calcul du deplacement des petits points ... */

  largfactor =
      ((float) goomdata->speedvar / 40.0f + (float) incvar / 50000.0f) / 1.5f;
  if (largfactor > 1.5f)
    largfactor = 1.5f;

  for (i = 1; i * 15 <= goomdata->speedvar + 15; i++) {
    goomdata->loopvar += goomdata->speedvar + 1;

    pointFilter (goomdata,
        YELLOW,
        ((pointWidth - 6.0f) * largfactor + 5.0f),
        ((pointHeight - 6.0f) * largfactor + 5.0f),
        i * 152.0f, 128.0f, goomdata->loopvar + i * 2032);
    pointFilter (goomdata, ORANGE,
        ((pointWidth / 2) * largfactor) / i + 10.0f * i,
        ((pointHeight / 2) * largfactor) / i + 10.0f * i,
        96.0f, i * 80.0f, goomdata->loopvar / i);
    pointFilter (goomdata, VIOLET,
        ((pointHeight / 3 + 5.0f) * largfactor) / i + 10.0f * i,
        ((pointHeight / 3 + 5.0f) * largfactor) / i + 10.0f * i,
        i + 122.0f, 134.0f, goomdata->loopvar / i);
    pointFilter (goomdata, BLACK,
        ((pointHeight / 3) * largfactor + 20.0f),
        ((pointHeight / 3) * largfactor + 20.0f),
        58.0f, i * 66.0f, goomdata->loopvar / i);
    pointFilter (goomdata, WHITE,
        (pointHeight * largfactor + 10.0f * i) / i,
        (pointHeight * largfactor + 10.0f * i) / i,
        66.0f, 74.0f, goomdata->loopvar + i * 500);
  }

  /* diminuer de 1 le temps de lockage */
  /* note pour ceux qui n'ont pas suivis : le lockvar permet d'empecher un */
  /* changement d'etat du plugins juste apres un autre changement d'etat. oki ? */
  if (--goomdata->lockvar < 0)
    goomdata->lockvar = 0;

  /* temps du goom */
  if (--goomdata->agoom < 0)
    goomdata->agoom = 0;

  /* on verifie qu'il ne se pas un truc interressant avec le son. */
  if ((accelvar > goomdata->goomlimit) || (accelvar < -goomdata->goomlimit)) {
    /* UN GOOM !!! YAHOO ! */
    goomdata->totalgoom++;
    goomdata->agoom = 20;       /* mais pdt 20 cycles, il n'y en aura plus. */
    goomdata->lineMode = (goomdata->lineMode + 1) % 20; /* Tous les 10 gooms on change de mode lineaire */

    /* changement eventuel de mode */
    switch (iRAND (goomdata, 10)) {
      case 0:
      case 1:
      case 2:
        pzfd->mode = WAVE_MODE;
        pzfd->vitesse = STOP_SPEED - 1;
        pzfd->reverse = 0;
        break;
      case 3:
      case 4:
        pzfd->mode = CRYSTAL_BALL_MODE;
        break;
      case 5:
        pzfd->mode = AMULETTE_MODE;
        break;
      case 6:
        pzfd->mode = WATER_MODE;
        break;
      case 7:
        pzfd->mode = SCRUNCH_MODE;
        break;
      default:
        pzfd->mode = NORMAL_MODE;
    }
  }

  /* tout ceci ne sera fait qu'en cas de non-blocage */
  if (goomdata->lockvar == 0) {
    /* reperage de goom (acceleration forte de l'acceleration du volume) */
    /* -> coup de boost de la vitesse si besoin.. */
    if ((accelvar > goomdata->goomlimit) || (accelvar < -goomdata->goomlimit)) {
      goomdata->goomvar++;
      /*if (goomvar % 1 == 0) */
      {
        guint32 vtmp;
        guint32 newvit;

        newvit = STOP_SPEED - goomdata->speedvar / 2;
        /* retablir le zoom avant.. */
        if ((pzfd->reverse) && (!(goomdata->cycle % 12)) && (rand () % 3 == 0)) {
          pzfd->reverse = 0;
          pzfd->vitesse = STOP_SPEED - 2;
          goomdata->lockvar = 50;
        }
        if (iRAND (goomdata, 10) == 0) {
          pzfd->reverse = 1;
          goomdata->lockvar = 100;
        }

        /* changement de milieu.. */
        switch (iRAND (goomdata, 20)) {
          case 0:
            pzfd->middleY = resoly - 1;
            pzfd->middleX = resolx / 2;
            break;
          case 1:
            pzfd->middleX = resolx - 1;
            break;
          case 2:
            pzfd->middleX = 1;
            break;
          default:
            pzfd->middleY = resoly / 2;
            pzfd->middleX = resolx / 2;
        }

        if (pzfd->mode == WATER_MODE) {
          pzfd->middleX = resolx / 2;
          pzfd->middleY = resoly / 2;
        }

        switch (vtmp = (iRAND (goomdata, 27))) {
          case 0:
            pzfd->vPlaneEffect = iRAND (goomdata, 3);
            pzfd->vPlaneEffect -= iRAND (goomdata, 3);
            pzfd->hPlaneEffect = iRAND (goomdata, 3);
            pzfd->hPlaneEffect -= iRAND (goomdata, 3);
            break;
          case 3:
            pzfd->vPlaneEffect = 0;
            pzfd->hPlaneEffect = iRAND (goomdata, 8);
            pzfd->hPlaneEffect -= iRAND (goomdata, 8);
            break;
          case 4:
          case 5:
          case 6:
          case 7:
            pzfd->vPlaneEffect = iRAND (goomdata, 5);
            pzfd->vPlaneEffect -= iRAND (goomdata, 5);
            pzfd->hPlaneEffect = -pzfd->vPlaneEffect;
            break;
          case 8:
            pzfd->hPlaneEffect = 5 + iRAND (goomdata, 8);
            pzfd->vPlaneEffect = -pzfd->hPlaneEffect;
            break;
          case 9:
            pzfd->vPlaneEffect = 5 + iRAND (goomdata, 8);
            pzfd->hPlaneEffect = -pzfd->hPlaneEffect;
            break;
          case 13:
            pzfd->hPlaneEffect = 0;
            pzfd->vPlaneEffect = iRAND (goomdata, 10);
            pzfd->vPlaneEffect -= iRAND (goomdata, 10);
            break;
          default:
            if (vtmp < 10) {
              pzfd->vPlaneEffect = 0;
              pzfd->hPlaneEffect = 0;
            }
        }

        if (iRAND (goomdata, 3) != 0)
          pzfd->noisify = 0;
        else {
          pzfd->noisify = iRAND (goomdata, 3) + 2;
          goomdata->lockvar *= 3;
        }

        if (pzfd->mode == AMULETTE_MODE) {
          pzfd->vPlaneEffect = 0;
          pzfd->hPlaneEffect = 0;
          pzfd->noisify = 0;
        }

        if ((pzfd->middleX == 1) || (pzfd->middleX == resolx - 1)) {
          pzfd->vPlaneEffect = 0;
          pzfd->hPlaneEffect = iRAND (goomdata, 2) ? 0 : pzfd->hPlaneEffect;
        }

        if (newvit < pzfd->vitesse) {   /* on accelere */
          zfd_update = 1;
          if (((newvit < STOP_SPEED - 7) &&
                  (pzfd->vitesse < STOP_SPEED - 6) &&
                  (goomdata->cycle % 3 == 0)) || (iRAND (goomdata, 40) == 0)) {
            pzfd->vitesse = STOP_SPEED - 1;
            pzfd->reverse = !pzfd->reverse;
          } else {
            pzfd->vitesse = (newvit + pzfd->vitesse * 4) / 5;
          }
          goomdata->lockvar += 50;
        }
      }
    }
    /* mode mega-lent */
    if (iRAND (goomdata, 1000) == 0) {
      /* 
         printf ("coup du sort...\n") ;
       */
      zfd_update = 1;
      pzfd->vitesse = STOP_SPEED - 1;
      pzfd->pertedec = 8;
      pzfd->sqrtperte = 16;
      goomdata->goomvar = 1;
      goomdata->lockvar += 70;
    }
  }

  /* gros frein si la musique est calme */
  if ((goomdata->speedvar < 1) && (pzfd->vitesse < STOP_SPEED - 4)
      && (goomdata->cycle % 16 == 0)) {
    /*
       printf ("++slow part... %i\n", zfd.vitesse) ;
     */
    zfd_update = 1;
    pzfd->vitesse += 3;
    pzfd->pertedec = 8;
    pzfd->sqrtperte = 16;
    goomdata->goomvar = 0;
    /*
       printf ("--slow part... %i\n", zfd.vitesse) ;
     */
  }

  /* baisser regulierement la vitesse... */
  if ((goomdata->cycle % 73 == 0) && (pzfd->vitesse < STOP_SPEED - 5)) {
    /*
       printf ("slow down...\n") ;
     */
    zfd_update = 1;
    pzfd->vitesse++;
  }

  /* arreter de decrémenter au bout d'un certain temps */
  if ((goomdata->cycle % 101 == 0) && (pzfd->pertedec == 7)) {
    zfd_update = 1;
    pzfd->pertedec = 8;
    pzfd->sqrtperte = 16;
  }

  /* Zoom here ! */
  zoomFilterFastRGB (goomdata, pzfd, zfd_update);

  /* si on est dans un goom : afficher les lignes... */
  if (goomdata->agoom > 15)
    goom_lines (goomdata, data, ((pzfd->middleX == resolx / 2)
            && (pzfd->middleY == resoly / 2)
            && (pzfd->mode != WATER_MODE))
        ? (goomdata->lineMode / 10) : 0, goomdata->p2, goomdata->agoom - 15);

  return_val = goomdata->p2;
  tmp = goomdata->p1;
  goomdata->p1 = goomdata->p2;
  goomdata->p2 = tmp;

  /* affichage et swappage des buffers.. */
  goomdata->cycle++;

  /* tous les 100 cycles : vérifier si le taux de goom est correct */
  /* et le modifier sinon.. */
  if (!(goomdata->cycle % 100)) {
    if (goomdata->totalgoom > 15) {
      /*  printf ("less gooms\n") ; */
      goomdata->goomlimit++;
    } else {
      if ((goomdata->totalgoom == 0) && (goomdata->goomlimit > 1))
        goomdata->goomlimit--;
    }
    goomdata->totalgoom = 0;
  }
  return return_val;
}

void
goom_close (GoomData * goomdata)
{
  if (goomdata->pixel != NULL)
    free (goomdata->pixel);
  if (goomdata->back != NULL)
    free (goomdata->back);
  if (goomdata->zfd != NULL) {
    zoomFilterDestroy (goomdata->zfd);
    goomdata->zfd = NULL;
  }
  goomdata->pixel = goomdata->back = NULL;
  RAND_CLOSE (goomdata);
}
