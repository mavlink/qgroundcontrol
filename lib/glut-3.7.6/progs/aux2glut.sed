#
# aux2glut.sed - a sed script for converting AUX code to GLUT
#
# You will still need to do some work, but this is a good start.
#
1i\
/* aux2glut conversion Copyright (c) Mark J. Kilgard, 1994, 1995 */
1i\

s/int main/void main/g
s/auxInitWindow/glutCreateWindow/g
s/AUX_SINGLE/GLUT_SINGLE/g
s/AUX_DOUBLE/GLUT_DOUBLE/g
s/AUX_RGB/GLUT_RGB/g
s/AUX_RGBA/GLUT_RGBA/g
s/AUX_ACCUM/GLUT_ACCUM/g
s/AUX_DEPTH/GLUT_DEPTH/g
s/AUX_STENCIL/GLUT_STENCIL/g
s/AUX_ALPHA/GLUT_ALPHA/g
s/AUX_MOUSEDOWN/GLUT_DOWN/g
s/AUX_MOUSEUP/GLUT_UP/g
s/AUX_LEFTBUTTON/GLUT_LEFT_BUTTON/g
s/AUX_MIDDLEBUTTON/GLUT_MIDDLE_BUTTON/g
s/AUX_RIGHTBUTTON/GLUT_RIGHT_BUTTON/g
s/(.*AUX_EVENTREC.*)/( int x, int y )/g
s/auxReshapeFunc/glutReshapeFunc/g
s/#include \"aux.h\"/#include <GL\/glut.h>/g
s/#include[ ]*\<aux.h\>/#include <GL\/glut.h>/g
s/\(initialize.*$\)/glutInit(\&argc, argv); \1/g
s/auxInitDisplayMode/glutInitDisplayMode/g
s/auxMainLoop(display)/glutDisplayFunc(display); glutMainLoop()/g
s/auxMainLoop[ ]*([ ]*drawScene[ ]*)/glutDisplayFunc(drawScene); glutMainLoop()/g
s/auxAnimation.*$/glutIdleFunc(drawScene);/g
s/auxGetScreenSize.*$/width = glutGet(GLUT_SCREEN_WIDTH); height = glutGet(GLUT_SCREEN_HEIGHT);/g
s/auxGetSize.*$/width = glutGet(GLUT_WINDOW_WIDTH); height = glutGet(GLUT_WINDOW_HEIGHT);/g
s/auxInitPosition(\(.*\),\(.*\),\(.*\),\(.*\));/glutInitWindowPosition(\1,\2); glutInitWindowSize(\3,\4);/g
s/auxSwapBuffers/glutSwapBuffers/g
s/auxWireIcosahedron/glutWireIcosahedron/g
s/auxSolidIcosahedron/glutSolidIcosahedron/g
s/auxSolidTorus/glutSolidTorus/g
s/auxWireTorus/glutWireTorus/g
s/auxSolidCube/glutSolidCube/g
s/auxWireCube/glutWireCube/g
s/auxSolidSphere/glutSolidSphere/g
s/auxWireSphere/glutWireSphere/g
s/auxSolidCone/glutSolidCone/g
s/auxWireCone/glutWireCone/g
s/auxSolidOctahedron/glutSolidOctahedron/g
s/auxWireOctahedron/glutWireOctahedron/g
s/auxSolidTeapot/glutSoliddTeapot/g
s/auxWireTeapot/glutWireTeapot/g
s/auxKeyFunc(.*,/glutKeyboardFunc(/g
s/auxMouseFunc(.*AUX_MOUSELOC,.*NULL,/glutMouseMotion(/g
s/auxMouseFunc/glutMouseFunc/g
s/auxDeleteMouseFunc( .*$/glutMouseMotion( NULL );/g
