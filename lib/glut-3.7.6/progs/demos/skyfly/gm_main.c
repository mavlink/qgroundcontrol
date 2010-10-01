
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#include "skyfly.h"

void init_misc(void);
void init_skyfly(void);
void sim_singlechannel(void);
void cull_proc(void);
void draw_proc(void);

#define SKY_R 0.23f
#define SKY_G 0.35f
#define SKY_B 0.78f

#define TERR_DARK_R 0.27f
#define TERR_DARK_G 0.18f
#define TERR_DARK_B 0.00f

#define TERR_LITE_R 0.24f
#define TERR_LITE_G 0.53f
#define TERR_LITE_B 0.05f

typedef struct {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
} palette_t;

palette_t       pal[256];
static int buttons[BUTCOUNT] = { 0 };
static int mouse_x, mouse_y;

GLboolean show_timer = GL_FALSE;
GLboolean fullscreen = GL_FALSE;

int Xgetbutton(int button)
{
        int b;
        if (button < 0 || button >= BUTCOUNT)
                return -1;
        b = buttons[button];
        if (button < LEFTMOUSE)
                buttons[button] = 0;
        return b;
}

int Xgetvaluator(int val)
{
    switch (val) {
                case MOUSEX:
                        return mouse_x;
                case MOUSEY:
                        return mouse_y;
                default:
                        return -1;
        }
}

void setPaletteIndex(int i, GLfloat r, GLfloat g, GLfloat b)
{
	pal[i].red = (255.0F * r);
	pal[i].green = (255.0F * g);
	pal[i].blue = (255.0F * b);      
}                                                           

void init_cmap(void)
{
	int 		ii, jj, color;
    GLfloat 	r0, g0, b0, r1, g1, b1;

    /* Set up color map */
	color = 10;
	memset(pal,0,sizeof(pal));

    /* Sky colors */
    sky_base = color;
    r0 = SKY_R; r1 = 1.0f;
    g0 = SKY_G; g1 = 1.0f;
    b0 = SKY_B; b1 = 1.0f;
    for (ii = 0; ii < SKY_COLORS; ii++) {
        GLfloat p, r, g, b;
        p = (GLfloat) ii / (SKY_COLORS-1);
        r = r0 + p * (r1 - r0);
        g = g0 + p * (g1 - g0);
        b = b0 + p * (b1 - b0);
        for (jj = 0; jj < FOG_LEVELS; jj++) {
            GLfloat fp, fr, fg, fb;
            fp = (FOG_LEVELS > 1) ? (GLfloat) jj / (FOG_LEVELS-1) : 0.0f;
            fr = r + fp * (fog_params[0] - r);
            fg = g + fp * (fog_params[1] - g);
            fb = b + fp * (fog_params[2] - b);
            setPaletteIndex(sky_base + (ii*FOG_LEVELS) + jj, fr, fg, fb);
        }
    }
    color += (SKY_COLORS * FOG_LEVELS);

    /* Terrain colors */
    terr_base = color;
    r0 = TERR_DARK_R;   r1 = TERR_LITE_R;
    g0 = TERR_DARK_G;   g1 = TERR_LITE_G;
    b0 = TERR_DARK_B;   b1 = TERR_LITE_B;
    for (ii = 0; ii < TERR_COLORS; ii++) {
        GLfloat p, r, g, b;
        p = (GLfloat) ii / (TERR_COLORS-1);
        r = r0 + p * (r1 - r0);
        g = g0 + p * (g1 - g0);
        b = b0 + p * (b1 - b0);
        for (jj = 0; jj < FOG_LEVELS; jj++) {
            GLfloat fp, fr, fg, fb;
            fp = (FOG_LEVELS > 1) ? (GLfloat) jj / (FOG_LEVELS-1) : 0.0f;
            fr = r + fp * (fog_params[0] - r);
            fg = g + fp * (fog_params[1] - g);
            fb = b + fp * (fog_params[2] - b);
            setPaletteIndex(terr_base + (ii*FOG_LEVELS) + jj, fr, fg, fb);
        }
	}
    color += (TERR_COLORS * FOG_LEVELS);

    /* Plane colors */
    plane_colors[0] = color;
    plane_colors[1] = color + (PLANE_COLORS/2);
    plane_colors[2] = color + (PLANE_COLORS-1);
    r0 = 0.4; r1 = 0.8;
    g0 = 0.4; g1 = 0.8;
    b0 = 0.1; b1 = 0.1;
    for (ii = 0; ii < PLANE_COLORS; ii++) {
        GLfloat p, r, g, b;
        p = (GLfloat) ii / (PLANE_COLORS);
        r = r0 + p * (r1 - r0);
        g = g0 + p * (g1 - g0);
        b = b0 + p * (b1 - b0);
        setPaletteIndex(plane_colors[0] + ii, r, g, b);
    }
	color += PLANE_COLORS;
#if 0
	GM_setPalette(pal,256,0);
	GM_realizePalette(256,0,true);
#endif
}

