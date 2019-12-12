/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "paint.h"
#include "gstmask.h"

enum
{
  BOX_VERTICAL = 1,
  BOX_HORIZONTAL = 2,
  BOX_CLOCK = 3,
  TRIGANLE_LINEAR = 4
};

static const gint boxes_1b[][7] = {
#define WIPE_B1_1       0
  {BOX_VERTICAL, 0, 0, 0, 1, 1, 1},
#define WIPE_B1_2       1
  {BOX_HORIZONTAL, 0, 0, 0, 1, 1, 1}
};

static const gint boxes_2b[][7 * 2] = {
#define WIPE_B2_21      0
  {BOX_VERTICAL, 0, 0, 1, 1, 2, 0,
      BOX_VERTICAL, 1, 0, 0, 2, 2, 1},
#define WIPE_B2_22      1
  {BOX_HORIZONTAL, 0, 0, 1, 2, 1, 0,
      BOX_HORIZONTAL, 0, 1, 0, 2, 2, 1},
};

static const gint box_clock_1b[][1 * 10] = {
#define WIPE_B1_241     0
  {BOX_CLOCK, 0, 0, 0, 1, 0, 0, 0, 1, 1},
#define WIPE_B1_242     1
  {BOX_CLOCK, 0, 1, 0, 1, 1, 0, 0, 0, 1},
#define WIPE_B1_243     2
  {BOX_CLOCK, 1, 1, 0, 0, 1, 0, 1, 0, 1},
#define WIPE_B1_244     3
  {BOX_CLOCK, 1, 0, 0, 0, 0, 0, 1, 1, 1},
};

#define WIPE_B2_221     0
static const gint box_clock_2b[][2 * 10] = {
#define WIPE_B2_221     0
  {BOX_CLOCK, 1, 0, 0, 2, 0, 0, 1, 2, 1,
      BOX_CLOCK, 1, 0, 0, 1, 2, 1, 0, 0, 2},
#define WIPE_B2_222     1
  {BOX_CLOCK, 2, 1, 0, 2, 2, 0, 0, 1, 1,
      BOX_CLOCK, 2, 1, 0, 0, 1, 1, 2, 0, 2},
#define WIPE_B2_223     2
  {BOX_CLOCK, 1, 2, 0, 0, 2, 0, 1, 0, 1,
      BOX_CLOCK, 1, 2, 0, 1, 0, 1, 2, 2, 2},
#define WIPE_B2_224     3
  {BOX_CLOCK, 0, 1, 0, 0, 0, 0, 2, 1, 1,
      BOX_CLOCK, 0, 1, 0, 2, 1, 1, 0, 2, 2},
#define WIPE_B2_225     4
  {BOX_CLOCK, 1, 0, 0, 2, 0, 0, 1, 2, 1,
      BOX_CLOCK, 1, 2, 0, 0, 2, 0, 1, 0, 1},
#define WIPE_B2_226     5
  {BOX_CLOCK, 0, 1, 0, 0, 0, 0, 2, 1, 1,
      BOX_CLOCK, 2, 1, 0, 2, 2, 0, 0, 1, 1},
#define WIPE_B2_231     6
  {BOX_CLOCK, 1, 0, 0, 1, 2, 0, 2, 0, 1,
      BOX_CLOCK, 1, 0, 0, 1, 2, 0, 0, 0, 1},
#define WIPE_B2_232     7
  {BOX_CLOCK, 2, 1, 0, 0, 1, 0, 2, 0, 1,
      BOX_CLOCK, 2, 1, 0, 0, 1, 0, 2, 2, 1},
#define WIPE_B2_233     8
  {BOX_CLOCK, 1, 2, 0, 1, 0, 0, 2, 2, 1,
      BOX_CLOCK, 1, 2, 0, 1, 0, 0, 0, 2, 1},
#define WIPE_B2_234     9
  {BOX_CLOCK, 0, 1, 0, 2, 1, 0, 0, 0, 1,
      BOX_CLOCK, 0, 1, 0, 2, 1, 0, 0, 2, 1},
#define WIPE_B2_251     10
  {BOX_CLOCK, 0, 0, 0, 1, 0, 0, 0, 2, 1,
      BOX_CLOCK, 2, 0, 0, 1, 0, 0, 2, 2, 1},
#define WIPE_B2_252     11
  {BOX_CLOCK, 0, 0, 0, 0, 1, 0, 2, 0, 1,
      BOX_CLOCK, 0, 2, 0, 0, 1, 0, 2, 2, 1},
#define WIPE_B2_253     12
  {BOX_CLOCK, 0, 2, 0, 1, 2, 0, 0, 0, 1,
      BOX_CLOCK, 2, 2, 0, 1, 2, 0, 2, 0, 1},
#define WIPE_B2_254     13
  {BOX_CLOCK, 2, 0, 0, 2, 1, 0, 0, 0, 1,
      BOX_CLOCK, 2, 2, 0, 2, 1, 0, 0, 2, 1},
};

