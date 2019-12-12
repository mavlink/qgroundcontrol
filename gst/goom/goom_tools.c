/* Goom Project
 * Copyright (C) <2003> iOS-Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#include "goom_tools.h"
#include <stdlib.h>

GoomRandom *
goom_random_init (int i)
{
  GoomRandom *grandom = (GoomRandom *) malloc (sizeof (GoomRandom));

  srand (i);
  grandom->pos = 1;
  goom_random_update_array (grandom, GOOM_NB_RAND);
  return grandom;
}

void
goom_random_free (GoomRandom * grandom)
{
  free (grandom);
}

void
goom_random_update_array (GoomRandom * grandom, int numberOfValuesToChange)
{
  while (numberOfValuesToChange > 0) {
#if RAND_MAX < 0x10000
    grandom->array[grandom->pos++] = ((rand () << 16) + rand ()) / 127;
#else
    grandom->array[grandom->pos++] = rand () / 127;
#endif
    numberOfValuesToChange--;
  }
}
