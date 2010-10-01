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

with System; use System;
with GL; use GL;

with Ada.Numerics; use Ada.Numerics;
with Ada.Numerics.Generic_Elementary_Functions;

package body Texturesurf_Procs is
   package GLdouble_GEF is new
      Generic_Elementary_Functions (GLfloat);

   use GLdouble_GEF;

   ctrlpoints : array (0 .. 3, 0 .. 3, 0 .. 2) of aliased GLfloat :=
      (((-1.5, -1.5, 4.0), (-0.5, -1.5, 2.0),
      (0.5, -1.5, -1.0), (1.5, -1.5, 2.0)),
      ((-1.5, -0.5, 1.0), (-0.5, -0.5, 3.0),
      (0.5, -0.5, 0.0), (1.5, -0.5, -1.0)),
      ((-1.5, 0.5, 4.0), (-0.5, 0.5, 0.0),
      (0.5, 0.5, 3.0), (1.5, 0.5, 4.0)),
      ((-1.5, 1.5, -2.0), (-0.5, 1.5, -2.0),
      (0.5, 1.5, 0.0), (1.5, 1.5, -1.0)));

   texpts : array (0 .. 1, 0 .. 1, 0 .. 1) of aliased GLfloat :=
      (((0.0, 0.0), (0.0, 1.0)), ((1.0, 0.0), (1.0, 1.0)));

   imageWidth  : constant := 64;
   imageHeight : constant := 64;
   image       : array (Integer range 0 .. (3*imageWidth*imageHeight))
      of aliased GLubyte;

   procedure makeImage is
      ti, tj : GLfloat;
   begin
      for i in 0 .. (imageWidth - 1) loop
         ti := 2.0*Pi*GLfloat (i)/GLfloat (imageWidth);
         for j in 0 .. (imageHeight - 1) loop
            tj := 2.0*Pi*GLfloat (j)/GLfloat (imageHeight);

            image (3 * (imageHeight * i + j)) :=
               GLubyte (127.0 * (1.0 + Sin (ti)));
            image (3 * (imageHeight * i + j) + 1) :=
               GLubyte (127.0 * (1.0 + Cos (2.0 * tj)));
            image (3 * (imageHeight * i + j) + 2) :=
               GLubyte (127.0 * (1.0 + Cos (ti + tj)));
         end loop;
      end loop;
   end makeImage;

   procedure DoInit is
   begin
      glMap2f (GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4,
         0.0, 1.0, 12, 4, ctrlpoints (0, 0, 0)'Access);
      glMap2f (GL_MAP2_TEXTURE_COORD_2, 0.0, 1.0, 2, 2,
         0.0, 1.0, 4, 2, texpts (0, 0, 0)'Access);

      glEnable (GL_MAP2_TEXTURE_COORD_2);
      glEnable (GL_MAP2_VERTEX_3);
      glMapGrid2f (20, 0.0, 1.0, 20, 0.0, 1.0);
      makeImage;
      glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexImage2D (GL_TEXTURE_2D, 0, 3, imageWidth, imageHeight, 0,
        GL_RGB, GL_UNSIGNED_BYTE, image(0)'Access);

      glEnable (GL_TEXTURE_2D);
      glEnable (GL_DEPTH_TEST);
      glEnable (GL_NORMALIZE);
      glShadeModel (GL_FLAT);
   end DoInit;

   procedure DoDisplay is
   begin
      glClear (GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
      glColor3f (1.0, 1.0, 1.0);
      glEvalMesh2 (GL_FILL, 0, 20, 0, 20);
      glFlush;
   end DoDisplay;


   procedure ReshapeCallback (w : Integer; h : Integer) is
   begin
      glViewport (0, 0, GLsizei(w), GLsizei(h));

      glMatrixMode (GL_PROJECTION);
      glLoadIdentity;

      if w <= h then
         glOrtho (-4.0, 4.0, GLdouble (-4.0*GLdouble (h)/GLdouble (w)),
            GLdouble (4.0*GLdouble (h)/GLdouble (w)), -4.0, 4.0);
      else
         glOrtho ((-4.0*GLdouble (w)/GLdouble (h)),
            GLdouble (4.0*GLdouble (w)/GLdouble (h)), -4.0, 4.0, -4.0, 4.0);
      end if;

      glMatrixMode (GL_MODELVIEW);
      glLoadIdentity;
      glRotatef (85.0, 1.0, 1.0, 1.0);
   end ReshapeCallback;
end Texturesurf_Procs;
