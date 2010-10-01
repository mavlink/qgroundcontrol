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
with Text_IO; use Text_IO;

package body Bezmesh_Procs is
   Bezier_Control_Points : array (1 .. 4, 1 .. 4, 1 .. 3) of aliased GLfloat :=
      (((-1.5, -1.5, 4.0), (-0.5, -1.5, 2.0),
        (0.5, -1.5, -1.0), (1.5,  -1.5, 2.0)),
       ((-1.5, -0.5, 1.0), (-0.5, -0.5, 3.0),
        (0.5,  -0.5,  0.0), (1.5,  -0.5, -1.0)),
       ((-1.5, 0.5, 4.0), (-0.5, 0.5, 0.0),
        (0.5, 0.5, 3.0), (1.5, 0.5, 4.0)),
       ((-1.5, 1.5, -2.0), (-0.5, 1.5, -2.0),
        (0.5, 1.5, 0.0), (1.5, 1.5, -1.0)));


   procedure Initialize is
      ambient : array (0 .. 3) of aliased GLfloat :=
         (0.2, 0.2, 0.2, 1.0);
      diffuse : array (0 .. 3) of aliased GLfloat :=
         (0.0, 0.0, 2.0, 1.0);
      position : array (0 .. 3) of aliased GLfloat :=
         (0.6, 0.6, 0.6, 1.0);

      mat_diffuse : array (0 .. 3) of aliased GLfloat :=
         (0.6, 0.6, 0.6, 1.0);
      mat_specular : array (0 .. 3) of aliased GLfloat :=
         (1.0, 1.0, 1.0, 1.0);
      mat_shininess : aliased GLfloat := 50.0;

   begin
      glClearColor (0.0, 0.0, 0.0, 1.0);
      glEnable (GL_DEPTH_TEST);
      glMap2f (GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 4,
         Bezier_Control_Points(1,1,1)'ACCESS);
      glEnable (GL_MAP2_VERTEX_3);
      glEnable (GL_AUTO_NORMAL);
      glEnable (GL_NORMALIZE);
      glMapGrid2f(20, 0.0, 1.0, 20, 0.0, 1.0);

      glEnable (GL_LIGHTING);
      glEnable (GL_LIGHT0);

      glLightfv (GL_LIGHT0, GL_AMBIENT, ambient (0)'access);
      glLightfv (GL_LIGHT0, GL_POSITION, position (0)'access);

      glMaterialfv (GL_FRONT, GL_DIFFUSE, mat_diffuse (0)'access);
      glMaterialfv (GL_FRONT, GL_SPECULAR, mat_specular (0)'access);
      glMaterialfv (GL_FRONT, GL_SHININESS, mat_shininess'access);
   end Initialize;

   procedure Display is
   begin
      glClear (GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);

      glPushMatrix;

--      Glloadidentity;
      Glrotatef (85.0, 1.0, 1.0, 1.0);
      glEvalMesh2 (GL_FILL, 0, 20, 0, 20);

      glPopMatrix;

      glFlush;
   end Display;

   procedure HandleReshape (w : Integer; h : Integer) is
   begin
      glViewport (0, 0, GLsizei(w), GLsizei(h));
      GlMatrixMode (GL_PROJECTION);
      GlLoadIdentity;
      if (W < H) then
         Glortho (-4.0, 4.0, -4.0 * Gldouble (H / W), 4.0 * Gldouble (H / W),
                  -4.0, 4.0);
      else
        Glortho (-4.0 * Gldouble (W / H), 4.0 *  Gldouble (W / H),
                 -4.0, 4.0, -4.0, 4.0);
      end if;
   end HandleReshape;
end Bezmesh_Procs;
