
/* tess.c - by David Blythe, SGI */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

static GLfloat spin = 0;
static int level = 4;
static int model = 0;
static GLfloat rotx, roty;
static int ox = -1, oy = -1;
static int mot;
#define PAN     1
#define ROT     2

void
movelight(int x, int y) {
    spin += (y-oy);
    ox = x; oy = y;
    if (spin > 360.) spin -= 360.;
    if (spin < -360.) spin -= -360.;
    glutPostRedisplay();
}

void
rotate(int x, int y) {
    rotx += x-ox;
    if (rotx > 360.) rotx -= 360.;
    else if (rotx < -360.) rotx += 360.;
    roty += y-oy;
    if (roty > 360.) roty -= 360.;
    else if (roty < -360.) roty += 360.;
    ox = x; oy = y;
    glutPostRedisplay();
}

void
motion(int x, int y) {
    if (mot == PAN) movelight(x, y);
    else if (mot == ROT) rotate(x,y);
}

void
mouse(int button, int state, int x, int y) {
    if(state == GLUT_DOWN) {
        switch(button) {
        case GLUT_LEFT_BUTTON:
            mot = PAN;
            motion(ox = x, oy = y);
            break;
        case GLUT_MIDDLE_BUTTON:
            mot = ROT;
            motion(ox = x, oy = y);
            break;
        case GLUT_RIGHT_BUTTON:
            break;
        }
    } else if (state == GLUT_UP) {
        mot = 0;
    }
}

void togglewire(void) {
    static int toggle = 0;
    toggle ^= 1;
    glPolygonMode(GL_FRONT_AND_BACK, toggle ? GL_LINE : GL_FILL);
}

void genmodel(void) {
    extern void sphere(int level);

    glNewList(1, GL_COMPILE);
    if (model) {
        GLUquadricObj *q = gluNewQuadric();

        gluSphere(q, 1.0, 10*level, 10*level);
        gluDeleteQuadric(q);
    } else {
        sphere(level-1);
    }
    glEndList();
}

void togglemodel(void) {
    model ^= 1;
    genmodel();
}

void levelup(void) {
    level += 1;
    if (level > 7) level = 7;
    genmodel();
}

void leveldown(void) {
    level -= 1;
    if (level <= 0) level = 1;
    genmodel();
}

void help(void) {
    printf("'h'      - help\n");
    printf("'t'      - tessellation style\n");
    printf("'UP'     - increase tessellation\n");
    printf("'DOWN'   - decrease tessellation\n");
    printf("left mouse     - rotate sphere\n");
    printf("middle mouse   - move light\n");
}

void init(void) {
    GLfloat specular[4] = { 1., 1., 1., 1. };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    genmodel();
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 30);
}

void display(void) {
    GLfloat position[] = { 0.0, 0.0, 3.5, 1.0 };

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    glTranslatef(0.0, 0.0, -5.0); 

    glPushMatrix();
    glRotatef(spin, 1.0, 0.0, 0.0);
    glRotatef(0.0, 1.0, 0.0, 0.0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glPopMatrix();

    glRotatef(rotx, 0., 1., 0.);
    glRotatef(roty, 1., 0., 0.);
    glCallList(1);
    glPopMatrix();
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(40.0, (GLfloat) w/(GLfloat) h, 1.0, 20.0);
    glMatrixMode(GL_MODELVIEW);
}

/* ARGSUSED1 */
void
key(unsigned char key, int x, int y) {
    switch(key) {
    case 't': togglemodel(); break;
    case 'w': togglewire(); break;
    case 'h': help(); break;
    case '\033': exit(0);
    default: break;
    }
    glutPostRedisplay();
}

/* ARGSUSED1 */
void
special(int key, int x, int y) {
    switch(key) {
    case GLUT_KEY_UP:   levelup(); break;
    case GLUT_KEY_DOWN: leveldown(); break;
    }
    glutPostRedisplay();
}

void
menu(int value)
{
    if(value<0)
      special(-value,0,0);
    else
       key((unsigned char) value,0,0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(512, 512);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH);
    (void)glutCreateWindow("Quality of sphere tesselation");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutCreateMenu(menu);
    glutAddMenuEntry("Toggle sphere model", 't');
    glutAddMenuEntry("Toggle solid/wireframe", 'w');
    glutAddMenuEntry("Increase tessellation", -GLUT_KEY_UP);
    glutAddMenuEntry("Decrease tessellation", -GLUT_KEY_DOWN);
    glutAddMenuEntry("Print help message", 'h');
    glutAddMenuEntry("Quit", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutMainLoop();
    return 0;             /* ANSI C requires main to return int. */
}
