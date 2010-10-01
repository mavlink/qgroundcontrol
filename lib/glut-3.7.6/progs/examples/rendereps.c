
/* Copyright (c) Mark J. Kilgard, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */

/* Example showing how to use OpenGL's feedback mode to capture
   transformed vertices and output them as Encapsulated PostScript.
   Handles limited hidden surface removal by sorting and does
   smooth shading (albeit limited due to PostScript). */

/* Compile: cc -o rendereps rendereps.c -lglut -lGLU -lGL -lXmu -lXext -lX11 -lm */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

/* OpenGL's GL_3D_COLOR feedback vertex format. */
typedef struct _Feedback3Dcolor {
  GLfloat x;
  GLfloat y;
  GLfloat z;
  GLfloat red;
  GLfloat green;
  GLfloat blue;
  GLfloat alpha;
} Feedback3Dcolor;

int blackBackground = 0;  /* Initially use a white background. */
int lighting = 0;       /* Initially disable lighting. */
int polygonMode = 1;    /* Initially show wireframe. */
int object = 1;         /* Initially show the torus. */

GLfloat angle = 0.0;    /* Angle of rotation for object. */
int moving, begin;      /* For interactive object rotation. */
int size = 1;           /* Size of lines and points. */

/* How many feedback buffer GLfloats each of the three objects need. */
int objectComplexity[3] =
{6000, 14000, 380000};  /* Teapot requires ~1.5 megabytes for
                           its feedback results! */

/* render gets called both by "display" (in OpenGL render mode)
   and by "outputEPS" (in OpenGL feedback mode). */
void
render(void)
{
  glPushMatrix();
  glRotatef(angle, 0.0, 1.0, 0.0);
  switch (object) {
  case 0:
    glutSolidSphere(1.0, 10, 10);
    break;
  case 1:
    glutSolidTorus(0.5, 1.0, 15, 15);
    break;
  case 2:
    glutSolidTeapot(1.0);
    break;
  }
  glPopMatrix();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  render();
  glutSwapBuffers();
}

void
updateBackground(void)
{
  if (blackBackground) {
    /* Clear to black. */
    glClearColor(0.0, 0.0, 0.0, 1.0);
  } else {
    /* Clear to white. */
    glClearColor(1.0, 1.0, 1.0, 1.0);
  }
}

void
updateLighting(void)
{
  if (lighting) {
    glEnable(GL_LIGHTING);
  } else {
    glDisable(GL_LIGHTING);
  }
}

void
updatePolygonMode(void)
{
  switch (polygonMode) {
  case 0:
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    break;
  case 1:
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  case 2:
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  }
}

/* Write contents of one vertex to stdout. */
void
print3DcolorVertex(GLint size, GLint * count,
  GLfloat * buffer)
{
  int i;

  printf("  ");
  for (i = 0; i < 7; i++) {
    printf("%4.2f ", buffer[size - (*count)]);
    *count = *count - 1;
  }
  printf("\n");
}

void
printBuffer(GLint size, GLfloat * buffer)
{
  GLint count;
  int token, nvertices;

  count = size;
  while (count) {
    token = buffer[size - count];
    count--;
    switch (token) {
    case GL_PASS_THROUGH_TOKEN:
      printf("GL_PASS_THROUGH_TOKEN\n");
      printf("  %4.2f\n", buffer[size - count]);
      count--;
      break;
    case GL_POINT_TOKEN:
      printf("GL_POINT_TOKEN\n");
      print3DcolorVertex(size, &count, buffer);
      break;
    case GL_LINE_TOKEN:
      printf("GL_LINE_TOKEN\n");
      print3DcolorVertex(size, &count, buffer);
      print3DcolorVertex(size, &count, buffer);
      break;
    case GL_LINE_RESET_TOKEN:
      printf("GL_LINE_RESET_TOKEN\n");
      print3DcolorVertex(size, &count, buffer);
      print3DcolorVertex(size, &count, buffer);
      break;
    case GL_POLYGON_TOKEN:
      printf("GL_POLYGON_TOKEN\n");
      nvertices = buffer[size - count];
      count--;
      for (; nvertices > 0; nvertices--) {
        print3DcolorVertex(size, &count, buffer);
      }
    }
  }
}

