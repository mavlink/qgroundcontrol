/*
 *  lines.c
 *  iTunesXPlugIn
 *
 *  Created by guillaum on Tue Aug 14 2001.
 *  Copyright (c) 2001 __CompanyName__. All rights reserved.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "lines.h"
#include <math.h>

static inline unsigned char
lighten (unsigned char value, unsigned char power)
{
  unsigned char i;

  for (i = 0; i < power; i++)
    value += (255 - value) / 5;
  return value;
}

void
goom_lines (GoomData * goomdata, gint16 data[2][512], unsigned int ID,
    unsigned int *p, guint32 power)
{
  guint32 color1;
  guint32 color2;
  guint32 resolx = goomdata->resolx;
  guint32 resoly = goomdata->resoly;
  unsigned char *color = 1 + (unsigned char *) &color1;

  switch (ID) {
    case 0:                    /* Horizontal stereo lines */
    {
      color1 = 0x0000AA00;
      color2 = 0x00AA0000;
      break;
    }

    case 1:                    /* Stereo circles */
    {
      color1 = 0x00AA33DD;
      color2 = 0x00AA33DD;
      break;
    }
    default:{
      color1 = color2 = 0;
      g_assert_not_reached ();
      break;
    }
  }
  *color = lighten (*color, power);
  color++;
  *color = lighten (*color, power);
  color++;
  *color = lighten (*color, power);
  color = 1 + (unsigned char *) &color2;
  *color = lighten (*color, power);
  color++;
  *color = lighten (*color, power);
  color++;
  *color = lighten (*color, power);

  switch (ID) {
    case 0:                    /* Horizontal stereo lines */
    {
      unsigned int i;

      for (i = 0; i < 512; i++) {
        guint32 plot;

        plot = i * resolx / 512 + (resoly / 4 + data[0][i] / 1600) * resolx;
        p[plot] = color1;
        p[plot + 1] = color1;
        plot = i * resolx / 512 + (resoly * 3 / 4 - data[1][i] / 1600) * resolx;
        p[plot] = color2;
        p[plot + 1] = color2;
      }
      break;
    }

    case 1:                    /* Stereo circles */
    {
      float z;
      unsigned int monX = resolx / 2;
      float monY = (float) resoly / 4;
      float monY2 = (float) resoly / 2;

      for (z = 0; z < 6.2832f; z += 1.0f / monY) {
        /* float offset1 = 128+data[1][(unsigned int)(z*81.33f)])/200000; */
        p[monX + (unsigned int) ((monY + ((float) resoly) * (128 +
                        data[1][(unsigned int) (z * 81.33f)]) / 200000) *
                cos (z) + resolx * (unsigned int) (monY2 + (monY +
                        ((float) resoly) * (128 +
                            data[1][(unsigned int) (z * 81.33f)]) / 400000) *
                    sin (z)))] = color1;
        p[monX + (unsigned int) ((monY - ((float) resoly) * (128 +
                        data[0][(unsigned int) (z * 81.33f)]) / 200000) *
                cos (z) + resolx * (unsigned int) (monY2 + (monY -
                        ((float) resoly) * (128 +
                            data[0][(unsigned int) (z * 81.33f)]) / 400000) *
                    sin (z)))] = color2;
      }
      break;
    }
  }
}
