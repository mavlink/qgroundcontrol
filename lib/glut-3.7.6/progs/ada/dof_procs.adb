--
--  (c) Copyright 1993,1994,1995,1996 Silicon Graphics, Inc.
--  ALL RIGHTS RESERVED
--  Permission to use, copy, modify, and distribute this software for
--  any purpose and without fee is hereby granted, provided that the above
--  copyright notice appear in all copies and that both the copyright notice
--  and this permission notice appear in supporting documentation, and that
--  the name of Silicon Graphics, Inc. not be used in advertising
--  or publicity pertaining to distribution of the software without specific,
--  written prior permission.
--
--  THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
--  AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
--  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
--  FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
--  GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
--  SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
--  KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
--  LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
--  THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
--  ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
--  ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
--  POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
--
--  US Government Users Restricted Rights
--  Use, duplication, or disclosure by the Government is subject to
--  restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
--  (c)(1)(ii) of the Rights in Technical Data and Computer Software
--  clause at DFARS 252.227-7013 and/or in similar or successor
--  clauses in the FAR or the DOD or NASA FAR Supplement.
--  Unpublished-- rights reserved under the copyright laws of the
--  United States.  Contractor/manufacturer is Silicon Graphics,
--  Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
--
--  OpenGL(TM) is a trademark of Silicon Graphics, Inc.
--

with GL; use GL;
with Glut; use Glut;
with Jitter;
with Ada.Numerics;
with Ada.Numerics.Generic_Elementary_Functions;