static const gint box_clock_4b[][4 * 10] = {
#define WIPE_B4_201     0
  {BOX_CLOCK, 1, 1, 0, 1, 0, 0, 2, 1, 1,
        BOX_CLOCK, 1, 1, 0, 2, 1, 1, 1, 2, 2,
        BOX_CLOCK, 1, 1, 0, 1, 2, 2, 0, 1, 3,
      BOX_CLOCK, 1, 1, 0, 0, 1, 3, 1, 0, 4},
#define WIPE_B4_202     1
  {BOX_CLOCK, 1, 1, 0, 1, 0, 3, 2, 1, 4,
        BOX_CLOCK, 1, 1, 0, 2, 1, 0, 1, 2, 1,
        BOX_CLOCK, 1, 1, 0, 1, 2, 1, 0, 1, 2,
      BOX_CLOCK, 1, 1, 0, 0, 1, 2, 1, 0, 3},
#define WIPE_B4_203     2
  {BOX_CLOCK, 1, 1, 0, 1, 0, 2, 2, 1, 3,
        BOX_CLOCK, 1, 1, 0, 2, 1, 3, 1, 2, 4,
        BOX_CLOCK, 1, 1, 0, 1, 2, 0, 0, 1, 1,
      BOX_CLOCK, 1, 1, 0, 0, 1, 1, 1, 0, 2},
#define WIPE_B4_204     3
  {BOX_CLOCK, 1, 1, 0, 1, 0, 1, 2, 1, 2,
        BOX_CLOCK, 1, 1, 0, 2, 1, 2, 1, 2, 3,
        BOX_CLOCK, 1, 1, 0, 1, 2, 3, 0, 1, 4,
      BOX_CLOCK, 1, 1, 0, 0, 1, 0, 1, 0, 1},
#define WIPE_B4_205     4
  {BOX_CLOCK, 1, 1, 0, 1, 0, 0, 2, 1, 1,
        BOX_CLOCK, 1, 1, 0, 2, 1, 1, 1, 2, 2,
        BOX_CLOCK, 1, 1, 0, 1, 2, 0, 0, 1, 1,
      BOX_CLOCK, 1, 1, 0, 0, 1, 1, 1, 0, 2},
#define WIPE_B4_206     5
  {BOX_CLOCK, 1, 1, 0, 1, 0, 1, 2, 1, 2,
        BOX_CLOCK, 1, 1, 0, 2, 1, 0, 1, 2, 1,
        BOX_CLOCK, 1, 1, 0, 1, 2, 1, 0, 1, 2,
      BOX_CLOCK, 1, 1, 0, 0, 1, 0, 1, 0, 1},
#define WIPE_B4_207     6
  {BOX_CLOCK, 1, 1, 0, 1, 0, 0, 2, 1, 1,
        BOX_CLOCK, 1, 1, 0, 2, 1, 0, 1, 2, 1,
        BOX_CLOCK, 1, 1, 0, 1, 2, 0, 0, 1, 1,
      BOX_CLOCK, 1, 1, 0, 0, 1, 0, 1, 0, 1},
#define WIPE_B4_211     7
  {BOX_CLOCK, 1, 1, 0, 1, 0, 0, 2, 1, 1,
        BOX_CLOCK, 1, 1, 0, 2, 1, 1, 1, 2, 2,
        BOX_CLOCK, 1, 1, 0, 1, 0, 0, 0, 1, 1,
      BOX_CLOCK, 1, 1, 0, 0, 1, 1, 1, 2, 2},
#define WIPE_B4_212     8
  {BOX_CLOCK, 1, 1, 0, 2, 1, 0, 1, 0, 1,
        BOX_CLOCK, 1, 1, 0, 1, 0, 1, 0, 1, 2,
        BOX_CLOCK, 1, 1, 0, 2, 1, 0, 1, 2, 1,
      BOX_CLOCK, 1, 1, 0, 1, 2, 1, 0, 1, 2},
#define WIPE_B4_213     9
  {BOX_CLOCK, 1, 1, 0, 1, 0, 0, 2, 1, 1,
        BOX_CLOCK, 1, 1, 0, 1, 0, 0, 0, 1, 1,
        BOX_CLOCK, 1, 1, 0, 1, 2, 0, 2, 1, 1,
      BOX_CLOCK, 1, 1, 0, 1, 2, 0, 0, 1, 1},
#define WIPE_B4_214     10
  {BOX_CLOCK, 1, 1, 0, 2, 1, 0, 1, 0, 1,
        BOX_CLOCK, 1, 1, 0, 2, 1, 0, 1, 2, 1,
        BOX_CLOCK, 1, 1, 0, 0, 1, 0, 1, 0, 1,
      BOX_CLOCK, 1, 1, 0, 0, 1, 0, 1, 2, 1},
#define WIPE_B4_227     11
  {BOX_CLOCK, 1, 0, 0, 2, 0, 0, 1, 1, 1,
        BOX_CLOCK, 1, 0, 0, 1, 1, 1, 0, 0, 2,
        BOX_CLOCK, 1, 2, 0, 2, 2, 0, 1, 1, 1,
      BOX_CLOCK, 1, 2, 0, 1, 1, 1, 0, 2, 2},
#define WIPE_B4_228     12
  {BOX_CLOCK, 0, 1, 0, 0, 0, 0, 1, 1, 1,
        BOX_CLOCK, 0, 1, 0, 1, 1, 1, 0, 2, 2,
        BOX_CLOCK, 2, 1, 0, 2, 0, 0, 1, 1, 1,
      BOX_CLOCK, 2, 1, 0, 1, 1, 1, 2, 2, 2},
#define WIPE_B4_235     13
  {BOX_CLOCK, 1, 0, 0, 1, 1, 0, 0, 0, 1,
        BOX_CLOCK, 1, 0, 0, 1, 1, 0, 2, 0, 1,
        BOX_CLOCK, 1, 2, 0, 1, 1, 0, 2, 2, 1,
      BOX_CLOCK, 1, 2, 0, 1, 1, 0, 0, 2, 1},
#define WIPE_B4_236     14
  {BOX_CLOCK, 0, 1, 0, 1, 1, 0, 0, 0, 1,
        BOX_CLOCK, 0, 1, 0, 1, 1, 0, 0, 2, 1,
        BOX_CLOCK, 2, 1, 0, 1, 1, 0, 2, 0, 1,
      BOX_CLOCK, 2, 1, 0, 1, 1, 0, 2, 2, 1},
};

