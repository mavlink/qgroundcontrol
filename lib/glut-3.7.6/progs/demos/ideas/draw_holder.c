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

float bn[5][3] = {
	{-1.0, 0.0, 0.0},
	{0.0, 1.0, 0.0},
	{1.0, 0.0, 0.0},
	{0.0, -1.0, 0.0},
	{0.0, 0.0, 1.0},
};

float bp[4][8][3] = {
    {   
	{-14.000000, -14.000000, 0.000000},
	{-14.000000, -14.000000, 4.000000},
	{-14.000000, 14.000000, 0.000000},
	{-14.000000, 14.000000, 4.000000},
	{14.000000, 14.000000, 0.000000},
	{14.000000, 14.000000, 4.000000},
	{14.000000, -14.000000, 0.000000},
	{14.000000, -14.000000, 4.000000},
    },
    {
	{-12.000000, -12.000000, 4.000000},
	{-12.000000, -12.000000, 8.000000},
	{-12.000000, 12.000000, 4.000000},
	{-12.000000, 12.000000, 8.000000},
	{12.000000, 12.000000, 4.000000},
	{12.000000, 12.000000, 8.000000},
	{12.000000, -12.000000, 4.000000},
	{12.000000, -12.000000, 8.000000},
    },
    {
	{-10.000000, -10.000000, 8.000000},
	{-10.000000, -10.000000, 12.000000},
	{-10.000000, 10.000000, 8.000000},
	{-10.000000, 10.000000, 12.000000},
	{10.000000, 10.000000, 8.000000},
	{10.000000, 10.000000, 12.000000},
	{10.000000, -10.000000, 8.000000},
	{10.000000, -10.000000, 12.000000},
    },
    {
	{-8.000000, -8.000000, 12.000000},
	{-8.000000, -8.000000, 8.000000},
	{-8.000000, 8.000000, 12.000000},
	{-8.000000, 8.000000, 8.000000},
	{8.000000, 8.000000, 12.000000},
	{8.000000, 8.000000, 8.000000},
	{8.000000, -8.000000, 12.000000},
	{8.000000, -8.000000, 8.000000},
    },
};

float tp[12][21][3] = {
     {
	{10.000000, 0.000000, 1.000000},
	{9.510565, -3.090170, 1.000000},
	{8.090170, -5.877852, 1.000000},
	{5.877852, -8.090170, 1.000000},
	{3.090170, -9.510565, 1.000000},
	{0.000000, -10.000000, 1.000000},
	{-3.090170, -9.510565, 1.000000},
	{-5.877852, -8.090170, 1.000000},
	{-8.090170, -5.877852, 1.000000},
	{-9.510565, -3.090170, 1.000000},
	{-10.000000, 0.000000, 1.000000},
	{-9.510565, 3.090170, 1.000000},
	{-8.090170, 5.877852, 1.000000},
	{-5.877852, 8.090170, 1.000000},
	{-3.090170, 9.510565, 1.000000},
	{0.000000, 10.000000, 1.000000},
	{3.090170, 9.510565, 1.000000},
	{5.877852, 8.090170, 1.000000},
	{8.090170, 5.877852, 1.000000},
	{9.510565, 3.090170, 1.000000},
	{10.000000, 0.000000, 1.000000},
    },

     {
	{10.540641, 0.000000, 0.841254},
	{10.024745, -3.257237, 0.841254},
	{8.527557, -6.195633, 0.841254},
	{6.195633, -8.527557, 0.841254},
	{3.257237, -10.024745, 0.841254},
	{0.000000, -10.540641, 0.841254},
	{-3.257237, -10.024745, 0.841254},
	{-6.195633, -8.527557, 0.841254},
	{-8.527557, -6.195633, 0.841254},
	{-10.024745, -3.257237, 0.841254},
	{-10.540641, 0.000000, 0.841254},
	{-10.024745, 3.257237, 0.841254},
	{-8.527557, 6.195633, 0.841254},
	{-6.195633, 8.527557, 0.841254},
	{-3.257237, 10.024745, 0.841254},
	{0.000000, 10.540641, 0.841254},
	{3.257237, 10.024745, 0.841254},
	{6.195633, 8.527557, 0.841254},
	{8.527557, 6.195633, 0.841254},
	{10.024745, 3.257237, 0.841254},
	{10.540641, 0.000000, 0.841254},
    },

     {
	{10.909632, 0.000000, 0.415415},
	{10.375676, -3.371262, 0.415415},
	{8.826077, -6.412521, 0.415415},
	{6.412521, -8.826077, 0.415415},
	{3.371262, -10.375676, 0.415415},
	{0.000000, -10.909632, 0.415415},
	{-3.371262, -10.375676, 0.415415},
	{-6.412521, -8.826077, 0.415415},
	{-8.826077, -6.412521, 0.415415},
	{-10.375676, -3.371262, 0.415415},
	{-10.909632, 0.000000, 0.415415},
	{-10.375676, 3.371262, 0.415415},
	{-8.826077, 6.412521, 0.415415},
	{-6.412521, 8.826077, 0.415415},
	{-3.371262, 10.375676, 0.415415},
	{0.000000, 10.909632, 0.415415},
	{3.371262, 10.375676, 0.415415},
	{6.412521, 8.826077, 0.415415},
	{8.826077, 6.412521, 0.415415},
	{10.375676, 3.371262, 0.415415},
	{10.909632, 0.000000, 0.415415},
    },

     {
	{10.989821, 0.000000, -0.142315},
	{10.451941, -3.396042, -0.142315},
	{8.890952, -6.459655, -0.142315},
	{6.459655, -8.890952, -0.142315},
	{3.396042, -10.451941, -0.142315},
	{0.000000, -10.989821, -0.142315},
	{-3.396042, -10.451941, -0.142315},
	{-6.459655, -8.890952, -0.142315},
	{-8.890952, -6.459655, -0.142315},
	{-10.451941, -3.396042, -0.142315},
	{-10.989821, 0.000000, -0.142315},
	{-10.451941, 3.396042, -0.142315},
	{-8.890952, 6.459655, -0.142315},
	{-6.459655, 8.890952, -0.142315},
	{-3.396042, 10.451941, -0.142315},
	{0.000000, 10.989821, -0.142315},
	{3.396042, 10.451941, -0.142315},
	{6.459655, 8.890952, -0.142315},
	{8.890952, 6.459655, -0.142315},
	{10.451941, 3.396042, -0.142315},
	{10.989821, 0.000000, -0.142315},
    },

     {
	{10.755750, 0.000000, -0.654861},
	{10.229325, -3.323709, -0.654861},
	{8.701584, -6.322071, -0.654861},
	{6.322071, -8.701584, -0.654861},
	{3.323709, -10.229325, -0.654861},
	{0.000000, -10.755750, -0.654861},
	{-3.323709, -10.229325, -0.654861},
	{-6.322071, -8.701584, -0.654861},
	{-8.701584, -6.322071, -0.654861},
	{-10.229325, -3.323709, -0.654861},
	{-10.755750, 0.000000, -0.654861},
	{-10.229325, 3.323709, -0.654861},
	{-8.701584, 6.322071, -0.654861},
	{-6.322071, 8.701584, -0.654861},
	{-3.323709, 10.229325, -0.654861},
	{0.000000, 10.755750, -0.654861},
	{3.323709, 10.229325, -0.654861},
	{6.322071, 8.701584, -0.654861},
	{8.701584, 6.322071, -0.654861},
	{10.229325, 3.323709, -0.654861},
	{10.755750, 0.000000, -0.654861},
    },

     {
	{10.281733, 0.000000, -0.959493},
	{9.778509, -3.177230, -0.959493},
	{8.318096, -6.043451, -0.959493},
	{6.043451, -8.318096, -0.959493},
	{3.177230, -9.778509, -0.959493},
	{0.000000, -10.281733, -0.959493},
	{-3.177230, -9.778509, -0.959493},
	{-6.043451, -8.318096, -0.959493},
	{-8.318096, -6.043451, -0.959493},
	{-9.778509, -3.177230, -0.959493},
	{-10.281733, 0.000000, -0.959493},
	{-9.778509, 3.177230, -0.959493},
	{-8.318096, 6.043451, -0.959493},
	{-6.043451, 8.318096, -0.959493},
	{-3.177230, 9.778509, -0.959493},
	{0.000000, 10.281733, -0.959493},
	{3.177230, 9.778509, -0.959493},
	{6.043451, 8.318096, -0.959493},
	{8.318096, 6.043451, -0.959493},
	{9.778509, 3.177230, -0.959493},
	{10.281733, 0.000000, -0.959493},
    },

     {
	{9.718267, 0.000000, -0.959493},
	{9.242621, -3.003110, -0.959493},
	{7.862244, -5.712255, -0.959493},
	{5.712255, -7.862244, -0.959493},
	{3.003110, -9.242621, -0.959493},
	{0.000000, -9.718267, -0.959493},
	{-3.003110, -9.242621, -0.959493},
	{-5.712255, -7.862244, -0.959493},
	{-7.862244, -5.712255, -0.959493},
	{-9.242621, -3.003110, -0.959493},
	{-9.718267, 0.000000, -0.959493},
	{-9.242621, 3.003110, -0.959493},
	{-7.862244, 5.712255, -0.959493},
	{-5.712255, 7.862244, -0.959493},
	{-3.003110, 9.242621, -0.959493},
	{0.000000, 9.718267, -0.959493},
	{3.003110, 9.242621, -0.959493},
	{5.712255, 7.862244, -0.959493},
	{7.862244, 5.712255, -0.959493},
	{9.242621, 3.003110, -0.959493},
	{9.718267, 0.000000, -0.959493},
    },

     {
	{9.244250, 0.000000, -0.654861},
	{8.791805, -2.856631, -0.654861},
	{7.478756, -5.433634, -0.654861},
	{5.433634, -7.478756, -0.654861},
	{2.856631, -8.791805, -0.654861},
	{0.000000, -9.244250, -0.654861},
	{-2.856631, -8.791805, -0.654861},
	{-5.433634, -7.478756, -0.654861},
	{-7.478756, -5.433634, -0.654861},
	{-8.791805, -2.856631, -0.654861},
	{-9.244250, 0.000000, -0.654861},
	{-8.791805, 2.856631, -0.654861},
	{-7.478756, 5.433634, -0.654861},
	{-5.433634, 7.478756, -0.654861},
	{-2.856631, 8.791805, -0.654861},
	{0.000000, 9.244250, -0.654861},
	{2.856631, 8.791805, -0.654861},
	{5.433634, 7.478756, -0.654861},
	{7.478756, 5.433634, -0.654861},
	{8.791805, 2.856631, -0.654861},
	{9.244250, 0.000000, -0.654861},
    },

     {
	{9.010179, 0.000000, -0.142315},
	{8.569189, -2.784298, -0.142315},
	{7.289388, -5.296050, -0.142315},
	{5.296050, -7.289388, -0.142315},
	{2.784298, -8.569189, -0.142315},
	{0.000000, -9.010179, -0.142315},
	{-2.784298, -8.569189, -0.142315},
	{-5.296050, -7.289388, -0.142315},
	{-7.289388, -5.296050, -0.142315},
	{-8.569189, -2.784298, -0.142315},
	{-9.010179, 0.000000, -0.142315},
	{-8.569189, 2.784298, -0.142315},
	{-7.289388, 5.296050, -0.142315},
	{-5.296050, 7.289388, -0.142315},
	{-2.784298, 8.569189, -0.142315},
	{0.000000, 9.010179, -0.142315},
	{2.784298, 8.569189, -0.142315},
	{5.296050, 7.289388, -0.142315},
	{7.289388, 5.296050, -0.142315},
	{8.569189, 2.784298, -0.142315},
	{9.010179, 0.000000, -0.142315},
    },

     {
	{9.090367, 0.000000, 0.415414},
	{8.645453, -2.809078, 0.415414},
	{7.354262, -5.343184, 0.415414},
	{5.343184, -7.354262, 0.415414},
	{2.809078, -8.645453, 0.415414},
	{0.000000, -9.090367, 0.415414},
	{-2.809078, -8.645453, 0.415414},
	{-5.343184, -7.354262, 0.415414},
	{-7.354262, -5.343184, 0.415414},
	{-8.645453, -2.809078, 0.415414},
	{-9.090367, 0.000000, 0.415414},
	{-8.645453, 2.809078, 0.415414},
	{-7.354262, 5.343184, 0.415414},
	{-5.343184, 7.354262, 0.415414},
	{-2.809078, 8.645453, 0.415414},
	{0.000000, 9.090367, 0.415414},
	{2.809078, 8.645453, 0.415414},
	{5.343184, 7.354262, 0.415414},
	{7.354262, 5.343184, 0.415414},
	{8.645453, 2.809078, 0.415414},
	{9.090367, 0.000000, 0.415414},
    },

     {
	{9.459358, 0.000000, 0.841253},
	{8.996385, -2.923103, 0.841253},
	{7.652781, -5.560071, 0.841253},
	{5.560071, -7.652781, 0.841253},
	{2.923103, -8.996385, 0.841253},
	{0.000000, -9.459358, 0.841253},
	{-2.923103, -8.996385, 0.841253},
	{-5.560071, -7.652781, 0.841253},
	{-7.652781, -5.560071, 0.841253},
	{-8.996385, -2.923103, 0.841253},
	{-9.459358, 0.000000, 0.841253},
	{-8.996385, 2.923103, 0.841253},
	{-7.652781, 5.560071, 0.841253},
	{-5.560071, 7.652781, 0.841253},
	{-2.923103, 8.996385, 0.841253},
	{0.000000, 9.459358, 0.841253},
	{2.923103, 8.996385, 0.841253},
	{5.560071, 7.652781, 0.841253},
	{7.652781, 5.560071, 0.841253},
	{8.996385, 2.923103, 0.841253},
	{9.459358, 0.000000, 0.841253},
    },

     {
	{9.999999, 0.000000, 1.000000},
	{9.510564, -3.090170, 1.000000},
	{8.090169, -5.877852, 1.000000},
	{5.877852, -8.090169, 1.000000},
	{3.090170, -9.510564, 1.000000},
	{0.000000, -9.999999, 1.000000},
	{-3.090170, -9.510564, 1.000000},
	{-5.877852, -8.090169, 1.000000},
	{-8.090169, -5.877852, 1.000000},
	{-9.510564, -3.090170, 1.000000},
	{-9.999999, 0.000000, 1.000000},
	{-9.510564, 3.090170, 1.000000},
	{-8.090169, 5.877852, 1.000000},
	{-5.877852, 8.090169, 1.000000},
	{-3.090170, 9.510564, 1.000000},
	{0.000000, 9.999999, 1.000000},
	{3.090170, 9.510564, 1.000000},
	{5.877852, 8.090169, 1.000000},
	{8.090169, 5.877852, 1.000000},
	{9.510564, 3.090170, 1.000000},
	{9.999999, 0.000000, 1.000000},
    },

};

