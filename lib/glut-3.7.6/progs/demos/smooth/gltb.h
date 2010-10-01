/* 
 *  Simple trackball-like motion adapted (ripped off) from projtex.c
 *  (written by David Yu and David Blythe).  See the SIGGRAPH '96
 *  Advanced OpenGL course notes.
 *
 *
 *  Usage:
 *  
 *  o  call gltbInit() in before any other gltb call
 *  o  call gltbReshape() from the reshape callback
 *  o  call gltbMatrix() to get the trackball matrix rotation
 *  o  call gltbStartMotion() to begin trackball movememt
 *  o  call gltbStopMotion() to stop trackball movememt
 *  o  call gltbMotion() from the motion callback
 *  o  call gltbAnimate(GL_TRUE) if you want the trackball to continue 
 *     spinning after the mouse button has been released
 *  o  call gltbAnimate(GL_FALSE) if you want the trackball to stop 
 *     spinning after the mouse button has been released
 *
 *  Typical setup:
 *
 *
    void
    init(void)
    {
      gltbInit(GLUT_MIDDLE_BUTTON);
      gltbAnimate(GL_TRUE);
      . . .
    }

    void
    reshape(int width, int height)
    {
      gltbReshape(width, height);
      . . .
    }

    void
    display(void)
    {
      glPushMatrix();

      gltbMatrix();
      . . . draw the scene . . .

      glPopMatrix();
    }

    void
    mouse(int button, int state, int x, int y)
    {
      gltbMouse(button, state, x, y);
      . . .
    }

    void
    motion(int x, int y)
    {
      gltbMotion(x, y);
      . . .
    }

    int
    main(int argc, char** argv)
    {
      . . .
      init();
      glutReshapeFunc(reshape);
      glutDisplayFunc(display);
      glutMouseFunc(mouse);
      glutMotionFunc(motion);
      . . .
    }
 *
 * */


/* functions */
void
gltbInit(GLuint button);

void
gltbMatrix(void);

void
gltbReshape(int width, int height);

void
gltbMouse(int button, int state, int x, int y);

void
gltbMotion(int x, int y);

void
gltbAnimate(GLboolean animate);