void draw(void);

void gameLogic(void)
{
	sim_singlechannel();
        draw();
}

/* When not visible, stop animating.  Restart when visible again. */
static void 
visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    glutIdleFunc(gameLogic);
  } else {
    glutIdleFunc(NULL);
  }
}

int                 lastCount;      /* Timer count for last fps update */
int                 frameCount;     /* Number of frames for timing */
int                 fpsRate;        /* Current frames per second rate */

void draw(void)
{
    int   newCount;
    char    buf[20];
    int   i, len;

    /* Draw the frame */
    cull_proc();
    draw_proc();

    /* Update the frames per second count if we have gone past at least
       a quarter of a second since the last update. */
    newCount = glutGet(GLUT_ELAPSED_TIME);
    frameCount++;
    if ((newCount - lastCount) > 1000) {
        fpsRate = (int) ((10000.0f / (newCount - lastCount)) * frameCount);
        lastCount = newCount;
        frameCount = 0;
    }
    if (show_timer) {
        sprintf(buf,"%3d.%d fps", fpsRate / 10, fpsRate % 10);
        glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_FOG);
        glDisable(GL_BLEND);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, Wxsize, 0, Wysize, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glColor3f(1.0f, 1.0f, 0.0f);
        glRasterPos2i(10, 10);
        len = strlen(buf);
        for (i = 0; i < len; i++)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, buf[i]);

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glPopAttrib();
    }

    glutSwapBuffers();
}

void
reshape(int width, int height)
{
    Wxsize = width;
    Wysize = height;

    mouse_x = Wxsize/2;
    mouse_y = Wysize/2;

    glViewport(0, 0, width, height);
}

/* ARGSUSED1 */
void
keyboard(unsigned char c, int x, int y)
{
    switch (c) {
    case 0x1B:
        exit(0);
        break;
    case 'r':
        buttons[RKEY] = 1;
        break;
    case '.':
        buttons[PERIODKEY] = 1;
        break;
    case ' ':
        buttons[SPACEKEY] = 1;
        break;
    case 'f':
        set_fog(!fog);
        break;
    case 'd':
        set_dither(!dither);
        break;
    case 't':
        show_timer = !show_timer;
        break;
    }
}

void mouse(int button, int state, int x, int y)
{
    mouse_x = x;
    mouse_y = y;
    switch (button) {
    case GLUT_LEFT_BUTTON:
        buttons[LEFTMOUSE] = (state == GLUT_DOWN);
        break;
    case GLUT_MIDDLE_BUTTON:
        buttons[MIDDLEMOUSE] = (state == GLUT_DOWN);
        break;
    case GLUT_RIGHT_BUTTON:
        buttons[RIGHTMOUSE] = (state == GLUT_DOWN);
        break;
    }
}

void motion(int x, int y)
{
    mouse_x = x;
    mouse_y = y;
}


/* ARGSUSED1 */
void special(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_LEFT:
        buttons[LEFTARROWKEY] = 1;
        break;
    case GLUT_KEY_UP:
        buttons[UPARROWKEY] = 1;
        break;
    case GLUT_KEY_RIGHT:
        buttons[RIGHTARROWKEY] = 1;
        break;
    case GLUT_KEY_DOWN:
        buttons[DOWNARROWKEY] = 1;
        break;
    case GLUT_KEY_PAGE_UP:
        buttons[PAGEUPKEY] = 1;
        break;
    case GLUT_KEY_PAGE_DOWN:
        buttons[PAGEDOWNKEY] = 1;
        break;
    }
}

int
main(int argc, char **argv)
{
  glutInit(&argc, argv);

  if (argc > 1 && !strcmp(argv[1], "-f"))
      fullscreen = GL_TRUE;

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
  if (fullscreen) {
      glutGameModeString("640x480:16@60");
      glutEnterGameMode();
  } else {
      glutInitWindowSize(400, 400);
      glutCreateWindow("GLUT-based OpenGL skyfly");
  }
  glutDisplayFunc(draw);
  glutReshapeFunc(reshape);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutPassiveMotionFunc(motion);
  glutSpecialFunc(special);

  init_misc();
  if (!rgbmode)
    init_cmap();
  init_skyfly();

  glutMainLoop();
  return 0;             /* ANSI C requires main to return int. */
}
