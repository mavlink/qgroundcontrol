#ifndef _GOOMCORE_H
#define _GOOMCORE_H

#include <glib.h>

typedef struct ZoomFilterData ZoomFilterData;

typedef struct
{
/*-----------------------------------------------------*
 *  SHARED DATA                                        *
 *-----------------------------------------------------*/
  guint32 *pixel;
  guint32 *back;
  guint32 *p1, *p2;
  guint32 cycle;

  guint32 resolx, resoly, buffsize;

  int lockvar;       /* pour empecher de nouveaux changements */
  int goomvar;       /* boucle des gooms */
  int totalgoom;     /* nombre de gooms par seconds */
  int agoom;         /* un goom a eu lieu..       */
  int loopvar;       /* mouvement des points */
  int speedvar;      /* vitesse des particules */
  int lineMode;      /* l'effet lineaire a dessiner */
  char goomlimit;    /* sensibilité du goom */

  ZoomFilterData *zfd;
  
  /* Random table */
  gint  *rand_tab;
  guint  rand_pos;
} GoomData;

void goom_init (GoomData *goomdata, guint32 resx, guint32 resy);
void goom_set_resolution (GoomData *goomdata, guint32 resx, guint32 resy);

guint32 *goom_update (GoomData *goomdata, gint16 data [2][512]);

void goom_close (GoomData *goomdata);

#endif
