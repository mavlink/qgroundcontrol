
/* softshadow2.c - by Simon Hui, 3Dfx Interactive */

/* Soft shadows using a shadow texture per polygon.  Based on an algorithm   */
/* described by Paul Heckbert and Michael Herf of CMU; see their web site    */
/* http://www.cs.cmu.edu/ph/shadow.html for details.                         */
/*                                                                           */
/* This program shows two methods of using precomputed, per-polygon textures */
/* to display soft shadows.  The first method is a simplified version of     */
/* Heckbert and Herf's algorithm: for each polygon a texture is created that */
/* encodes the full radiance, including illumination and shadows, of the     */
/* polygon. The texture is created in a preprocessing step by rendering the  */
/* entire scene onto the polygon from the point of view of the light. The    */
/* advantage of this method is that the scene can be rerendered quickly (if  */
/* only the eye moves and the scene is static), since all lighting effects   */
/* have been precomputed and encoded in the texture. This method requires    */
/* GL_RGB textures.                                                          */
/*                                                                           */
/* The second method uses the texture as an occlusion map: the texels        */
/* encode only the amount of occlusion by shadowing objects, not the full    */
/* radiance.  The texture is then used to modulate the lighting of the       */
/* polygon during the rendering pass.  This has the disadvantage of          */
/* requiring OpenGL lighting during scene rendering, but it does retain some */
/* of the benefit of the first method in that all shadow effects are         */
/* precomputed. This method requires GL_LUMINANCE textures.                  */
/*                                                                           */
/* The reason for including the occlusion map method is that some OpenGL     */
/* implementations support GL_RGB textures with low color resolution,        */
/* resulting in noticeable banding when using radiance maps.  However, these */
/* implementations may support a higher color resolution for GL_LUMINANCE    */
/* textures.                                                                 */
/*                                                                           */
/* To use occlusion maps instead of rediance maps, run this program with     */
/* "-o" on the command line.                                                 */

#include <GL/glut.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if !defined(GL_VERSION_1_1) && !defined(GL_VERSION_1_2)
#define glBindTexture glBindTextureEXT
#define glCopyTexImage2D glCopyTexImage2DEXT
#endif

/* whether to use radiance maps or occlusion maps */
static GLboolean radianceMap = GL_TRUE;

static GLint winxsize = 480, winysize = 480;
static GLint texxsize = 128, texysize = 128;

/* texture object names */
const GLuint floorTexture = 1;
const GLuint shadowTextures = 2;

static GLfloat lightpos[4] = { 70.f, 70.f, -320.f, 1.f };

/* number of shadow textures to make and use */
static GLint numShadowTex;

/* list of polygons that have shadow textures */
GLfloat pts[][4][3] = {
  /* floor */
  -100.f, -100.f, -320.f,
  -100.f, -100.f, -520.f,
   100.f, -100.f, -320.f,
   100.f, -100.f, -520.f,

  /* left wall */
  -100.f, -100.f, -320.f,
  -100.f,  100.f, -320.f,
  -100.f, -100.f, -520.f,
  -100.f,  100.f, -520.f,

  /* back wall */
  -100.f, -100.f, -520.f,
  -100.f,  100.f, -520.f,
   100.f, -100.f, -520.f,
   100.f,  100.f, -520.f,

  /* right wall */
   100.f, -100.f, -520.f,
   100.f,  100.f, -520.f,
   100.f, -100.f, -320.f,
   100.f,  100.f, -320.f,

  /* ceiling */
  -100.f,  100.f, -520.f,
  -100.f,  100.f, -320.f,
   100.f,  100.f, -520.f,
   100.f,  100.f, -320.f,

  /* blue panel */
   -60.f,  -40.f, -400.f,
   -60.f,   70.f, -400.f,
   -30.f,  -40.f, -480.f,
   -30.f,   70.f, -480.f,

  /* yellow panel */
   -40.f,  -50.f, -400.f,
   -40.f,   50.f, -400.f,
   -10.f,  -50.f, -450.f,
   -10.f,   50.f, -450.f,

  /* red panel */
   -20.f,  -60.f, -400.f,
   -20.f,   30.f, -400.f,
    10.f,  -60.f, -420.f,
    10.f,   30.f, -420.f,

  /* green panel */
     0.f,  -70.f, -400.f,
     0.f,   10.f, -400.f,
    30.f,  -70.f, -395.f,
    30.f,   10.f, -395.f,
};