GLfloat pointSize;

static char *gouraudtriangleEPS[] =
{
  "/bd{bind def}bind def /triangle { aload pop   setrgbcolor  aload pop 5 3",
  "roll 4 2 roll 3 2 roll exch moveto lineto lineto closepath fill } bd",
  "/computediff1 { 2 copy sub abs threshold ge {pop pop pop true} { exch 2",
  "index sub abs threshold ge { pop pop true} { sub abs threshold ge } ifelse",
  "} ifelse } bd /computediff3 { 3 copy 0 get 3 1 roll 0 get 3 1 roll 0 get",
  "computediff1 {true} { 3 copy 1 get 3 1 roll 1 get 3 1 roll 1 get",
  "computediff1 {true} { 3 copy 2 get 3 1 roll  2 get 3 1 roll 2 get",
  "computediff1 } ifelse } ifelse } bd /middlecolor { aload pop 4 -1 roll",
  "aload pop 4 -1 roll add 2 div 5 1 roll 3 -1 roll add 2 div 3 1 roll add 2",
  "div 3 1 roll exch 3 array astore } bd /gouraudtriangle { computediff3 { 4",
  "-1 roll aload 7 1 roll 6 -1 roll pop 3 -1 roll pop add 2 div 3 1 roll add",
  "2 div exch 3 -1 roll aload 7 1 roll exch pop 4 -1 roll pop add 2 div 3 1",
  "roll add 2 div exch 3 -1 roll aload 7 1 roll pop 3 -1 roll pop add 2 div 3",
  "1 roll add 2 div exch 7 3 roll 10 -3 roll dup 3 index middlecolor 4 1 roll",
  "2 copy middlecolor 4 1 roll 3 copy pop middlecolor 4 1 roll 13 -1 roll",
  "aload pop 17 index 6 index 15 index 19 index 6 index 17 index 6 array",
  "astore 10 index 10 index 14 index gouraudtriangle 17 index 5 index 17",
  "index 19 index 5 index 19 index 6 array astore 10 index 9 index 13 index",
  "gouraudtriangle 13 index 16 index 5 index 15 index 18 index 5 index 6",
  "array astore 12 index 12 index 9 index gouraudtriangle 17 index 16 index",
  "15 index 19 index 18 index 17 index 6 array astore 10 index 12 index 14",
  "index gouraudtriangle 18 {pop} repeat } { aload pop 5 3 roll aload pop 7 3",
  "roll aload pop 9 3 roll 4 index 6 index 4 index add add 3 div 10 1 roll 7",
  "index 5 index 3 index add add 3 div 10 1 roll 6 index 4 index 2 index add",
  "add 3 div 10 1 roll 9 {pop} repeat 3 array astore triangle } ifelse } bd",
  NULL
};

