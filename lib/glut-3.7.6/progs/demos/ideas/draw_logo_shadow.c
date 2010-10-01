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

static float ep[9][9][3] = {
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
    {1.000000, 0.019215, 0.195090},
    {0.707107, 0.712735, 0.057141},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.712735, 0.057141},
    {-1.000000, 0.019215, 0.195090},
    {-0.707107, -0.674305, 0.333040},
    {0.000000, -0.961571, 0.390181},
    {0.707107, -0.674305, 0.333040},
    {1.000000, 0.019215, 0.195090},
  },
  
  {
    {1.000000, 0.076120, 0.382683},
    {0.707107, 0.729402, 0.112085},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.729402, 0.112085},
    {-1.000000, 0.076120, 0.382683},
    {-0.707107, -0.577161, 0.653282},
    {0.000000, -0.847759, 0.765367},
    {0.707107, -0.577161, 0.653282},
    {1.000000, 0.076120, 0.382683},
  },
  
  {
    {1.000000, 0.168530, 0.555570},
    {0.707107, 0.756468, 0.162723},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.756468, 0.162723},
    {-1.000000, 0.168530, 0.555570},
    {-0.707107, -0.419407, 0.948418},
    {0.000000, -0.662939, 1.111140},
    {0.707107, -0.419407, 0.948418},
    {1.000000, 0.168530, 0.555570},
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
    {1.000000, 0.444430, 0.831470},
    {0.707107, 0.837277, 0.243532},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.837277, 0.243532},
    {-1.000000, 0.444430, 0.831470},
    {-0.707107, 0.051582, 1.419407},
    {0.000000, -0.111140, 1.662939},
    {0.707107, 0.051582, 1.419407},
    {1.000000, 0.444430, 0.831470},
  },
  
  {
    {1.000000, 0.617317, 0.923880},
    {0.707107, 0.887915, 0.270598},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.887915, 0.270598},
    {-1.000000, 0.617317, 0.923880},
    {-0.707107, 0.346719, 1.577161},
    {0.000000, 0.234633, 1.847759},
    {0.707107, 0.346719, 1.577161},
    {1.000000, 0.617317, 0.923880},
  },
  
  {
    {1.000000, 0.804910, 0.980785},
    {0.707107, 0.942859, 0.287265},
    {0.000000, 1.000000, 0.000000},
    {-0.707107, 0.942859, 0.287265},
    {-1.000000, 0.804910, 0.980785},
    {-0.707107, 0.666960, 1.674305},
    {0.000000, 0.609819, 1.961571},
    {0.707107, 0.666960, 1.674305},
    {1.000000, 0.804910, 0.980785},
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

static void draw_single_cylinder(void) {
  
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(scp[0]);
  glVertex3fv(scp[1]);
  glVertex3fv(scp[2]);
  glVertex3fv(scp[3]);
  glVertex3fv(scp[4]);
  glVertex3fv(scp[5]);
  glVertex3fv(scp[6]);
  glVertex3fv(scp[7]);
  glVertex3fv(scp[8]);
  glVertex3fv(scp[9]);
  glVertex3fv(scp[10]);
  glVertex3fv(scp[11]);
  glVertex3fv(scp[12]);
  glVertex3fv(scp[13]);
  glVertex3fv(scp[14]);
  glVertex3fv(scp[15]);
  glVertex3fv(scp[16]);
  glVertex3fv(scp[17]);
  glEnd();
}

static void draw_double_cylinder(void) {
  
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(dcp[0]);
  glVertex3fv(dcp[1]);
  glVertex3fv(dcp[2]);
  glVertex3fv(dcp[3]);
  glVertex3fv(dcp[4]);
  glVertex3fv(dcp[5]);
  glVertex3fv(dcp[6]);
  glVertex3fv(dcp[7]);
  glVertex3fv(dcp[8]);
  glVertex3fv(dcp[9]);
  glVertex3fv(dcp[10]);
  glVertex3fv(dcp[11]);
  glVertex3fv(dcp[12]);
  glVertex3fv(dcp[13]);
  glVertex3fv(dcp[14]);
  glVertex3fv(dcp[15]);
  glVertex3fv(dcp[16]);
  glVertex3fv(dcp[17]);
  glEnd();
}

static void  draw_elbow(void) {
  
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(ep[0][0]);
  glVertex3fv(ep[1][0]);
  glVertex3fv(ep[0][1]);
  glVertex3fv(ep[1][1]);
  glVertex3fv(ep[0][2]);
  glVertex3fv(ep[1][2]);
  glVertex3fv(ep[0][3]);
  glVertex3fv(ep[1][3]);
  glVertex3fv(ep[0][4]);
  glVertex3fv(ep[1][4]);
  glVertex3fv(ep[0][5]);
  glVertex3fv(ep[1][5]);
  glVertex3fv(ep[0][6]);
  glVertex3fv(ep[1][6]);
  glVertex3fv(ep[0][7]);
  glVertex3fv(ep[1][7]);
  glVertex3fv(ep[0][8]);
  glVertex3fv(ep[1][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(ep[1][0]);
  glVertex3fv(ep[2][0]);
  glVertex3fv(ep[1][1]);
  glVertex3fv(ep[2][1]);
  glVertex3fv(ep[1][2]);
  glVertex3fv(ep[2][2]);
  glVertex3fv(ep[1][3]);
  glVertex3fv(ep[2][3]);
  glVertex3fv(ep[1][4]);
  glVertex3fv(ep[2][4]);
  glVertex3fv(ep[1][5]);
  glVertex3fv(ep[2][5]);
  glVertex3fv(ep[1][6]);
  glVertex3fv(ep[2][6]);
  glVertex3fv(ep[1][7]);
  glVertex3fv(ep[2][7]);
  glVertex3fv(ep[1][8]);
  glVertex3fv(ep[2][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(ep[2][0]);
  glVertex3fv(ep[3][0]);
  glVertex3fv(ep[2][1]);
  glVertex3fv(ep[3][1]);
  glVertex3fv(ep[2][2]);
  glVertex3fv(ep[3][2]);
  glVertex3fv(ep[2][3]);
  glVertex3fv(ep[3][3]);
  glVertex3fv(ep[2][4]);
  glVertex3fv(ep[3][4]);
  glVertex3fv(ep[2][5]);
  glVertex3fv(ep[3][5]);
  glVertex3fv(ep[2][6]);
  glVertex3fv(ep[3][6]);
  glVertex3fv(ep[2][7]);
  glVertex3fv(ep[3][7]);
  glVertex3fv(ep[2][8]);
  glVertex3fv(ep[3][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(ep[3][0]);
  glVertex3fv(ep[4][0]);
  glVertex3fv(ep[3][1]);
  glVertex3fv(ep[4][1]);
  glVertex3fv(ep[3][2]);
  glVertex3fv(ep[4][2]);
  glVertex3fv(ep[3][3]);
  glVertex3fv(ep[4][3]);
  glVertex3fv(ep[3][4]);
  glVertex3fv(ep[4][4]);
  glVertex3fv(ep[3][5]);
  glVertex3fv(ep[4][5]);
  glVertex3fv(ep[3][6]);
  glVertex3fv(ep[4][6]);
  glVertex3fv(ep[3][7]);
  glVertex3fv(ep[4][7]);
  glVertex3fv(ep[3][8]);
  glVertex3fv(ep[4][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(ep[4][0]);
  glVertex3fv(ep[5][0]);
  glVertex3fv(ep[4][1]);
  glVertex3fv(ep[5][1]);
  glVertex3fv(ep[4][2]);
  glVertex3fv(ep[5][2]);
  glVertex3fv(ep[4][3]);
  glVertex3fv(ep[5][3]);
  glVertex3fv(ep[4][4]);
  glVertex3fv(ep[5][4]);
  glVertex3fv(ep[4][5]);
  glVertex3fv(ep[5][5]);
  glVertex3fv(ep[4][6]);
  glVertex3fv(ep[5][6]);
  glVertex3fv(ep[4][7]);
  glVertex3fv(ep[5][7]);
  glVertex3fv(ep[4][8]);
  glVertex3fv(ep[5][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(ep[5][0]);
  glVertex3fv(ep[6][0]);
  glVertex3fv(ep[5][1]);
  glVertex3fv(ep[6][1]);
  glVertex3fv(ep[5][2]);
  glVertex3fv(ep[6][2]);
  glVertex3fv(ep[5][3]);
  glVertex3fv(ep[6][3]);
  glVertex3fv(ep[5][4]);
  glVertex3fv(ep[6][4]);
  glVertex3fv(ep[5][5]);
  glVertex3fv(ep[6][5]);
  glVertex3fv(ep[5][6]);
  glVertex3fv(ep[6][6]);
  glVertex3fv(ep[5][7]);
  glVertex3fv(ep[6][7]);
  glVertex3fv(ep[5][8]);
  glVertex3fv(ep[6][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(ep[6][0]);
  glVertex3fv(ep[7][0]);
  glVertex3fv(ep[6][1]);
  glVertex3fv(ep[7][1]);
  glVertex3fv(ep[6][2]);
  glVertex3fv(ep[7][2]);
  glVertex3fv(ep[6][3]);
  glVertex3fv(ep[7][3]);
  glVertex3fv(ep[6][4]);
  glVertex3fv(ep[7][4]);
  glVertex3fv(ep[6][5]);
  glVertex3fv(ep[7][5]);
  glVertex3fv(ep[6][6]);
  glVertex3fv(ep[7][6]);
  glVertex3fv(ep[6][7]);
  glVertex3fv(ep[7][7]);
  glVertex3fv(ep[6][8]);
  glVertex3fv(ep[7][8]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glVertex3fv(ep[7][0]);
  glVertex3fv(ep[8][0]);
  glVertex3fv(ep[7][1]);
  glVertex3fv(ep[8][1]);
  glVertex3fv(ep[7][2]);
  glVertex3fv(ep[8][2]);
  glVertex3fv(ep[7][3]);
  glVertex3fv(ep[8][3]);
  glVertex3fv(ep[7][4]);
  glVertex3fv(ep[8][4]);
  glVertex3fv(ep[7][5]);
  glVertex3fv(ep[8][5]);
  glVertex3fv(ep[7][6]);
  glVertex3fv(ep[8][6]);
  glVertex3fv(ep[7][7]);
  glVertex3fv(ep[8][7]);
  glVertex3fv(ep[7][8]);
  glVertex3fv(ep[8][8]);
  glEnd();
}

static void bend_forward(void) {
  
  glTranslatef(0.0, 1.000000, 0.0);
  glRotatef (0.1 * (900), 1.0, 0.0, 0.0);
  glTranslatef(0.0, -1.000000, 0.0);
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

void draw_logo_shadow(void) {
  
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
}