static const gint box_clock_8b[][8 * 10] = {
#define WIPE_B8_261     0
  {BOX_CLOCK, 2, 1, 0, 2, 2, 0, 4, 1, 1,
        BOX_CLOCK, 2, 1, 0, 4, 1, 1, 2, 0, 2,
        BOX_CLOCK, 2, 1, 0, 2, 0, 2, 0, 1, 3,
        BOX_CLOCK, 2, 1, 0, 0, 1, 3, 2, 2, 4,
        BOX_CLOCK, 2, 3, 0, 2, 2, 0, 4, 3, 1,
        BOX_CLOCK, 2, 3, 0, 4, 3, 1, 2, 4, 2,
        BOX_CLOCK, 2, 3, 0, 2, 4, 2, 0, 3, 3,
      BOX_CLOCK, 2, 3, 0, 0, 3, 3, 2, 2, 4},
#define WIPE_B8_262     1
  {BOX_CLOCK, 1, 2, 0, 2, 2, 0, 1, 0, 1,
        BOX_CLOCK, 1, 2, 0, 1, 0, 1, 0, 2, 2,
        BOX_CLOCK, 1, 2, 0, 0, 2, 2, 1, 4, 3,
        BOX_CLOCK, 1, 2, 0, 1, 4, 3, 2, 2, 4,
        BOX_CLOCK, 3, 2, 0, 2, 2, 0, 3, 0, 1,
        BOX_CLOCK, 3, 2, 0, 3, 0, 1, 4, 2, 2,
        BOX_CLOCK, 3, 2, 0, 4, 2, 2, 3, 4, 3,
      BOX_CLOCK, 3, 2, 0, 3, 4, 3, 2, 2, 4},
#define WIPE_B8_263     2
  {BOX_CLOCK, 2, 1, 0, 2, 0, 0, 4, 1, 1,
        BOX_CLOCK, 2, 1, 0, 4, 1, 1, 2, 2, 2,
        BOX_CLOCK, 2, 1, 0, 2, 0, 0, 0, 1, 1,
        BOX_CLOCK, 2, 1, 0, 0, 1, 1, 2, 2, 2,
        BOX_CLOCK, 2, 3, 0, 2, 4, 0, 4, 3, 1,
        BOX_CLOCK, 2, 3, 0, 4, 3, 1, 2, 2, 2,
        BOX_CLOCK, 2, 3, 0, 2, 4, 0, 0, 3, 1,
      BOX_CLOCK, 2, 3, 0, 0, 3, 1, 2, 2, 2},
#define WIPE_B8_264     3
  {BOX_CLOCK, 1, 2, 0, 0, 2, 0, 1, 0, 1,
        BOX_CLOCK, 1, 2, 0, 1, 0, 1, 2, 2, 2,
        BOX_CLOCK, 1, 2, 0, 0, 2, 0, 1, 4, 1,
        BOX_CLOCK, 1, 2, 0, 1, 4, 1, 2, 2, 2,
        BOX_CLOCK, 3, 2, 0, 4, 2, 0, 3, 0, 1,
        BOX_CLOCK, 3, 2, 0, 3, 0, 1, 2, 2, 2,
        BOX_CLOCK, 3, 2, 0, 4, 2, 0, 3, 4, 1,
      BOX_CLOCK, 3, 2, 0, 3, 4, 1, 2, 2, 2},
};

static const gint triangles_2t[][2 * 9] = {
  /* 3 -> 6 */
#define WIPE_T2_3       0
  {0, 0, 0, 0, 1, 1, 1, 1, 1,
      1, 0, 1, 0, 0, 0, 1, 1, 1},
#define WIPE_T2_4       WIPE_T2_3+1
  {0, 0, 1, 1, 0, 0, 0, 1, 1,
      1, 0, 0, 0, 1, 1, 1, 1, 1},
#define WIPE_T2_5       WIPE_T2_4+1
  {0, 0, 1, 0, 1, 1, 1, 1, 0,
      1, 0, 1, 0, 0, 1, 1, 1, 0},
#define WIPE_T2_6       WIPE_T2_5+1
  {0, 0, 1, 1, 0, 1, 0, 1, 0,
      1, 0, 1, 0, 1, 0, 1, 1, 1},
#define WIPE_T2_41      WIPE_T2_6+1
  {0, 0, 0, 1, 0, 1, 0, 1, 1,
      1, 0, 1, 0, 1, 1, 1, 1, 2},
#define WIPE_T2_42      WIPE_T2_41+1
  {0, 0, 1, 1, 0, 0, 1, 1, 1,
      0, 0, 1, 0, 1, 2, 1, 1, 1},
#define WIPE_T2_45      WIPE_T2_42+1
  {0, 0, 1, 1, 0, 0, 0, 1, 0,
      1, 0, 0, 0, 1, 0, 1, 1, 1},
#define WIPE_T2_46      WIPE_T2_45+1
  {0, 0, 0, 1, 0, 1, 1, 1, 0,
      0, 0, 0, 0, 1, 1, 1, 1, 0},
#define WIPE_T2_245     WIPE_T2_46+1
  {0, 0, 0, 2, 0, 0, 2, 2, 1,
      2, 2, 0, 0, 2, 0, 0, 0, 1},
#define WIPE_T2_246     WIPE_T2_245+1
  {0, 2, 0, 0, 0, 0, 2, 0, 1,
      2, 0, 0, 2, 2, 0, 0, 2, 1},
};

static const gint triangles_3t[][3 * 9] = {
  /* 23 -> 26 */
#define WIPE_T3_23      0
  {0, 0, 1, 1, 0, 0, 0, 2, 1,
        1, 0, 0, 0, 2, 1, 2, 2, 1,
      1, 0, 0, 2, 0, 1, 2, 2, 1},
#define WIPE_T3_24      1
  {0, 0, 1, 2, 0, 1, 2, 1, 0,
        0, 0, 1, 2, 1, 0, 0, 2, 1,
      2, 1, 0, 0, 2, 1, 2, 2, 1},
#define WIPE_T3_25      2
  {0, 0, 1, 0, 2, 1, 1, 2, 0,
        0, 0, 1, 2, 0, 1, 1, 2, 0,
      2, 0, 1, 1, 2, 0, 2, 2, 1},
#define WIPE_T3_26      3
  {0, 0, 1, 2, 0, 1, 0, 1, 0,
        2, 0, 1, 0, 1, 0, 2, 2, 1,
      0, 1, 0, 0, 2, 1, 2, 2, 1},
};

