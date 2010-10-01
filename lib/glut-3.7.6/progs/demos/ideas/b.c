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

float b_data[][2] = {
	{1.437827, 15.482255},
	{3.711599, 15.716075},
	{2.658307, 15.315240},
	{3.477534, 15.081420},
	{2.741902, 14.997912},
	{2.006270, 7.983298},
	{0.234065, 3.139875},
	{1.103448, 4.192067},
	{1.538140, 3.139875},
	{1.404389, 3.958246},
	{2.374086, 3.306889},
	{2.792058, 3.807933},
	{3.243469, 3.691023},
	{3.544410, 4.158664},
	{4.497388, 4.776618},
	{3.979101, 4.759916},
	{4.815047, 5.227557},
	{4.413793, 5.979123},
	{5.400209, 6.864301},
	{4.497388, 8.133612},
	{5.667712, 8.734864},
	{4.263323, 9.002088},
	{5.416928, 9.686848},
	{4.012539, 9.219207},
	{4.898642, 10.020877},
	{3.494253, 9.118998},
	{3.745037, 9.620042},
	{2.775340, 8.684760},
	{2.708464, 8.835073},
	{1.805643, 7.382046},
	{1.688610, 7.582463},

};

void draw_b(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(b_data[0]);
	glVertex2fv(b_data[1]);
	glVertex2fv(b_data[2]);
	glVertex2fv(b_data[3]);
	glVertex2fv(b_data[4]);
	glVertex2fv(b_data[5]);
	glVertex2fv(b_data[6]);
	glVertex2fv(b_data[7]);
	glVertex2fv(b_data[8]);
	glVertex2fv(b_data[9]);
	glVertex2fv(b_data[10]);
	glVertex2fv(b_data[11]);
	glVertex2fv(b_data[12]);
	glVertex2fv(b_data[13]);
	glVertex2fv(b_data[14]);
	glVertex2fv(b_data[15]);
	glVertex2fv(b_data[16]);
	glVertex2fv(b_data[17]);
	glVertex2fv(b_data[18]);
	glVertex2fv(b_data[19]);
	glVertex2fv(b_data[20]);
	glVertex2fv(b_data[21]);
	glVertex2fv(b_data[22]);
	glVertex2fv(b_data[23]);
	glVertex2fv(b_data[24]);
	glVertex2fv(b_data[25]);
	glVertex2fv(b_data[26]);
	glVertex2fv(b_data[27]);
	glVertex2fv(b_data[28]);
	glVertex2fv(b_data[29]);
	glVertex2fv(b_data[30]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(b_data[0]);
	glVertex2fv(b_data[2]);
	glVertex2fv(b_data[4]);
	glVertex2fv(b_data[6]);
	glVertex2fv(b_data[8]);
	glVertex2fv(b_data[10]);
	glVertex2fv(b_data[12]);
	glVertex2fv(b_data[14]);
	glVertex2fv(b_data[16]);
	glVertex2fv(b_data[18]);
	glVertex2fv(b_data[20]);
	glVertex2fv(b_data[22]);
	glVertex2fv(b_data[24]);
	glVertex2fv(b_data[26]);
	glVertex2fv(b_data[28]);
	glVertex2fv(b_data[30]);
	glVertex2fv(b_data[29]);
	glVertex2fv(b_data[27]);
	glVertex2fv(b_data[25]);
	glVertex2fv(b_data[23]);
	glVertex2fv(b_data[21]);
	glVertex2fv(b_data[19]);
	glVertex2fv(b_data[17]);
	glVertex2fv(b_data[15]);
	glVertex2fv(b_data[13]);
	glVertex2fv(b_data[11]);
	glVertex2fv(b_data[9]);
	glVertex2fv(b_data[7]);
	glVertex2fv(b_data[5]);
	glVertex2fv(b_data[3]);
	glVertex2fv(b_data[1]);
	glVertex2fv(b_data[0]);
    glEnd();

}

