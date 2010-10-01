/*  
 *  CS 453 - Final project : An OpenGL version of the pegboard game IQ
 *  Due : June 5, 1997
 *  Author : Kiri Wagstaff
 *
 * File : pick.c
 * Description : Routines for picking ability.  MANY thanks to Nate
 *                Robins since all of this code is his.  I couldn't
 *                have done it without him. :)
 *
 */

#include <stdarg.h>
#include "gliq.h"

int picked=0;         /* Which piece has been selected? */
GLuint    select_buffer[SELECT_BUFFER];
GLboolean selection = GL_FALSE;

GLuint pick(int x, int y);
void passive(int x, int y);

GLuint pick(int x, int y)
{
  GLuint    i, hits, num_names, picked;
  GLuint*   p;
  GLboolean save;
  GLuint    depth = (GLuint)-1;
  GLint     viewport[4];
  int height = glutGet(GLUT_WINDOW_HEIGHT);
  int width = glutGet(GLUT_WINDOW_WIDTH);

  /* fill in the current viewport parameters */
  viewport[0] = 0;
  viewport[1] = 0;
  viewport[2] = width;
  viewport[3] = height;

  /* set the render mode to selection */
  glRenderMode(GL_SELECT);
  selection = GL_TRUE;
  glInitNames();
  glPushName(0);

  /* setup a picking matrix and render into selection buffer */
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glLoadIdentity();
  gluPickMatrix(x, viewport[3] - y, 5.0, 5.0, viewport);
  gluPerspective(60.0, (GLfloat)viewport[3]/(GLfloat)viewport[2], 1.0, 128.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0, -2.0, -15.0);

  switch(curstate)
    {
    case SELBOARD:
      /* Draw the quit button */
      drawquit(7.0, 9.0, 0.4, 1.0);
      displaybuttons();
      break;
    case PLAY:
      /* Draw the quit button */
      drawquit(7.0, 9.0, 0.4, 1.0);
      glPushMatrix();
      glRotatef(45.0, 1.0, 0.0, 0.0);
      tbMatrix();
      drawpegs();
      glPopMatrix();
      break;
    case HIGHSC:
      break;
    case VIEWSCORES:
      break;
    default:
      printf("Unknown state %d, exiting.\n", curstate);
      exit(1);
    }

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  hits = glRenderMode(GL_RENDER);

  selection = GL_FALSE;

  p = select_buffer;
  picked = 0;
  for (i = 0; i < hits; i++) {
    save = GL_FALSE;
    num_names = *p;			/* number of names in this hit */
    p++;

    if (*p <= depth) {			/* check the 1st depth value */
      depth = *p;
      save = GL_TRUE;
    }
    p++;
    if (*p <= depth) {			/* check the 2nd depth value */
      depth = *p;
      save = GL_TRUE;
    }
    p++;

    if (save)
      picked = *p;

    p += num_names;			/* skip over the rest of the names */
  }

  return picked;
}

void passive(int x, int y) 
{
  picked = pick(x,y);
  glutPostRedisplay();
}

/* text: general purpose text routine.  draws a string according to
 * format in a stroke font at x, y after scaling it by the scale
 * specified (scale is in window-space (lower-left origin) pixels).  
 *
 * x      - position in x (in window-space)
 * y      - position in y (in window-space)
 * scale  - scale in pixels
 * format - as in printf()
 */
void 
text(GLfloat x, GLfloat y, GLfloat scale, char* format, ...)
{
  va_list args;
  char buffer[255], *p;
  GLfloat font_scale = 119.05 + 33.33;

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT));

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_DEPTH_TEST);
  glTranslatef(x, y, 0.0);

  glScalef(scale/font_scale, scale/font_scale, scale/font_scale);

  for(p = buffer; *p; p++)
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
  
  glPopAttrib();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}