GLfloat *
spewPrimitiveEPS(FILE * file, GLfloat * loc)
{
  int token;
  int nvertices, i;
  GLfloat red, green, blue;
  int smooth;
  GLfloat dx, dy, dr, dg, db, absR, absG, absB, colormax;
  int steps;
  Feedback3Dcolor *vertex;
  GLfloat xstep, ystep, rstep, gstep, bstep;
  GLfloat xnext, ynext, rnext, gnext, bnext, distance;

  token = *loc;
  loc++;
  switch (token) {
  case GL_LINE_RESET_TOKEN:
  case GL_LINE_TOKEN:
    vertex = (Feedback3Dcolor *) loc;

    dr = vertex[1].red - vertex[0].red;
    dg = vertex[1].green - vertex[0].green;
    db = vertex[1].blue - vertex[0].blue;

    if (dr != 0 || dg != 0 || db != 0) {
      /* Smooth shaded line. */
      dx = vertex[1].x - vertex[0].x;
      dy = vertex[1].y - vertex[0].y;
      distance = sqrt(dx * dx + dy * dy);

      absR = fabs(dr);
      absG = fabs(dg);
      absB = fabs(db);

#define Max(a,b) (((a)>(b))?(a):(b))

#define EPS_SMOOTH_LINE_FACTOR 0.06  /* Lower for better smooth 

                                        lines. */

      colormax = Max(absR, Max(absG, absB));
      steps = Max(1.0, colormax * distance * EPS_SMOOTH_LINE_FACTOR);

      xstep = dx / steps;
      ystep = dy / steps;

      rstep = dr / steps;
      gstep = dg / steps;
      bstep = db / steps;

      xnext = vertex[0].x;
      ynext = vertex[0].y;
      rnext = vertex[0].red;
      gnext = vertex[0].green;
      bnext = vertex[0].blue;

      /* Back up half a step; we want the end points to be
         exactly the their endpoint colors. */
      xnext -= xstep / 2.0;
      ynext -= ystep / 2.0;
      rnext -= rstep / 2.0;
      gnext -= gstep / 2.0;
      bnext -= bstep / 2.0;
    } else {
      /* Single color line. */
      steps = 0;
    }

    fprintf(file, "%g %g %g setrgbcolor\n",
      vertex[0].red, vertex[0].green, vertex[0].blue);
    fprintf(file, "%g %g moveto\n", vertex[0].x, vertex[0].y);

    for (i = 0; i < steps; i++) {
      xnext += xstep;
      ynext += ystep;
      rnext += rstep;
      gnext += gstep;
      bnext += bstep;
      fprintf(file, "%g %g lineto stroke\n", xnext, ynext);
      fprintf(file, "%g %g %g setrgbcolor\n", rnext, gnext, bnext);
      fprintf(file, "%g %g moveto\n", xnext, ynext);
    }
    fprintf(file, "%g %g lineto stroke\n", vertex[1].x, vertex[1].y);

    loc += 14;          /* Each vertex element in the feedback
                           buffer is 7 GLfloats. */

    break;
  case GL_POLYGON_TOKEN:
    nvertices = *loc;
    loc++;

    vertex = (Feedback3Dcolor *) loc;

    if (nvertices > 0) {
      red = vertex[0].red;
      green = vertex[0].green;
      blue = vertex[0].blue;
      smooth = 0;
      for (i = 1; i < nvertices; i++) {
        if (red != vertex[i].red || green != vertex[i].green || blue != vertex[i].blue) {
          smooth = 1;
          break;
        }
      }
      if (smooth) {
        /* Smooth shaded polygon; varying colors at vetices. */
        /* Break polygon into "nvertices-2" triangle fans. */
        for (i = 0; i < nvertices - 2; i++) {
          fprintf(file, "[%g %g %g %g %g %g]",
            vertex[0].x, vertex[i + 1].x, vertex[i + 2].x,
            vertex[0].y, vertex[i + 1].y, vertex[i + 2].y);
          fprintf(file, " [%g %g %g] [%g %g %g] [%g %g %g] gouraudtriangle\n",
            vertex[0].red, vertex[0].green, vertex[0].blue,
            vertex[i + 1].red, vertex[i + 1].green, vertex[i + 1].blue,
            vertex[i + 2].red, vertex[i + 2].green, vertex[i + 2].blue);
        }
      } else {
        /* Flat shaded polygon; all vertex colors the same. */
        fprintf(file, "newpath\n");
        fprintf(file, "%g %g %g setrgbcolor\n", red, green, blue);

        /* Draw a filled triangle. */
        fprintf(file, "%g %g moveto\n", vertex[0].x, vertex[0].y);
        for (i = 1; i < nvertices; i++) {
          fprintf(file, "%g %g lineto\n", vertex[i].x, vertex[i].y);
        }
        fprintf(file, "closepath fill\n\n");
      }
    }
    loc += nvertices * 7;  /* Each vertex element in the
                              feedback buffer is 7 GLfloats. */
    break;
  case GL_POINT_TOKEN:
    vertex = (Feedback3Dcolor *) loc;
    fprintf(file, "%g %g %g setrgbcolor\n", vertex[0].red, vertex[0].green, vertex[0].blue);
    fprintf(file, "%g %g %g 0 360 arc fill\n\n", vertex[0].x, vertex[0].y, pointSize / 2.0);
    loc += 7;           /* Each vertex element in the feedback
                           buffer is 7 GLfloats. */
    break;
  default:
    /* XXX Left as an excersie to the reader. */
    printf("Incomplete implementation.  Unexpected token (%d).\n", token);
    exit(1);
  }
  return loc;
}

