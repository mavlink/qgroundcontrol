
/* tr.c */

/*
 * Tiled Rendering library
 *
 * Brian Paul
 * April 1997
 */


#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "tr.h"



struct _TRctx {
   GLint ImageWidth, ImageHeight;
   GLubyte *Image;

   GLint TileWidth, TileHeight;

   GLboolean Perspective;
   GLdouble Left;
   GLdouble Right;
   GLdouble Bottom;
   GLdouble Top;
   GLdouble Near;
   GLdouble Far;

   GLint Rows, Columns;
   GLint CurrentTile;
   GLint CurrentTileWidth, CurrentTileHeight;
   GLint CurrentRow, CurrentColumn;

   GLint ViewportSave[4];
};



TRcontext *trNew(void)
{
   return (TRcontext *) calloc(1, sizeof(TRcontext));
}


void trDelete(TRcontext *tr)
{
   if (tr)
      free(tr);
}


void trSetup(TRcontext *tr,
	     GLint imageWidth, GLint imageHeight, GLubyte *image,
	     GLint tileWidth, GLint tileHeight)
{
   if (!tr || !image)
      return;

   tr->ImageWidth = imageWidth;
   tr->ImageHeight = imageHeight;
   tr->Image = image;
   tr->TileWidth = tileWidth;
   tr->TileHeight = tileHeight;

   tr->Columns = (tr->ImageWidth + tr->TileWidth - 1) / tr->TileWidth;
   tr->Rows = (tr->ImageHeight + tr->TileHeight - 1) / tr->TileHeight;
   tr->CurrentTile = 0;

   assert(tr->Columns >= 1);
   assert(tr->Rows >= 1);
}


void trOrtho(TRcontext *tr,
	     GLdouble left, GLdouble right,
	     GLdouble bottom, GLdouble top,
	     GLdouble nnear, GLdouble ffar)
{
   if (!tr)
      return;

   tr->Perspective = GL_FALSE;
   tr->Left = left;
   tr->Right = right;
   tr->Bottom = bottom;
   tr->Top = top;
   tr->Near = nnear;
   tr->Far = ffar;
   tr->CurrentTile = 0;
}


void trFrustum(TRcontext *tr,
	       GLdouble left, GLdouble right,
	       GLdouble bottom, GLdouble top,
	       GLdouble zNear, GLdouble zFar)
{
   if (!tr)
      return;

   tr->Perspective = GL_TRUE;
   tr->Left = left;
   tr->Right = right;
   tr->Bottom = bottom;
   tr->Top = top;
   tr->Near = zNear;
   tr->Far = zFar;
   tr->CurrentTile = 0;
}


void trPerspective(TRcontext *tr,
		   GLdouble fovy, GLdouble aspect,
		   GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;
   ymax = zNear * tan(fovy * 3.14159265 / 360.0);
   ymin = -ymax;
   xmin = ymin * aspect;
   xmax = ymax * aspect;
   trFrustum(tr, xmin, xmax, ymin, ymax, zNear, zFar);
}


void trBeginTile(TRcontext *tr)
{
   GLint matrixMode;
   int tileWidth, tileHeight;
   GLdouble left, right, bottom, top;

   if (!tr)
      return;

   if (tr->CurrentTile==0) {
      /* Save user's viewport, will be restored after last tile rendered */
      glGetIntegerv(GL_VIEWPORT, tr->ViewportSave);
   }

   /* which tile (by row and column) we're about to render */
   tr->CurrentRow = tr->CurrentTile / tr->Columns;
   tr->CurrentColumn = tr->CurrentTile % tr->Columns;

   /* actual size of this tile */
   if (tr->CurrentRow < tr->Rows-1)
      tileHeight = tr->TileHeight;
   else
      tileHeight = tr->ImageHeight - (tr->Rows-1) * tr->TileHeight;

   if (tr->CurrentColumn < tr->Columns-1)
      tileWidth = tr->TileWidth;
   else
      tileWidth = tr->ImageWidth - (tr->Columns-1) * tr->TileWidth;


   tr->CurrentTileWidth = tileWidth;
   tr->CurrentTileHeight = tileHeight;

   glViewport(0, 0, tileWidth, tileHeight);

   /* save current matrix mode */
   glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   /* compute projection parameters */
   left = tr->Left + (tr->Right - tr->Left)
        * (tr->CurrentColumn * tr->TileWidth) / tr->ImageWidth;
   right = left + (tr->Right - tr->Left) * tileWidth / tr->ImageWidth;
   bottom = tr->Bottom + (tr->Top - tr->Bottom)
          * (tr->CurrentRow * tr->TileHeight) / tr->ImageHeight;
   top = bottom + (tr->Top - tr->Bottom) * tileHeight / tr->ImageHeight;

   if (tr->Perspective)
      glFrustum(left, right, bottom, top, tr->Near, tr->Far);
   else
      glOrtho(left, right, bottom, top, tr->Near, tr->Far);

   /* restore user's matrix mode */
   glMatrixMode(matrixMode);
}



int trEndTile(TRcontext *tr)
{
   GLint prevRowLength, prevSkipRows, prevSkipPixels, prevAlignment;
   GLint x, y;

   if (!tr)
      return 0;

   /* be sure OpenGL rendering is finished */
   glFlush();

   /* save current glPixelStore values */
   glGetIntegerv(GL_PACK_ROW_LENGTH, &prevRowLength);
   glGetIntegerv(GL_PACK_SKIP_ROWS, &prevSkipRows);
   glGetIntegerv(GL_PACK_SKIP_PIXELS, &prevSkipPixels);
   glGetIntegerv(GL_PACK_ALIGNMENT, &prevAlignment);

   x = tr->TileWidth * tr->CurrentColumn;
   y = tr->TileHeight * tr->CurrentRow;

   /* setup pixel store for glReadPixels */
   glPixelStorei(GL_PACK_ROW_LENGTH, tr->ImageWidth);
   glPixelStorei(GL_PACK_SKIP_ROWS, y);
   glPixelStorei(GL_PACK_SKIP_PIXELS, x);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   /* read the tile into the final image */
   glReadPixels(0, 0, tr->CurrentTileWidth, tr->CurrentTileHeight,
		GL_RGBA, GL_UNSIGNED_BYTE, tr->Image);

   /* restore previous glPixelStore values */
   glPixelStorei(GL_PACK_ROW_LENGTH, prevRowLength);
   glPixelStorei(GL_PACK_SKIP_ROWS, prevSkipRows);
   glPixelStorei(GL_PACK_SKIP_PIXELS, prevSkipPixels);
   glPixelStorei(GL_PACK_ALIGNMENT, prevAlignment);

   /* increment tile counter, return 1 if more tiles left to render */
   tr->CurrentTile++;
   if (tr->CurrentTile >= tr->Rows * tr->Columns) {
      /* restore user's viewport */
      glViewport(tr->ViewportSave[0], tr->ViewportSave[1],
		 tr->ViewportSave[2], tr->ViewportSave[3]);
      return 0;
   }
   else
      return 1;
}