static const gint triangles_4t[][4 * 9] = {
#define WIPE_T4_61      0
  {0, 0, 1, 1, 0, 0, 1, 2, 1,
        0, 0, 1, 0, 2, 2, 1, 2, 1,
        1, 0, 0, 2, 0, 1, 1, 2, 1,
      2, 0, 1, 1, 2, 1, 2, 2, 2},
#define WIPE_T4_62      1
  {0, 0, 2, 2, 0, 1, 0, 1, 1,
        2, 0, 1, 0, 1, 1, 2, 1, 0,
        0, 1, 1, 2, 1, 0, 2, 2, 1,
      0, 1, 1, 0, 2, 2, 2, 2, 1},
#define WIPE_T4_63      2
  {0, 0, 2, 1, 0, 1, 0, 2, 1,
        1, 0, 1, 0, 2, 1, 1, 2, 0,
        1, 0, 1, 1, 2, 0, 2, 2, 1,
      1, 0, 1, 2, 0, 2, 2, 2, 1},
#define WIPE_T4_64      3
  {0, 0, 1, 2, 0, 2, 2, 1, 1,
        0, 0, 1, 0, 1, 0, 2, 1, 1,
        0, 1, 0, 2, 1, 1, 0, 2, 1,
      2, 1, 1, 0, 2, 1, 2, 2, 2},
#define WIPE_T4_65      4
  {0, 0, 0, 1, 0, 1, 1, 2, 0,
        0, 0, 0, 0, 2, 1, 1, 2, 0,
        1, 0, 1, 2, 0, 0, 1, 2, 0,
      2, 0, 0, 1, 2, 0, 2, 2, 1},
#define WIPE_T4_66      5
  {0, 0, 1, 2, 0, 0, 0, 1, 0,
        2, 0, 0, 0, 1, 0, 2, 1, 1,
        0, 1, 0, 2, 1, 1, 2, 2, 0,
      0, 1, 0, 0, 2, 1, 2, 2, 0},
#define WIPE_T4_67      6
  {0, 0, 1, 1, 0, 0, 0, 2, 0,
        1, 0, 0, 0, 2, 0, 1, 2, 1,
        1, 0, 0, 1, 2, 1, 2, 2, 0,
      1, 0, 0, 2, 0, 1, 2, 2, 0},
#define WIPE_T4_68      7
  {0, 0, 0, 2, 0, 1, 2, 1, 0,
        0, 0, 0, 0, 1, 1, 2, 1, 0,
        0, 1, 1, 2, 1, 0, 0, 2, 0,
      2, 1, 0, 0, 2, 0, 2, 2, 1},
#define WIPE_T4_101     8
  {0, 0, 1, 2, 0, 1, 1, 1, 0,
        0, 0, 1, 1, 1, 0, 0, 2, 1,
        1, 1, 0, 0, 2, 1, 2, 2, 1,
      2, 0, 1, 1, 1, 0, 2, 2, 1},
};

static const gint triangles_8t[][8 * 9] = {
  /* 7 */
#define WIPE_T8_7       0
  {0, 0, 0, 1, 0, 1, 1, 1, 1,
        1, 0, 1, 2, 0, 0, 1, 1, 1,
        2, 0, 0, 1, 1, 1, 2, 1, 1,
        1, 1, 1, 2, 1, 1, 2, 2, 0,
        1, 1, 1, 1, 2, 1, 2, 2, 0,
        1, 1, 1, 0, 2, 0, 1, 2, 1,
        0, 1, 1, 1, 1, 1, 0, 2, 0,
      0, 0, 0, 0, 1, 1, 1, 1, 1},
#define WIPE_T8_43      1
  {0, 0, 1, 1, 0, 0, 1, 1, 1,
        1, 0, 0, 2, 0, 1, 1, 1, 1,
        2, 0, 1, 1, 1, 1, 2, 1, 2,
        1, 1, 1, 2, 1, 2, 2, 2, 1,
        1, 1, 1, 1, 2, 0, 2, 2, 1,
        1, 1, 1, 0, 2, 1, 1, 2, 0,
        0, 1, 2, 1, 1, 1, 0, 2, 1,
      0, 0, 1, 0, 1, 2, 1, 1, 1},
#define WIPE_T8_44      2
  {0, 0, 1, 1, 0, 2, 1, 1, 1,
        1, 0, 2, 2, 0, 1, 1, 1, 1,
        2, 0, 1, 1, 1, 1, 2, 1, 0,
        1, 1, 1, 2, 1, 0, 2, 2, 1,
        1, 1, 1, 1, 2, 2, 2, 2, 1,
        1, 1, 1, 0, 2, 1, 1, 2, 2,
        0, 1, 0, 1, 1, 1, 0, 2, 1,
      0, 0, 1, 0, 1, 0, 1, 1, 1},
#define WIPE_T8_47      3
  {0, 0, 0, 1, 0, 1, 1, 1, 0,
        1, 0, 1, 2, 0, 0, 1, 1, 0,
        2, 0, 0, 1, 1, 0, 2, 1, 1,
        1, 1, 0, 2, 1, 1, 2, 2, 0,
        1, 1, 0, 1, 2, 1, 2, 2, 0,
        1, 1, 0, 0, 2, 0, 1, 2, 1,
        0, 1, 1, 1, 1, 0, 0, 2, 0,
      0, 0, 0, 0, 1, 1, 1, 1, 0},
#define WIPE_T8_48      4
  {0, 0, 1, 1, 0, 0, 0, 1, 0,
        1, 0, 0, 0, 1, 0, 1, 1, 1,
        1, 0, 0, 2, 0, 1, 2, 1, 0,
        1, 0, 0, 1, 1, 1, 2, 1, 0,
        0, 1, 0, 1, 1, 1, 1, 2, 0,
        0, 1, 0, 0, 2, 1, 1, 2, 0,
        1, 1, 1, 2, 1, 0, 1, 2, 0,
      2, 1, 0, 1, 2, 0, 2, 2, 1},
};

static const gint triangles_16t[][16 * 9] = {
  /* 8 */
#define WIPE_T16_8      0
  {0, 0, 1, 2, 0, 1, 1, 1, 0,
        2, 0, 1, 1, 1, 0, 2, 2, 1,
        1, 1, 0, 0, 2, 1, 2, 2, 1,
        0, 0, 1, 1, 1, 0, 0, 2, 1,
        2, 0, 1, 4, 0, 1, 3, 1, 0,
        4, 0, 1, 3, 1, 0, 4, 2, 1,
        3, 1, 0, 2, 2, 1, 4, 2, 1,
        2, 0, 1, 3, 1, 0, 2, 2, 1,
        0, 2, 1, 2, 2, 1, 1, 3, 0,
        2, 2, 1, 1, 3, 0, 2, 4, 1,
        1, 3, 0, 0, 4, 1, 2, 4, 1,
        0, 2, 1, 1, 3, 0, 0, 4, 1,
        2, 2, 1, 4, 2, 1, 3, 3, 0,
        4, 2, 1, 3, 3, 0, 4, 4, 1,
        3, 3, 0, 2, 4, 1, 4, 4, 1,
      2, 2, 1, 3, 3, 0, 2, 4, 1}
};

typedef struct _GstWipeConfig GstWipeConfig;

struct _GstWipeConfig
{
  const gint *objects;
  gint nobjects;
  gint xscale;
  gint yscale;
  gint cscale;
};

