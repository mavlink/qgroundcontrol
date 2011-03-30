#ifndef QGCGLUT_H
#define QGCGLUT_H


/* Copyright (c) Mark J. Kilgard, 1994, 1997. */

/**
(c) Copyright 1993, Silicon Graphics, Inc.

ALL RIGHTS RESERVED

Permission to use, copy, modify, and distribute this software
for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that
both the copyright notice and this permission notice appear in
supporting documentation, and that the name of Silicon
Graphics, Inc. not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.

THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU
"AS-IS" AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR
OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  IN NO
EVENT SHALL SILICON GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE
ELSE FOR ANY DIRECT, SPECIAL, INCIDENTAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER,
INCLUDING WITHOUT LIMITATION, LOSS OF PROFIT, LOSS OF USE,
SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD PARTIES, WHETHER OR
NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF THE POSSIBILITY
OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE OR
PERFORMANCE OF THIS SOFTWARE.

US Government Users Restricted Rights

Use, duplication, or disclosure by the Government is subject to
restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
(c)(1)(ii) of the Rights in Technical Data and Computer
Software clause at DFARS 252.227-7013 and/or in similar or
successor clauses in the FAR or the DOD or NASA FAR
Supplement.  Unpublished-- rights reserved under the copyright
laws of the United States.  Contractor/manufacturer is Silicon
Graphics, Inc., 2011 N.  Shoreline Blvd., Mountain View, CA
94039-7311.

OpenGL(TM) is a trademark of Silicon Graphics, Inc.
*/

#include <cmath>
#include "OpenGL/glu.h"

