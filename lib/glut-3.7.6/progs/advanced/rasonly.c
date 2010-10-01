/*
 * rasonly.c -
 *      Demonstrates the use of OpenGL for rasterization-only, with
 *      perspective-correct texture mapping.
 *
 * Michael I. Gold <gold@sgi.com>
 * Silicon Graphics Computer Systems, May 1997
 *
 * Since current low-end 3D accelerators support only rasterization in
 * hardware, a number of developers have expressed interested in using
 * OpenGL as an interface to rasterization hardware while retaining
 * control of transformations and lighting in the application code.
 * Many OpenGL implementations detect and optimize for identity xforms,
 * so this approach is entirely reasonable.
 *
 * Setting up rasterization-only is fairly straightforward.  The projection
 * matrix is set up as a one-to-one mapping between eye and clip coordinates,
 * and the modelview matrix is set up as identity, e.g. object coordinates
 * map directly to eye coordinates.  This can be achieved as follows:
 *
 *      glMatrixMode(GL_PROJECTION);
 *      glLoadIdentity();
 *      glOrtho(0.0f, (GLfloat) width, 0.0f, (GLfloat) height, -1.0f, 1.0f);
 *      glMatrixMode(GL_MODELVIEW);
 *      glLoadIdentity();
 *      glViewport(0, 0, width, height);
 *
 * where (width, height) represent the window dimensions.
 *
 * Now transformed geometry may be specified directly through the standard
 * interfaces (e.g. glVertex*()).  The only tricky part that remains is
 * specifying texture coordinates such that perspective correction may
 * occur.  The answer is to use glTexCoord4*(), and perform the perspective
 * divide on the texture coordinates directly.
 */

#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

GLboolean motion = GL_TRUE;

/* Matrices */
GLfloat rot = 0.0f;
GLfloat ModelView[16];
GLfloat Projection[16];
GLfloat Viewport[4];

/* Sample geometry */
GLfloat quadV[][4] = {
    { -1.0f, 0.0f, -1.0f, 1.0f },
    {  1.0f, 0.0f, -1.0f, 1.0f },
    {  1.0f, 0.5f, -0.2f, 1.0f },
    { -1.0f, 0.5f, -0.2f, 1.0f },
};

GLfloat quadC[][3] = {
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f },
};

GLfloat quadT[][2] = {
    { 0.0f, 0.0f },
    { 0.0f, 1.0f },
    { 1.0f, 1.0f },
    { 1.0f, 0.0f },
};

/*********************************************************************
 * Utility functions
 */

int texWidth = 128;
int texHeight = 128;

/* Create and download the application texture map */
static void
setCheckedTexture(void)
{
    int texSize;
    void *textureBuf;
    GLubyte *p;
    int i,j;

    /* malloc for rgba as worst case */
    texSize = texWidth*texHeight*4;

    textureBuf = malloc(texSize);
    if (NULL == textureBuf) return;

    p = (GLubyte *)textureBuf;
    for (i=0; i < texWidth; i++) {
        for (j=0; j < texHeight; j++) {
            if ((i ^ j) & 8) {
                p[0] = 0xff; p[1] = 0xff; p[2] = 0xff; p[3] = 0xff;
            } else {
                p[0] = 0x08; p[1] = 0x08; p[2] = 0x08; p[3] = 0xff;
            }
            p += 4;
        }
    }
    gluBuild2DMipmaps(GL_TEXTURE_2D, 4, texWidth, texHeight, 
                 GL_RGBA, GL_UNSIGNED_BYTE, textureBuf);
    free(textureBuf);
}

/* Perform one transform operation */
static void
Transform(GLfloat *matrix, GLfloat *in, GLfloat *out)
{
    int ii;

    for (ii=0; ii<4; ii++) {
        out[ii] = 
            in[0] * matrix[0*4+ii] +
            in[1] * matrix[1*4+ii] +
            in[2] * matrix[2*4+ii] +
            in[3] * matrix[3*4+ii];
    }
}

/* Transform a vertex from object coordinates to window coordinates.
 * Lighting is left as an exercise for the reader.
 */
