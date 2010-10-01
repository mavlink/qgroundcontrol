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

package body Scenebamb_Procs is
   procedure DoInit is
      ambient : array (0 .. 3) of aliased GLfloat :=
         (0.0, 0.0, 1.0, 1.0);
      diffuse : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 1.0);
      specular : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 1.0);
      position : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 0.0);
   begin
      glLightfv (GL_LIGHT0, GL_AMBIENT, ambient (0)'access);
      glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse (0)'access);
      glLightfv (GL_LIGHT0, GL_POSITION, position (0)'access);

      glEnable (GL_LIGHTING);
      glEnable (GL_LIGHT0);
      glEnable (GL_DEPTH_TEST);
      glDepthFunc (GL_LESS);
   end DoInit;

   procedure DoDisplay is
   begin
      --  16#4100# = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
      glClear (GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);

      glPushMatrix;
      glRotatef (20.0, 1.0, 0.0, 0.0);

      glPushMatrix;
      glTranslatef (-0.75, 0.5, 0.0);
      glRotatef (90.0, 1.0, 0.0, 0.0);
      glutSolidTorus (0.275, 0.85, 15, 15);
      glPopMatrix;

      glPushMatrix;
      glTranslatef (-0.75, -0.5, 0.0);
      glRotatef (270.0, 1.0, 0.0, 0.0);
      glutSolidCone (1.0, 2.0, 15, 15);
      glPopMatrix;

      glPushMatrix;
      glTranslatef (0.75, 0.0, -1.0);
      glutSolidSphere (1.0, 15, 15);
      glPopMatrix;

      glPopMatrix;

      glFlush;
   end DoDisplay;


   procedure ReshapeCallback (w : Integer; h : Integer) is
   begin
      glViewport (0, 0, GLsizei(w), GLsizei(h));
      glMatrixMode (GL_PROJECTION);
      glLoadIdentity;

      if w <= h then
         glOrtho (-2.5, 2.5, GLdouble (-2.5*Float (h)/ Float (w)),
          GLdouble (2.5*Float (h)/Float (w)), -10.0, 10.0);
      else
         glOrtho (GLdouble (-2.5*Float (w)/Float (h)),
          GLdouble (2.5*Float (w)/Float (h)), -2.5, 2.5, -10.0, 10.0);
      end if;

      glMatrixMode (GL_MODELVIEW);
   end ReshapeCallback;
end Scenebamb_Procs;
