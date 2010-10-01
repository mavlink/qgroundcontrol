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

package body Texgen_Procs is
   stripeImage : array (0 .. 95) of aliased GLubyte;

   procedure makeStripeImage is
   begin
      for j in 0 .. 31 loop
         if j <= 4 then stripeImage (3*j) := 255;
         else stripeImage (3*j) := 0;
         end if;

         if j > 4 then stripeImage (3*j+1) := 255;
         else stripeImage (3*j+1) := 0;
         end if;

         stripeImage (3*j+2) := 0;
      end loop;
   end makeStripeImage;

   sgenparams : array (0 .. 3) of aliased GLfloat :=
      (1.0, 1.0, 1.0, 0.0);

   procedure DoInit is
   begin
      glClearColor (0.0, 0.0, 0.0, 0.0);

      makeStripeImage;
      glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
      glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexImage1D (GL_TEXTURE_1D, 0, 3, 32, 0,
         GL_RGB, GL_UNSIGNED_BYTE, stripeImage(0)'Access);

      glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
      glTexGenfv (GL_S, GL_OBJECT_PLANE, sgenparams (0)'ACCESS);

      glEnable (GL_DEPTH_TEST);
      glDepthFunc (GL_LESS);
      glEnable (GL_TEXTURE_GEN_S);
      glEnable (GL_TEXTURE_1D);
      glEnable (GL_CULL_FACE);
      glEnable (GL_LIGHTING);
      glEnable (GL_LIGHT0);
      glEnable (GL_AUTO_NORMAL);
      glEnable (GL_NORMALIZE);
      glFrontFace (GL_CW);
      glCullFace (GL_BACK);
      glMaterialf (GL_FRONT, GL_SHININESS, 64.0);
   end DoInit;

   procedure DoDisplay is
   begin
      glClear (GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
      glPushMatrix;
      glRotatef (45.0, 0.0, 0.0, 1.0);
      glutSolidTeapot (2.0);
      glPopMatrix;
      glFlush;
   end DoDisplay;


   procedure ReshapeCallback (w : Integer; h : Integer) is
   begin
      glViewport (0, 0, GLsizei(w), GLsizei(h));

      glMatrixMode (GL_PROJECTION);
      glLoadIdentity;

      if w <= h then
         glOrtho (-3.5, 3.5, GLdouble (-3.5*GLdouble (h)/GLdouble (w)),
            GLdouble (3.5*GLdouble (h)/GLdouble (w)), -3.5, 3.5);
      else
         glOrtho ((-3.5*GLdouble (w)/GLdouble (h)),
            GLdouble (3.5*GLdouble (w)/GLdouble (h)), -3.5, 3.5, -3.5, 3.5);
      end if;

      glMatrixMode (GL_MODELVIEW);
      glLoadIdentity;
   end ReshapeCallback;
end Texgen_Procs;