static const GstWipeConfig wipe_config[] = {
#define WIPE_CONFIG_1   0
  {boxes_1b[WIPE_B1_1], 1, 0, 0, 0},    /* 1 */
#define WIPE_CONFIG_2   WIPE_CONFIG_1+1
  {boxes_1b[WIPE_B1_2], 1, 0, 0, 0},    /* 2 */
#define WIPE_CONFIG_3   WIPE_CONFIG_2+1
  {triangles_2t[WIPE_T2_3], 2, 0, 0, 0},        /* 3 */
#define WIPE_CONFIG_4   WIPE_CONFIG_3+1
  {triangles_2t[WIPE_T2_4], 2, 0, 0, 0},        /* 4 */
#define WIPE_CONFIG_5   WIPE_CONFIG_4+1
  {triangles_2t[WIPE_T2_5], 2, 0, 0, 0},        /* 5 */
#define WIPE_CONFIG_6   WIPE_CONFIG_5+1
  {triangles_2t[WIPE_T2_6], 2, 0, 0, 0},        /* 6 */
#define WIPE_CONFIG_7   WIPE_CONFIG_6+1
  {triangles_8t[WIPE_T8_7], 8, 1, 1, 0},        /* 7 */
#define WIPE_CONFIG_8   WIPE_CONFIG_7+1
  {triangles_16t[WIPE_T16_8], 16, 2, 2, 0},     /* 8 */

#define WIPE_CONFIG_21  WIPE_CONFIG_8+1
  {boxes_2b[WIPE_B2_21], 2, 1, 1, 0},   /* 21 */
#define WIPE_CONFIG_22  WIPE_CONFIG_21+1
  {boxes_2b[WIPE_B2_22], 2, 1, 1, 0},   /* 22 */

#define WIPE_CONFIG_23  WIPE_CONFIG_22+1
  {triangles_3t[WIPE_T3_23], 3, 1, 1, 0},       /* 23 */
#define WIPE_CONFIG_24  WIPE_CONFIG_23+1
  {triangles_3t[WIPE_T3_24], 3, 1, 1, 0},       /* 24 */
#define WIPE_CONFIG_25  WIPE_CONFIG_24+1
  {triangles_3t[WIPE_T3_25], 3, 1, 1, 0},       /* 25 */
#define WIPE_CONFIG_26  WIPE_CONFIG_25+1
  {triangles_3t[WIPE_T3_26], 3, 1, 1, 0},       /* 26 */
#define WIPE_CONFIG_41  WIPE_CONFIG_26+1
  {triangles_2t[WIPE_T2_41], 2, 0, 0, 1},       /* 41 */
#define WIPE_CONFIG_42  WIPE_CONFIG_41+1
  {triangles_2t[WIPE_T2_42], 2, 0, 0, 1},       /* 42 */
#define WIPE_CONFIG_43  WIPE_CONFIG_42+1
  {triangles_8t[WIPE_T8_43], 8, 1, 1, 1},       /* 43 */
#define WIPE_CONFIG_44  WIPE_CONFIG_43+1
  {triangles_8t[WIPE_T8_44], 8, 1, 1, 1},       /* 44 */
#define WIPE_CONFIG_45  WIPE_CONFIG_44+1
  {triangles_2t[WIPE_T2_45], 2, 0, 0, 0},       /* 45 */
#define WIPE_CONFIG_46  WIPE_CONFIG_45+1
  {triangles_2t[WIPE_T2_46], 2, 0, 0, 0},       /* 46 */
#define WIPE_CONFIG_47  WIPE_CONFIG_46+1
  {triangles_8t[WIPE_T8_47], 8, 1, 1, 0},       /* 47 */
#define WIPE_CONFIG_48  WIPE_CONFIG_47+1
  {triangles_8t[WIPE_T8_48], 8, 1, 1, 0},       /* 48 */
#define WIPE_CONFIG_61  WIPE_CONFIG_48+1
  {triangles_4t[WIPE_T4_61], 4, 1, 1, 1},       /* 61 */
#define WIPE_CONFIG_62  WIPE_CONFIG_61+1
  {triangles_4t[WIPE_T4_62], 4, 1, 1, 1},       /* 62 */
#define WIPE_CONFIG_63  WIPE_CONFIG_62+1
  {triangles_4t[WIPE_T4_63], 4, 1, 1, 1},       /* 63 */
#define WIPE_CONFIG_64  WIPE_CONFIG_63+1
  {triangles_4t[WIPE_T4_64], 4, 1, 1, 1},       /* 64 */
#define WIPE_CONFIG_65  WIPE_CONFIG_64+1
  {triangles_4t[WIPE_T4_65], 4, 1, 1, 0},       /* 65 */
#define WIPE_CONFIG_66  WIPE_CONFIG_65+1
  {triangles_4t[WIPE_T4_66], 4, 1, 1, 0},       /* 66 */
#define WIPE_CONFIG_67  WIPE_CONFIG_66+1
  {triangles_4t[WIPE_T4_67], 4, 1, 1, 0},       /* 67 */
#define WIPE_CONFIG_68  WIPE_CONFIG_67+1
  {triangles_4t[WIPE_T4_68], 4, 1, 1, 0},       /* 68 */
#define WIPE_CONFIG_101 WIPE_CONFIG_68+1
  {triangles_4t[WIPE_T4_101], 4, 1, 1, 0},      /* 101 */
#define WIPE_CONFIG_201 WIPE_CONFIG_101+1
  {box_clock_4b[WIPE_B4_201], 4, 1, 1, 2},      /* 201 */
#define WIPE_CONFIG_202 WIPE_CONFIG_201+1
  {box_clock_4b[WIPE_B4_202], 4, 1, 1, 2},      /* 202 */
#define WIPE_CONFIG_203 WIPE_CONFIG_202+1
  {box_clock_4b[WIPE_B4_203], 4, 1, 1, 2},      /* 203 */
#define WIPE_CONFIG_204 WIPE_CONFIG_203+1
  {box_clock_4b[WIPE_B4_204], 4, 1, 1, 2},      /* 204 */
#define WIPE_CONFIG_205 WIPE_CONFIG_204+1
  {box_clock_4b[WIPE_B4_205], 4, 1, 1, 1},      /* 205 */
#define WIPE_CONFIG_206 WIPE_CONFIG_205+1
  {box_clock_4b[WIPE_B4_206], 4, 1, 1, 1},      /* 206 */
#define WIPE_CONFIG_207 WIPE_CONFIG_206+1
  {box_clock_4b[WIPE_B4_207], 4, 1, 1, 0},      /* 207 */
#define WIPE_CONFIG_211 WIPE_CONFIG_207+1
  {box_clock_4b[WIPE_B4_211], 4, 1, 1, 1},      /* 211 */
#define WIPE_CONFIG_212 WIPE_CONFIG_211+1
  {box_clock_4b[WIPE_B4_212], 4, 1, 1, 1},      /* 212 */
#define WIPE_CONFIG_213 WIPE_CONFIG_212+1
  {box_clock_4b[WIPE_B4_213], 4, 1, 1, 0},      /* 213 */
#define WIPE_CONFIG_214 WIPE_CONFIG_213+1
  {box_clock_4b[WIPE_B4_214], 4, 1, 1, 0},      /* 214 */
#define WIPE_CONFIG_221 WIPE_CONFIG_214+1
  {box_clock_2b[WIPE_B2_221], 2, 1, 1, 1},      /* 221 */
#define WIPE_CONFIG_222 WIPE_CONFIG_221+1
  {box_clock_2b[WIPE_B2_222], 2, 1, 1, 1},      /* 222 */
#define WIPE_CONFIG_223 WIPE_CONFIG_222+1
  {box_clock_2b[WIPE_B2_223], 2, 1, 1, 1},      /* 223 */
#define WIPE_CONFIG_224 WIPE_CONFIG_223+1
  {box_clock_2b[WIPE_B2_224], 2, 1, 1, 1},      /* 224 */
#define WIPE_CONFIG_225 WIPE_CONFIG_224+1
  {box_clock_2b[WIPE_B2_225], 2, 1, 1, 0},      /* 225 */
#define WIPE_CONFIG_226 WIPE_CONFIG_225+1
  {box_clock_2b[WIPE_B2_226], 2, 1, 1, 0},      /* 226 */
#define WIPE_CONFIG_227 WIPE_CONFIG_226+1
  {box_clock_4b[WIPE_B4_227], 4, 1, 1, 1},      /* 227 */
#define WIPE_CONFIG_228 WIPE_CONFIG_227+1
  {box_clock_4b[WIPE_B4_228], 4, 1, 1, 1},      /* 228 */
#define WIPE_CONFIG_231 WIPE_CONFIG_228+1
  {box_clock_2b[WIPE_B2_231], 2, 1, 1, 0},      /* 231 */
#define WIPE_CONFIG_232 WIPE_CONFIG_231+1
  {box_clock_2b[WIPE_B2_232], 2, 1, 1, 0},      /* 232 */
#define WIPE_CONFIG_233 WIPE_CONFIG_232+1
  {box_clock_2b[WIPE_B2_233], 2, 1, 1, 0},      /* 233 */
#define WIPE_CONFIG_234 WIPE_CONFIG_233+1
  {box_clock_2b[WIPE_B2_234], 2, 1, 1, 0},      /* 234 */
#define WIPE_CONFIG_235 WIPE_CONFIG_234+1
  {box_clock_4b[WIPE_B4_235], 4, 1, 1, 0},      /* 235 */
#define WIPE_CONFIG_236 WIPE_CONFIG_235+1
  {box_clock_4b[WIPE_B4_236], 4, 1, 1, 0},      /* 236 */
#define WIPE_CONFIG_241 WIPE_CONFIG_236+1
  {box_clock_1b[WIPE_B1_241], 1, 0, 0, 0},      /* 241 */
#define WIPE_CONFIG_242 WIPE_CONFIG_241+1
  {box_clock_1b[WIPE_B1_242], 1, 0, 0, 0},      /* 242 */
#define WIPE_CONFIG_243 WIPE_CONFIG_242+1
  {box_clock_1b[WIPE_B1_243], 1, 0, 0, 0},      /* 243 */
#define WIPE_CONFIG_244 WIPE_CONFIG_243+1
  {box_clock_1b[WIPE_B1_244], 1, 0, 0, 0},      /* 244 */
#define WIPE_CONFIG_245 WIPE_CONFIG_244+1
  {triangles_2t[WIPE_T2_245], 2, 1, 1, 0},      /* 245 */
#define WIPE_CONFIG_246 WIPE_CONFIG_245+1
  {triangles_2t[WIPE_T2_246], 2, 1, 1, 0},      /* 246 */
#define WIPE_CONFIG_251 WIPE_CONFIG_246+1
  {box_clock_2b[WIPE_B2_251], 2, 1, 1, 0},      /* 251 */
#define WIPE_CONFIG_252 WIPE_CONFIG_251+1
  {box_clock_2b[WIPE_B2_252], 2, 1, 1, 0},      /* 252 */
#define WIPE_CONFIG_253 WIPE_CONFIG_252+1
  {box_clock_2b[WIPE_B2_253], 2, 1, 1, 0},      /* 253 */
#define WIPE_CONFIG_254 WIPE_CONFIG_253+1
  {box_clock_2b[WIPE_B2_254], 2, 1, 1, 0},      /* 254 */

#define WIPE_CONFIG_261 WIPE_CONFIG_254+1
  {box_clock_8b[WIPE_B8_261], 8, 2, 2, 2},      /* 261 */
#define WIPE_CONFIG_262 WIPE_CONFIG_261+1
  {box_clock_8b[WIPE_B8_262], 8, 2, 2, 2},      /* 262 */
#define WIPE_CONFIG_263 WIPE_CONFIG_262+1
  {box_clock_8b[WIPE_B8_263], 8, 2, 2, 1},      /* 263 */
#define WIPE_CONFIG_264 WIPE_CONFIG_263+1
  {box_clock_8b[WIPE_B8_264], 8, 2, 2, 1},      /* 264 */
};

