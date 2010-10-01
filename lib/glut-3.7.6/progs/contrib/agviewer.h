/*
 * agviewer.h  (version 1.0)
 *
 * AGV: a glut viewer. Routines for viewing a 3d scene w/ glut
 *
 * The two view movement modes are POLAR and FLYING.  Both move the eye, NOT
 * THE OBJECT.  You can never be upside down or twisted (roll) in either mode.
 *
 * A nice addition would be an examiner type trackball mode where you are
 * moving the object and so could see it from any angle.  Also less restricted
 * flying and polar modes (fly upside down, do rolls, etc.).
 *
 * Controls for Polar are just left and middle buttons -- for flying it's
 * those plus 0-9 number keys and +/- for speed adjustment.
 *
 * See agv_example.c and agviewer.c for more info.  Probably want to make
 * a copy of these and then edit for each program.  This isn't meant to be
 * a library, just something to graft onto your own programs.
 *
 * I welcome any feedback or improved versions.
 *
 * Philip Winston - 4/11/95
 * pwinston@hmc.edu
 * http://www.cs.hmc.edu/people/pwinston
 */


 /*
  * Call agvInit() with glut's current window set to the window in 
  * which you want to run the viewer. Right after creating it is fine.  It
  * will remember that window for possible later use (see below) and
  * registers mouse, motion, and keyboard handlers for that window (see below).
  *
  * allowidle is 1 or 0 depnding on whether you will let AGV install
  * and uninstall an idle function.  0 means you will not let it (because
  * you will be having your own idle function). In this case it is your
  * responsibility to put a statement like:
  *
  *     if (agvMoving)
  *       agvMove();
  *
  * at the end of your idle function, to let AGV update the viewpoint if it
  * is moving. 
  *
  * If allowidle is 1 it means AGV will install its own idle which
  * will update the viewpoint as needed and send glutPostRedisplay() to the
  * window which was current when agvInit() was called.
  *
  * agvSetIdleAllow changes this value so you can let AGV install its idle
  * when your idle isn't installed. 
  *
  */
void agvInit(int allowidle);
void agvSetAllowIdle(int allowidle);


 /*
  * Set which movement mode you are in.
  */
typedef enum { FLYING, POLAR } MovementType;
void agvSwitchMoveMode(int move);

 /*
  * agvViewTransform basically does the appropriate gluLookAt() for the 
  * current position.  So call it in your display on the projection matrix
  */
void agvViewTransform(void);

 /*
  * agvMoving will be set by AGV according to whether it needs you to call
  * agvMove() at the end of your idle function.  You only need these if 
  * you aren't allowing AGV to do its own idle.
  * (Don't change the value of agvMoving)
  */
extern int agvMoving;
void agvMove(void);

 /*
  * These are the routines AGV registers to deal with mouse and keyboard input.
  * Keyboard input only matters in flying mode, and then only to set speed.
  * Mouse input only uses left two buttons in both modes.
  * These are all registered with agvInit(), but you could register
  * something else which called these, or reregister these as needed 
  */
void agvHandleButton(int button, int state, int x, int y);
void agvHandleMotion(int x, int y);
void agvHandleKeys(unsigned char key, int x, int y);

 /*
  * Just an extra routine which makes an x-y-z axes (about 10x10x10)
  * which is nice for aligning things and debugging.  Pass it an available
  * displaylist number.
  */
void agvMakeAxesList(int displaylist);