GLfloat materials[][4] = {
  1.0f, 1.0f, 1.0f, 1.0f, /* floor        */
  1.0f, 1.0f, 1.0f, 1.0f, /* left wall    */
  1.0f, 1.0f, 1.0f, 1.0f, /* back wall    */
  1.0f, 1.0f, 1.0f, 1.0f, /* right wall   */
  1.0f, 1.0f, 1.0f, 1.0f, /* ceiling      */
  0.2f, 0.5f, 1.0f, 1.0f, /* blue panel   */
  1.0f, 0.6f, 0.0f, 1.0f, /* yellow panel */
  1.0f, 0.2f, 0.2f, 1.0f, /* red panel    */
  0.3f, 0.9f, 0.6f, 1.0f, /* green panel  */
};

/* some simple vector utility routines */

void
vcopy(GLfloat a[3], GLfloat b[3])
{
  b[0] = a[0];
  b[1] = a[1];
  b[2] = a[2];
}

void
vnormalize(GLfloat v[3])
{
  float m = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  v[0] /= m;
  v[1] /= m;
  v[2] /= m;
}

void
vadd(GLfloat a[3], GLfloat b[3], GLfloat c[3])
{
  c[0] = a[0] + b[0];
  c[1] = a[1] + b[1];
  c[2] = a[2] + b[2];
}

void
vsub(GLfloat a[3], GLfloat b[3], GLfloat c[3])
{
  c[0] = a[0] - b[0];
  c[1] = a[1] - b[1];
  c[2] = a[2] - b[2];
}

void
vcross(GLfloat a[3], GLfloat b[3], GLfloat c[3])
{
  c[0] = a[1] * b[2] - a[2] * b[1];
  c[1] = -(a[0] * b[2] - a[2] * b[0]);
  c[2] = a[0] * b[1] - a[1] * b[0];
}

float
vdot(GLfloat a[3], GLfloat b[3])
{
  return (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]);
}

void
findNormal(GLfloat pts[][3], GLfloat normal[3]) {
  GLfloat a[3], b[3];
  
  vsub(pts[1], pts[0], a);
  vsub(pts[2], pts[0], b);
  vcross(b, a, normal);
  vnormalize(normal);
}

static GLfloat origin[4] = { 0.f, 0.f, 0.f, 1.f };
static GLfloat black[4] = { 0.f, 0.f, 0.f, 1.f };
static GLfloat ambient[4] = { 0.2f, 0.2f, 0.2f, 1.f };

