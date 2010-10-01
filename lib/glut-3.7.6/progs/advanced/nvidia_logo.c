
/* Copyright (c) Mark J. Kilgard, 1998.  */

/* This program is freely distributable without licensing fees and is
   provided without guarantee or warrantee expressed or implied.  This
   program is -not- in the public domain. */

#include <math.h>
#include <stdlib.h>
#include <GL/glut.h>

GLfloat nvlogo0[][2] = {
  { -0.474465, -1.259490 },
  { 0.115919, -1.113297 },
  { 0.588227, -0.899634 },
  { 0.942455, -0.652235 },
  { 1.296687, -0.348609 },
  { 1.690275, -0.033738 },
  { 1.926431, 0.269888 },
  { 2.123226, 0.494796 },
  { 1.847713, 0.832160 },
  { 1.532842, 1.124540 },
  { 1.178611, 1.394430 },
  { 0.706303, 1.709300 },
  { 0.076562, 1.967940 },
  { -0.395747, 2.080400 },
  { -1.064784, 2.058940 },
  { -1.064847, 2.811350 },
  { 3.973113, 2.811350 },
  { 3.973113, -2.811350 },
  { -1.025490, -2.811350 },
  { -1.025490, -2.159120 },
  { -0.474465, -2.159120 },
  { 0.155277, -2.102890 },
  { 0.706303, -1.979190 },
  { 1.178611, -1.833000 },
  { 1.690275, -1.653080 },
  { 2.201941, -1.450660 },
  { 2.674248, -1.214510 },
  { 3.107212, -0.955861 },
  { 3.343357, -0.719707 },
  { 2.438097, -0.179928 },
  { 2.005147, -0.517290 },
  { 1.690275, -0.820916 },
  { 1.296687, -1.079560 },
  { 0.863740, -1.338200 },
  { 0.273356, -1.585600 },
  { -0.198952, -1.709300 },
  { -1.025490, -1.731790 },
  { -1.025490, -1.248240 },
};
GLfloat nvlogo1[][2] = {
  { -0.493508, 0.560265 },
  { -0.233835, 0.218981 },
  { -0.078033, -0.107463 },
  { 0.545180, 0.441557 },
  { 0.285509, 0.753164 },
  { -0.129966, 1.005420 },
  { -0.545442, 1.153800 },
  { -1.034999, 1.167860 },
  { -1.064784, 1.658310 },
  { -0.233835, 1.598950 },
  { 0.233576, 1.361540 },
  { 0.649050, 1.094450 },
  { 1.012591, 0.753164 },
  { 1.324197, 0.426719 },
  { 1.064524, 0.189305 },
  { 0.804852, -0.166817 },
  { 0.389378, -0.508100 },
  { -0.078033, -0.745515 },
  { -0.441573, -0.879060 },
  { -1.013530, -0.889070 },
  { -1.012851, 0.723487 },
};
GLfloat nvlogo2[][2] = {
  { -1.025490, -2.159120 },
  { -1.843800, -1.962260 },
  { -2.415081, -1.635820 },
  { -2.934425, -1.205510 },
  { -3.297966, -0.760353 },
  { -3.609571, -0.315201 },
  { -3.869244, 0.204143 },
  { -3.973113, 0.545426 },
  { -3.505702, 0.960900 },
  { -2.830556, 1.435730 },
  { -2.051539, 1.851210 },
  { -1.064784, 2.058940 },
  { -1.064784, 1.658310 },
  { -1.791868, 1.495080 },
  { -2.363145, 1.183480 },
  { -2.830556, 0.842190 },
  { -3.194097, 0.471234 },
  { -3.090228, 0.055759 },
  { -2.830556, -0.315201 },
  { -2.570884, -0.760353 },
  { -2.103473, -1.220340 },
  { -1.584129, -1.531950 },
  { -1.025490, -1.731790 },
};
GLfloat nvlogo3[][2] = {
  { -1.025490, -1.248240 },
  { -1.472016, -1.099371 },
  { -1.794030, -0.875934 },
  { -2.047038, -0.606495 },
  { -2.254046, -0.337056 },
  { -2.415053, -0.047902 },
  { -2.530060, 0.260968 },
  { -2.392054, 0.536978 },
  { -2.047038, 0.806418 },
  { -1.633023, 1.016710 },
  { -1.034999, 1.167860 },
  { -1.012851, 0.723487 },
  { -1.380012, 0.681555 },
  { -1.610022, 0.530407 },
  { -1.863033, 0.326685 },
  { -1.909033, 0.076960 },
  { -1.748027, -0.159620 },
  { -1.541019, -0.448774 },
  { -1.311010, -0.685355 },
  { -1.013530, -0.889070 },
};