float tn[12][21][3] = {
    {
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
	{0.000000, 0.000000, 1.000000},
    },

    {
	{0.540641, 0.000000, 0.841254},
	{0.514180, -0.167067, 0.841254},
	{0.437388, -0.317781, 0.841254},
	{0.317781, -0.437388, 0.841254},
	{0.167067, -0.514180, 0.841254},
	{0.000000, -0.540641, 0.841254},
	{-0.167067, -0.514180, 0.841254},
	{-0.317781, -0.437388, 0.841254},
	{-0.437388, -0.317781, 0.841254},
	{-0.514180, -0.167067, 0.841254},
	{-0.540641, 0.000000, 0.841254},
	{-0.514180, 0.167067, 0.841254},
	{-0.437388, 0.317781, 0.841254},
	{-0.317781, 0.437388, 0.841254},
	{-0.167067, 0.514180, 0.841254},
	{0.000000, 0.540641, 0.841254},
	{0.167067, 0.514180, 0.841254},
	{0.317781, 0.437388, 0.841254},
	{0.437388, 0.317781, 0.841254},
	{0.514180, 0.167067, 0.841254},
	{0.540641, 0.000000, 0.841254},
    },

    {
	{0.909632, 0.000000, 0.415415},
	{0.865111, -0.281092, 0.415415},
	{0.735908, -0.534668, 0.415415},
	{0.534668, -0.735908, 0.415415},
	{0.281092, -0.865111, 0.415415},
	{0.000000, -0.909632, 0.415415},
	{-0.281092, -0.865111, 0.415415},
	{-0.534668, -0.735908, 0.415415},
	{-0.735908, -0.534668, 0.415415},
	{-0.865111, -0.281092, 0.415415},
	{-0.909632, 0.000000, 0.415415},
	{-0.865111, 0.281092, 0.415415},
	{-0.735908, 0.534668, 0.415415},
	{-0.534668, 0.735908, 0.415415},
	{-0.281092, 0.865111, 0.415415},
	{0.000000, 0.909632, 0.415415},
	{0.281092, 0.865111, 0.415415},
	{0.534668, 0.735908, 0.415415},
	{0.735908, 0.534668, 0.415415},
	{0.865111, 0.281092, 0.415415},
	{0.909632, 0.000000, 0.415415},
    },

    {
	{0.989821, 0.000000, -0.142315},
	{0.941376, -0.305872, -0.142315},
	{0.800782, -0.581802, -0.142315},
	{0.581802, -0.800782, -0.142315},
	{0.305872, -0.941376, -0.142315},
	{0.000000, -0.989821, -0.142315},
	{-0.305872, -0.941376, -0.142315},
	{-0.581802, -0.800782, -0.142315},
	{-0.800782, -0.581802, -0.142315},
	{-0.941376, -0.305872, -0.142315},
	{-0.989821, 0.000000, -0.142315},
	{-0.941376, 0.305872, -0.142315},
	{-0.800782, 0.581802, -0.142315},
	{-0.581802, 0.800782, -0.142315},
	{-0.305872, 0.941376, -0.142315},
	{0.000000, 0.989821, -0.142315},
	{0.305872, 0.941376, -0.142315},
	{0.581802, 0.800782, -0.142315},
	{0.800782, 0.581802, -0.142315},
	{0.941376, 0.305872, -0.142315},
	{0.989821, 0.000000, -0.142315},
    },

    {
	{0.755750, 0.000000, -0.654861},
	{0.718761, -0.233539, -0.654861},
	{0.611414, -0.444218, -0.654861},
	{0.444218, -0.611414, -0.654861},
	{0.233539, -0.718761, -0.654861},
	{0.000000, -0.755750, -0.654861},
	{-0.233539, -0.718761, -0.654861},
	{-0.444218, -0.611414, -0.654861},
	{-0.611414, -0.444218, -0.654861},
	{-0.718761, -0.233539, -0.654861},
	{-0.755750, 0.000000, -0.654861},
	{-0.718761, 0.233539, -0.654861},
	{-0.611414, 0.444218, -0.654861},
	{-0.444218, 0.611414, -0.654861},
	{-0.233539, 0.718761, -0.654861},
	{0.000000, 0.755750, -0.654861},
	{0.233539, 0.718761, -0.654861},
	{0.444218, 0.611414, -0.654861},
	{0.611414, 0.444218, -0.654861},
	{0.718761, 0.233539, -0.654861},
	{0.755750, 0.000000, -0.654861},
    },

    {
	{0.281733, 0.000000, -0.959493},
	{0.267944, -0.087060, -0.959493},
	{0.227927, -0.165598, -0.959493},
	{0.165598, -0.227927, -0.959493},
	{0.087060, -0.267944, -0.959493},
	{0.000000, -0.281733, -0.959493},
	{-0.087060, -0.267944, -0.959493},
	{-0.165598, -0.227927, -0.959493},
	{-0.227927, -0.165598, -0.959493},
	{-0.267944, -0.087060, -0.959493},
	{-0.281733, 0.000000, -0.959493},
	{-0.267944, 0.087060, -0.959493},
	{-0.227927, 0.165598, -0.959493},
	{-0.165598, 0.227927, -0.959493},
	{-0.087060, 0.267944, -0.959493},
	{0.000000, 0.281733, -0.959493},
	{0.087060, 0.267944, -0.959493},
	{0.165598, 0.227927, -0.959493},
	{0.227927, 0.165598, -0.959493},
	{0.267944, 0.087060, -0.959493},
	{0.281733, 0.000000, -0.959493},
    },

    {
	{-0.281732, 0.000000, -0.959493},
	{-0.267943, 0.087060, -0.959493},
	{-0.227926, 0.165598, -0.959493},
	{-0.165598, 0.227926, -0.959493},
	{-0.087060, 0.267943, -0.959493},
	{0.000000, 0.281732, -0.959493},
	{0.087060, 0.267943, -0.959493},
	{0.165598, 0.227926, -0.959493},
	{0.227926, 0.165598, -0.959493},
	{0.267943, 0.087060, -0.959493},
	{0.281732, 0.000000, -0.959493},
	{0.267943, -0.087060, -0.959493},
	{0.227926, -0.165598, -0.959493},
	{0.165598, -0.227926, -0.959493},
	{0.087060, -0.267943, -0.959493},
	{0.000000, -0.281732, -0.959493},
	{-0.087060, -0.267943, -0.959493},
	{-0.165598, -0.227926, -0.959493},
	{-0.227926, -0.165598, -0.959493},
	{-0.267943, -0.087060, -0.959493},
	{-0.281732, 0.000000, -0.959493},
    },

    {
	{-0.755749, 0.000000, -0.654861},
	{-0.718760, 0.233539, -0.654861},
	{-0.611414, 0.444218, -0.654861},
	{-0.444218, 0.611414, -0.654861},
	{-0.233539, 0.718760, -0.654861},
	{0.000000, 0.755749, -0.654861},
	{0.233539, 0.718760, -0.654861},
	{0.444218, 0.611414, -0.654861},
	{0.611414, 0.444218, -0.654861},
	{0.718760, 0.233539, -0.654861},
	{0.755749, 0.000000, -0.654861},
	{0.718760, -0.233539, -0.654861},
	{0.611414, -0.444218, -0.654861},
	{0.444218, -0.611414, -0.654861},
	{0.233539, -0.718760, -0.654861},
	{0.000000, -0.755749, -0.654861},
	{-0.233539, -0.718760, -0.654861},
	{-0.444218, -0.611414, -0.654861},
	{-0.611414, -0.444218, -0.654861},
	{-0.718760, -0.233539, -0.654861},
	{-0.755749, 0.000000, -0.654861},
    },

    {
	{-0.989821, 0.000000, -0.142315},
	{-0.941376, 0.305872, -0.142315},
	{-0.800782, 0.581802, -0.142315},
	{-0.581802, 0.800782, -0.142315},
	{-0.305872, 0.941376, -0.142315},
	{0.000000, 0.989821, -0.142315},
	{0.305872, 0.941376, -0.142315},
	{0.581802, 0.800782, -0.142315},
	{0.800782, 0.581802, -0.142315},
	{0.941376, 0.305872, -0.142315},
	{0.989821, 0.000000, -0.142315},
	{0.941376, -0.305872, -0.142315},
	{0.800782, -0.581802, -0.142315},
	{0.581802, -0.800782, -0.142315},
	{0.305872, -0.941376, -0.142315},
	{0.000000, -0.989821, -0.142315},
	{-0.305872, -0.941376, -0.142315},
	{-0.581802, -0.800782, -0.142315},
	{-0.800782, -0.581802, -0.142315},
	{-0.941376, -0.305872, -0.142315},
	{-0.989821, 0.000000, -0.142315},
    },

    {
	{-0.909632, 0.000000, 0.415414},
	{-0.865112, 0.281092, 0.415414},
	{-0.735908, 0.534668, 0.415414},
	{-0.534668, 0.735908, 0.415414},
	{-0.281092, 0.865112, 0.415414},
	{0.000000, 0.909632, 0.415414},
	{0.281092, 0.865112, 0.415414},
	{0.534668, 0.735908, 0.415414},
	{0.735908, 0.534668, 0.415414},
	{0.865112, 0.281092, 0.415414},
	{0.909632, 0.000000, 0.415414},
	{0.865112, -0.281092, 0.415414},
	{0.735908, -0.534668, 0.415414},
	{0.534668, -0.735908, 0.415414},
	{0.281092, -0.865112, 0.415414},
	{0.000000, -0.909632, 0.415414},
	{-0.281092, -0.865112, 0.415414},
	{-0.534668, -0.735908, 0.415414},
	{-0.735908, -0.534668, 0.415414},
	{-0.865112, -0.281092, 0.415414},
	{-0.909632, 0.000000, 0.415414},
    },

    {
	{-0.540642, 0.000000, 0.841253},
	{-0.514181, 0.167067, 0.841253},
	{-0.437388, 0.317781, 0.841253},
	{-0.317781, 0.437388, 0.841253},
	{-0.167067, 0.514181, 0.841253},
	{0.000000, 0.540642, 0.841253},
	{0.167067, 0.514181, 0.841253},
	{0.317781, 0.437388, 0.841253},
	{0.437388, 0.317781, 0.841253},
	{0.514181, 0.167067, 0.841253},
	{0.540642, 0.000000, 0.841253},
	{0.514181, -0.167067, 0.841253},
	{0.437388, -0.317781, 0.841253},
	{0.317781, -0.437388, 0.841253},
	{0.167067, -0.514181, 0.841253},
	{0.000000, -0.540642, 0.841253},
	{-0.167067, -0.514181, 0.841253},
	{-0.317781, -0.437388, 0.841253},
	{-0.437388, -0.317781, 0.841253},
	{-0.514181, -0.167067, 0.841253},
	{-0.540642, 0.000000, 0.841253},
    },

    {
	{-0.000001, 0.000000, 1.000000},
	{-0.000001, 0.000000, 1.000000},
	{-0.000001, 0.000001, 1.000000},
	{-0.000001, 0.000001, 1.000000},
	{0.000000, 0.000001, 1.000000},
	{0.000000, 0.000001, 1.000000},
	{0.000000, 0.000001, 1.000000},
	{0.000001, 0.000001, 1.000000},
	{0.000001, 0.000001, 1.000000},
	{0.000001, 0.000000, 1.000000},
	{0.000001, 0.000000, 1.000000},
	{0.000001, 0.000000, 1.000000},
	{0.000001, -0.000001, 1.000000},
	{0.000001, -0.000001, 1.000000},
	{0.000000, -0.000001, 1.000000},
	{0.000000, -0.000001, 1.000000},
	{0.000000, -0.000001, 1.000000},
	{-0.000001, -0.000001, 1.000000},
	{-0.000001, -0.000001, 1.000000},
	{-0.000001, 0.000000, 1.000000},
	{-0.000001, 0.000000, 1.000000},
    },

};