void
make_shadow_texture(int index, GLfloat eyept[3], GLfloat dx, GLfloat dy)
{
  GLfloat xaxis[3], yaxis[3], zaxis[3];
  GLfloat cov[3]; /* center of view */
  GLfloat pte[3]; /* plane to eye */
  GLfloat eye[3];
  GLfloat tmp[3], normal[3], dist;
  GLfloat (*qpts)[3] = pts[index];
  GLfloat left, right, bottom, top;
  GLfloat znear = 10.f, zfar = 600.f;
  GLint n;

  /* For simplicity, we don't compute the transformation matrix described */
  /* in Heckbert and Herf's paper.  The transformation and frustum used   */
  /* here is much simpler.                                                */

  vcopy(eyept, eye);
  vsub(qpts[1], qpts[0], yaxis);
  vsub(qpts[2], qpts[0], xaxis);
  vcross(yaxis, xaxis, zaxis);

  vnormalize(zaxis);
  vnormalize(xaxis); /* x-axis of eye coord system, in object space */
  vnormalize(yaxis); /* y-axis of eye coord system, in object space */

  /* jitter the eyepoint */
  eye[0] += xaxis[0] * dx;
  eye[1] += xaxis[1] * dx;
  eye[2] += xaxis[2] * dx;
  eye[0] += yaxis[0] * dy;
  eye[1] += yaxis[1] * dy;
  eye[2] += yaxis[2] * dy;

  /* center of view is just eyepoint offset in direction of normal */ 
  vadd(eye, zaxis, cov);

  /* set up viewing matrix */
  glPushMatrix();
  glLoadIdentity();
  gluLookAt(eye[0], eye[1], eye[2],
	    cov[0], cov[1], cov[2],
	    yaxis[0], yaxis[1], yaxis[2]);

  /* compute a frustum that just encloses the polygon */
  vsub(qpts[0], eye, tmp); /* from eye to 0th vertex */
  left = vdot(tmp, xaxis);
  vsub(qpts[2], eye, tmp); /* from eye to 2nd vertex */
  right = vdot(tmp, xaxis);
  vsub(qpts[0], eye, tmp); /* from eye to 0th vertex */
  bottom = vdot(tmp, yaxis);
  vsub(qpts[1], eye, tmp); /* from eye to 1st vertex */
  top = vdot(tmp, yaxis);

  /* scale the frustum values based on the distance to the polygon */
  vsub(qpts[0], eye, pte);
  dist = fabs(vdot(zaxis, pte));
  left *= (znear/dist);
  right *= (znear/dist);
  bottom *= (znear/dist);
  top *= (znear/dist);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glFrustum(left, right, bottom, top, znear, zfar);
  glMatrixMode(GL_MODELVIEW);

  if (radianceMap) {
    glEnable(GL_LIGHTING);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materials[index]);
  } else {
    glDisable(GL_LIGHTING);
  }
  glDisable(GL_TEXTURE_2D);

  for (n=0; n < numShadowTex; n++) {
    qpts = pts[n];

    if (radianceMap) {
      glColor3f(1.f, 1.f, 1.f);
      findNormal(qpts, normal);
      glNormal3fv(normal);
      if (n == index) {
	/* draw this poly with ambient and diffuse lighting */
	glEnable(GL_LIGHT0);
      } else {
	/* draw other polys with ambient lighting only */
	glDisable(GL_LIGHT0);
      }
    } else {
      if (n == index) {
	/* this poly has full intensity, no occlusion */
	glColor3f(1.f, 1.f, 1.f);
      } else {
	/* all other polys just occlude the light */
	glColor3f(0.f, 0.f, 0.f);
      }
    }
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3fv(qpts[0]);
    glVertex3fv(qpts[1]);
    glVertex3fv(qpts[2]);
    glVertex3fv(qpts[3]);
    glEnd();
  }
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void make_all_shadow_textures(float eye[3], float dx, float dy) {
  GLint texPerRow;
  GLint n;
  GLfloat x, y;

  texPerRow = (winxsize / texxsize);
  for (n=0; n < numShadowTex; n++) {
    y = (n / texPerRow) * texysize;
    x = (n % texPerRow) * texxsize;
    glViewport(x, y, texxsize, texysize);
    make_shadow_texture(n, eye, dx, dy);
  }
  glViewport(0, 0, winxsize, winysize);
}

void store_all_shadow_textures(void) {
  GLint texPerRow;
  GLint n, x, y;
  GLubyte *texbuf;
  
  texbuf = (GLubyte *) malloc(texxsize * texysize * sizeof(int));

  /* how many shadow textures can fit in the window */
  texPerRow = (winxsize / texxsize);

  for (n=0; n < numShadowTex; n++) {
    GLenum format;

    x = (n % texPerRow) * texxsize;
    y = (n / texPerRow) * texysize;

    glBindTexture(GL_TEXTURE_2D, shadowTextures + n);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    if (radianceMap) {
      format = GL_RGB;
    } else {
      format = GL_LUMINANCE;
    }
    glCopyTexImage2D(GL_TEXTURE_2D, 0, format, x, y, texxsize, texysize, 0);
  }
  free(texbuf);
}

/* menu choices */
enum {
  NOSHADOWS, SOFTSHADOWS, HARDSHADOWS, VIEWTEXTURE, VIEWSCENE, QUIT
};

GLint shadowMode = HARDSHADOWS;
GLboolean viewTextures = GL_FALSE;

