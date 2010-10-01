/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <GL/glut.h>

#include "objects.h"

static float scp[18][3] = {
  {1.000000, 0.000000, 0.000000},	{1.000000, 0.000000, 5.000000},
  {0.707107, 0.707107, 0.000000},	{0.707107, 0.707107, 5.000000},
  {0.000000, 1.000000, 0.000000},	{0.000000, 1.000000, 5.000000},
  {-0.707107, 0.707107, 0.000000},	{-0.707107, 0.707107, 5.000000},
  {-1.000000, 0.000000, 0.000000},	{-1.000000, 0.000000, 5.000000},
  {-0.707107, -0.707107, 0.000000},	{-0.707107, -0.707107, 5.000000},
  {0.000000, -1.000000, 0.000000},	{0.000000, -1.000000, 5.000000},
  {0.707107, -0.707107, 0.000000},	{0.707107, -0.707107, 5.000000},
  {1.000000, 0.000000, 0.000000},	{1.000000, 0.000000, 5.000000},
};

static float dcp[18][3] = {
  {1.000000, 0.000000, 0.000000},	{1.000000, 0.000000, 7.000000},
  {0.707107, 0.707107, 0.000000},	{0.707107, 0.707107, 7.000000},
  {0.000000, 1.000000, 0.000000},	{0.000000, 1.000000, 7.000000},
  {-0.707107, 0.707107, 0.000000},	{-0.707107, 0.707107, 7.000000},
  {-1.000000, 0.000000, 0.000000},	{-1.000000, 0.000000, 7.000000},
  {-0.707107, -0.707107, 0.000000},	{-0.707107, -0.707107, 7.000000},
  {0.000000, -1.000000, 0.000000},	{0.000000, -1.000000, 7.000000},
  {0.707107, -0.707107, 0.000000},	{0.707107, -0.707107, 7.000000},
  {1.000000, 0.000000, 0.000000},	{1.000000, 0.000000, 7.000000},
};

static float ep[7][9][3] = {
  {
    {1.000000, 0.000000, 0.000000},
    {0.707107, 0.707107, 0.000000},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.707107, 0.000000},
    {-1.000000, 0.000000, 0.000000},
    {-0.707107, -0.707107, 0.000000},
    {0.000000, -1.000000, 0.000000},
    {0.707107, -0.707107, 0.000000},
    {1.000000, 0.000000, 0.000000},
  },
  
  {
    {1.000000, 0.034074, 0.258819},
    {0.707107, 0.717087, 0.075806},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.717087, 0.075806},
    {-1.000000, 0.034074, 0.258819},
    {-0.707107, -0.648939, 0.441832},
    {0.000000, -0.931852, 0.517638},
    {0.707107, -0.648939, 0.441832},
    {1.000000, 0.034074, 0.258819},
  },
  
  {
    {1.000000, 0.133975, 0.500000},
    {0.707107, 0.746347, 0.146447},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.746347, 0.146447},
    {-1.000000, 0.133975, 0.500000},
    {-0.707107, -0.478398, 0.853553},
    {0.000000, -0.732051, 1.000000},
    {0.707107, -0.478398, 0.853553},
    {1.000000, 0.133975, 0.500000},
  },
  
  {
    {1.000000, 0.292893, 0.707107},
    {0.707107, 0.792893, 0.207107},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.792893, 0.207107},
    {-1.000000, 0.292893, 0.707107},
    {-0.707107, -0.207107, 1.207107},
    {0.000000, -0.414214, 1.414214},
    {0.707107, -0.207107, 1.207107},
    {1.000000, 0.292893, 0.707107},
  },
  
  {
    {1.000000, 0.500000, 0.866025},
    {0.707107, 0.853553, 0.253653},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.853553, 0.253653},
    {-1.000000, 0.500000, 0.866025},
    {-0.707107, 0.146447, 1.478398},
    {0.000000, 0.000000, 1.732051},
    {0.707107, 0.146447, 1.478398},
    {1.000000, 0.500000, 0.866025},
  },
  
  {
    {1.000000, 0.741181, 0.965926},
    {0.707107, 0.924194, 0.282913},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.924194, 0.282913},
    {-1.000000, 0.741181, 0.965926},
    {-0.707107, 0.558168, 1.648939},
    {0.000000, 0.482362, 1.931852},
    {0.707107, 0.558168, 1.648939},
    {1.000000, 0.741181, 0.965926},
  },
  
  {
    {1.000000, 1.000000, 1.000000},
    {0.707107, 1.000000, 0.292893},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 1.000000, 0.292893},
    {-1.000000, 1.000000, 1.000000},
    {-0.707107, 1.000000, 1.707107},
    {0.000000, 1.000000, 2.000000},
    {0.707107, 1.000000, 1.707107},
    {1.000000, 1.000000, 1.000000},
  },
  
};