package body Dof_Procs is
   package Num renames Ada.Numerics;
   package GLdouble_GEF is new
      Num.Generic_Elementary_Functions (GLdouble);
   use GLdouble_GEF;

   procedure accFrustum
      (left  : GLdouble; right : GLdouble; bottom : GLdouble;
       top   : GLdouble; near  : GLdouble; far    : GLdouble;
       pixdx : GLdouble; pixdy : GLdouble; eyedx  : GLdouble;
       eyedy : GLdouble; focus : GLdouble)
   is
      xwsize, ywsize : GLdouble;
      dx, dy : GLdouble;
      viewport : array (0 .. 3) of aliased GLint;
   begin
      glGetIntegerv (GL_VIEWPORT, viewport (0)'Access);
      
      xwsize := right - left;
      ywsize := top - bottom;
      
      dx := -(pixdx * xwsize / GLdouble (viewport (2)) + eyedx * near / focus);
      dy := -(pixdy * ywsize / GLdouble (viewport (3)) + eyedy * near / focus);

      glMatrixMode (GL_PROJECTION);
      glLoadIdentity;
      glFrustum (left + dx, right + dx, bottom + dy, top + dy, near, far);
      glMatrixMode (GL_MODELVIEW);
      glLoadIdentity;
      glTranslated (-eyedx, -eyedy, 0.0);
   end accFrustum;
   
   procedure accPerspective
      (fovy  : GLdouble; aspect : GLdouble; near  : GLdouble;
       far   : GLdouble; pixdx  : GLdouble; pixdy : GLdouble;
       eyedx : GLdouble; eyedy  : GLdouble; focus : GLdouble)
   is
      fov2, left, right, bottom, top : GLdouble;
   begin
      fov2 := ((fovy * Num.Pi) / 180.0) / 2.0;
      
      top := near / (Cos (fov2) / Sin (fov2));
      bottom := -top;
      
      right := top * aspect;
      left := -right;
      
      accFrustum (left, right, bottom, top, near, far, pixdx,
         pixdy, eyedx, eyedy, focus);
   end accPerspective;

   procedure DoInit is
      ambient : array (0 .. 3) of aliased GLfloat :=
         (0.0, 0.0, 0.0, 1.0);
      diffuse : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 1.0);
      position : array (0 .. 3) of aliased GLfloat :=
         (0.0, 3.0, 3.0, 0.0);

      lmodel_ambient : array (0 .. 3) of aliased GLfloat :=
         (0.2, 0.2, 0.2, 1.0);
      local_view : aliased GLfloat := 0.0;
   begin
      glLightfv (GL_LIGHT0, GL_AMBIENT, ambient (0)'Access);
      glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse (0)'Access);
      glLightfv (GL_LIGHT0, GL_POSITION, position (0)'Access);

      glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient (0)'Access);
      glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, local_view'Access);
      
      glFrontFace (GL_CW);
      glEnable (GL_LIGHTING);
      glEnable (GL_LIGHT0);
      glEnable (GL_AUTO_NORMAL);
      glEnable (GL_NORMALIZE);
      
      glEnable (GL_DEPTH_TEST);
      glDepthFunc (GL_LESS);
      
      glMatrixMode (GL_MODELVIEW);
      glLoadIdentity;
      
      glClearColor (0.0, 0.0, 0.0, 0.0);
      glClearAccum (0.0, 0.0, 0.0, 0.0);
   end DoInit;

   procedure renderTeapot
      (x     : GLfloat; y     : GLfloat; z     : GLfloat;
       ambr  : GLfloat; ambg  : GLfloat; ambb  : GLfloat;
       difr  : GLfloat; difg  : GLfloat; difb  : GLfloat;
       specr : GLfloat; specg : GLfloat; specb : GLfloat;
       shine : GLfloat)
   is
      mat : array (0 .. 3) of aliased GLfloat;
   begin
      glPushMatrix;

      glTranslatef (x, y, z);

      mat (0) := ambr;
      mat (1) := ambg;
      mat (2) := ambb;
      mat (3) := 1.0;
      glMaterialfv (GL_FRONT, GL_AMBIENT, mat (0)'Access);

      mat (0) := difr;
      mat (1) := difg;
      mat (2) := difb;
      glMaterialfv (GL_FRONT, GL_DIFFUSE, mat (0)'Access);

      mat (0) := specr;
      mat (1) := specg;
      mat (2) := specb;
      glMaterialfv (GL_FRONT, GL_SPECULAR, mat (0)'Access);

      glMaterialf (GL_FRONT, GL_SHININESS, shine * 128.0);

      glutSolidTeapot (0.5);
      glPopMatrix;
   end renderTeapot;

   procedure DoDisplay is
      viewport : array (0 .. 3) of aliased GLint;
   begin
      glGetIntegerv (GL_VIEWPORT, viewport (0)'Access);
      glClear (GL_ACCUM_BUFFER_BIT);
      
      for jitval in 1 .. 8 loop
         glClear (GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
            accPerspective (45.0, GLdouble (viewport (2)) /
            GLdouble (viewport (3)),
            1.0, 15.0, 0.0, 0.0, GLdouble (0.33 * Jitter.j8 (jitval).x),
            GLdouble (0.33 * Jitter.j8 (jitval).y), 5.0);

         renderTeapot (-1.1, -0.5, -4.5, 0.1745, 0.01175, 0.01175,
            0.61424, 0.04136, 0.04136, 0.727811, 0.626959, 0.626959, 0.6);
         renderTeapot (-0.5, -0.5, -5.0, 0.24725, 0.1995, 0.0745,
            0.75164, 0.60648, 0.22648, 0.628281, 0.555802, 0.366065, 0.4);
         renderTeapot (0.2, -0.5, -5.5, 0.19225, 0.19225, 0.19225,
            0.50754, 0.50754, 0.50754, 0.508273, 0.508273, 0.508273, 0.4);
         renderTeapot (1.0, -0.5, -6.0, 0.0215, 0.1745, 0.0215,
            0.07568, 0.61424, 0.07568, 0.633, 0.727811, 0.633, 0.6);
         renderTeapot (1.8, -0.5, -6.5, 0.0, 0.1, 0.06, 0.0, 0.50980392,
            0.50980392, 0.50196078, 0.50196078, 0.50196078, 0.25);
         glAccum (GL_ACCUM, 0.125);
      end loop;
      
      glAccum (GL_RETURN, 1.0);
      glFlush;
   end DoDisplay;


   procedure ReshapeCallback (w : Integer; h : Integer) is
   begin
      glViewport (0, 0, GLsizei(w), GLsizei(h));
   end ReshapeCallback;
end Dof_Procs;