static void
gst_wipe_boxes_draw (GstMask * mask)
{
  const GstWipeConfig *config = mask->user_data;
  const gint *impacts = config->objects;
  gint width = (mask->width >> config->xscale);
  gint height = (mask->height >> config->yscale);
  gint depth = (1 << mask->bpp) >> config->cscale;

  gint i;

  for (i = 0; i < config->nobjects; i++) {
    switch (impacts[0]) {
      case BOX_VERTICAL:
        /* vbox does not draw last pixels */
        gst_smpte_paint_vbox (mask->data, mask->width,
            impacts[1] * width, impacts[2] * height, impacts[3] * depth,
            impacts[4] * width, impacts[5] * height, impacts[6] * depth);
        impacts += 7;
        break;
      case BOX_HORIZONTAL:
        /* hbox does not draw last pixels */
        gst_smpte_paint_hbox (mask->data, mask->width,
            impacts[1] * width, impacts[2] * height, impacts[3] * depth,
            impacts[4] * width, impacts[5] * height, impacts[6] * depth);
        impacts += 7;
        break;
      case BOX_CLOCK:
      {
        gint x0, y0, x1, y1, x2, y2;

        /* make sure not to draw outside the area */
        x0 = MIN (impacts[1] * width, mask->width - 1);
        y0 = MIN (impacts[2] * height, mask->height - 1);
        x1 = MIN (impacts[4] * width, mask->width - 1);
        y1 = MIN (impacts[5] * height, mask->height - 1);
        x2 = MIN (impacts[7] * width, mask->width - 1);
        y2 = MIN (impacts[8] * height, mask->height - 1);

        gst_smpte_paint_box_clock (mask->data, mask->width,
            x0, y0, impacts[3] * depth, x1, y1, impacts[6] * depth,
            x2, y2, impacts[9] * depth);
        impacts += 10;
      }
      default:
        break;
    }
  }
}