void
redraw(void)
{
  GLint n;
  GLfloat normal[3];
  GLfloat (*qpts)[3];

  glPushMatrix();
  glLoadIdentity();
  if (radianceMap && (shadowMode != NOSHADOWS)) {
    glLightfv(GL_LIGHT0, GL_POSITION, origin);
  } else {
    glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
  }
  glPopMatrix();

  if (shadowMode == SOFTSHADOWS) {
    GLfloat jitterSize;
    GLfloat dx, dy;
    GLint numSteps, i, j;

    /* size of the area to jitter the light in */
    jitterSize = 15.0;

    /* number of times along x and y to jitter */
    numSteps = 5;

    glClear(GL_ACCUM_BUFFER_BIT);
    for (j=0; j < numSteps; j++) {
      for (i=0; i < numSteps; i++) {

	/* compute jitter amount, centering the jitter steps around zero */
	dx = (i - (numSteps - 1.0) / 2.0) / (numSteps - 1.0) * jitterSize;
	dy = (j - (numSteps - 1.0) / 2.0) / (numSteps - 1.0) * jitterSize;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	make_all_shadow_textures(lightpos, dx, dy);
	glAccum(GL_ACCUM, 1.0 / (numSteps * numSteps));
	if (viewTextures) {
	  glutSwapBuffers();
	}
      }
    }
    glAccum(GL_RETURN, 1.0);
    store_all_shadow_textures();

  } else if (shadowMode == HARDSHADOWS) {

    /* make shadow textures from just one frame */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT);
    make_all_shadow_textures(lightpos, 0, 0);
    store_all_shadow_textures();
    if (viewTextures) {
      glutSwapBuffers();
    }
  }
  if (viewTextures) {
    glutSwapBuffers();
    return;
  }

  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glLoadIdentity();

  glColor3f(1.f, 1.f, 1.f);
  if (shadowMode == NOSHADOWS) {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0); 
    glDisable(GL_TEXTURE_2D);
    for (n=0; n < numShadowTex; n++) {
      qpts = pts[n];
      findNormal(qpts, normal);
      glNormal3fv(normal);
      glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materials[n]);
      glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(0,0); glVertex3fv(qpts[0]);
      glTexCoord2f(0,1); glVertex3fv(qpts[1]);
      glTexCoord2f(1,0); glVertex3fv(qpts[2]);
      glTexCoord2f(1,1); glVertex3fv(qpts[3]);
      glEnd();
    }
  } else {
    glEnable(GL_TEXTURE_2D);

    if (radianceMap) {
      glDisable(GL_LIGHTING);
      for (n=0; n < numShadowTex; n++) {
	qpts = pts[n];
	glBindTexture(GL_TEXTURE_2D, shadowTextures + n);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0,0); glVertex3fv(qpts[0]);
	glTexCoord2f(0,1); glVertex3fv(qpts[1]);
	glTexCoord2f(1,0); glVertex3fv(qpts[2]);
	glTexCoord2f(1,1); glVertex3fv(qpts[3]);
	glEnd();
      }
    } else {

      /* Unfortunately, using the texture as an occlusion map requires two */
      /* passes: one in which the occlusion map modulates the diffuse      */
      /* lighting, and one in which the ambient lighting is added in. It's */
      /* incorrect to modulate the ambient lighting, but if the result is  */
      /* acceptable to you, you can include it in the first pass and       */
      /* omit the second pass. */

      /* draw only with diffuse light, modulating it with the texture */
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0); 
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
      for (n=0; n < numShadowTex; n++) {
	qpts = pts[n];
	findNormal(qpts, normal);
	glNormal3fv(normal);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materials[n]);
	glBindTexture(GL_TEXTURE_2D, shadowTextures + n);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0,0); glVertex3fv(qpts[0]);
	glTexCoord2f(0,1); glVertex3fv(qpts[1]);
	glTexCoord2f(1,0); glVertex3fv(qpts[2]);
	glTexCoord2f(1,1); glVertex3fv(qpts[3]);
	glEnd();
      }

      /* add in the ambient lighting */
      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
      glDepthFunc(GL_LEQUAL);
      for (n=0; n < numShadowTex; n++) {
	qpts = pts[n];
	glColor4f(ambient[0] * materials[n][0],
		  ambient[1] * materials[n][1],
		  ambient[2] * materials[n][2],
		  ambient[3] * materials[n][3]);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0,0); glVertex3fv(qpts[0]);
	glTexCoord2f(0,1); glVertex3fv(qpts[1]);
	glTexCoord2f(1,0); glVertex3fv(qpts[2]);
	glTexCoord2f(1,1); glVertex3fv(qpts[3]);
	glEnd();
      }
      /* restore the ambient colors to their defaults */
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    }
  }

  /* blend in the checkerboard floor */
  glEnable(GL_BLEND);
  glBlendFunc(GL_ZERO, GL_SRC_COLOR);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, floorTexture);
  glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, materials[0]);
  glTranslatef(0.0f, 0.05f, 0.0f);
  glColor3f(1.f, 1.f, 1.f);
  glBegin(GL_TRIANGLE_STRIP);
  glNormal3f(0.f, 1.f, 0.f);
  glTexCoord2f(0.f, 0.f); glVertex3fv(pts[0][0]);
  glTexCoord2f(0.f, 1.f); glVertex3fv(pts[0][1]);
  glTexCoord2f(1.f, 0.f); glVertex3fv(pts[0][2]);
  glTexCoord2f(1.f, 1.f); glVertex3fv(pts[0][3]);
  glEnd();

  /* undo some state settings that we did above */
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
  glDepthFunc(GL_LESS);
  glTranslatef(0.0f, -0.05f, 0.0f);