namespace sgi
{

/* Some <math.h> files do not define M_PI... */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static GLUquadricObj *quadObj;

#define QUAD_OBJ_INIT() { if(!quadObj) initQuadObj(); }

static void
initQuadObj(void)
{
    quadObj = gluNewQuadric();
    if (!quadObj)
        printf("out of memory.");
}

/* CENTRY */
void
glutWireSphere(GLdouble radius, GLint slices, GLint stacks)
{
    QUAD_OBJ_INIT();
    gluQuadricDrawStyle(quadObj, GLU_LINE);
    gluQuadricNormals(quadObj, GLU_SMOOTH);
    /* If we ever changed/used the texture or orientation state
       of quadObj, we'd need to change it to the defaults here
       with gluQuadricTexture and/or gluQuadricOrientation. */
    gluSphere(quadObj, radius, slices, stacks);
}

void
glutSolidSphere(GLdouble radius, GLint slices, GLint stacks)
{
    QUAD_OBJ_INIT();
    gluQuadricDrawStyle(quadObj, GLU_FILL);
    gluQuadricNormals(quadObj, GLU_SMOOTH);
    /* If we ever changed/used the texture or orientation state
       of quadObj, we'd need to change it to the defaults here
       with gluQuadricTexture and/or gluQuadricOrientation. */
    gluSphere(quadObj, radius, slices, stacks);
}

void
glutWireCone(GLdouble base, GLdouble height,
             GLint slices, GLint stacks)
{
    QUAD_OBJ_INIT();
    gluQuadricDrawStyle(quadObj, GLU_LINE);
    gluQuadricNormals(quadObj, GLU_SMOOTH);
    /* If we ever changed/used the texture or orientation state
       of quadObj, we'd need to change it to the defaults here
       with gluQuadricTexture and/or gluQuadricOrientation. */
    gluCylinder(quadObj, base, 0.0, height, slices, stacks);
}

void
glutSolidCone(GLdouble base, GLdouble height,
              GLint slices, GLint stacks)
{
    QUAD_OBJ_INIT();
    gluQuadricDrawStyle(quadObj, GLU_FILL);
    gluQuadricNormals(quadObj, GLU_SMOOTH);
    /* If we ever changed/used the texture or orientation state
       of quadObj, we'd need to change it to the defaults here
       with gluQuadricTexture and/or gluQuadricOrientation. */
    gluCylinder(quadObj, base, 0.0, height, slices, stacks);
}

/* ENDCENTRY */

static void
drawBox(GLfloat size, GLenum type)
{
    static GLfloat n[6][3] = {
        {-1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, -1.0, 0.0},
        {0.0, 0.0, 1.0},
        {0.0, 0.0, -1.0}
    };
    static GLint faces[6][4] = {
        {0, 1, 2, 3},
        {3, 2, 6, 7},
        {7, 6, 5, 4},
        {4, 5, 1, 0},
        {5, 6, 2, 1},
        {7, 4, 0, 3}
    };
    GLfloat v[8][3];
    GLint i;

    v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
    v[4][0] = v[5][0] = v[6][0] = v[7][0] = size / 2;
    v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
    v[2][1] = v[3][1] = v[6][1] = v[7][1] = size / 2;
    v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
    v[1][2] = v[2][2] = v[5][2] = v[6][2] = size / 2;

    for (i = 5; i >= 0; i--) {
        glBegin(type);
        glNormal3fv(&n[i][0]);
        glVertex3fv(&v[faces[i][0]][0]);
        glVertex3fv(&v[faces[i][1]][0]);
        glVertex3fv(&v[faces[i][2]][0]);
        glVertex3fv(&v[faces[i][3]][0]);
        glEnd();
    }
}

/* CENTRY */
void
glutWireCube(GLdouble size)
{
    drawBox(size, GL_LINE_LOOP);
}

void
glutSolidCube(GLdouble size)
{
    drawBox(size, GL_QUADS);
}

/* ENDCENTRY */

static void
doughnut(GLfloat r, GLfloat R, GLint nsides, GLint rings)
{
    int i, j;
    GLfloat theta, phi, theta1;
    GLfloat cosTheta, sinTheta;
    GLfloat cosTheta1, sinTheta1;
    GLfloat ringDelta, sideDelta;

    ringDelta = 2.0 * M_PI / rings;
    sideDelta = 2.0 * M_PI / nsides;

    theta = 0.0;
    cosTheta = 1.0;
    sinTheta = 0.0;
    for (i = rings - 1; i >= 0; i--) {
        theta1 = theta + ringDelta;
        cosTheta1 = cos(theta1);
        sinTheta1 = sin(theta1);
        glBegin(GL_QUAD_STRIP);
        phi = 0.0;
        for (j = nsides; j >= 0; j--) {
            GLfloat cosPhi, sinPhi, dist;

            phi += sideDelta;
            cosPhi = cos(phi);
            sinPhi = sin(phi);
            dist = R + r * cosPhi;

            glNormal3f(cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
            glVertex3f(cosTheta1 * dist, -sinTheta1 * dist, r * sinPhi);
            glNormal3f(cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
            glVertex3f(cosTheta * dist, -sinTheta * dist,  r * sinPhi);
        }
        glEnd();
        theta = theta1;
        cosTheta = cosTheta1;
        sinTheta = sinTheta1;
    }
}

/* CENTRY */
void
glutWireTorus(GLdouble innerRadius, GLdouble outerRadius,
              GLint nsides, GLint rings)
{
    glPushAttrib(GL_POLYGON_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    doughnut(innerRadius, outerRadius, nsides, rings);
    glPopAttrib();
}

void
glutSolidTorus(GLdouble innerRadius, GLdouble outerRadius,
               GLint nsides, GLint rings)
{
    doughnut(innerRadius, outerRadius, nsides, rings);
}

/* ENDCENTRY */

static GLfloat dodec[20][3];

static void
initDodecahedron(void)
{
    GLfloat alpha, beta;

    alpha = sqrt(2.0 / (3.0 + sqrt(5.0)));
    beta = 1.0 + sqrt(6.0 / (3.0 + sqrt(5.0)) -
                      2.0 + 2.0 * sqrt(2.0 / (3.0 + sqrt(5.0))));
    /* *INDENT-OFF* */
    dodec[0][0] = -alpha;
    dodec[0][1] = 0;
    dodec[0][2] = beta;
    dodec[1][0] = alpha;
    dodec[1][1] = 0;
    dodec[1][2] = beta;
    dodec[2][0] = -1;
    dodec[2][1] = -1;
    dodec[2][2] = -1;
    dodec[3][0] = -1;
    dodec[3][1] = -1;
    dodec[3][2] = 1;
    dodec[4][0] = -1;
    dodec[4][1] = 1;
    dodec[4][2] = -1;
    dodec[5][0] = -1;
    dodec[5][1] = 1;
    dodec[5][2] = 1;
    dodec[6][0] = 1;
    dodec[6][1] = -1;
    dodec[6][2] = -1;
    dodec[7][0] = 1;
    dodec[7][1] = -1;
    dodec[7][2] = 1;
    dodec[8][0] = 1;
    dodec[8][1] = 1;
    dodec[8][2] = -1;
    dodec[9][0] = 1;
    dodec[9][1] = 1;
    dodec[9][2] = 1;
    dodec[10][0] = beta;
    dodec[10][1] = alpha;
    dodec[10][2] = 0;
    dodec[11][0] = beta;
    dodec[11][1] = -alpha;
    dodec[11][2] = 0;
    dodec[12][0] = -beta;
    dodec[12][1] = alpha;
    dodec[12][2] = 0;
    dodec[13][0] = -beta;
    dodec[13][1] = -alpha;
    dodec[13][2] = 0;
    dodec[14][0] = -alpha;
    dodec[14][1] = 0;
    dodec[14][2] = -beta;
    dodec[15][0] = alpha;
    dodec[15][1] = 0;
    dodec[15][2] = -beta;
    dodec[16][0] = 0;
    dodec[16][1] = beta;
    dodec[16][2] = alpha;
    dodec[17][0] = 0;
    dodec[17][1] = beta;
    dodec[17][2] = -alpha;
    dodec[18][0] = 0;
    dodec[18][1] = -beta;
    dodec[18][2] = alpha;
    dodec[19][0] = 0;
    dodec[19][1] = -beta;
    dodec[19][2] = -alpha;
    /* *INDENT-ON* */

}

#define DIFF3(_a,_b,_c) { \
    (_c)[0] = (_a)[0] - (_b)[0]; \
    (_c)[1] = (_a)[1] - (_b)[1]; \
    (_c)[2] = (_a)[2] - (_b)[2]; \
}

static void
crossprod(GLfloat v1[3], GLfloat v2[3], GLfloat prod[3])
{
    GLfloat p[3];         /* in case prod == v1 or v2 */

    p[0] = v1[1] * v2[2] - v2[1] * v1[2];
    p[1] = v1[2] * v2[0] - v2[2] * v1[0];
    p[2] = v1[0] * v2[1] - v2[0] * v1[1];
    prod[0] = p[0];
    prod[1] = p[1];
    prod[2] = p[2];
}

static void
normalize(GLfloat v[3])
{
    GLfloat d;

    d = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (d == 0.0) {
        printf("normalize: zero length vector");
        v[0] = d = 1.0;
    }
    d = 1 / d;
    v[0] *= d;
    v[1] *= d;
    v[2] *= d;
}

static void
pentagon(int a, int b, int c, int d, int e, GLenum shadeType)
{
    GLfloat n0[3], d1[3], d2[3];

    DIFF3(dodec[a], dodec[b], d1);
    DIFF3(dodec[b], dodec[c], d2);
    crossprod(d1, d2, n0);
    normalize(n0);

    glBegin(shadeType);
    glNormal3fv(n0);
    glVertex3fv(&dodec[a][0]);
    glVertex3fv(&dodec[b][0]);
    glVertex3fv(&dodec[c][0]);
    glVertex3fv(&dodec[d][0]);
    glVertex3fv(&dodec[e][0]);
    glEnd();
}

static void
dodecahedron(GLenum type)
{
    static int inited = 0;

    if (inited == 0) {
        inited = 1;
        initDodecahedron();
    }
    pentagon(0, 1, 9, 16, 5, type);
    pentagon(1, 0, 3, 18, 7, type);
    pentagon(1, 7, 11, 10, 9, type);
    pentagon(11, 7, 18, 19, 6, type);
    pentagon(8, 17, 16, 9, 10, type);
    pentagon(2, 14, 15, 6, 19, type);
    pentagon(2, 13, 12, 4, 14, type);
    pentagon(2, 19, 18, 3, 13, type);
    pentagon(3, 0, 5, 12, 13, type);
    pentagon(6, 15, 8, 10, 11, type);
    pentagon(4, 17, 8, 15, 14, type);
    pentagon(4, 12, 5, 16, 17, type);
}

/* CENTRY */
void
glutWireDodecahedron(void)
{
    dodecahedron(GL_LINE_LOOP);
}

void
glutSolidDodecahedron(void)
{
    dodecahedron(GL_TRIANGLE_FAN);
}

/* ENDCENTRY */

static void
recorditem(GLfloat * n1, GLfloat * n2, GLfloat * n3,
           GLenum shadeType)
{
    GLfloat q0[3], q1[3];

    DIFF3(n1, n2, q0);
    DIFF3(n2, n3, q1);
    crossprod(q0, q1, q1);
    normalize(q1);

    glBegin(shadeType);
    glNormal3fv(q1);
    glVertex3fv(n1);
    glVertex3fv(n2);
    glVertex3fv(n3);
    glEnd();
}

static void
subdivide(GLfloat * v0, GLfloat * v1, GLfloat * v2,
          GLenum shadeType)
{
    int depth;
    GLfloat w0[3], w1[3], w2[3];
    GLfloat l;
    int i, j, k, n;

    depth = 1;
    for (i = 0; i < depth; i++) {
        for (j = 0; i + j < depth; j++) {
            k = depth - i - j;
            for (n = 0; n < 3; n++) {
                w0[n] = (i * v0[n] + j * v1[n] + k * v2[n]) / depth;
                w1[n] = ((i + 1) * v0[n] + j * v1[n] + (k - 1) * v2[n])
                        / depth;
                w2[n] = (i * v0[n] + (j + 1) * v1[n] + (k - 1) * v2[n])
                        / depth;
            }
            l = sqrt(w0[0] * w0[0] + w0[1] * w0[1] + w0[2] * w0[2]);
            w0[0] /= l;
            w0[1] /= l;
            w0[2] /= l;
            l = sqrt(w1[0] * w1[0] + w1[1] * w1[1] + w1[2] * w1[2]);
            w1[0] /= l;
            w1[1] /= l;
            w1[2] /= l;
            l = sqrt(w2[0] * w2[0] + w2[1] * w2[1] + w2[2] * w2[2]);
            w2[0] /= l;
            w2[1] /= l;
            w2[2] /= l;
            recorditem(w1, w0, w2, shadeType);
        }
    }
}

static void
drawtriangle(int i, GLfloat data[][3], int ndx[][3],
             GLenum shadeType)
{
    GLfloat *x0, *x1, *x2;

    x0 = data[ndx[i][0]];
    x1 = data[ndx[i][1]];
    x2 = data[ndx[i][2]];
    subdivide(x0, x1, x2, shadeType);
}

/* octahedron data: The octahedron produced is centered at the
   origin and has radius 1.0 */
static GLfloat odata[6][3] = {
    {1.0, 0.0, 0.0},
    {-1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, -1.0, 0.0},
    {0.0, 0.0, 1.0},
    {0.0, 0.0, -1.0}
};

static int ondex[8][3] = {
    {0, 4, 2},
    {1, 2, 4},
    {0, 3, 4},
    {1, 4, 3},
    {0, 2, 5},
    {1, 5, 2},
    {0, 5, 3},
    {1, 3, 5}
};

static void
octahedron(GLenum shadeType)
{
    int i;

    for (i = 7; i >= 0; i--) {
        drawtriangle(i, odata, ondex, shadeType);
    }
}

/* CENTRY */
void
glutWireOctahedron(void)
{
    octahedron(GL_LINE_LOOP);
}

void
glutSolidOctahedron(void)
{
    octahedron(GL_TRIANGLES);
}

/* ENDCENTRY */

/* icosahedron data: These numbers are rigged to make an
   icosahedron of radius 1.0 */

#define X .525731112119133606
#define Z .850650808352039932

static GLfloat idata[12][3] = {
    {-X, 0, Z},
    {X, 0, Z},
    {-X, 0, -Z},
    {X, 0, -Z},
    {0, Z, X},
    {0, Z, -X},
    {0, -Z, X},
    {0, -Z, -X},
    {Z, X, 0},
    {-Z, X, 0},
    {Z, -X, 0},
    {-Z, -X, 0}
};

static int indextetra[20][3] = {
    {0, 4, 1},
    {0, 9, 4},
    {9, 5, 4},
    {4, 5, 8},
    {4, 8, 1},
    {8, 10, 1},
    {8, 3, 10},
    {5, 3, 8},
    {5, 2, 3},
    {2, 7, 3},
    {7, 10, 3},
    {7, 6, 10},
    {7, 11, 6},
    {11, 0, 6},
    {0, 1, 6},
    {6, 1, 10},
    {9, 0, 11},
    {9, 11, 2},
    {9, 2, 5},
    {7, 2, 11},
};

static void
icosahedron(GLenum shadeType)
{
    int i;

    for (i = 19; i >= 0; i--) {
        drawtriangle(i, idata, indextetra, shadeType);
    }
}

/* CENTRY */
void
glutWireIcosahedron(void)
{
    icosahedron(GL_LINE_LOOP);
}

void
glutSolidIcosahedron(void)
{
    icosahedron(GL_TRIANGLES);
}

/* ENDCENTRY */

/* tetrahedron data: */

#define T       1.73205080756887729

static GLfloat tdata[4][3] = {
    {T, T, T},
    {T, -T, -T},
    {-T, T, -T},
    {-T, -T, T}
};

static int tndex[4][3] = {
    {0, 1, 3},
    {2, 1, 0},
    {3, 2, 0},
    {1, 2, 3}
};

static void
tetrahedron(GLenum shadeType)
{
    int i;

    for (i = 3; i >= 0; i--)
        drawtriangle(i, tdata, tndex, shadeType);
}

/* CENTRY */
void
glutWireTetrahedron(void)
{
    tetrahedron(GL_LINE_LOOP);
}

void
glutSolidTetrahedron(void)
{
    tetrahedron(GL_TRIANGLES);
}

/* ENDCENTRY */
}

#endif // QGCGLUT_H