void
spewUnsortedFeedback(FILE * file, GLint size, GLfloat * buffer)
{
  GLfloat *loc, *end;

  loc = buffer;
  end = buffer + size;
  while (loc < end) {
    loc = spewPrimitiveEPS(file, loc);
  }
}

typedef struct _DepthIndex {
  GLfloat *ptr;
  GLfloat depth;
} DepthIndex;

static int
compare(const void *a, const void *b)
{
  DepthIndex *p1 = (DepthIndex *) a;
  DepthIndex *p2 = (DepthIndex *) b;
  GLfloat diff = p2->depth - p1->depth;

  if (diff > 0.0) {
    return 1;
  } else if (diff < 0.0) {
    return -1;
  } else {
    return 0;
  }
}

void
spewSortedFeedback(FILE * file, GLint size, GLfloat * buffer)
{
  int token;
  GLfloat *loc, *end;
  Feedback3Dcolor *vertex;
  GLfloat depthSum;
  int nprimitives, item;
  DepthIndex *prims;
  int nvertices, i;

  end = buffer + size;

  /* Count how many primitives there are. */
  nprimitives = 0;
  loc = buffer;
  while (loc < end) {
    token = *loc;
    loc++;
    switch (token) {
    case GL_LINE_TOKEN:
    case GL_LINE_RESET_TOKEN:
      loc += 14;
      nprimitives++;
      break;
    case GL_POLYGON_TOKEN:
      nvertices = *loc;
      loc++;
      loc += (7 * nvertices);
      nprimitives++;
      break;
    case GL_POINT_TOKEN:
      loc += 7;
      nprimitives++;
      break;
    default:
      /* XXX Left as an excersie to the reader. */
      printf("Incomplete implementation.  Unexpected token (%d).\n",
        token);
      exit(1);
    }
  }

  /* Allocate an array of pointers that will point back at
     primitives in the feedback buffer.  There will be one
     entry per primitive.  This array is also where we keep the
     primitive's average depth.  There is one entry per
     primitive  in the feedback buffer. */
  prims = (DepthIndex *) malloc(sizeof(DepthIndex) * nprimitives);

  item = 0;
  loc = buffer;
  while (loc < end) {
    prims[item].ptr = loc;  /* Save this primitive's location. */
    token = *loc;
    loc++;
    switch (token) {
    case GL_LINE_TOKEN:
    case GL_LINE_RESET_TOKEN:
      vertex = (Feedback3Dcolor *) loc;
      depthSum = vertex[0].z + vertex[1].z;
      prims[item].depth = depthSum / 2.0;
      loc += 14;
      break;
    case GL_POLYGON_TOKEN:
      nvertices = *loc;
      loc++;
      vertex = (Feedback3Dcolor *) loc;
      depthSum = vertex[0].z;
      for (i = 1; i < nvertices; i++) {
        depthSum += vertex[i].z;
      }
      prims[item].depth = depthSum / nvertices;
      loc += (7 * nvertices);
      break;
    case GL_POINT_TOKEN:
      vertex = (Feedback3Dcolor *) loc;
      prims[item].depth = vertex[0].z;
      loc += 7;
      break;
    default:
      /* XXX Left as an excersie to the reader. */
      assert(1);
    }
    item++;
  }
  assert(item == nprimitives);

  /* Sort the primitives back to front. */
  qsort(prims, nprimitives, sizeof(DepthIndex), compare);

  /* XXX Understand that sorting by a primitives average depth
     doesn't allow us to disambiguate some cases like self
     intersecting polygons.  Handling these cases would require
     breaking up the primitives.  That's too involved for this
     example.  Sorting by depth is good enough for lots of
     applications. */

  /* Emit the Encapsulated PostScript for the primitives in
     back to front order. */
  for (item = 0; item < nprimitives; item++) {
    (void) spewPrimitiveEPS(file, prims[item].ptr);
  }

  free(prims);
}

