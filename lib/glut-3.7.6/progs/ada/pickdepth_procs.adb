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
with GLUT; use GLUT;
with Text_IO;

package body PickDepth_Procs is
   package tio renames Text_IO;

   procedure DoInit is
   begin
      glClearColor (0.0, 0.0, 0.0, 0.0);
      glDepthFunc (GL_LESS);
      glEnable (GL_DEPTH_TEST);
      glShadeModel (GL_FLAT);
      glDepthRange (0.0, 1.0);
   end DoInit;

   procedure DrawRects (mode : RenderingMode) is
   begin
      if mode = GL_SELECT then glLoadName (1); end if;
      glBegin (GL_QUADS);
      glColor3f (1.0, 1.0, 0.0);
      glVertex3i (2, 0, 0);
      glVertex3i (2, 6, 0);
      glVertex3i (6, 6, 0);
      glVertex3i (6, 0, 0);
      glEnd;

      if mode = GL_SELECT then glLoadName (2); end if;
      glBegin (GL_QUADS);
      glColor3f (0.0, 1.0, 1.0);
      glVertex3i (3, 2, -1);
      glVertex3i (3, 8, -1);
      glVertex3i (8, 8, -1);
      glVertex3i (8, 2, -1);
      glEnd;

      if mode = GL_SELECT then glLoadName (3); end if;
      glBegin (GL_QUADS);
      glColor3f (1.0, 0.0, 1.0);
      glVertex3i (0, 2, -2);
      glVertex3i (0, 7, -2);
      glVertex3i (5, 7, -2);
      glVertex3i (5, 2, -2);
      glEnd;
   end DrawRects;

   type int_ar is array (Integer range <>) of aliased GLuint;

   procedure ProcessHits (hits : GLint; buffer : in int_ar) is
      j : Integer := buffer'First;
   begin
      tio.Put_Line ("Hits = " & GLint'Image (hits));

      if hits /= 0 then
         for i in
            Integer (buffer'First) ..
               Integer (buffer'First + Integer (hits) - 1)
         loop
            tio.Put_Line (" number of names for hit = " &
               GLuint'Image (buffer (j)));
            j := j + 1;
            tio.Put (" z1 is " & GLuint'Image (buffer (j)));
            j := j + 1;
            tio.Put ("; z2 is " & GLuint'Image (buffer (j)));
            j := j + 1;
            tio.New_Line;
            tio.Put ("  names:");

            for k in 1 .. Integer (buffer (buffer'First)) loop
               tio.Put (" " & GLuint'Image (buffer (j)));
               j := j + 1;
            end loop;
            tio.New_Line;
         end loop;
      end if;
   end ProcessHits;

   BUFSIZE : constant := 512;

   procedure PickRects (btn : Integer; state: Integer; x, y: Integer) is
      selectBuf : array (1 .. BUFSIZE) of aliased GLuint;
      hits : GLint;
      viewport : array (0 .. 3) of aliased GLint;
   begin
    if state = GLUT_LEFT_BUTTON then
     if state = GLUT_DOWN then
      glGetIntegerv (GL_VIEWPORT, viewport (0)'Access);

      glSelectBuffer (BUFSIZE, selectBuf (1)'Access);
      hits := glRenderMode (GL_SELECT);

      glInitNames;
      glPushName (-1);

      glMatrixMode (GL_PROJECTION);
      glPushMatrix;
      glLoadIdentity;
      gluPickMatrix (GLdouble (x), GLdouble (viewport (3) - GLint(y)),
         5.0, 5.0, viewport (0)'Access);
      glOrtho (0.0, 8.0, 0.0, 8.0, -0.5, 2.5);
      DrawRects (GL_SELECT);
      glPopMatrix;
      glFlush;

      hits := glRenderMode (GL_RENDER);
      ProcessHits (hits, int_ar (selectBuf));
     end if;
    end if;
   end PickRects;

   procedure DoDisplay is
   begin
      glClear (GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT);
      DrawRects (GL_RENDER);
      glFlush;
   end DoDisplay;


   procedure ReshapeCallback (w : Integer; h : Integer) is
   begin
      glViewport (0, 0, GLsizei(w), GLsizei(h));
      glMatrixMode (GL_PROJECTION);
      glLoadIdentity;
      glOrtho (0.0, 8.0, 0.0, 8.0, -0.5, 2.5);
      glMatrixMode (GL_MODELVIEW);
      glLoadIdentity;
   end ReshapeCallback;
end PickDepth_Procs;
