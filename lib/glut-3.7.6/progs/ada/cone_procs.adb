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
with GLU; use GLU;

package body Cone_Procs is
   procedure DoInit is
      mat_ambient : array (0 .. 3) of aliased GLfloat :=
         (0.2, 0.2, 0.2, 1.0);
      mat_diffuse : array (0 .. 3) of aliased GLfloat :=
         (0.8, 0.8, 0.8, 1.0);
      --  Specular and Shininess are not default values
      mat_specular : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 1.0);
      mat_shininess : aliased GLfloat := 50.0;

      light_ambient : array (0 .. 3) of aliased GLfloat :=
         (0.0, 0.0, 0.0, 1.0);
      light_diffuse : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 1.0);
      light_specular : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 1.0);
      light_position : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 0.0);

      lmodel_ambient : array (0 .. 3) of aliased GLfloat :=
         (0.2, 0.2, 0.2, 1.0);
   begin
      glMaterialfv (GL_FRONT, GL_AMBIENT, mat_ambient (0)'Access);
      glMaterialfv (GL_FRONT, GL_DIFFUSE, mat_diffuse (0)'Access);
      glMaterialfv (GL_FRONT, GL_SPECULAR, mat_specular (0)'Access);
      glMaterialfv (GL_FRONT, GL_SHININESS, mat_shininess'Access);

      glLightfv (GL_LIGHT0, GL_AMBIENT, light_ambient (0)'Access);
      glLightfv (GL_LIGHT0, GL_DIFFUSE, light_diffuse (0)'Access);
      glLightfv (GL_LIGHT0, GL_SPECULAR, light_specular (0)'Access);
      glLightfv (GL_LIGHT0, GL_POSITION, light_position (0)'Access);

      glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient (0)'Access);

      glEnable (GL_LIGHTING);
      glEnable (GL_LIGHT0);
      glEnable (GL_DEPTH_TEST);
   end DoInit;

   procedure solidCone (base : Float; height : Float) is
      quadObj : GLUquadricObj_Ptr;
   begin
      quadObj := gluNewQuadric;
      gluQuadricDrawStyle (quadObj, GLU_FILL);
      gluQuadricNormals (quadObj, GLU_SMOOTH);
      gluCylinder (quadObj, GLdouble(base), 0.0, GLdouble(height), 15, 10);
      gluDeleteQuadric (quadObj);
   end solidCone;

   procedure DoDisplay is
   begin
      glClear (GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);

      glPushMatrix;
      glTranslatef (0.0, -1.0, 0.0);
      glRotatef (250.0, 1.0, 0.0, 0.0);
      solidCone (1.0, 2.0);
      glPopMatrix;
      glFlush;
   end DoDisplay;


   procedure ReshapeCallback (w : Integer; h : Integer) is
   begin
      glViewport (0, 0, GLsizei(w), GLsizei(h));
      glMatrixMode (GL_PROJECTION);
      glLoadIdentity;

      if w <= h then
         glOrtho (-1.5, 1.5, GLdouble (-1.5*Float (h)/Float (w)),
          GLdouble (1.5*Float (h)/Float (w)), -10.0, 10.0);
      else
         glOrtho (GLdouble (-1.5*Float (w)/Float (h)),
          GLdouble (1.5*Float (w)/Float (h)), -1.5, 1.5, -10.0, 10.0);
      end if;

      glMatrixMode (GL_MODELVIEW);
   end ReshapeCallback;
end Cone_Procs;