#define EPS_GOURAUD_THRESHOLD 0.1  /* Lower for better (slower) 

                                      smooth shading. */

void
spewWireFrameEPS(FILE * file, int doSort, GLint size, GLfloat * buffer, char *creator)
{
  GLfloat clearColor[4], viewport[4];
  GLfloat lineWidth;
  int i;

  /* Read back a bunch of OpenGL state to help make the EPS
     consistent with the OpenGL clear color, line width, point
     size, and viewport. */
  glGetFloatv(GL_VIEWPORT, viewport);
  glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
  glGetFloatv(GL_LINE_WIDTH, &lineWidth);
  glGetFloatv(GL_POINT_SIZE, &pointSize);

  /* Emit EPS header. */
  fputs("%!PS-Adobe-2.0 EPSF-2.0\n", file);
  /* Notice %% for a single % in the fprintf calls. */
  fprintf(file, "%%%%Creator: %s (using OpenGL feedback)\n", creator);
  fprintf(file, "%%%%BoundingBox: %g %g %g %g\n",
    viewport[0], viewport[1], viewport[2], viewport[3]);
  fputs("%%EndComments\n", file);
  fputs("\n", file);
  fputs("gsave\n", file);
  fputs("\n", file);

  /* Output Frederic Delhoume's "gouraudtriangle" PostScript
     fragment. */
  fputs("% the gouraudtriangle PostScript fragement below is free\n", file);
  fputs("% written by Frederic Delhoume (delhoume@ilog.fr)\n", file);
  fprintf(file, "/threshold %g def\n", EPS_GOURAUD_THRESHOLD);
  for (i = 0; gouraudtriangleEPS[i]; i++) {
    fprintf(file, "%s\n", gouraudtriangleEPS[i]);
  }

  fprintf(file, "\n%g setlinewidth\n", lineWidth);

  /* Clear the background like OpenGL had it. */
  fprintf(file, "%g %g %g setrgbcolor\n",
    clearColor[0], clearColor[1], clearColor[2]);
  fprintf(file, "%g %g %g %g rectfill\n\n",
    viewport[0], viewport[1], viewport[2], viewport[3]);

  if (doSort) {
    spewSortedFeedback(file, size, buffer);
  } else {
    spewUnsortedFeedback(file, size, buffer);
  }

  /* Emit EPS trailer. */
  fputs("grestore\n\n", file);
  fputs("%Add `showpage' to the end of this file to be able to print to a printer.\n",
    file);

  fclose(file);
}

void
outputEPS(int size, int doSort, char *filename)
{
  GLfloat *feedbackBuffer;
  GLint returned;
  FILE *file;

  feedbackBuffer = calloc(size, sizeof(GLfloat));
  glFeedbackBuffer(size, GL_3D_COLOR, feedbackBuffer);
  (void) glRenderMode(GL_FEEDBACK);
  render();
  returned = glRenderMode(GL_RENDER);
  if (filename) {
    file = fopen(filename, "w");
    if (file) {
      spewWireFrameEPS(file, doSort, returned, feedbackBuffer, "rendereps");
    } else {
      printf("Could not open %s\n", filename);
    }
  } else {
    /* Helps debugging to be able to see the decode feedback
       buffer as text. */
    printBuffer(returned, feedbackBuffer);
  }
  free(feedbackBuffer);
}