static void
DoTransform(GLfloat *in, GLfloat *out)
{
    GLfloat tmp[4];
    GLfloat invW;       /* 1/w */

    /* Modelview xform */
    Transform(ModelView, in, tmp);

    /* Lighting calculation goes here! */

    /* Projection xform */
    Transform(Projection, tmp, out);

    if (out[3] == 0.0f) /* do what? */
        return;

    invW = 1.0f / out[3];

    /* Perspective divide */
    out[0] *= invW;
    out[1] *= invW;
    out[2] *= invW;

    /* Map to 0..1 range */
    out[0] = out[0] * 0.5f + 0.5f;
    out[1] = out[1] * 0.5f + 0.5f;
    out[2] = out[2] * 0.5f + 0.5f;

    /* Map to viewport */
    out[0] = out[0] * Viewport[2] + Viewport[0];
    out[1] = out[1] * Viewport[3] + Viewport[1];

    /* Store inverted w for performance */
    out[3] = invW;
}

/*********************************************************************
 * Application code begins here
 */

/* For the sake of brevity, I'm use OpenGL to compute my matrices. */
void UpdateModelView(void)
{
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(0.0f, 1.0f, -4.0f,
              0.0f, 0.0f, 0.0f,
              0.0f, 1.0f, 0.0f);
    glRotatef(rot, 0.0f, 1.0f, 0.0f);
    /* Retrieve the matrix */
    glGetFloatv(GL_MODELVIEW_MATRIX, ModelView);
    glPopMatrix();
}

void InitMatrices(void)
{
    /* Calculate projection matrix */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(45.0f, 1.0f, 1.0f, 100.0f);
    /* Retrieve the matrix */
    glGetFloatv(GL_PROJECTION_MATRIX, Projection);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    UpdateModelView();
}

void Init(void)
{
    glClearColor(0.2f, 0.2f, 0.6f, 1.0f);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    setCheckedTexture();

    InitMatrices();
}

void Redraw(void)
{
    GLfloat tmp[4];
    int ii;

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glBegin(GL_QUADS);

    for (ii = 0; ii < 4; ii++) {

        /* Transform a vertex from object to window coordinates.
         * 1/w is returned as tmp[3] for perspective-correcting
         * the texture coordinates.
         */
        DoTransform(quadV[ii], tmp);

        /* Ideally the colors will be computed by the lighting equation,
         * but I've hard-coded values for this example.
         */
        glColor3fv(quadC[ii]);

        /* Scale by 1/w (stored in tmp[3]) */
        glTexCoord4f(quadT[ii][0] * tmp[3],
                     quadT[ii][1] * tmp[3], 0.0f, tmp[3]);

        /* Note I am using Vertex3, not Vertex4, since we have already
         * performed the perspective divide.
         */
        glVertex3fv(tmp);
    }

    glEnd();

    glutSwapBuffers();
}

void Motion(void)
{
    rot += 3.0f;
    if (rot >= 360.0f) rot -= 360.0f;
    UpdateModelView();
    Redraw();
}

/* ARGSUSED1 */
void Key(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:
        exit(0);
    case 'm':
        motion = !motion;
        glutIdleFunc(motion ? Motion : NULL);
        break;
    }
}

/* ARGSUSED1 */
void Button(int button, int state, int x, int y)
{
    switch (button) {
    case GLUT_LEFT_BUTTON:
        if (state == GLUT_DOWN) {
            rot -= 15.0f;
            UpdateModelView();
            Redraw();
        }
        break;
    case GLUT_RIGHT_BUTTON:
        if (state == GLUT_DOWN) {
            rot += 15.0f;
            UpdateModelView();
            Redraw();
        }
        break;
    }
}

void Reshape(int width, int height)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, (GLfloat) width, 0.0f, (GLfloat) height, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0, 0, width, height);

    Viewport[0] = Viewport[1] = 0.0f;
    Viewport[2] = (GLfloat) width;
    Viewport[3] = (GLfloat) height;
}

int
main(int argc, char *argv[])
{
    char *t;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
    glutInitWindowSize(400, 400);
    glutCreateWindow((t=strrchr(argv[0], '\\')) != NULL ? t+1 : argv[0]);

    Init();

    glutDisplayFunc(Redraw);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Key);
    glutMouseFunc(Button);
    glutIdleFunc(Motion);

    glutMainLoop();

    return 0;
}