static float en[7][9][3] = {
  {
    {1.000000, 0.000000, 0.000000},
    {0.707107, 0.707107, 0.000000},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.707107, 0.000000},
    {-1.000000, 0.000000, 0.000000},
    {-0.707107, -0.707107, 0.000000},
    {0.000000, -1.000000, 0.000000},
    {0.707107, -0.707107, 0.000000},
    {1.000000, 0.000000, 0.000000},
  },
  
  {
    {1.000000, 0.000000, 0.000000},
    {0.707107, 0.683013, -0.183013},
    {0.000000, 0.965926, -0.258819},
    {-0.707107, 0.683013, -0.183013},
    {-1.000000, 0.000000, 0.000000},
    {-0.707107, -0.683013, 0.183013},
    {0.000000, -0.965926, 0.258819},
    {0.707107, -0.683013, 0.183013},
    {1.000000, 0.000000, 0.000000},
  },
  
  {
    {1.000000, 0.000000, 0.000000},
    {0.707107, 0.612372, -0.353553},
    {0.000000, 0.866025, -0.500000},
    {-0.707107, 0.612372, -0.353553},
    {-1.000000, 0.000000, 0.000000},
    {-0.707107, -0.612372, 0.353553},
    {0.000000, -0.866025, 0.500000},
    {0.707107, -0.612372, 0.353553},
    {1.000000, 0.000000, 0.000000},
  },
  
  {
    {1.000000, 0.000000, 0.000000},
    {0.707107, 0.500000, -0.500000},
    {0.000000, 0.707107, -0.707107},
    {-0.707107, 0.500000, -0.500000},
    {-1.000000, 0.000000, 0.000000},
    {-0.707107, -0.500000, 0.500000},
    {0.000000, -0.707107, 0.707107},
    {0.707107, -0.500000, 0.500000},
    {1.000000, 0.000000, 0.000000},
  },
  
  {
    {1.000000, 0.000000, 0.000000},
    {0.707107, 0.353553, -0.612372},
    {0.000000, 0.500000, -0.866025},
    {-0.707107, 0.353553, -0.612372},
    {-1.000000, 0.000000, 0.000000},
    {-0.707107, -0.353553, 0.612372},
    {0.000000, -0.500000, 0.866025},
    {0.707107, -0.353553, 0.612372},
    {1.000000, 0.000000, 0.000000},
  },
  
  {
    {1.000000, 0.000000, 0.000000},
    {0.707107, 0.183013, -0.683013},
    {0.000000, 0.258819, -0.965926},
    {-0.707107, 0.183013, -0.683013},
    {-1.000000, 0.000000, 0.000000},
    {-0.707107, -0.183013, 0.683013},
    {0.000000, -0.258819, 0.965926},
    {0.707107, -0.183013, 0.683013},
    {1.000000, 0.000000, 0.000000},
  },
  
  {
    {1.000000, 0.000000, 0.000000},
    {0.707107, 0.000000, -0.707107},
    {0.000000, 0.000000, -1.000000},
    {-0.707107, 0.000000, -0.707107},
    {-1.000000, 0.000000, 0.000000},
    {-0.707107, 0.000000, 0.707107},
    {0.000000, 0.000000, 1.000000},
    {0.707107, 0.000000, 0.707107},
    {1.000000, 0.000000, 0.000000},
  },
  
};