void
choice(int value)
{
  switch (value) {
  case 0:
    glutSetCursor(GLUT_CURSOR_WAIT);
    outputEPS(objectComplexity[object], 1, "render.eps");
    glutSetCursor(GLUT_CURSOR_INHERIT);
    break;
  case 1:
    glutSetCursor(GLUT_CURSOR_WAIT);
    outputEPS(objectComplexity[object], 0, "render.eps");
    glutSetCursor(GLUT_CURSOR_INHERIT);
    break;
  case 2:
    /* Try to start GNU "ghostview" to preview the EPS. */
    system("ghostview render.eps &");
    break;
  case 3:
    glutSetCursor(GLUT_CURSOR_WAIT);
    outputEPS(objectComplexity[object], 0, NULL);
    glutSetCursor(GLUT_CURSOR_INHERIT);
    break;
  case 4:
    blackBackground = 1 - blackBackground;
    updateBackground();
    glutPostRedisplay();
    break;
  case 5:
    lighting = 1 - lighting;
    updateLighting();
    glutPostRedisplay();
    break;
  case 6:
    polygonMode = (polygonMode + 1) % 3;
    updatePolygonMode();
    glutPostRedisplay();
    break;
  case 7:
    size = (size % 5) + 1;
    glLineWidth(size);
    glPointSize(size);
    glutPostRedisplay();
    break;
  case 8:
    object = (object + 1) % 3;
    glutPostRedisplay();
    break;
  case 666:
    exit(0);
    break;
  }
}

/* ARGSUSED2 */
void
mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    moving = 1;
    begin = x;
  }
  if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
    moving = 0;
  }
}

/* ARGSUSED1 */
void
motion(int x, int y)
{
  if (moving) {
    angle = angle + (x - begin);
    begin = x;
    glutPostRedisplay();
  }
}

GLfloat light_diffuse[] =
{0.0, 1.0, 0.0, 1.0};   /* Green light. */
GLfloat light_position[] =
{1.0, 1.0, 1.0, 0.0};

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow("rendereps");
  glutDisplayFunc(display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);

  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glEnable(GL_LIGHT0);

  glMatrixMode(GL_PROJECTION);
  gluPerspective( /* field of view in degree */ 22.0,
  /* aspect ratio */ 1.0,
    /* Z near */ 5.0, /* Z far */ 10.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 0.0, 5.0,  /* eye is at (0,0,5) */
    0.0, 0.0, 0.0,      /* center is at (0,0,0) */
    0.0, 1.0, 0.);      /* up is in postivie Y direction */
  glTranslatef(0.0, 0.0, -3.0);

  /* Give the object an "interesting" orientation. */
  glRotatef(25, 1.0, 0.0, 0.0);

  glutCreateMenu(choice);
  glutAddMenuEntry("Write out Encapsulated PS (sorted)", 0);
  glutAddMenuEntry("Write out Encapsulated PS (UNsorted)", 1);
  glutAddMenuEntry("Spawn ghostview to view EPS", 2);
  glutAddMenuEntry("Display feedback buffer", 3);
  glutAddMenuEntry("Toggle black/white background", 4);
  glutAddMenuEntry("Toggle lighting", 5);
  glutAddMenuEntry("Switch fill mode (line, poly, point)", 6);
  glutAddMenuEntry("Switch line/point size", 7);
  glutAddMenuEntry("Switch object", 8);
  glutAddMenuEntry("Quit", 666);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  updateBackground();
  updateLighting();
  updatePolygonMode();

  glEnable(GL_DEPTH_TEST);
  glColor3f(1.0, 0.0, 0.0);  /* Geometry should appear red. */

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