static void
gst_wipe_triangles_clock_draw (GstMask * mask)
{
  const GstWipeConfig *config = mask->user_data;
  const gint *impacts = config->objects;
  gint width = (mask->width >> config->xscale);
  gint height = (mask->height >> config->yscale);
  gint depth = (1 << mask->bpp) >> config->cscale;
  gint i;

  for (i = 0; i < config->nobjects; i++) {
    gint x0, y0, x1, y1, x2, y2;

    /* make sure not to draw outside the area */
    x0 = MIN (impacts[0] * width, mask->width - 1);
    y0 = MIN (impacts[1] * height, mask->height - 1);
    x1 = MIN (impacts[3] * width, mask->width - 1);
    y1 = MIN (impacts[4] * height, mask->height - 1);
    x2 = MIN (impacts[6] * width, mask->width - 1);
    y2 = MIN (impacts[7] * height, mask->height - 1);

    gst_smpte_paint_triangle_clock (mask->data, mask->width,
        x0, y0, impacts[2] * depth, x1, y1, impacts[5] * depth,
        x2, y2, impacts[8] * depth);
    impacts += 9;
  }
}

static void
gst_wipe_triangles_draw (GstMask * mask)
{
  const GstWipeConfig *config = mask->user_data;
  const gint *impacts = config->objects;
  gint width = (mask->width >> config->xscale);
  gint height = (mask->height >> config->yscale);
  gint depth = (1 << mask->bpp) >> config->cscale;

  gint i;

  for (i = 0; i < config->nobjects; i++) {
    gint x0, y0, x1, y1, x2, y2;

    /* make sure not to draw outside the area */
    x0 = MIN (impacts[0] * width, mask->width - 1);
    y0 = MIN (impacts[1] * height, mask->height - 1);
    x1 = MIN (impacts[3] * width, mask->width - 1);
    y1 = MIN (impacts[4] * height, mask->height - 1);
    x2 = MIN (impacts[6] * width, mask->width - 1);
    y2 = MIN (impacts[7] * height, mask->height - 1);

    gst_smpte_paint_triangle_linear (mask->data, mask->width,
        x0, y0, impacts[2] * depth, x1, y1, impacts[5] * depth,
        x2, y2, impacts[8] * depth);
    impacts += 9;
  }
}

/* see also:
 * http://www.w3c.rl.ac.uk/pasttalks/slidemaker/XML_Multimedia/htmls/transitions.html
 */
