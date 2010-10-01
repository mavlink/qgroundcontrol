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
with Text_IO; use Text_IO;
with Unchecked_Conversion;

package body Fog_Procs is
   package tio renames Text_IO;

   function FogToInt is new
      Unchecked_Conversion (Source => FogMode, Target => GLint);

   procedure CycleFog (btn: Integer; state: Integer; x, y: Integer) is
   begin
    if btn = GLUT_LEFT_BUTTON then
     if state = GLUT_DOWN then
      if fogType = GL_EXP then
         fogType := GL_EXP2;
      elsif fogType = GL_EXP2 then
         fogType := GL_LINEAR;
         glFogf (GL_FOG_START, 1.0);
         glFogf (GL_FOG_END, 5.0);
      elsif fogType = GL_LINEAR then
         fogType := GL_EXP;
      end if;

--    tio.Put_Line("Fog mode is " & FogMode'IMAGE (fogType));

      glFogi (GL_FOG_MODE, FogToInt (fogType));
      glutPostRedisplay;
     end if;
    end if;
   end CycleFog;

   procedure Initialize is
      position : array (0 .. 3) of aliased GLfloat :=
         (0.0, 3.0, 3.0, 0.0);
      local_view : aliased GLfloat := 0.0;

      fogColor : array (0 .. 3) of aliased GLfloat :=
         (0.5, 0.5, 0.5, 1.0);
   begin
      glEnable (GL_DEPTH_TEST);
      glDepthFunc (GL_LESS);

      glLightfv (GL_LIGHT0, GL_POSITION, position (0)'Access);
      glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, local_view'Access);

      glFrontFace (GL_CW);
      glEnable (GL_LIGHTING);
      glEnable (GL_LIGHT0);
      glEnable (GL_AUTO_NORMAL);
      glEnable (GL_NORMALIZE);
      glEnable (GL_FOG);

      fogType := GL_EXP;

      glFogi (GL_FOG_MODE, FogToInt (fogType));
      glFogfv (GL_FOG_COLOR, fogColor (0)'Access);
      glFogf (GL_FOG_DENSITY, 0.35);
      glHint (GL_FOG_HINT, GL_DONT_CARE);
      glClearColor (0.5, 0.5, 0.5, 1.0);
   end Initialize;

   procedure RenderRedTeapot (x : GLfloat; y : GLfloat; z : GLfloat) is
      mat : array (0 .. 3) of aliased GLfloat;
   begin
      glPushMatrix;

      glTranslatef (x, y, z);

      mat (0) := 0.1745;
      mat (1) := 0.01175;
      mat (2) := 0.01175;
      mat (3) := 1.0;
      glMaterialfv (GL_FRONT, GL_AMBIENT, mat (0)'Access);

      mat (0) := 0.61424;
      mat (1) := 0.04136;
      mat (2) := 0.04136;
      glMaterialfv (GL_FRONT, GL_DIFFUSE, mat (0)'Access);

      mat (0) := 0.727811;
      mat (1) := 0.626959;
      mat (2) := 0.626959;
      glMaterialfv (GL_FRONT, GL_SPECULAR, mat (0)'Access);

      glMaterialf (GL_FRONT, GL_SHININESS, 0.6*128.0);

      glutSolidTeapot (1.0);

      glPopMatrix;
   end RenderRedTeapot;

   procedure Display is
   begin
      glClear (GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
      RenderRedTeapot (-4.0, -0.5, -1.0);
      RenderRedTeapot (-2.0, -0.5, -2.0);
      RenderRedTeapot (0.0, -0.5, -3.0);
      RenderRedTeapot (2.0, -0.5, -4.0);
      RenderRedTeapot (4.0, -0.5, -5.0);
      glFlush;
   end Display;

   procedure HandleReshape (w : Integer; h : Integer) is
   begin
      glViewport (0, 0, GLsizei(w), GLsizei(h));
      glMatrixMode (GL_PROJECTION);

      glLoadIdentity;

      if w <= (h * 3) then
         glOrtho (-6.0, 6.0,
            GLdouble (-2.0 * (GLdouble (h) * 3.0) / GLdouble (w)),
            GLdouble (2.0 * (GLdouble (h) * 3.0 / GLdouble (w))), 0.0, 10.0);
      else
         glOrtho (GLdouble (-6.0 * GLdouble (w) / (GLdouble (h) * 3.0)),
            GLdouble (6.0 * GLdouble (w) / (GLdouble (h) * 3.0)),
            -2.0, 2.0, 0.0, 10.0);
      end if;

      glMatrixMode (GL_MODELVIEW);
      glLoadIdentity;
   end HandleReshape;
end Fog_Procs;