static void draw_single_cylinder(void) {
  
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(scp[0]); glVertex3fv(scp[0]);
  glNormal3fv(scp[0]); glVertex3fv(scp[1]);
  glNormal3fv(scp[2]); glVertex3fv(scp[2]);
  glNormal3fv(scp[2]); glVertex3fv(scp[3]);
  glNormal3fv(scp[4]); glVertex3fv(scp[4]);
  glNormal3fv(scp[4]); glVertex3fv(scp[5]);
  glNormal3fv(scp[6]); glVertex3fv(scp[6]);
  glNormal3fv(scp[6]); glVertex3fv(scp[7]);
  glNormal3fv(scp[8]); glVertex3fv(scp[8]);
  glNormal3fv(scp[8]); glVertex3fv(scp[9]);
  glNormal3fv(scp[10]); glVertex3fv(scp[10]);
  glNormal3fv(scp[10]); glVertex3fv(scp[11]);
  glNormal3fv(scp[12]); glVertex3fv(scp[12]);
  glNormal3fv(scp[12]); glVertex3fv(scp[13]);
  glNormal3fv(scp[14]); glVertex3fv(scp[14]);
  glNormal3fv(scp[14]); glVertex3fv(scp[15]);
  glNormal3fv(scp[16]); glVertex3fv(scp[16]);
  glNormal3fv(scp[16]); glVertex3fv(scp[17]);
  glEnd();
}

static void draw_double_cylinder(void) {
  
  glEnable(GL_NORMALIZE);
  
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(dcp[0]); glVertex3fv(dcp[0]);
  glNormal3fv(dcp[0]);
  glVertex3fv(dcp[1]);
  glNormal3fv(dcp[2]); glVertex3fv(dcp[2]);
  glNormal3fv(dcp[2]);
  glVertex3fv(dcp[3]);
  glNormal3fv(dcp[4]); glVertex3fv(dcp[4]);
  glNormal3fv(dcp[4]);
  glVertex3fv(dcp[5]);
  glNormal3fv(dcp[6]); glVertex3fv(dcp[6]);
  glNormal3fv(dcp[6]);
  glVertex3fv(dcp[7]);
  glNormal3fv(dcp[8]); glVertex3fv(dcp[8]);
  glNormal3fv(dcp[8]);
  glVertex3fv(dcp[9]);
  glNormal3fv(dcp[10]); glVertex3fv(dcp[10]);
  glNormal3fv(dcp[10]);
  glVertex3fv(dcp[11]);
  glNormal3fv(dcp[12]); glVertex3fv(dcp[12]);
  glNormal3fv(dcp[12]);
  glVertex3fv(dcp[13]);
  glNormal3fv(dcp[14]); glVertex3fv(dcp[14]);
  glNormal3fv(dcp[14]);
  glVertex3fv(dcp[15]);
  glNormal3fv(dcp[16]); glVertex3fv(dcp[16]);
  glNormal3fv(dcp[16]);
  glVertex3fv(dcp[17]);
  glEnd();
}