static const GstMaskDefinition definitions[] = {
  {1, "bar-wipe-lr",
        "A bar moves from left to right",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_1]},
  {2, "bar-wipe-tb",
        "A bar moves from top to bottom",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_2]},
  {3, "box-wipe-tl",
        "A box expands from the upper-left corner to the lower-right corner",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_3]},
  {4, "box-wipe-tr",
        "A box expands from the upper-right corner to the lower-left corner",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_4]},
  {5, "box-wipe-br",
        "A box expands from the lower-right corner to the upper-left corner",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_5]},
  {6, "box-wipe-bl",
        "A box expands from the lower-left corner to the upper-right corner",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_6]},
  {7, "four-box-wipe-ci",
        "A box shape expands from each of the four corners toward the center",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_7]},
  {8, "four-box-wipe-co",
        "A box shape expands from the center of each quadrant toward the corners of each quadrant",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_8]},
  {21, "barndoor-v",
        "A central, vertical line splits and expands toward the left and right edges",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_21]},
  {22, "barndoor-h",
        "A central, horizontal line splits and expands toward the top and bottom edges",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_22]},
  {23, "box-wipe-tc",
        "A box expands from the top edge's midpoint to the bottom corners",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_23]},
  {24, "box-wipe-rc",
        "A box expands from the right edge's midpoint to the left corners",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_24]},
  {25, "box-wipe-bc",
        "A box expands from the bottom edge's midpoint to the top corners",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_25]},
  {26, "box-wipe-lc",
        "A box expands from the left edge's midpoint to the right corners",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_26]},
  {41, "diagonal-tl",
        "A diagonal line moves from the upper-left corner to the lower-right corner",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_41]},
  {42, "diagonal-tr",
        "A diagonal line moves from the upper right corner to the lower-left corner",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_42]},
  {43, "bowtie-v",
        "Two wedge shapes slide in from the top and bottom edges toward the center",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_43]},
  {44, "bowtie-h",
        "Two wedge shapes slide in from the left and right edges toward the center",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_44]},
  {45, "barndoor-dbl",
        "A diagonal line from the lower-left to upper-right corners splits and expands toward the opposite corners",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_45]},
  {46, "barndoor-dtl",
        "A diagonal line from upper-left to lower-right corners splits and expands toward the opposite corners",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_46]},
  {47, "misc-diagonal-dbd",
        "Four wedge shapes split from the center and retract toward the four edges",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_47]},
  {48, "misc-diagonal-dd",
        "A diamond connecting the four edge midpoints simultaneously contracts toward the center and expands toward the edges",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_48]},
  {61, "vee-d",
        "A wedge shape moves from top to bottom",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_61]},
  {62, "vee-l",
        "A wedge shape moves from right to left",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_62]},
  {63, "vee-u",
        "A wedge shape moves from bottom to top",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_63]},
  {64, "vee-r",
        "A wedge shape moves from left to right",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_64]},
  {65, "barnvee-d",
        "A 'V' shape extending from the bottom edge's midpoint to the opposite corners contracts toward the center and expands toward the edges",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_65]},
  {66, "barnvee-l",
        "A 'V' shape extending from the left edge's midpoint to the opposite corners contracts toward the center and expands toward the edges",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_66]},
  {67, "barnvee-u",
        "A 'V' shape extending from the top edge's midpoint to the opposite corners contracts toward the center and expands toward the edges",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_67]},
  {68, "barnvee-r",
        "A 'V' shape extending from the right edge's midpoint to the opposite corners contracts toward the center and expands toward the edges",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_68]},
  {101, "iris-rect",
        "A rectangle expands from the center.",
        gst_wipe_triangles_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_101]},
  {201, "clock-cw12",
        "A radial hand sweeps clockwise from the twelve o'clock position",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_201]},
  {202, "clock-cw3",
        "A radial hand sweeps clockwise from the three o'clock position",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_202]},
  {203, "clock-cw6",
        "A radial hand sweeps clockwise from the six o'clock position",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_203]},
  {204, "clock-cw9",
        "A radial hand sweeps clockwise from the nine o'clock position",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_204]},
  {205, "pinwheel-tbv",
        "Two radial hands sweep clockwise from the twelve and six o'clock positions",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_205]},
  {206, "pinwheel-tbh",
        "Two radial hands sweep clockwise from the nine and three o'clock positions",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_206]},
  {207, "pinwheel-fb",
        "Four radial hands sweep clockwise",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_207]},
  {211, "fan-ct",
        "A fan unfolds from the top edge, the fan axis at the center",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_211]},
  {212, "fan-cr",
        "A fan unfolds from the right edge, the fan axis at the center",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_212]},
  {213, "doublefan-fov",
        "Two fans, their axes at the center, unfold from the top and bottom",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_213]},
  {214, "doublefan-foh",
        "Two fans, their axes at the center, unfold from the left and right",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_214]},
  {221, "singlesweep-cwt",
        "A radial hand sweeps clockwise from the top edge's midpoint",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_221]},
  {222, "singlesweep-cwr",
        "A radial hand sweeps clockwise from the right edge's midpoint",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_222]},
  {223, "singlesweep-cwb",
        "A radial hand sweeps clockwise from the bottom edge's midpoint",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_223]},
  {224, "singlesweep-cwl",
        "A radial hand sweeps clockwise from the left edge's midpoint",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_224]},
  {225, "doublesweep-pv",
        "Two radial hands sweep clockwise and counter-clockwise from the top and bottom edges' midpoints",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_225]},
  {226, "doublesweep-pd",
        "Two radial hands sweep clockwise and counter-clockwise from the left and right edges' midpoints",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_226]},
  {227, "doublesweep-ov",
        "Two radial hands attached at the top and bottom edges' midpoints sweep from right to left",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_227]},
  {228, "doublesweep-oh",
        "Two radial hands attached at the left and right edges' midpoints sweep from top to bottom",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_228]},
  {231, "fan-t",
        "A fan unfolds from the bottom, the fan axis at the top edge's midpoint",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_231]},
  {232, "fan-r",
        "A fan unfolds from the left, the fan axis at the right edge's midpoint",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_232]},
  {233, "fan-b",
        "A fan unfolds from the top, the fan axis at the bottom edge's midpoint",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_233]},
  {234, "fan-l",
        "A fan unfolds from the right, the fan axis at the left edge's midpoint",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_234]},
  {235, "doublefan-fiv",
        "Two fans, their axes at the top and bottom, unfold from the center",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_235]},
  {236, "doublefan-fih",
        "Two fans, their axes at the left and right, unfold from the center",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_236]},
  {241, "singlesweep-cwtl",
        "A radial hand sweeps clockwise from the upper-left corner",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_241]},
  {242, "singlesweep-cwbl",
        "A radial hand sweeps counter-clockwise from the lower-left corner.",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_242]},
  {243, "singlesweep-cwbr",
        "A radial hand sweeps clockwise from the lower-right corner",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_243]},
  {244, "singlesweep-cwtr",
        "A radial hand sweeps counter-clockwise from the upper-right corner",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_244]},
  {245, "doublesweep-pdtl",
        "Two radial hands attached at the upper-left and lower-right corners sweep down and up",
        gst_wipe_triangles_clock_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_245]},
  {246, "doublesweep-pdbl",
        "Two radial hands attached at the lower-left and upper-right corners sweep down and up",
        gst_wipe_triangles_clock_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_246]},
  {251, "saloondoor-t",
        "Two radial hands attached at the upper-left and upper-right corners sweep down",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_251]},
  {252, "saloondoor-l",
        "Two radial hands attached at the upper-left and lower-left corners sweep to the right",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_252]},
  {253, "saloondoor-b",
        "Two radial hands attached at the lower-left and lower-right corners sweep up",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_253]},
  {254, "saloondoor-r",
        "Two radial hands attached at the upper-right and lower-right corners sweep to the left",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_254]},
  {261, "windshield-r",
        "Two radial hands attached at the midpoints of the top and bottom halves sweep from right to left",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_261]},
  {262, "windshield-u",
        "Two radial hands attached at the midpoints of the left and right halves sweep from top to bottom",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_262]},
  {263, "windshield-v",
        "Two sets of radial hands attached at the midpoints of the top and bottom halves sweep from top to bottom and bottom to top",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_263]},
  {264, "windshield-h",
        "Two sets of radial hands attached at the midpoints of the left and right halves sweep from left to right and right to left",
        gst_wipe_boxes_draw, _gst_mask_default_destroy,
      &wipe_config[WIPE_CONFIG_264]},
  {0, NULL, NULL, NULL}
};

void
_gst_barboxwipes_register (void)
{
  static gsize id = 0;

  if (g_once_init_enter (&id)) {
    gint i = 0;

    while (definitions[i].short_name) {
      _gst_mask_register (&definitions[i]);
      i++;
    }

    g_once_init_leave (&id, 1);
  }
}