void draw_base(void) {
  
  glCallList( MAT_HOLDER_BASE); 
  
  glBegin(GL_POLYGON);
  glNormal3fv(bn[0]);
  glVertex3fv(bp[0][0]);
  glVertex3fv(bp[0][1]);
  glVertex3fv(bp[0][3]);
  glVertex3fv(bp[0][2]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[1]);
  glVertex3fv(bp[0][2]);
  glVertex3fv(bp[0][3]);
  glVertex3fv(bp[0][5]);
  glVertex3fv(bp[0][4]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[2]);
  glVertex3fv(bp[0][4]);
  glVertex3fv(bp[0][5]);
  glVertex3fv(bp[0][7]);
  glVertex3fv(bp[0][6]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[3]);
  glVertex3fv(bp[0][6]);
  glVertex3fv(bp[0][7]);
  glVertex3fv(bp[0][1]);
  glVertex3fv(bp[0][0]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(bn[4]);
  glVertex3fv(bp[0][1]);
  glVertex3fv(bp[1][0]);
  glVertex3fv(bp[0][3]);
  glVertex3fv(bp[1][2]);
  glVertex3fv(bp[0][5]);
  glVertex3fv(bp[1][4]);
  glVertex3fv(bp[0][7]);
  glVertex3fv(bp[1][6]);
  glVertex3fv(bp[0][1]);
  glVertex3fv(bp[1][0]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[0]);
  glVertex3fv(bp[1][0]);
  glVertex3fv(bp[1][1]);
  glVertex3fv(bp[1][3]);
  glVertex3fv(bp[1][2]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[1]);
  glVertex3fv(bp[1][2]);
  glVertex3fv(bp[1][3]);
  glVertex3fv(bp[1][5]);
  glVertex3fv(bp[1][4]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[2]);
  glVertex3fv(bp[1][4]);
  glVertex3fv(bp[1][5]);
  glVertex3fv(bp[1][7]);
  glVertex3fv(bp[1][6]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[3]);
  glVertex3fv(bp[1][6]);
  glVertex3fv(bp[1][7]);
  glVertex3fv(bp[1][1]);
  glVertex3fv(bp[1][0]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(bn[4]);
  glVertex3fv(bp[1][1]);
  glVertex3fv(bp[2][0]);
  glVertex3fv(bp[1][3]);
  glVertex3fv(bp[2][2]);
  glVertex3fv(bp[1][5]);
  glVertex3fv(bp[2][4]);
  glVertex3fv(bp[1][7]);
  glVertex3fv(bp[2][6]);
  glVertex3fv(bp[1][1]);
  glVertex3fv(bp[2][0]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[0]);
  glVertex3fv(bp[2][0]);
  glVertex3fv(bp[2][1]);
  glVertex3fv(bp[2][3]);
  glVertex3fv(bp[2][2]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[1]);
  glVertex3fv(bp[2][2]);
  glVertex3fv(bp[2][3]);
  glVertex3fv(bp[2][5]);
  glVertex3fv(bp[2][4]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[2]);
  glVertex3fv(bp[2][4]);
  glVertex3fv(bp[2][5]);
  glVertex3fv(bp[2][7]);
  glVertex3fv(bp[2][6]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[3]);
  glVertex3fv(bp[2][6]);
  glVertex3fv(bp[2][7]);
  glVertex3fv(bp[2][1]);
  glVertex3fv(bp[2][0]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(bn[4]);
  glVertex3fv(bp[2][1]);
  glVertex3fv(bp[3][0]);
  glVertex3fv(bp[2][3]);
  glVertex3fv(bp[3][2]);
  glVertex3fv(bp[2][5]);
  glVertex3fv(bp[3][4]);
  glVertex3fv(bp[2][7]);
  glVertex3fv(bp[3][6]);
  glVertex3fv(bp[2][1]);
  glVertex3fv(bp[3][0]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[2]);
  glVertex3fv(bp[3][0]);
  glVertex3fv(bp[3][1]);
  glVertex3fv(bp[3][3]);
  glVertex3fv(bp[3][2]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[3]);
  glVertex3fv(bp[3][2]);
  glVertex3fv(bp[3][3]);
  glVertex3fv(bp[3][5]);
  glVertex3fv(bp[3][4]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[0]);
  glVertex3fv(bp[3][4]);
  glVertex3fv(bp[3][5]);
  glVertex3fv(bp[3][7]);
  glVertex3fv(bp[3][6]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[1]);
  glVertex3fv(bp[3][6]);
  glVertex3fv(bp[3][7]);
  glVertex3fv(bp[3][1]);
  glVertex3fv(bp[3][0]);
  glEnd();

  glBegin(GL_POLYGON);
  glNormal3fv(bn[4]);
  glVertex3fv(bp[3][1]);
  glVertex3fv(bp[3][3]);
  glVertex3fv(bp[3][5]);
  glVertex3fv(bp[3][7]);
  glEnd();
}

void draw_torus(void) {
  
  glCallList( MAT_HOLDER_RINGS);
  
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[0][0]); glVertex3fv(tp[0][0]);
  glNormal3fv(tn[1][0]); glVertex3fv(tp[1][0]);
  glNormal3fv(tn[0][1]); glVertex3fv(tp[0][1]);
  glNormal3fv(tn[1][1]); glVertex3fv(tp[1][1]);
  glNormal3fv(tn[0][2]); glVertex3fv(tp[0][2]);
  glNormal3fv(tn[1][2]); glVertex3fv(tp[1][2]);
  glNormal3fv(tn[0][3]); glVertex3fv(tp[0][3]);
  glNormal3fv(tn[1][3]); glVertex3fv(tp[1][3]);
  glNormal3fv(tn[0][4]); glVertex3fv(tp[0][4]);
  glNormal3fv(tn[1][4]); glVertex3fv(tp[1][4]);
  glNormal3fv(tn[0][5]); glVertex3fv(tp[0][5]);
  glNormal3fv(tn[1][5]); glVertex3fv(tp[1][5]);
  glNormal3fv(tn[0][6]); glVertex3fv(tp[0][6]);
  glNormal3fv(tn[1][6]); glVertex3fv(tp[1][6]);
  glNormal3fv(tn[0][7]); glVertex3fv(tp[0][7]);
  glNormal3fv(tn[1][7]); glVertex3fv(tp[1][7]);
  glNormal3fv(tn[0][8]); glVertex3fv(tp[0][8]);
  glNormal3fv(tn[1][8]); glVertex3fv(tp[1][8]);
  glNormal3fv(tn[0][9]); glVertex3fv(tp[0][9]);
  glNormal3fv(tn[1][9]); glVertex3fv(tp[1][9]);
  glNormal3fv(tn[0][10]); glVertex3fv(tp[0][10]);
  glNormal3fv(tn[1][10]); glVertex3fv(tp[1][10]);
  glNormal3fv(tn[0][11]); glVertex3fv(tp[0][11]);
  glNormal3fv(tn[1][11]); glVertex3fv(tp[1][11]);
  glNormal3fv(tn[0][12]); glVertex3fv(tp[0][12]);
  glNormal3fv(tn[1][12]); glVertex3fv(tp[1][12]);
  glNormal3fv(tn[0][13]); glVertex3fv(tp[0][13]);
  glNormal3fv(tn[1][13]); glVertex3fv(tp[1][13]);
  glNormal3fv(tn[0][14]); glVertex3fv(tp[0][14]);
  glNormal3fv(tn[1][14]); glVertex3fv(tp[1][14]);
  glNormal3fv(tn[0][15]); glVertex3fv(tp[0][15]);
  glNormal3fv(tn[1][15]); glVertex3fv(tp[1][15]);
  glNormal3fv(tn[0][16]); glVertex3fv(tp[0][16]);
  glNormal3fv(tn[1][16]); glVertex3fv(tp[1][16]);
  glNormal3fv(tn[0][17]); glVertex3fv(tp[0][17]);
  glNormal3fv(tn[1][17]); glVertex3fv(tp[1][17]);
  glNormal3fv(tn[0][18]); glVertex3fv(tp[0][18]);
  glNormal3fv(tn[1][18]); glVertex3fv(tp[1][18]);
  glNormal3fv(tn[0][19]); glVertex3fv(tp[0][19]);
  glNormal3fv(tn[1][19]); glVertex3fv(tp[1][19]);
  glNormal3fv(tn[0][20]); glVertex3fv(tp[0][20]);
  glNormal3fv(tn[1][20]); glVertex3fv(tp[1][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[1][0]); glVertex3fv(tp[1][0]);
  glNormal3fv(tn[2][0]); glVertex3fv(tp[2][0]);
  glNormal3fv(tn[1][1]); glVertex3fv(tp[1][1]);
  glNormal3fv(tn[2][1]); glVertex3fv(tp[2][1]);
  glNormal3fv(tn[1][2]); glVertex3fv(tp[1][2]);
  glNormal3fv(tn[2][2]); glVertex3fv(tp[2][2]);
  glNormal3fv(tn[1][3]); glVertex3fv(tp[1][3]);
  glNormal3fv(tn[2][3]); glVertex3fv(tp[2][3]);
  glNormal3fv(tn[1][4]); glVertex3fv(tp[1][4]);
  glNormal3fv(tn[2][4]); glVertex3fv(tp[2][4]);
  glNormal3fv(tn[1][5]); glVertex3fv(tp[1][5]);
  glNormal3fv(tn[2][5]); glVertex3fv(tp[2][5]);
  glNormal3fv(tn[1][6]); glVertex3fv(tp[1][6]);
  glNormal3fv(tn[2][6]); glVertex3fv(tp[2][6]);
  glNormal3fv(tn[1][7]); glVertex3fv(tp[1][7]);
  glNormal3fv(tn[2][7]); glVertex3fv(tp[2][7]);
  glNormal3fv(tn[1][8]); glVertex3fv(tp[1][8]);
  glNormal3fv(tn[2][8]); glVertex3fv(tp[2][8]);
  glNormal3fv(tn[1][9]); glVertex3fv(tp[1][9]);
  glNormal3fv(tn[2][9]); glVertex3fv(tp[2][9]);
  glNormal3fv(tn[1][10]); glVertex3fv(tp[1][10]);
  glNormal3fv(tn[2][10]); glVertex3fv(tp[2][10]);
  glNormal3fv(tn[1][11]); glVertex3fv(tp[1][11]);
  glNormal3fv(tn[2][11]); glVertex3fv(tp[2][11]);
  glNormal3fv(tn[1][12]); glVertex3fv(tp[1][12]);
  glNormal3fv(tn[2][12]); glVertex3fv(tp[2][12]);
  glNormal3fv(tn[1][13]); glVertex3fv(tp[1][13]);
  glNormal3fv(tn[2][13]); glVertex3fv(tp[2][13]);
  glNormal3fv(tn[1][14]); glVertex3fv(tp[1][14]);
  glNormal3fv(tn[2][14]); glVertex3fv(tp[2][14]);
  glNormal3fv(tn[1][15]); glVertex3fv(tp[1][15]);
  glNormal3fv(tn[2][15]); glVertex3fv(tp[2][15]);
  glNormal3fv(tn[1][16]); glVertex3fv(tp[1][16]);
  glNormal3fv(tn[2][16]); glVertex3fv(tp[2][16]);
  glNormal3fv(tn[1][17]); glVertex3fv(tp[1][17]);
  glNormal3fv(tn[2][17]); glVertex3fv(tp[2][17]);
  glNormal3fv(tn[1][18]); glVertex3fv(tp[1][18]);
  glNormal3fv(tn[2][18]); glVertex3fv(tp[2][18]);
  glNormal3fv(tn[1][19]); glVertex3fv(tp[1][19]);
  glNormal3fv(tn[2][19]); glVertex3fv(tp[2][19]);
  glNormal3fv(tn[1][20]); glVertex3fv(tp[1][20]);
  glNormal3fv(tn[2][20]); glVertex3fv(tp[2][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[2][0]); glVertex3fv(tp[2][0]);
  glNormal3fv(tn[3][0]); glVertex3fv(tp[3][0]);
  glNormal3fv(tn[2][1]); glVertex3fv(tp[2][1]);
  glNormal3fv(tn[3][1]); glVertex3fv(tp[3][1]);
  glNormal3fv(tn[2][2]); glVertex3fv(tp[2][2]);
  glNormal3fv(tn[3][2]); glVertex3fv(tp[3][2]);
  glNormal3fv(tn[2][3]); glVertex3fv(tp[2][3]);
  glNormal3fv(tn[3][3]); glVertex3fv(tp[3][3]);
  glNormal3fv(tn[2][4]); glVertex3fv(tp[2][4]);
  glNormal3fv(tn[3][4]); glVertex3fv(tp[3][4]);
  glNormal3fv(tn[2][5]); glVertex3fv(tp[2][5]);
  glNormal3fv(tn[3][5]); glVertex3fv(tp[3][5]);
  glNormal3fv(tn[2][6]); glVertex3fv(tp[2][6]);
  glNormal3fv(tn[3][6]); glVertex3fv(tp[3][6]);
  glNormal3fv(tn[2][7]); glVertex3fv(tp[2][7]);
  glNormal3fv(tn[3][7]); glVertex3fv(tp[3][7]);
  glNormal3fv(tn[2][8]); glVertex3fv(tp[2][8]);
  glNormal3fv(tn[3][8]); glVertex3fv(tp[3][8]);
  glNormal3fv(tn[2][9]); glVertex3fv(tp[2][9]);
  glNormal3fv(tn[3][9]); glVertex3fv(tp[3][9]);
  glNormal3fv(tn[2][10]); glVertex3fv(tp[2][10]);
  glNormal3fv(tn[3][10]); glVertex3fv(tp[3][10]);
  glNormal3fv(tn[2][11]); glVertex3fv(tp[2][11]);
  glNormal3fv(tn[3][11]); glVertex3fv(tp[3][11]);
  glNormal3fv(tn[2][12]); glVertex3fv(tp[2][12]);
  glNormal3fv(tn[3][12]); glVertex3fv(tp[3][12]);
  glNormal3fv(tn[2][13]); glVertex3fv(tp[2][13]);
  glNormal3fv(tn[3][13]); glVertex3fv(tp[3][13]);
  glNormal3fv(tn[2][14]); glVertex3fv(tp[2][14]);
  glNormal3fv(tn[3][14]); glVertex3fv(tp[3][14]);
  glNormal3fv(tn[2][15]); glVertex3fv(tp[2][15]);
  glNormal3fv(tn[3][15]); glVertex3fv(tp[3][15]);
  glNormal3fv(tn[2][16]); glVertex3fv(tp[2][16]);
  glNormal3fv(tn[3][16]); glVertex3fv(tp[3][16]);
  glNormal3fv(tn[2][17]); glVertex3fv(tp[2][17]);
  glNormal3fv(tn[3][17]); glVertex3fv(tp[3][17]);
  glNormal3fv(tn[2][18]); glVertex3fv(tp[2][18]);
  glNormal3fv(tn[3][18]); glVertex3fv(tp[3][18]);
  glNormal3fv(tn[2][19]); glVertex3fv(tp[2][19]);
  glNormal3fv(tn[3][19]); glVertex3fv(tp[3][19]);
  glNormal3fv(tn[2][20]); glVertex3fv(tp[2][20]);
  glNormal3fv(tn[3][20]); glVertex3fv(tp[3][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[3][0]); glVertex3fv(tp[3][0]);
  glNormal3fv(tn[4][0]); glVertex3fv(tp[4][0]);
  glNormal3fv(tn[3][1]); glVertex3fv(tp[3][1]);
  glNormal3fv(tn[4][1]); glVertex3fv(tp[4][1]);
  glNormal3fv(tn[3][2]); glVertex3fv(tp[3][2]);
  glNormal3fv(tn[4][2]); glVertex3fv(tp[4][2]);
  glNormal3fv(tn[3][3]); glVertex3fv(tp[3][3]);
  glNormal3fv(tn[4][3]); glVertex3fv(tp[4][3]);
  glNormal3fv(tn[3][4]); glVertex3fv(tp[3][4]);
  glNormal3fv(tn[4][4]); glVertex3fv(tp[4][4]);
  glNormal3fv(tn[3][5]); glVertex3fv(tp[3][5]);
  glNormal3fv(tn[4][5]); glVertex3fv(tp[4][5]);
  glNormal3fv(tn[3][6]); glVertex3fv(tp[3][6]);
  glNormal3fv(tn[4][6]); glVertex3fv(tp[4][6]);
  glNormal3fv(tn[3][7]); glVertex3fv(tp[3][7]);
  glNormal3fv(tn[4][7]); glVertex3fv(tp[4][7]);
  glNormal3fv(tn[3][8]); glVertex3fv(tp[3][8]);
  glNormal3fv(tn[4][8]); glVertex3fv(tp[4][8]);
  glNormal3fv(tn[3][9]); glVertex3fv(tp[3][9]);
  glNormal3fv(tn[4][9]); glVertex3fv(tp[4][9]);
  glNormal3fv(tn[3][10]); glVertex3fv(tp[3][10]);
  glNormal3fv(tn[4][10]); glVertex3fv(tp[4][10]);
  glNormal3fv(tn[3][11]); glVertex3fv(tp[3][11]);
  glNormal3fv(tn[4][11]); glVertex3fv(tp[4][11]);
  glNormal3fv(tn[3][12]); glVertex3fv(tp[3][12]);
  glNormal3fv(tn[4][12]); glVertex3fv(tp[4][12]);
  glNormal3fv(tn[3][13]); glVertex3fv(tp[3][13]);
  glNormal3fv(tn[4][13]); glVertex3fv(tp[4][13]);
  glNormal3fv(tn[3][14]); glVertex3fv(tp[3][14]);
  glNormal3fv(tn[4][14]); glVertex3fv(tp[4][14]);
  glNormal3fv(tn[3][15]); glVertex3fv(tp[3][15]);
  glNormal3fv(tn[4][15]); glVertex3fv(tp[4][15]);
  glNormal3fv(tn[3][16]); glVertex3fv(tp[3][16]);
  glNormal3fv(tn[4][16]); glVertex3fv(tp[4][16]);
  glNormal3fv(tn[3][17]); glVertex3fv(tp[3][17]);
  glNormal3fv(tn[4][17]); glVertex3fv(tp[4][17]);
  glNormal3fv(tn[3][18]); glVertex3fv(tp[3][18]);
  glNormal3fv(tn[4][18]); glVertex3fv(tp[4][18]);
  glNormal3fv(tn[3][19]); glVertex3fv(tp[3][19]);
  glNormal3fv(tn[4][19]); glVertex3fv(tp[4][19]);
  glNormal3fv(tn[3][20]); glVertex3fv(tp[3][20]);
  glNormal3fv(tn[4][20]); glVertex3fv(tp[4][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[4][0]); glVertex3fv(tp[4][0]);
  glNormal3fv(tn[5][0]); glVertex3fv(tp[5][0]);
  glNormal3fv(tn[4][1]); glVertex3fv(tp[4][1]);
  glNormal3fv(tn[5][1]); glVertex3fv(tp[5][1]);
  glNormal3fv(tn[4][2]); glVertex3fv(tp[4][2]);
  glNormal3fv(tn[5][2]); glVertex3fv(tp[5][2]);
  glNormal3fv(tn[4][3]); glVertex3fv(tp[4][3]);
  glNormal3fv(tn[5][3]); glVertex3fv(tp[5][3]);
  glNormal3fv(tn[4][4]); glVertex3fv(tp[4][4]);
  glNormal3fv(tn[5][4]); glVertex3fv(tp[5][4]);
  glNormal3fv(tn[4][5]); glVertex3fv(tp[4][5]);
  glNormal3fv(tn[5][5]); glVertex3fv(tp[5][5]);
  glNormal3fv(tn[4][6]); glVertex3fv(tp[4][6]);
  glNormal3fv(tn[5][6]); glVertex3fv(tp[5][6]);
  glNormal3fv(tn[4][7]); glVertex3fv(tp[4][7]);
  glNormal3fv(tn[5][7]); glVertex3fv(tp[5][7]);
  glNormal3fv(tn[4][8]); glVertex3fv(tp[4][8]);
  glNormal3fv(tn[5][8]); glVertex3fv(tp[5][8]);
  glNormal3fv(tn[4][9]); glVertex3fv(tp[4][9]);
  glNormal3fv(tn[5][9]); glVertex3fv(tp[5][9]);
  glNormal3fv(tn[4][10]); glVertex3fv(tp[4][10]);
  glNormal3fv(tn[5][10]); glVertex3fv(tp[5][10]);
  glNormal3fv(tn[4][11]); glVertex3fv(tp[4][11]);
  glNormal3fv(tn[5][11]); glVertex3fv(tp[5][11]);
  glNormal3fv(tn[4][12]); glVertex3fv(tp[4][12]);
  glNormal3fv(tn[5][12]); glVertex3fv(tp[5][12]);
  glNormal3fv(tn[4][13]); glVertex3fv(tp[4][13]);
  glNormal3fv(tn[5][13]); glVertex3fv(tp[5][13]);
  glNormal3fv(tn[4][14]); glVertex3fv(tp[4][14]);
  glNormal3fv(tn[5][14]); glVertex3fv(tp[5][14]);
  glNormal3fv(tn[4][15]); glVertex3fv(tp[4][15]);
  glNormal3fv(tn[5][15]); glVertex3fv(tp[5][15]);
  glNormal3fv(tn[4][16]); glVertex3fv(tp[4][16]);
  glNormal3fv(tn[5][16]); glVertex3fv(tp[5][16]);
  glNormal3fv(tn[4][17]); glVertex3fv(tp[4][17]);
  glNormal3fv(tn[5][17]); glVertex3fv(tp[5][17]);
  glNormal3fv(tn[4][18]); glVertex3fv(tp[4][18]);
  glNormal3fv(tn[5][18]); glVertex3fv(tp[5][18]);
  glNormal3fv(tn[4][19]); glVertex3fv(tp[4][19]);
  glNormal3fv(tn[5][19]); glVertex3fv(tp[5][19]);
  glNormal3fv(tn[4][20]); glVertex3fv(tp[4][20]);
  glNormal3fv(tn[5][20]); glVertex3fv(tp[5][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[5][0]); glVertex3fv(tp[5][0]);
  glNormal3fv(tn[6][0]); glVertex3fv(tp[6][0]);
  glNormal3fv(tn[5][1]); glVertex3fv(tp[5][1]);
  glNormal3fv(tn[6][1]); glVertex3fv(tp[6][1]);
  glNormal3fv(tn[5][2]); glVertex3fv(tp[5][2]);
  glNormal3fv(tn[6][2]); glVertex3fv(tp[6][2]);
  glNormal3fv(tn[5][3]); glVertex3fv(tp[5][3]);
  glNormal3fv(tn[6][3]); glVertex3fv(tp[6][3]);
  glNormal3fv(tn[5][4]); glVertex3fv(tp[5][4]);
  glNormal3fv(tn[6][4]); glVertex3fv(tp[6][4]);
  glNormal3fv(tn[5][5]); glVertex3fv(tp[5][5]);
  glNormal3fv(tn[6][5]); glVertex3fv(tp[6][5]);
  glNormal3fv(tn[5][6]); glVertex3fv(tp[5][6]);
  glNormal3fv(tn[6][6]); glVertex3fv(tp[6][6]);
  glNormal3fv(tn[5][7]); glVertex3fv(tp[5][7]);
  glNormal3fv(tn[6][7]); glVertex3fv(tp[6][7]);
  glNormal3fv(tn[5][8]); glVertex3fv(tp[5][8]);
  glNormal3fv(tn[6][8]); glVertex3fv(tp[6][8]);
  glNormal3fv(tn[5][9]); glVertex3fv(tp[5][9]);
  glNormal3fv(tn[6][9]); glVertex3fv(tp[6][9]);
  glNormal3fv(tn[5][10]); glVertex3fv(tp[5][10]);
  glNormal3fv(tn[6][10]); glVertex3fv(tp[6][10]);
  glNormal3fv(tn[5][11]); glVertex3fv(tp[5][11]);
  glNormal3fv(tn[6][11]); glVertex3fv(tp[6][11]);
  glNormal3fv(tn[5][12]); glVertex3fv(tp[5][12]);
  glNormal3fv(tn[6][12]); glVertex3fv(tp[6][12]);
  glNormal3fv(tn[5][13]); glVertex3fv(tp[5][13]);
  glNormal3fv(tn[6][13]); glVertex3fv(tp[6][13]);
  glNormal3fv(tn[5][14]); glVertex3fv(tp[5][14]);
  glNormal3fv(tn[6][14]); glVertex3fv(tp[6][14]);
  glNormal3fv(tn[5][15]); glVertex3fv(tp[5][15]);
  glNormal3fv(tn[6][15]); glVertex3fv(tp[6][15]);
  glNormal3fv(tn[5][16]); glVertex3fv(tp[5][16]);
  glNormal3fv(tn[6][16]); glVertex3fv(tp[6][16]);
  glNormal3fv(tn[5][17]); glVertex3fv(tp[5][17]);
  glNormal3fv(tn[6][17]); glVertex3fv(tp[6][17]);
  glNormal3fv(tn[5][18]); glVertex3fv(tp[5][18]);
  glNormal3fv(tn[6][18]); glVertex3fv(tp[6][18]);
  glNormal3fv(tn[5][19]); glVertex3fv(tp[5][19]);
  glNormal3fv(tn[6][19]); glVertex3fv(tp[6][19]);
  glNormal3fv(tn[5][20]); glVertex3fv(tp[5][20]);
  glNormal3fv(tn[6][20]); glVertex3fv(tp[6][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[6][0]); glVertex3fv(tp[6][0]);
  glNormal3fv(tn[7][0]); glVertex3fv(tp[7][0]);
  glNormal3fv(tn[6][1]); glVertex3fv(tp[6][1]);
  glNormal3fv(tn[7][1]); glVertex3fv(tp[7][1]);
  glNormal3fv(tn[6][2]); glVertex3fv(tp[6][2]);
  glNormal3fv(tn[7][2]); glVertex3fv(tp[7][2]);
  glNormal3fv(tn[6][3]); glVertex3fv(tp[6][3]);
  glNormal3fv(tn[7][3]); glVertex3fv(tp[7][3]);
  glNormal3fv(tn[6][4]); glVertex3fv(tp[6][4]);
  glNormal3fv(tn[7][4]); glVertex3fv(tp[7][4]);
  glNormal3fv(tn[6][5]); glVertex3fv(tp[6][5]);
  glNormal3fv(tn[7][5]); glVertex3fv(tp[7][5]);
  glNormal3fv(tn[6][6]); glVertex3fv(tp[6][6]);
  glNormal3fv(tn[7][6]); glVertex3fv(tp[7][6]);
  glNormal3fv(tn[6][7]); glVertex3fv(tp[6][7]);
  glNormal3fv(tn[7][7]); glVertex3fv(tp[7][7]);
  glNormal3fv(tn[6][8]); glVertex3fv(tp[6][8]);
  glNormal3fv(tn[7][8]); glVertex3fv(tp[7][8]);
  glNormal3fv(tn[6][9]); glVertex3fv(tp[6][9]);
  glNormal3fv(tn[7][9]); glVertex3fv(tp[7][9]);
  glNormal3fv(tn[6][10]); glVertex3fv(tp[6][10]);
  glNormal3fv(tn[7][10]); glVertex3fv(tp[7][10]);
  glNormal3fv(tn[6][11]); glVertex3fv(tp[6][11]);
  glNormal3fv(tn[7][11]); glVertex3fv(tp[7][11]);
  glNormal3fv(tn[6][12]); glVertex3fv(tp[6][12]);
  glNormal3fv(tn[7][12]); glVertex3fv(tp[7][12]);
  glNormal3fv(tn[6][13]); glVertex3fv(tp[6][13]);
  glNormal3fv(tn[7][13]); glVertex3fv(tp[7][13]);
  glNormal3fv(tn[6][14]); glVertex3fv(tp[6][14]);
  glNormal3fv(tn[7][14]); glVertex3fv(tp[7][14]);
  glNormal3fv(tn[6][15]); glVertex3fv(tp[6][15]);
  glNormal3fv(tn[7][15]); glVertex3fv(tp[7][15]);
  glNormal3fv(tn[6][16]); glVertex3fv(tp[6][16]);
  glNormal3fv(tn[7][16]); glVertex3fv(tp[7][16]);
  glNormal3fv(tn[6][17]); glVertex3fv(tp[6][17]);
  glNormal3fv(tn[7][17]); glVertex3fv(tp[7][17]);
  glNormal3fv(tn[6][18]); glVertex3fv(tp[6][18]);
  glNormal3fv(tn[7][18]); glVertex3fv(tp[7][18]);
  glNormal3fv(tn[6][19]); glVertex3fv(tp[6][19]);
  glNormal3fv(tn[7][19]); glVertex3fv(tp[7][19]);
  glNormal3fv(tn[6][20]); glVertex3fv(tp[6][20]);
  glNormal3fv(tn[7][20]); glVertex3fv(tp[7][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[7][0]); glVertex3fv(tp[7][0]);
  glNormal3fv(tn[8][0]); glVertex3fv(tp[8][0]);
  glNormal3fv(tn[7][1]); glVertex3fv(tp[7][1]);
  glNormal3fv(tn[8][1]); glVertex3fv(tp[8][1]);
  glNormal3fv(tn[7][2]); glVertex3fv(tp[7][2]);
  glNormal3fv(tn[8][2]); glVertex3fv(tp[8][2]);
  glNormal3fv(tn[7][3]); glVertex3fv(tp[7][3]);
  glNormal3fv(tn[8][3]); glVertex3fv(tp[8][3]);
  glNormal3fv(tn[7][4]); glVertex3fv(tp[7][4]);
  glNormal3fv(tn[8][4]); glVertex3fv(tp[8][4]);
  glNormal3fv(tn[7][5]); glVertex3fv(tp[7][5]);
  glNormal3fv(tn[8][5]); glVertex3fv(tp[8][5]);
  glNormal3fv(tn[7][6]); glVertex3fv(tp[7][6]);
  glNormal3fv(tn[8][6]); glVertex3fv(tp[8][6]);
  glNormal3fv(tn[7][7]); glVertex3fv(tp[7][7]);
  glNormal3fv(tn[8][7]); glVertex3fv(tp[8][7]);
  glNormal3fv(tn[7][8]); glVertex3fv(tp[7][8]);
  glNormal3fv(tn[8][8]); glVertex3fv(tp[8][8]);
  glNormal3fv(tn[7][9]); glVertex3fv(tp[7][9]);
  glNormal3fv(tn[8][9]); glVertex3fv(tp[8][9]);
  glNormal3fv(tn[7][10]); glVertex3fv(tp[7][10]);
  glNormal3fv(tn[8][10]); glVertex3fv(tp[8][10]);
  glNormal3fv(tn[7][11]); glVertex3fv(tp[7][11]);
  glNormal3fv(tn[8][11]); glVertex3fv(tp[8][11]);
  glNormal3fv(tn[7][12]); glVertex3fv(tp[7][12]);
  glNormal3fv(tn[8][12]); glVertex3fv(tp[8][12]);
  glNormal3fv(tn[7][13]); glVertex3fv(tp[7][13]);
  glNormal3fv(tn[8][13]); glVertex3fv(tp[8][13]);
  glNormal3fv(tn[7][14]); glVertex3fv(tp[7][14]);
  glNormal3fv(tn[8][14]); glVertex3fv(tp[8][14]);
  glNormal3fv(tn[7][15]); glVertex3fv(tp[7][15]);
  glNormal3fv(tn[8][15]); glVertex3fv(tp[8][15]);
  glNormal3fv(tn[7][16]); glVertex3fv(tp[7][16]);
  glNormal3fv(tn[8][16]); glVertex3fv(tp[8][16]);
  glNormal3fv(tn[7][17]); glVertex3fv(tp[7][17]);
  glNormal3fv(tn[8][17]); glVertex3fv(tp[8][17]);
  glNormal3fv(tn[7][18]); glVertex3fv(tp[7][18]);
  glNormal3fv(tn[8][18]); glVertex3fv(tp[8][18]);
  glNormal3fv(tn[7][19]); glVertex3fv(tp[7][19]);
  glNormal3fv(tn[8][19]); glVertex3fv(tp[8][19]);
  glNormal3fv(tn[7][20]); glVertex3fv(tp[7][20]);
  glNormal3fv(tn[8][20]); glVertex3fv(tp[8][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[8][0]); glVertex3fv(tp[8][0]);
  glNormal3fv(tn[9][0]); glVertex3fv(tp[9][0]);
  glNormal3fv(tn[8][1]); glVertex3fv(tp[8][1]);
  glNormal3fv(tn[9][1]); glVertex3fv(tp[9][1]);
  glNormal3fv(tn[8][2]); glVertex3fv(tp[8][2]);
  glNormal3fv(tn[9][2]); glVertex3fv(tp[9][2]);
  glNormal3fv(tn[8][3]); glVertex3fv(tp[8][3]);
  glNormal3fv(tn[9][3]); glVertex3fv(tp[9][3]);
  glNormal3fv(tn[8][4]); glVertex3fv(tp[8][4]);
  glNormal3fv(tn[9][4]); glVertex3fv(tp[9][4]);
  glNormal3fv(tn[8][5]); glVertex3fv(tp[8][5]);
  glNormal3fv(tn[9][5]); glVertex3fv(tp[9][5]);
  glNormal3fv(tn[8][6]); glVertex3fv(tp[8][6]);
  glNormal3fv(tn[9][6]); glVertex3fv(tp[9][6]);
  glNormal3fv(tn[8][7]); glVertex3fv(tp[8][7]);
  glNormal3fv(tn[9][7]); glVertex3fv(tp[9][7]);
  glNormal3fv(tn[8][8]); glVertex3fv(tp[8][8]);
  glNormal3fv(tn[9][8]); glVertex3fv(tp[9][8]);
  glNormal3fv(tn[8][9]); glVertex3fv(tp[8][9]);
  glNormal3fv(tn[9][9]); glVertex3fv(tp[9][9]);
  glNormal3fv(tn[8][10]); glVertex3fv(tp[8][10]);
  glNormal3fv(tn[9][10]); glVertex3fv(tp[9][10]);
  glNormal3fv(tn[8][11]); glVertex3fv(tp[8][11]);
  glNormal3fv(tn[9][11]); glVertex3fv(tp[9][11]);
  glNormal3fv(tn[8][12]); glVertex3fv(tp[8][12]);
  glNormal3fv(tn[9][12]); glVertex3fv(tp[9][12]);
  glNormal3fv(tn[8][13]); glVertex3fv(tp[8][13]);
  glNormal3fv(tn[9][13]); glVertex3fv(tp[9][13]);
  glNormal3fv(tn[8][14]); glVertex3fv(tp[8][14]);
  glNormal3fv(tn[9][14]); glVertex3fv(tp[9][14]);
  glNormal3fv(tn[8][15]); glVertex3fv(tp[8][15]);
  glNormal3fv(tn[9][15]); glVertex3fv(tp[9][15]);
  glNormal3fv(tn[8][16]); glVertex3fv(tp[8][16]);
  glNormal3fv(tn[9][16]); glVertex3fv(tp[9][16]);
  glNormal3fv(tn[8][17]); glVertex3fv(tp[8][17]);
  glNormal3fv(tn[9][17]); glVertex3fv(tp[9][17]);
  glNormal3fv(tn[8][18]); glVertex3fv(tp[8][18]);
  glNormal3fv(tn[9][18]); glVertex3fv(tp[9][18]);
  glNormal3fv(tn[8][19]); glVertex3fv(tp[8][19]);
  glNormal3fv(tn[9][19]); glVertex3fv(tp[9][19]);
  glNormal3fv(tn[8][20]); glVertex3fv(tp[8][20]);
  glNormal3fv(tn[9][20]); glVertex3fv(tp[9][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[9][0]); glVertex3fv(tp[9][0]);
  glNormal3fv(tn[10][0]); glVertex3fv(tp[10][0]);
  glNormal3fv(tn[9][1]); glVertex3fv(tp[9][1]);
  glNormal3fv(tn[10][1]); glVertex3fv(tp[10][1]);
  glNormal3fv(tn[9][2]); glVertex3fv(tp[9][2]);
  glNormal3fv(tn[10][2]); glVertex3fv(tp[10][2]);
  glNormal3fv(tn[9][3]); glVertex3fv(tp[9][3]);
  glNormal3fv(tn[10][3]); glVertex3fv(tp[10][3]);
  glNormal3fv(tn[9][4]); glVertex3fv(tp[9][4]);
  glNormal3fv(tn[10][4]); glVertex3fv(tp[10][4]);
  glNormal3fv(tn[9][5]); glVertex3fv(tp[9][5]);
  glNormal3fv(tn[10][5]); glVertex3fv(tp[10][5]);
  glNormal3fv(tn[9][6]); glVertex3fv(tp[9][6]);
  glNormal3fv(tn[10][6]); glVertex3fv(tp[10][6]);
  glNormal3fv(tn[9][7]); glVertex3fv(tp[9][7]);
  glNormal3fv(tn[10][7]); glVertex3fv(tp[10][7]);
  glNormal3fv(tn[9][8]); glVertex3fv(tp[9][8]);
  glNormal3fv(tn[10][8]); glVertex3fv(tp[10][8]);
  glNormal3fv(tn[9][9]); glVertex3fv(tp[9][9]);
  glNormal3fv(tn[10][9]); glVertex3fv(tp[10][9]);
  glNormal3fv(tn[9][10]); glVertex3fv(tp[9][10]);
  glNormal3fv(tn[10][10]); glVertex3fv(tp[10][10]);
  glNormal3fv(tn[9][11]); glVertex3fv(tp[9][11]);
  glNormal3fv(tn[10][11]); glVertex3fv(tp[10][11]);
  glNormal3fv(tn[9][12]); glVertex3fv(tp[9][12]);
  glNormal3fv(tn[10][12]); glVertex3fv(tp[10][12]);
  glNormal3fv(tn[9][13]); glVertex3fv(tp[9][13]);
  glNormal3fv(tn[10][13]); glVertex3fv(tp[10][13]);
  glNormal3fv(tn[9][14]); glVertex3fv(tp[9][14]);
  glNormal3fv(tn[10][14]); glVertex3fv(tp[10][14]);
  glNormal3fv(tn[9][15]); glVertex3fv(tp[9][15]);
  glNormal3fv(tn[10][15]); glVertex3fv(tp[10][15]);
  glNormal3fv(tn[9][16]); glVertex3fv(tp[9][16]);
  glNormal3fv(tn[10][16]); glVertex3fv(tp[10][16]);
  glNormal3fv(tn[9][17]); glVertex3fv(tp[9][17]);
  glNormal3fv(tn[10][17]); glVertex3fv(tp[10][17]);
  glNormal3fv(tn[9][18]); glVertex3fv(tp[9][18]);
  glNormal3fv(tn[10][18]); glVertex3fv(tp[10][18]);
  glNormal3fv(tn[9][19]); glVertex3fv(tp[9][19]);
  glNormal3fv(tn[10][19]); glVertex3fv(tp[10][19]);
  glNormal3fv(tn[9][20]); glVertex3fv(tp[9][20]);
  glNormal3fv(tn[10][20]); glVertex3fv(tp[10][20]);
  glEnd();
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3fv(tn[10][0]); glVertex3fv(tp[10][0]);
  glNormal3fv(tn[11][0]); glVertex3fv(tp[11][0]);
  glNormal3fv(tn[10][1]); glVertex3fv(tp[10][1]);
  glNormal3fv(tn[11][1]); glVertex3fv(tp[11][1]);
  glNormal3fv(tn[10][2]); glVertex3fv(tp[10][2]);
  glNormal3fv(tn[11][2]); glVertex3fv(tp[11][2]);
  glNormal3fv(tn[10][3]); glVertex3fv(tp[10][3]);
  glNormal3fv(tn[11][3]); glVertex3fv(tp[11][3]);
  glNormal3fv(tn[10][4]); glVertex3fv(tp[10][4]);
  glNormal3fv(tn[11][4]); glVertex3fv(tp[11][4]);
  glNormal3fv(tn[10][5]); glVertex3fv(tp[10][5]);
  glNormal3fv(tn[11][5]); glVertex3fv(tp[11][5]);
  glNormal3fv(tn[10][6]); glVertex3fv(tp[10][6]);
  glNormal3fv(tn[11][6]); glVertex3fv(tp[11][6]);
  glNormal3fv(tn[10][7]); glVertex3fv(tp[10][7]);
  glNormal3fv(tn[11][7]); glVertex3fv(tp[11][7]);
  glNormal3fv(tn[10][8]); glVertex3fv(tp[10][8]);
  glNormal3fv(tn[11][8]); glVertex3fv(tp[11][8]);
  glNormal3fv(tn[10][9]); glVertex3fv(tp[10][9]);
  glNormal3fv(tn[11][9]); glVertex3fv(tp[11][9]);
  glNormal3fv(tn[10][10]); glVertex3fv(tp[10][10]);
  glNormal3fv(tn[11][10]); glVertex3fv(tp[11][10]);
  glNormal3fv(tn[10][11]); glVertex3fv(tp[10][11]);
  glNormal3fv(tn[11][11]); glVertex3fv(tp[11][11]);
  glNormal3fv(tn[10][12]); glVertex3fv(tp[10][12]);
  glNormal3fv(tn[11][12]); glVertex3fv(tp[11][12]);
  glNormal3fv(tn[10][13]); glVertex3fv(tp[10][13]);
  glNormal3fv(tn[11][13]); glVertex3fv(tp[11][13]);
  glNormal3fv(tn[10][14]); glVertex3fv(tp[10][14]);
  glNormal3fv(tn[11][14]); glVertex3fv(tp[11][14]);
  glNormal3fv(tn[10][15]); glVertex3fv(tp[10][15]);
  glNormal3fv(tn[11][15]); glVertex3fv(tp[11][15]);
  glNormal3fv(tn[10][16]); glVertex3fv(tp[10][16]);
  glNormal3fv(tn[11][16]); glVertex3fv(tp[11][16]);
  glNormal3fv(tn[10][17]); glVertex3fv(tp[10][17]);
  glNormal3fv(tn[11][17]); glVertex3fv(tp[11][17]);
  glNormal3fv(tn[10][18]); glVertex3fv(tp[10][18]);
  glNormal3fv(tn[11][18]); glVertex3fv(tp[11][18]);
  glNormal3fv(tn[10][19]); glVertex3fv(tp[10][19]);
  glNormal3fv(tn[11][19]); glVertex3fv(tp[11][19]);
  glNormal3fv(tn[10][20]); glVertex3fv(tp[10][20]);
  glNormal3fv(tn[11][20]); glVertex3fv(tp[11][20]);
  glEnd();
}

void draw_holder(void) {
  
  glCallList( MAT_HOLDER_RINGS); 
  
  glPushMatrix();
  draw_base();
  glTranslatef(0.0,  0.0,  20.000000);
  draw_torus();
  glTranslatef(0.0,  0.0,  5.000000);
  draw_torus();
  glTranslatef(0.0,  0.0,  5.000000);
  draw_torus();
  glPopMatrix();
}

