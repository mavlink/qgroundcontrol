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

float s_data[][2] = {
	{0.860393, 5.283798},
	{0.529473, 3.550052},
	{0.992761, 4.491228},
	{0.910031, 3.368421},
	{1.240951, 3.830753},
	{1.456050, 3.104231},
	{1.935884, 3.517028},
	{2.002068, 2.988648},
	{2.763185, 3.533540},
	{3.061013, 3.120743},
	{3.391934, 3.748194},
	{4.053774, 3.632611},
	{3.822130, 4.540764},
	{4.550155, 4.590299},
	{3.656670, 5.465428},
	{4.517063, 5.713106},
	{3.276112, 5.894737},
	{3.921407, 6.538700},
	{2.299896, 6.736842},
	{3.044467, 7.430341},
	{1.886246, 7.496388},
	{2.581179, 8.222910},
	{1.902792, 8.751290},
	{2.680455, 8.883385},
	{2.283350, 9.312694},
	{3.358842, 9.609907},
	{3.507756, 9.907121},
	{4.285419, 9.758514},
	{5.112720, 9.973168},
	{4.748707, 9.593395},

};

void draw_s(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(s_data[0]);
	glVertex2fv(s_data[1]);
	glVertex2fv(s_data[2]);
	glVertex2fv(s_data[3]);
	glVertex2fv(s_data[4]);
	glVertex2fv(s_data[5]);
	glVertex2fv(s_data[6]);
	glVertex2fv(s_data[7]);
	glVertex2fv(s_data[8]);
	glVertex2fv(s_data[9]);
	glVertex2fv(s_data[10]);
	glVertex2fv(s_data[11]);
	glVertex2fv(s_data[12]);
	glVertex2fv(s_data[13]);
	glVertex2fv(s_data[14]);
	glVertex2fv(s_data[15]);
	glVertex2fv(s_data[16]);
	glVertex2fv(s_data[17]);
	glVertex2fv(s_data[18]);
	glVertex2fv(s_data[19]);
	glVertex2fv(s_data[20]);
	glVertex2fv(s_data[21]);
	glVertex2fv(s_data[22]);
	glVertex2fv(s_data[23]);
	glVertex2fv(s_data[24]);
	glVertex2fv(s_data[25]);
	glVertex2fv(s_data[26]);
	glVertex2fv(s_data[27]);
	glVertex2fv(s_data[28]);
	glVertex2fv(s_data[29]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(s_data[0]);
	glVertex2fv(s_data[2]);
	glVertex2fv(s_data[4]);
	glVertex2fv(s_data[6]);
	glVertex2fv(s_data[8]);
	glVertex2fv(s_data[10]);
	glVertex2fv(s_data[12]);
	glVertex2fv(s_data[14]);
	glVertex2fv(s_data[16]);
	glVertex2fv(s_data[18]);
	glVertex2fv(s_data[20]);
	glVertex2fv(s_data[22]);
	glVertex2fv(s_data[24]);
	glVertex2fv(s_data[26]);
	glVertex2fv(s_data[28]);
	glVertex2fv(s_data[29]);
	glVertex2fv(s_data[27]);
	glVertex2fv(s_data[25]);
	glVertex2fv(s_data[23]);
	glVertex2fv(s_data[21]);
	glVertex2fv(s_data[19]);
	glVertex2fv(s_data[17]);
	glVertex2fv(s_data[15]);
	glVertex2fv(s_data[13]);
	glVertex2fv(s_data[11]);
	glVertex2fv(s_data[9]);
	glVertex2fv(s_data[7]);
	glVertex2fv(s_data[5]);
	glVertex2fv(s_data[3]);
	glVertex2fv(s_data[1]);
    glEnd();

}