glutSwapBuffers();
}

void
menu(int mode)
{
  switch (mode) {
  case NOSHADOWS:
  case SOFTSHADOWS:
  case HARDSHADOWS:
    shadowMode = mode;
    break;
  case VIEWTEXTURE:
    viewTextures = GL_TRUE;
    break;
  case VIEWSCENE:
    viewTextures = GL_FALSE;
    break;
  case QUIT:
    exit(0);
  }
  glutPostRedisplay();
}

/* Make a checkerboard texture for the floor. */
GLfloat *
make_texture(int maxs, int maxt)
{
  GLint s, t;
  static GLfloat *texture;

  texture = (GLfloat *) malloc(maxs * maxt * sizeof(GLfloat));
  for (t = 0; t < maxt; t++) {
    for (s = 0; s < maxs; s++) {
      texture[s + maxs * t] = ((s >> 4) & 0x1) ^ ((t >> 4) & 0x1);
    }
  }
  return texture;
}

/* ARGSUSED1 */
void
keyboard(unsigned char key, int x, int y)
{
  if (key == 27)  /* ESC */
    exit(0);
}

int
main(int argc, char *argv[])
{
  GLfloat *tex;
  GLint i;

  for (i = 1; i < argc; ++i) {
    if (!strcmp("-o", argv[i])) {
      /* use textures as occlusion maps rather than radiance maps */
      radianceMap = GL_FALSE;
    }
  }

  glutInit(&argc, argv);
  glutInitWindowSize(winxsize, winysize);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_ACCUM | GLUT_SINGLE);
  (void) glutCreateWindow("soft shadows");
  glutDisplayFunc(redraw);
  glutKeyboardFunc(keyboard);

  glutCreateMenu(menu);
  glutAddMenuEntry("No Shadows", NOSHADOWS);
  glutAddMenuEntry("Soft Shadows", SOFTSHADOWS);
  glutAddMenuEntry("Hard Shadows", HARDSHADOWS);
  glutAddMenuEntry("View Textures", VIEWTEXTURE);
  glutAddMenuEntry("View Scene", VIEWSCENE);
  glutAddMenuEntry("Quit", QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  /* set up perspective projection */
  glMatrixMode(GL_PROJECTION);
  glFrustum(-30., 30., -30., 30., 100., 640.);
  glMatrixMode(GL_MODELVIEW);

  /* turn on features */
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glCullFace(GL_BACK);
  glLightfv(GL_LIGHT0, GL_AMBIENT, black);

  /* number of shadow textures to make */
  numShadowTex = sizeof(pts) / sizeof(pts[0]);

  tex = make_texture(texxsize, texysize);
  glBindTexture(GL_TEXTURE_2D, floorTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, 1, texxsize, texysize, 0, GL_RED, GL_FLOAT, 
	       tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  free(tex);

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