static void draw_elbow(void) {
  
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(en[0][0]); glVertex3fv(ep[0][0]);
  glNormal3fv(en[1][0]); glVertex3fv(ep[1][0]);
  glNormal3fv(en[0][1]); glVertex3fv(ep[0][1]);
  glNormal3fv(en[1][1]); glVertex3fv(ep[1][1]);
  glNormal3fv(en[0][2]); glVertex3fv(ep[0][2]);
  glNormal3fv(en[1][2]); glVertex3fv(ep[1][2]);
  glNormal3fv(en[0][3]); glVertex3fv(ep[0][3]);
  glNormal3fv(en[1][3]); glVertex3fv(ep[1][3]);
  glNormal3fv(en[0][4]); glVertex3fv(ep[0][4]);
  glNormal3fv(en[1][4]); glVertex3fv(ep[1][4]);
  glNormal3fv(en[0][5]); glVertex3fv(ep[0][5]);
  glNormal3fv(en[1][5]); glVertex3fv(ep[1][5]);
  glNormal3fv(en[0][6]); glVertex3fv(ep[0][6]);
  glNormal3fv(en[1][6]); glVertex3fv(ep[1][6]);
  glNormal3fv(en[0][7]); glVertex3fv(ep[0][7]);
  glNormal3fv(en[1][7]); glVertex3fv(ep[1][7]);
  glNormal3fv(en[0][8]); glVertex3fv(ep[0][8]);
  glNormal3fv(en[1][8]); glVertex3fv(ep[1][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(en[1][0]); glVertex3fv(ep[1][0]);
  glNormal3fv(en[2][0]); glVertex3fv(ep[2][0]);
  glNormal3fv(en[1][1]); glVertex3fv(ep[1][1]);
  glNormal3fv(en[2][1]); glVertex3fv(ep[2][1]);
  glNormal3fv(en[1][2]); glVertex3fv(ep[1][2]);
  glNormal3fv(en[2][2]); glVertex3fv(ep[2][2]);
  glNormal3fv(en[1][3]); glVertex3fv(ep[1][3]);
  glNormal3fv(en[2][3]); glVertex3fv(ep[2][3]);
  glNormal3fv(en[1][4]); glVertex3fv(ep[1][4]);
  glNormal3fv(en[2][4]); glVertex3fv(ep[2][4]);
  glNormal3fv(en[1][5]); glVertex3fv(ep[1][5]);
  glNormal3fv(en[2][5]); glVertex3fv(ep[2][5]);
  glNormal3fv(en[1][6]); glVertex3fv(ep[1][6]);
  glNormal3fv(en[2][6]); glVertex3fv(ep[2][6]);
  glNormal3fv(en[1][7]); glVertex3fv(ep[1][7]);
  glNormal3fv(en[2][7]); glVertex3fv(ep[2][7]);
  glNormal3fv(en[1][8]); glVertex3fv(ep[1][8]);
  glNormal3fv(en[2][8]); glVertex3fv(ep[2][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(en[2][0]); glVertex3fv(ep[2][0]);
  glNormal3fv(en[3][0]); glVertex3fv(ep[3][0]);
  glNormal3fv(en[2][1]); glVertex3fv(ep[2][1]);
  glNormal3fv(en[3][1]); glVertex3fv(ep[3][1]);
  glNormal3fv(en[2][2]); glVertex3fv(ep[2][2]);
  glNormal3fv(en[3][2]); glVertex3fv(ep[3][2]);
  glNormal3fv(en[2][3]); glVertex3fv(ep[2][3]);
  glNormal3fv(en[3][3]); glVertex3fv(ep[3][3]);
  glNormal3fv(en[2][4]); glVertex3fv(ep[2][4]);
  glNormal3fv(en[3][4]); glVertex3fv(ep[3][4]);
  glNormal3fv(en[2][5]); glVertex3fv(ep[2][5]);
  glNormal3fv(en[3][5]); glVertex3fv(ep[3][5]);
  glNormal3fv(en[2][6]); glVertex3fv(ep[2][6]);
  glNormal3fv(en[3][6]); glVertex3fv(ep[3][6]);
  glNormal3fv(en[2][7]); glVertex3fv(ep[2][7]);
  glNormal3fv(en[3][7]); glVertex3fv(ep[3][7]);
  glNormal3fv(en[2][8]); glVertex3fv(ep[2][8]);
  glNormal3fv(en[3][8]); glVertex3fv(ep[3][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(en[3][0]); glVertex3fv(ep[3][0]);
  glNormal3fv(en[4][0]); glVertex3fv(ep[4][0]);
  glNormal3fv(en[3][1]); glVertex3fv(ep[3][1]);
  glNormal3fv(en[4][1]); glVertex3fv(ep[4][1]);
  glNormal3fv(en[3][2]); glVertex3fv(ep[3][2]);
  glNormal3fv(en[4][2]); glVertex3fv(ep[4][2]);
  glNormal3fv(en[3][3]); glVertex3fv(ep[3][3]);
  glNormal3fv(en[4][3]); glVertex3fv(ep[4][3]);
  glNormal3fv(en[3][4]); glVertex3fv(ep[3][4]);
  glNormal3fv(en[4][4]); glVertex3fv(ep[4][4]);
  glNormal3fv(en[3][5]); glVertex3fv(ep[3][5]);
  glNormal3fv(en[4][5]); glVertex3fv(ep[4][5]);
  glNormal3fv(en[3][6]); glVertex3fv(ep[3][6]);
  glNormal3fv(en[4][6]); glVertex3fv(ep[4][6]);
  glNormal3fv(en[3][7]); glVertex3fv(ep[3][7]);
  glNormal3fv(en[4][7]); glVertex3fv(ep[4][7]);
  glNormal3fv(en[3][8]); glVertex3fv(ep[3][8]);
  glNormal3fv(en[4][8]); glVertex3fv(ep[4][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(en[4][0]); glVertex3fv(ep[4][0]);
  glNormal3fv(en[5][0]); glVertex3fv(ep[5][0]);
  glNormal3fv(en[4][1]); glVertex3fv(ep[4][1]);
  glNormal3fv(en[5][1]); glVertex3fv(ep[5][1]);
  glNormal3fv(en[4][2]); glVertex3fv(ep[4][2]);
  glNormal3fv(en[5][2]); glVertex3fv(ep[5][2]);
  glNormal3fv(en[4][3]); glVertex3fv(ep[4][3]);
  glNormal3fv(en[5][3]); glVertex3fv(ep[5][3]);
  glNormal3fv(en[4][4]); glVertex3fv(ep[4][4]);
  glNormal3fv(en[5][4]); glVertex3fv(ep[5][4]);
  glNormal3fv(en[4][5]); glVertex3fv(ep[4][5]);
  glNormal3fv(en[5][5]); glVertex3fv(ep[5][5]);
  glNormal3fv(en[4][6]); glVertex3fv(ep[4][6]);
  glNormal3fv(en[5][6]); glVertex3fv(ep[5][6]);
  glNormal3fv(en[4][7]); glVertex3fv(ep[4][7]);
  glNormal3fv(en[5][7]); glVertex3fv(ep[5][7]);
  glNormal3fv(en[4][8]); glVertex3fv(ep[4][8]);
  glNormal3fv(en[5][8]); glVertex3fv(ep[5][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(en[5][0]); glVertex3fv(ep[5][0]);
  glNormal3fv(en[6][0]); glVertex3fv(ep[6][0]);
  glNormal3fv(en[5][1]); glVertex3fv(ep[5][1]);
  glNormal3fv(en[6][1]); glVertex3fv(ep[6][1]);
  glNormal3fv(en[5][2]); glVertex3fv(ep[5][2]);
  glNormal3fv(en[6][2]); glVertex3fv(ep[6][2]);
  glNormal3fv(en[5][3]); glVertex3fv(ep[5][3]);
  glNormal3fv(en[6][3]); glVertex3fv(ep[6][3]);
  glNormal3fv(en[5][4]); glVertex3fv(ep[5][4]);
  glNormal3fv(en[6][4]); glVertex3fv(ep[6][4]);
  glNormal3fv(en[5][5]); glVertex3fv(ep[5][5]);
  glNormal3fv(en[6][5]); glVertex3fv(ep[6][5]);
  glNormal3fv(en[5][6]); glVertex3fv(ep[5][6]);
  glNormal3fv(en[6][6]); glVertex3fv(ep[6][6]);
  glNormal3fv(en[5][7]); glVertex3fv(ep[5][7]);
  glNormal3fv(en[6][7]); glVertex3fv(ep[6][7]);
  glNormal3fv(en[5][8]); glVertex3fv(ep[5][8]);
  glNormal3fv(en[6][8]); glVertex3fv(ep[6][8]);
  glEnd();
}

static void bend_forward(void) {
  
  glTranslatef(0.0,  1.000000,  0.0);
  glRotatef (0.1 * (900), 1.0, 0.0, 0.0);
  glTranslatef(0.0,  -1.000000,  0.0);
}

static void bend_left(void) {
  glRotatef (0.1 * (-900), 0.0, 0.0, 1.0);
  glTranslatef(0.0, 1.000000, 0.0);
  glRotatef (0.1 * (900), 1.0, 0.0, 0.0);
  glTranslatef(0.0, -1.000000, 0.0);
}

static void bend_right(void) {
  glRotatef (0.1 * (900), 0.0, 0.0, 1.0);
  glTranslatef(0.0, 1.000000, 0.0);
  glRotatef (0.1 * (900), 1.0, 0.0, 0.0);
  glTranslatef(0.0, -1.000000, 0.0);
}

void draw_logo(void) {
  
  glCallList( MAT_LOGO); 
  
  glTranslatef(5.500000,  -3.500000,  4.500000);
  
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -5.000000);
  draw_single_cylinder();
  bend_right();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -5.000000);
  draw_single_cylinder();
  bend_left();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -5.000000);
  draw_single_cylinder();
  bend_right();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -5.000000);
  draw_single_cylinder();
  bend_left();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -5.000000);
  draw_single_cylinder();
  bend_right();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -7.000000);
  draw_double_cylinder();
  bend_forward();
  draw_elbow();
  glTranslatef(0.0,  0.0,  -5.000000);
  draw_single_cylinder();
  bend_left();
  draw_elbow();
  
  glDisable(GL_NORMALIZE);
}

