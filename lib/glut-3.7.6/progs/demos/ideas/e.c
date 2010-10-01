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

float e_data[][2] = {
	{1.095436, 6.190871},
	{2.107884, 6.970954},
	{2.556017, 7.020747},
	{3.020747, 7.867220},
	{3.518672, 8.033195},
	{3.269710, 8.531120},
	{4.165975, 8.929461},
	{3.302905, 9.062241},
	{4.331950, 9.626556},
	{3.286307, 9.344398},
	{4.116183, 9.958507},
	{3.004149, 9.510373},
	{3.518672, 9.991701},
	{2.705394, 9.493776},
	{2.091286, 9.311203},
	{2.041494, 9.062241},
	{1.178423, 8.514523},
	{1.443983, 8.165976},
	{0.481328, 7.535270},
	{1.045643, 6.904564},
	{0.149378, 6.091286},
	{1.095436, 5.410789},
	{0.464730, 4.232365},
	{1.377593, 4.497925},
	{1.261411, 3.136930},
	{1.925311, 3.950207},
	{2.240664, 3.037344},
	{2.589212, 3.834025},
	{3.087137, 3.269710},
	{3.236515, 3.867220},
	{3.684647, 3.867220},
	{3.867220, 4.448133},
	{4.398340, 5.128631},

};

void draw_e(void) {

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2fv(e_data[0]);
	glVertex2fv(e_data[1]);
	glVertex2fv(e_data[2]);
	glVertex2fv(e_data[3]);
	glVertex2fv(e_data[4]);
	glVertex2fv(e_data[5]);
	glVertex2fv(e_data[6]);
	glVertex2fv(e_data[7]);
	glVertex2fv(e_data[8]);
	glVertex2fv(e_data[9]);
	glVertex2fv(e_data[10]);
	glVertex2fv(e_data[11]);
	glVertex2fv(e_data[12]);
	glVertex2fv(e_data[13]);
	glVertex2fv(e_data[14]);
	glVertex2fv(e_data[15]);
	glVertex2fv(e_data[16]);
	glVertex2fv(e_data[17]);
	glVertex2fv(e_data[18]);
	glVertex2fv(e_data[19]);
	glVertex2fv(e_data[20]);
	glVertex2fv(e_data[21]);
	glVertex2fv(e_data[22]);
	glVertex2fv(e_data[23]);
	glVertex2fv(e_data[24]);
	glVertex2fv(e_data[25]);
	glVertex2fv(e_data[26]);
	glVertex2fv(e_data[27]);
	glVertex2fv(e_data[28]);
	glVertex2fv(e_data[29]);
	glVertex2fv(e_data[30]);
	glVertex2fv(e_data[31]);
	glVertex2fv(e_data[32]);
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2fv(e_data[0]);
	glVertex2fv(e_data[2]);
	glVertex2fv(e_data[4]);
	glVertex2fv(e_data[6]);
	glVertex2fv(e_data[8]);
	glVertex2fv(e_data[10]);
	glVertex2fv(e_data[12]);
	glVertex2fv(e_data[14]);
	glVertex2fv(e_data[16]);
	glVertex2fv(e_data[18]);
	glVertex2fv(e_data[20]);
	glVertex2fv(e_data[22]);
	glVertex2fv(e_data[24]);
	glVertex2fv(e_data[26]);
	glVertex2fv(e_data[28]);
	glVertex2fv(e_data[30]);
	glVertex2fv(e_data[32]);
	glVertex2fv(e_data[31]);
	glVertex2fv(e_data[29]);
	glVertex2fv(e_data[27]);
	glVertex2fv(e_data[25]);
	glVertex2fv(e_data[23]);
	glVertex2fv(e_data[21]);
	glVertex2fv(e_data[19]);
	glVertex2fv(e_data[17]);
	glVertex2fv(e_data[15]);
	glVertex2fv(e_data[13]);
	glVertex2fv(e_data[11]);
	glVertex2fv(e_data[9]);
	glVertex2fv(e_data[7]);
	glVertex2fv(e_data[5]);
	glVertex2fv(e_data[3]);
	glVertex2fv(e_data[1]);
    glEnd();

}

