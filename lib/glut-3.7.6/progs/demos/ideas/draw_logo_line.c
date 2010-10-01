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

float scp[14][3] = {
    {1.000000, 0.000000, 0.000000},	{1.000000, 0.000000, 5.000000},
    {0.500000, 0.866025, 0.000000},	{0.500000, 0.866025, 5.000000},
    {-0.500000, 0.866025, 0.000000},	{-0.500000, 0.866025, 5.000000},
    {-1.000000, 0.000000, 0.000000},	{-1.000000, 0.000000, 5.000000},
    {-0.500000, -0.866025, 0.000000},	{-0.500000, -0.866025, 5.000000},
    {0.500000, -0.866025, 0.000000},	{0.500000, -0.866025, 5.000000},
    {1.000000, 0.000000, 0.000000},	{1.000000, 0.000000, 5.000000},
};

float dcp[14][3] = {
    {1.000000, 0.000000, 0.000000},	{1.000000, 0.000000, 7.000000},
    {0.500000, 0.866025, 0.000000},	{0.500000, 0.866025, 7.000000},
    {-0.500000, 0.866025, 0.000000},	{-0.500000, 0.866025, 7.000000},
    {-1.000000, 0.000000, 0.000000},	{-1.000000, 0.000000, 7.000000},
    {-0.500000, -0.866025, 0.000000},	{-0.500000, -0.866025, 7.000000},
    {0.500000, -0.866025, 0.000000},	{0.500000, -0.866025, 7.000000},
    {1.000000, 0.000000, 0.000000},	{1.000000, 0.000000, 7.000000},
};

float ep[7][7][3] = {
    {
	{1.000000, 0.000000, 0.000000},
	{0.500000, 0.866025, 0.000000},
	{-0.500000, 0.866025, 0.000000},
	{-1.000000, 0.000000, 0.000000},
	{-0.500000, -0.866025, 0.000000},
	{0.500000, -0.866025, 0.000000},
	{1.000000, 0.000000, 0.000000},
    },

    {
	{1.000000, 0.034074, 0.258819},
	{0.500000, 0.870590, 0.034675},
	{-0.500000, 0.870590, 0.034675},
	{-1.000000, 0.034074, 0.258819},
	{-0.500000, -0.802442, 0.482963},
	{0.500000, -0.802442, 0.482963},
	{1.000000, 0.034074, 0.258819},
    },

    {
	{1.000000, 0.133975, 0.500000},
	{0.500000, 0.883975, 0.066987},
	{-0.500000, 0.883975, 0.066987},
	{-1.000000, 0.133975, 0.500000},
	{-0.500000, -0.616025, 0.933013},
	{0.500000, -0.616025, 0.933013},
	{1.000000, 0.133975, 0.500000},
    },

    {
	{1.000000, 0.292893, 0.707107},
	{0.500000, 0.905266, 0.094734},
	{-0.500000, 0.905266, 0.094734},
	{-1.000000, 0.292893, 0.707107},
	{-0.500000, -0.319479, 1.319479},
	{0.500000, -0.319479, 1.319479},
	{1.000000, 0.292893, 0.707107},
    },

    {
	{1.000000, 0.500000, 0.866025},
	{0.500000, 0.933013, 0.116025},
	{-0.500000, 0.933013, 0.116025},
	{-1.000000, 0.500000, 0.866025},
	{-0.500000, 0.066987, 1.616025},
	{0.500000, 0.066987, 1.616025},
	{1.000000, 0.500000, 0.866025},
    },

    {
	{1.000000, 0.741181, 0.965926},
	{0.500000, 0.965325, 0.129410},
	{-0.500000, 0.965325, 0.129410},
	{-1.000000, 0.741181, 0.965926},
	{-0.500000, 0.517037, 1.802442},
	{0.500000, 0.517037, 1.802442},
	{1.000000, 0.741181, 0.965926},
    },

    {
	{1.000000, 1.000000, 1.000000},
	{0.500000, 1.000000, 0.133975},
	{-0.500000, 1.000000, 0.133975},
	{-1.000000, 1.000000, 1.000000},
	{-0.500000, 1.000000, 1.866025},
	{0.500000, 1.000000, 1.866025},
	{1.000000, 1.000000, 1.000000},
    },

};

void draw_single_cylinder(void) {

	glBegin(GL_LINE_STRIP);
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
	glEnd();
}

void draw_double_cylinder(void) {

	glBegin(GL_LINE_STRIP);
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
	glEnd();
}

void draw_elbow(void) {

	glBegin(GL_LINE_STRIP);
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
	glEnd();

	glBegin(GL_LINE_STRIP);
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
	glEnd();

	glBegin(GL_LINE_STRIP);
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
	glEnd();

	glBegin(GL_LINE_STRIP);
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
	glEnd();

	glBegin(GL_LINE_STRIP);
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
	glEnd();

	glBegin(GL_LINE_STRIP);
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
	glEnd();
}

void bend_forward(void) {
  glTranslatef(0.0, 1.000000, 0.0);
  glRotatef (0.1 * (900), 1.0, 0.0, 0.0);
  glTranslatef(0.0, -1.000000, 0.0);
}

void bend_left(void) {
  glRotatef (0.1 * (-900), 0.0, 0.0, 1.0);
  glTranslatef(0.0, 1.000000, 0.0);
  glRotatef (0.1 * (900), 1.0, 0.0, 0.0);
  glTranslatef(0.0, -1.000000, 0.0);
}

void bend_right(void) {
  glRotatef (0.1 * (900), 0.0, 0.0, 1.0);
  glTranslatef(0.0, 1.000000, 0.0);
  glRotatef (0.1 * (900), 1.0, 0.0, 0.0);
  glTranslatef(0.0, -1.000000, 0.0);
}

void draw_logo_line(void) {

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
}