#define SIZE(a) (sizeof(a)/sizeof(a[0]))

void
extrudeSolidFromPolygon(GLfloat data[][2], unsigned int dataSize,
  GLdouble thickness, GLuint side, GLuint edge, GLuint whole,
  float breakAngle)
{
  static GLUtriangulatorObj *tobj = NULL;
  GLdouble vertex[3], dx, dy, len;
  GLdouble ndx, ndy, ondx, ondy, dot;
  int i;
  int count = (int) (dataSize / (2 * sizeof(GLfloat)));

  if (tobj == NULL) {
    tobj = gluNewTess();  /* create and initialize a GLU
                             polygon tesselation object */
    gluTessCallback(tobj, GLU_BEGIN, glBegin);
    gluTessCallback(tobj, GLU_VERTEX, glVertex2fv);  /* semi-tricky */
    gluTessCallback(tobj, GLU_END, glEnd);
  }
  glNewList(side, GL_COMPILE);
  gluBeginPolygon(tobj);
  for (i = 0; i < count; i++) {
    vertex[0] = data[i][0];
    vertex[1] = data[i][1];
    vertex[2] = 0;
    gluTessVertex(tobj, vertex, data[i]);
  }
  gluEndPolygon(tobj);
  glEndList();
  glNewList(edge, GL_COMPILE);
  glBegin(GL_QUAD_STRIP);

  dx = data[0][1] - data[-1 % count][1];
  dy = data[-1 % count][0] - data[0][0];
  len = sqrt(dx * dx + dy * dy);
  ondx = dx / len;
  ondy = dy / len;

  for (i = 0; i <= count; i++) {
    /* mod function handles closing the edge */
    /* Calculate a unit normal by dividing by Euclidean
       distance. We could be lazy and use
       glEnable(GL_NORMALIZE) so we could pass in arbitrary
       normals for a very slight performance hit. */
    dx = data[(i + 1) % count][1] - data[i % count][1];
    dy = data[i % count][0] - data[(i +1) % count][0];
    len = sqrt(dx * dx + dy * dy);
    ndx = dx / len;
    ndy = dy / len;

    dot = fabs(acos(ndx * ondx + ndy * ondy) * 180.0/3.14159);

    if (dot > breakAngle) {
      glVertex3f(data[i % count][0], data[i % count][1], thickness);
      glVertex3f(data[i % count][0], data[i % count][1], 0.0);
      glNormal3f(ndx, ndy, 0.0);
    } else {
      GLdouble adx, ady, nadx, nady;

      adx = ndx + ondx;
      ady = ndy + ondy;
      len = sqrt(adx*adx + ady*ady);
      nadx = adx / len;
      nady = ady / len;
      glNormal3f(nadx, nady, 0.0);
    }
    glVertex3f(data[i % count][0], data[i % count][1], thickness);
    glVertex3f(data[i % count][0], data[i % count][1], 0.0);    

    ondx = ndx;
    ondy = ndy;
  }
  glEnd();
  glEndList();
  glNewList(whole, GL_COMPILE);

  glFrontFace(GL_CW);
  glCallList(edge);

  glPushMatrix();
  glTranslatef(0.0, 0.0, thickness);
  glFrontFace(GL_CW);
  glNormal3f(0.0, 0.0, 1.0);  /* opposite normal for other side */
  glCallList(side);
  glPopMatrix();

  glFrontFace(GL_CCW);
  glNormal3f(0.0, 0.0, -1.0);  /* constant normal for side */
  glCallList(side);

  glEndList();
}

GLuint
makeNVidiaLogo(GLuint dlistBase)
{
  const float extrudeWidth = 1.0;
  const float breakAngle = 30.0;

  extrudeSolidFromPolygon(nvlogo0, sizeof(nvlogo0), extrudeWidth,
      dlistBase+1, dlistBase+2, dlistBase+3, breakAngle);
  extrudeSolidFromPolygon(nvlogo1, sizeof(nvlogo1), extrudeWidth,
      dlistBase+4, dlistBase+5, dlistBase+6, breakAngle);
  extrudeSolidFromPolygon(nvlogo2, sizeof(nvlogo2), extrudeWidth,
      dlistBase+7, dlistBase+8, dlistBase+9, breakAngle);
  extrudeSolidFromPolygon(nvlogo3, sizeof(nvlogo3), extrudeWidth,
      dlistBase+10, dlistBase+11, dlistBase+12, breakAngle);
  glNewList(dlistBase, GL_COMPILE);
  glCallList(dlistBase+3);
  glCallList(dlistBase+6);
  glCallList(dlistBase+9);
  glCallList(dlistBase+12);
  glEndList();
  return dlistBase;
}

