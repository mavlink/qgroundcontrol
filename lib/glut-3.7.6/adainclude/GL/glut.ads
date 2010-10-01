--  Generated from glut.h
--  Date: Sun Apr  6 14:32:02 1997
--
--  Command line definitions:
--      -D__ANSI_C__ -D_LANGUAGE_C -DGENERATING_ADA_BINDING -D__unix -D__sgi
--      -D__mips -D__host_mips -D__EXTENSIONS__ -D__EDG -D__DSO__ -D__STDC__
--      -D_SYSTYPE_SVR4 -D_MODERN_C -D_MIPS_SZPTR=32 -D_MIPS_SZLONG=32
--      -D_MIPS_SZINT=32 -D_MIPS_SIM=_MIPS_SIM_ABI32
--      -D_MIPS_ISA=_MIPS_ISA_MIPS1 -D_MIPS_FPSET=16 -D_MIPSEB
--

with Interfaces.C;
with Interfaces.C.Extensions;
with Interfaces.C.Strings;
with GL;
package Glut is

   --  Copyright (c) Mark J. Kilgard, 1994, 1995, 1996.
   --  This program is freely distributable without licensing fees  and is
   --  provided without guarantee or warrantee expressed or  implied. This
   --  program is -not- in the public domain. *
   --  **
   --  GLUT API revision history:
   --  
   --  GLUT_API_VERSION is updated to reflect incompatible GLUT
   --  API changes (interface changes, semantic changes, deletions,
   --  or additions).
   --  
   --  GLUT_API_VERSION=1  First public release of GLUT.  11/29/94
   --  
   --  GLUT_API_VERSION=2  Added support for OpenGL/GLX multisampling,
   --  extension.  Supports new input devices like tablet, dial and button
   --  box, and Spaceball.  Easy to query OpenGL extensions.
   --  
   --  GLUT_API_VERSION=3  glutMenuStatus added.
   --  
   --  *

   GLUT_API_VERSION : constant := 4;  --  VERSION 4 API NOT FINALIZED

--  **
--  GLUT implementation revision history:
--  
--  GLUT_XLIB_IMPLEMENTATION is updated to reflect both GLUT
--  API revisions and implementation revisions (ie, bug fixes).
--  
--  GLUT_XLIB_IMPLEMENTATION=1  mjk's first public release of
--  GLUT Xlib-based implementation.  11/29/94
--  
--  GLUT_XLIB_IMPLEMENTATION=2  mjk's second public release of
--  GLUT Xlib-based implementation providing GLUT version 2
--  interfaces.
--  
--  GLUT_XLIB_IMPLEMENTATION=3  mjk's GLUT 2.2 images. 4/17/95
--  
--  GLUT_XLIB_IMPLEMENTATION=4  mjk's GLUT 2.3 images. 6/?/95
--  
--  GLUT_XLIB_IMPLEMENTATION=5  mjk's GLUT 3.0 images. 10/?/95
--  
--  GLUT_XLIB_IMPLEMENTATION=7  mjk's GLUT 3.1+ with glutWarpPoitner.  7/24/96
--  
--  GLUT_XLIB_IMPLEMENTATION=8  mjk's GLUT 3.1+ with glutWarpPoitner
--  and video resize.  1/3/97
--  *

   GLUT_XLIB_IMPLEMENTATION : constant := 7;

   --  Display mode bit masks.

   GLUT_RGB                 : constant := 0;
   GLUT_RGBA                : constant := 0;
   GLUT_INDEX               : constant := 1;
   GLUT_SINGLE              : constant := 0;
   GLUT_DOUBLE              : constant := 2;
   GLUT_ACCUM               : constant := 4;
   GLUT_ALPHA               : constant := 8;
   GLUT_DEPTH               : constant := 16;
   GLUT_STENCIL             : constant := 32;
   GLUT_MULTISAMPLE         : constant := 128;
   GLUT_STEREO              : constant := 256;
   GLUT_LUMINANCE           : constant := 512;

   --  Mouse buttons.

   GLUT_LEFT_BUTTON         : constant := 0;
   GLUT_MIDDLE_BUTTON       : constant := 1;
   GLUT_RIGHT_BUTTON        : constant := 2;

   --  Mouse button callback state.

   GLUT_DOWN                : constant := 0;
   GLUT_UP                  : constant := 1;

   --  function keys

   GLUT_KEY_F1              : constant := 1;
   GLUT_KEY_F2              : constant := 2;
   GLUT_KEY_F3              : constant := 3;
   GLUT_KEY_F4              : constant := 4;
   GLUT_KEY_F5              : constant := 5;
   GLUT_KEY_F6              : constant := 6;
   GLUT_KEY_F7              : constant := 7;
   GLUT_KEY_F8              : constant := 8;
   GLUT_KEY_F9              : constant := 9;
   GLUT_KEY_F10             : constant := 10;
   GLUT_KEY_F11             : constant := 11;
   GLUT_KEY_F12             : constant := 12;

   --  directional keys

   GLUT_KEY_LEFT            : constant := 100;
   GLUT_KEY_UP              : constant := 101;
   GLUT_KEY_RIGHT           : constant := 102;
   GLUT_KEY_DOWN            : constant := 103;
   GLUT_KEY_PAGE_UP         : constant := 104;
   GLUT_KEY_PAGE_DOWN       : constant := 105;
   GLUT_KEY_HOME            : constant := 106;
   GLUT_KEY_END             : constant := 107;
   GLUT_KEY_INSERT          : constant := 108;

   --  Entry/exit callback state.

   GLUT_LEFT                : constant := 0;
   GLUT_ENTERED             : constant := 1;

   --  Menu usage callback state.

   GLUT_MENU_NOT_IN_USE     : constant := 0;
   GLUT_MENU_IN_USE         : constant := 1;

   --  Visibility callback state.

   GLUT_NOT_VISIBLE         : constant := 0;
   GLUT_VISIBLE             : constant := 1;

   --  Window status callback state.

   GLUT_HIDDEN              : constant := 0;
   GLUT_FULLY_RETAINED      : constant := 1;
   GLUT_PARTIALLY_RETAINED  : constant := 2;
   GLUT_FULLY_COVERED       : constant := 3;

   --  Color index component selection values.

   GLUT_RED                 : constant := 0;
   GLUT_GREEN               : constant := 1;
   GLUT_BLUE                : constant := 2;

   --  Layers for use.

   GLUT_NORMAL              : constant := 0;
   GLUT_OVERLAY             : constant := 1;

   --  Stroke font opaque addresses (use constants instead in source code).

   glutStrokeRoman : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, glutStrokeRoman, "glutStrokeRoman", "glutStrokeRoman");

   glutStrokeMonoRoman : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, 
                  glutStrokeMonoRoman, 
                  "glutStrokeMonoRoman", 
                  "glutStrokeMonoRoman");

   --  Stroke font constants (use these in GLUT program).
   --  Bitmap font opaque addresses (use constants instead in source code).

   glutBitmap9By15 : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, glutBitmap9By15, "glutBitmap9By15", "glutBitmap9By15");

   glutBitmap8By13 : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, glutBitmap8By13, "glutBitmap8By13", "glutBitmap8By13");

   glutBitmapTimesRoman10 : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, 
                  glutBitmapTimesRoman10, 
                  "glutBitmapTimesRoman10", 
                  "glutBitmapTimesRoman10");

   glutBitmapTimesRoman24 : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, 
                  glutBitmapTimesRoman24, 
                  "glutBitmapTimesRoman24", 
                  "glutBitmapTimesRoman24");

   glutBitmapHelvetica10 : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, 
                  glutBitmapHelvetica10, 
                  "glutBitmapHelvetica10", 
                  "glutBitmapHelvetica10");

   glutBitmapHelvetica12 : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, 
                  glutBitmapHelvetica12, 
                  "glutBitmapHelvetica12", 
                  "glutBitmapHelvetica12");

   glutBitmapHelvetica18 : aliased Interfaces.C.Extensions.Void_Ptr;
   pragma Import (C, 
                  glutBitmapHelvetica18, 
                  "glutBitmapHelvetica18", 
                  "glutBitmapHelvetica18");

   --  Bitmap font constants (use these in GLUT program).
   --  glutGet parameters.

   GLUT_WINDOW_X                  : constant := 100;
   GLUT_WINDOW_Y                  : constant := 101;
   GLUT_WINDOW_WIDTH              : constant := 102;
   GLUT_WINDOW_HEIGHT             : constant := 103;
   GLUT_WINDOW_BUFFER_SIZE        : constant := 104;
   GLUT_WINDOW_STENCIL_SIZE       : constant := 105;
   GLUT_WINDOW_DEPTH_SIZE         : constant := 106;
   GLUT_WINDOW_RED_SIZE           : constant := 107;
   GLUT_WINDOW_GREEN_SIZE         : constant := 108;
   GLUT_WINDOW_BLUE_SIZE          : constant := 109;
   GLUT_WINDOW_ALPHA_SIZE         : constant := 110;
   GLUT_WINDOW_ACCUM_RED_SIZE     : constant := 111;
   GLUT_WINDOW_ACCUM_GREEN_SIZE   : constant := 112;
   GLUT_WINDOW_ACCUM_BLUE_SIZE    : constant := 113;
   GLUT_WINDOW_ACCUM_ALPHA_SIZE   : constant := 114;
   GLUT_WINDOW_DOUBLEBUFFER       : constant := 115;
   GLUT_WINDOW_RGBA               : constant := 116;
   GLUT_WINDOW_PARENT             : constant := 117;
   GLUT_WINDOW_NUM_CHILDREN       : constant := 118;
   GLUT_WINDOW_COLORMAP_SIZE      : constant := 119;
   GLUT_WINDOW_NUM_SAMPLES        : constant := 120;
   GLUT_WINDOW_STEREO             : constant := 121;
   GLUT_WINDOW_CURSOR             : constant := 122;
   GLUT_SCREEN_WIDTH              : constant := 200;
   GLUT_SCREEN_HEIGHT             : constant := 201;
   GLUT_SCREEN_WIDTH_MM           : constant := 202;
   GLUT_SCREEN_HEIGHT_MM          : constant := 203;
   GLUT_MENU_NUM_ITEMS            : constant := 300;
   GLUT_DISPLAY_MODE_POSSIBLE     : constant := 400;
   GLUT_INIT_WINDOW_X             : constant := 500;
   GLUT_INIT_WINDOW_Y             : constant := 501;
   GLUT_INIT_WINDOW_WIDTH         : constant := 502;
   GLUT_INIT_WINDOW_HEIGHT        : constant := 503;
   GLUT_INIT_DISPLAY_MODE         : constant := 504;
   GLUT_ELAPSED_TIME              : constant := 700;

   --  glutDeviceGet parameters.

   GLUT_HAS_KEYBOARD              : constant := 600;
   GLUT_HAS_MOUSE                 : constant := 601;
   GLUT_HAS_SPACEBALL             : constant := 602;
   GLUT_HAS_DIAL_AND_BUTTON_BOX   : constant := 603;
   GLUT_HAS_TABLET                : constant := 604;
   GLUT_NUM_MOUSE_BUTTONS         : constant := 605;
   GLUT_NUM_SPACEBALL_BUTTONS     : constant := 606;
   GLUT_NUM_BUTTON_BOX_BUTTONS    : constant := 607;
   GLUT_NUM_DIALS                 : constant := 608;
   GLUT_NUM_TABLET_BUTTONS        : constant := 609;

   --  glutLayerGet parameters.

   GLUT_OVERLAY_POSSIBLE          : constant := 800;
   GLUT_LAYER_IN_USE              : constant := 801;
   GLUT_HAS_OVERLAY               : constant := 802;
   GLUT_TRANSPARENT_INDEX         : constant := 803;
   GLUT_NORMAL_DAMAGED            : constant := 804;
   GLUT_OVERLAY_DAMAGED           : constant := 805;

   --  glutVideoResizeGet parameters.

   GLUT_VIDEO_RESIZE_POSSIBLE     : constant := 900;
   GLUT_VIDEO_RESIZE_IN_USE       : constant := 901;
   GLUT_VIDEO_RESIZE_X_DELTA      : constant := 902;
   GLUT_VIDEO_RESIZE_Y_DELTA      : constant := 903;
   GLUT_VIDEO_RESIZE_WIDTH_DELTA  : constant := 904;
   GLUT_VIDEO_RESIZE_HEIGHT_DELTA : constant := 905;
   GLUT_VIDEO_RESIZE_X            : constant := 906;
   GLUT_VIDEO_RESIZE_Y            : constant := 907;
   GLUT_VIDEO_RESIZE_WIDTH        : constant := 908;
   GLUT_VIDEO_RESIZE_HEIGHT       : constant := 909;

   --  glutUseLayer parameters.
   --  glutGetModifiers return mask.

   GLUT_ACTIVE_SHIFT               : constant := 1;
   GLUT_ACTIVE_CTRL                : constant := 2;
   GLUT_ACTIVE_ALT                 : constant := 4;

   --  glutSetCursor parameters.
   --  Basic arrows.

   GLUT_CURSOR_RIGHT_ARROW         : constant := 0;
   GLUT_CURSOR_LEFT_ARROW          : constant := 1;

   --  Symbolic cursor shapes.

   GLUT_CURSOR_INFO                : constant := 2;
   GLUT_CURSOR_DESTROY             : constant := 3;
   GLUT_CURSOR_HELP                : constant := 4;
   GLUT_CURSOR_CYCLE               : constant := 5;
   GLUT_CURSOR_SPRAY               : constant := 6;
   GLUT_CURSOR_WAIT                : constant := 7;
   GLUT_CURSOR_TEXT                : constant := 8;
   GLUT_CURSOR_CROSSHAIR           : constant := 9;

   --  Directional cursors.

   GLUT_CURSOR_UP_DOWN             : constant := 10;
   GLUT_CURSOR_LEFT_RIGHT          : constant := 11;

   --  Sizing cursors.

   GLUT_CURSOR_TOP_SIDE            : constant := 12;
   GLUT_CURSOR_BOTTOM_SIDE         : constant := 13;
   GLUT_CURSOR_LEFT_SIDE           : constant := 14;
   GLUT_CURSOR_RIGHT_SIDE          : constant := 15;
   GLUT_CURSOR_TOP_LEFT_CORNER     : constant := 16;
   GLUT_CURSOR_TOP_RIGHT_CORNER    : constant := 17;
   GLUT_CURSOR_BOTTOM_RIGHT_CORNER : constant := 18;
   GLUT_CURSOR_BOTTOM_LEFT_CORNER  : constant := 19;

   --  Inherit from parent window.

   GLUT_CURSOR_INHERIT             : constant := 100;

   --  Blank cursor.

   GLUT_CURSOR_NONE                : constant := 101;

   --  Fullscreen crosshair (if available).

   GLUT_CURSOR_FULL_CROSSHAIR      : constant := 102;

   --  GLUT initialization sub-API.

   procedure glutInit (argcp : access Integer;
                       argv  : access Interfaces.C.Strings.Chars_Ptr);
   pragma Import (C, glutInit, "glutInit", "glutInit");

   procedure glutInitDisplayMode (mode : Interfaces.C.unsigned);
   pragma Import (C, 
                  glutInitDisplayMode, 
                  "glutInitDisplayMode", 
                  "glutInitDisplayMode");

   procedure glutInitDisplayString (string : Interfaces.C.Strings.Chars_Ptr);
   pragma Import (C, 
                  glutInitDisplayString, 
                  "glutInitDisplayString", 
                  "glutInitDisplayString");

   procedure glutInitWindowPosition (x : Integer; y : Integer);
   pragma Import (C, 
                  glutInitWindowPosition, 
                  "glutInitWindowPosition", 
                  "glutInitWindowPosition");

   procedure glutInitWindowSize (width : Integer; height : Integer);
   pragma Import (C, 
                  glutInitWindowSize, 
                  "glutInitWindowSize", 
                  "glutInitWindowSize");

   procedure glutMainLoop;
   pragma Import (C, glutMainLoop, "glutMainLoop", "glutMainLoop");

   --  GLUT window sub-API.

   function glutCreateWindow
      (title : Interfaces.C.Strings.Chars_Ptr) return Integer;
   pragma Import (C, glutCreateWindow, "glutCreateWindow", "glutCreateWindow");

   function glutCreateWindow (title : String) return Integer;

   function glutCreateSubWindow (win    : Integer;
                                 x      : Integer;
                                 y      : Integer;
                                 width  : Integer;
                                 height : Integer) return Integer;
   pragma Import (C, 
                  glutCreateSubWindow, 
                  "glutCreateSubWindow", 
                  "glutCreateSubWindow");

   procedure glutDestroyWindow (win : Integer);
   pragma Import (C, 
                  glutDestroyWindow, 
                  "glutDestroyWindow", 
                  "glutDestroyWindow");

   procedure glutPostRedisplay;
   pragma Import (C, 
                  glutPostRedisplay, 
                  "glutPostRedisplay", 
                  "glutPostRedisplay");

   procedure glutPostWindowRedisplay (win : Integer);
   pragma Import (C, 
                  glutPostWindowRedisplay, 
                  "glutPostWindowRedisplay", 
                  "glutPostWindowRedisplay");

   procedure glutSwapBuffers;
   pragma Import (C, glutSwapBuffers, "glutSwapBuffers", "glutSwapBuffers");

   function glutGetWindow return Integer;
   pragma Import (C, glutGetWindow, "glutGetWindow", "glutGetWindow");

   procedure glutSetWindow (win : Integer);
   pragma Import (C, glutSetWindow, "glutSetWindow", "glutSetWindow");

   procedure glutSetWindowTitle (title : Interfaces.C.Strings.Chars_Ptr);
   pragma Import (C, 
                  glutSetWindowTitle, 
                  "glutSetWindowTitle", 
                  "glutSetWindowTitle");

   procedure glutSetWindowTitle (title : String);

   procedure glutSetIconTitle (title : Interfaces.C.Strings.Chars_Ptr);
   pragma Import (C, glutSetIconTitle, "glutSetIconTitle", "glutSetIconTitle");

   procedure glutSetIconTitle (title : String);

   procedure glutPositionWindow (x : Integer; y : Integer);
   pragma Import (C, 
                  glutPositionWindow, 
                  "glutPositionWindow", 
                  "glutPositionWindow");

   procedure glutReshapeWindow (width : Integer; height : Integer);
   pragma Import (C, 
                  glutReshapeWindow, 
                  "glutReshapeWindow", 
                  "glutReshapeWindow");

   procedure glutPopWindow;
   pragma Import (C, glutPopWindow, "glutPopWindow", "glutPopWindow");

   procedure glutPushWindow;
   pragma Import (C, glutPushWindow, "glutPushWindow", "glutPushWindow");

   procedure glutIconifyWindow;
   pragma Import (C, 
                  glutIconifyWindow, 
                  "glutIconifyWindow", 
                  "glutIconifyWindow");

   procedure glutShowWindow;
   pragma Import (C, glutShowWindow, "glutShowWindow", "glutShowWindow");

   procedure glutHideWindow;
   pragma Import (C, glutHideWindow, "glutHideWindow", "glutHideWindow");

   procedure glutFullScreen;
   pragma Import (C, glutFullScreen, "glutFullScreen", "glutFullScreen");

   procedure glutSetCursor (cursor : Integer);
   pragma Import (C, glutSetCursor, "glutSetCursor", "glutSetCursor");

   procedure glutWarpPointer (x : Integer; y : Integer);
   pragma Import (C, glutWarpPointer, "glutWarpPointer", "glutWarpPointer");

   --  GLUT overlay sub-API.

   procedure glutEstablishOverlay;
   pragma Import (C, 
                  glutEstablishOverlay, 
                  "glutEstablishOverlay", 
                  "glutEstablishOverlay");

   procedure glutRemoveOverlay;
   pragma Import (C, 
                  glutRemoveOverlay, 
                  "glutRemoveOverlay", 
                  "glutRemoveOverlay");

   procedure glutUseLayer (layer : GL.GLenum);
   pragma Import (C, glutUseLayer, "glutUseLayer", "glutUseLayer");

   procedure glutPostOverlayRedisplay;
   pragma Import (C, 
                  glutPostOverlayRedisplay, 
                  "glutPostOverlayRedisplay", 
                  "glutPostOverlayRedisplay");

   procedure glutPostWindowOverlayRedisplay (win : Integer);
   pragma Import (C, 
                  glutPostWindowOverlayRedisplay, 
                  "glutPostWindowOverlayRedisplay", 
                  "glutPostWindowOverlayRedisplay");

   procedure glutShowOverlay;
   pragma Import (C, glutShowOverlay, "glutShowOverlay", "glutShowOverlay");

   procedure glutHideOverlay;
   pragma Import (C, glutHideOverlay, "glutHideOverlay", "glutHideOverlay");

   --  GLUT menu sub-API.

   type Glut_proc_1 is access procedure (P1 : Integer);

   function glutCreateMenu (P1 : Glut_proc_1) return Integer;
   pragma Import (C, glutCreateMenu, "glutCreateMenu", "glutCreateMenu");

   procedure glutDestroyMenu (menu : Integer);
   pragma Import (C, glutDestroyMenu, "glutDestroyMenu", "glutDestroyMenu");

   function glutGetMenu return Integer;
   pragma Import (C, glutGetMenu, "glutGetMenu", "glutGetMenu");

   procedure glutSetMenu (menu : Integer);
   pragma Import (C, glutSetMenu, "glutSetMenu", "glutSetMenu");

   procedure glutAddMenuEntry (label : Interfaces.C.Strings.Chars_Ptr;
                               value : Integer);
   pragma Import (C, glutAddMenuEntry, "glutAddMenuEntry", "glutAddMenuEntry");

   procedure glutAddMenuEntry (label : String; value : Integer);

   procedure glutAddSubMenu (label   : Interfaces.C.Strings.Chars_Ptr;
                             submenu : Integer);
   pragma Import (C, glutAddSubMenu, "glutAddSubMenu", "glutAddSubMenu");

   procedure glutAddSubMenu (label : String; submenu : Integer);

   procedure glutChangeToMenuEntry (item  : Integer;
                                    label : Interfaces.C.Strings.Chars_Ptr;
                                    value : Integer);
   pragma Import (C, 
                  glutChangeToMenuEntry, 
                  "glutChangeToMenuEntry", 
                  "glutChangeToMenuEntry");

   procedure glutChangeToMenuEntry (item  : Integer;
                                    label : String;
                                    value : Integer);

   procedure glutChangeToSubMenu (item    : Integer;
                                  label   : Interfaces.C.Strings.Chars_Ptr;
                                  submenu : Integer);
   pragma Import (C, 
                  glutChangeToSubMenu, 
                  "glutChangeToSubMenu", 
                  "glutChangeToSubMenu");

   procedure glutChangeToSubMenu (item    : Integer;
                                  label   : String;
                                  submenu : Integer);

   procedure glutRemoveMenuItem (item : Integer);
   pragma Import (C, 
                  glutRemoveMenuItem, 
                  "glutRemoveMenuItem", 
                  "glutRemoveMenuItem");

   procedure glutAttachMenu (button : Integer);
   pragma Import (C, glutAttachMenu, "glutAttachMenu", "glutAttachMenu");

   procedure glutDetachMenu (button : Integer);
   pragma Import (C, glutDetachMenu, "glutDetachMenu", "glutDetachMenu");

   --  GLUT callback sub-API.

   type Glut_proc_2 is access procedure;

   procedure glutDisplayFunc (P1 : Glut_proc_2);
   pragma Import (C, glutDisplayFunc, "glutDisplayFunc", "glutDisplayFunc");

   type Glut_proc_3 is access procedure (width : Integer; height : Integer);

   procedure glutReshapeFunc (P1 : Glut_proc_3);
   pragma Import (C, glutReshapeFunc, "glutReshapeFunc", "glutReshapeFunc");

   type Glut_proc_4 is access
      procedure (key : Interfaces.C.unsigned_char; x : Integer; y : Integer);

   procedure glutKeyboardFunc (P1 : Glut_proc_4);
   pragma Import (C, glutKeyboardFunc, "glutKeyboardFunc", "glutKeyboardFunc");

   type Glut_proc_5 is access
      procedure (button : Integer; state : Integer; x : Integer; y : Integer);

   procedure glutMouseFunc (P1 : Glut_proc_5);
   pragma Import (C, glutMouseFunc, "glutMouseFunc", "glutMouseFunc");

   type Glut_proc_6 is access procedure (x : Integer; y : Integer);

   procedure glutMotionFunc (P1 : Glut_proc_6);
   pragma Import (C, glutMotionFunc, "glutMotionFunc", "glutMotionFunc");

   type Glut_proc_7 is access procedure (x : Integer; y : Integer);

   procedure glutPassiveMotionFunc (P1 : Glut_proc_7);
   pragma Import (C, 
                  glutPassiveMotionFunc, 
                  "glutPassiveMotionFunc", 
                  "glutPassiveMotionFunc");

   type Glut_proc_8 is access procedure (state : Integer);

   procedure glutEntryFunc (P1 : Glut_proc_8);
   pragma Import (C, glutEntryFunc, "glutEntryFunc", "glutEntryFunc");

   type Glut_proc_9 is access procedure (state : Integer);

   procedure glutVisibilityFunc (P1 : Glut_proc_9);
   pragma Import (C, 
                  glutVisibilityFunc, 
                  "glutVisibilityFunc", 
                  "glutVisibilityFunc");

   type Glut_proc_10 is access procedure;

   procedure glutIdleFunc (P1 : Glut_proc_10);
   pragma Import (C, glutIdleFunc, "glutIdleFunc", "glutIdleFunc");

   type Glut_proc_11 is access procedure (value : Integer);

   procedure glutTimerFunc (millis : Interfaces.C.unsigned;
                            P2     : Glut_proc_11;
                            value  : Integer);
   pragma Import (C, glutTimerFunc, "glutTimerFunc", "glutTimerFunc");

   type Glut_proc_12 is access procedure (state : Integer);

   procedure glutMenuStateFunc (P1 : Glut_proc_12);
   pragma Import (C, 
                  glutMenuStateFunc, 
                  "glutMenuStateFunc", 
                  "glutMenuStateFunc");

   type Glut_proc_13 is access
      procedure (key : Integer; x : Integer; y : Integer);

   procedure glutSpecialFunc (P1 : Glut_proc_13);
   pragma Import (C, glutSpecialFunc, "glutSpecialFunc", "glutSpecialFunc");

   type Glut_proc_14 is access
      procedure (x : Integer; y : Integer; z : Integer);

   procedure glutSpaceballMotionFunc (P1 : Glut_proc_14);
   pragma Import (C, 
                  glutSpaceballMotionFunc, 
                  "glutSpaceballMotionFunc", 
                  "glutSpaceballMotionFunc");

   type Glut_proc_15 is access
      procedure (x : Integer; y : Integer; z : Integer);

   procedure glutSpaceballRotateFunc (P1 : Glut_proc_15);
   pragma Import (C, 
                  glutSpaceballRotateFunc, 
                  "glutSpaceballRotateFunc", 
                  "glutSpaceballRotateFunc");

   type Glut_proc_16 is access procedure (button : Integer; state : Integer);

   procedure glutSpaceballButtonFunc (P1 : Glut_proc_16);
   pragma Import (C, 
                  glutSpaceballButtonFunc, 
                  "glutSpaceballButtonFunc", 
                  "glutSpaceballButtonFunc");

   type Glut_proc_17 is access procedure (button : Integer; state : Integer);

   procedure glutButtonBoxFunc (P1 : Glut_proc_17);
   pragma Import (C, 
                  glutButtonBoxFunc, 
                  "glutButtonBoxFunc", 
                  "glutButtonBoxFunc");

   type Glut_proc_18 is access procedure (dial : Integer; value : Integer);

   procedure glutDialsFunc (P1 : Glut_proc_18);
   pragma Import (C, glutDialsFunc, "glutDialsFunc", "glutDialsFunc");

   type Glut_proc_19 is access procedure (x : Integer; y : Integer);

   procedure glutTabletMotionFunc (P1 : Glut_proc_19);
   pragma Import (C, 
                  glutTabletMotionFunc, 
                  "glutTabletMotionFunc", 
                  "glutTabletMotionFunc");

   type Glut_proc_20 is access
      procedure (button : Integer; state : Integer; x : Integer; y : Integer);

   procedure glutTabletButtonFunc (P1 : Glut_proc_20);
   pragma Import (C, 
                  glutTabletButtonFunc, 
                  "glutTabletButtonFunc", 
                  "glutTabletButtonFunc");

   type Glut_proc_21 is access
      procedure (status : Integer; x : Integer; y : Integer);

   procedure glutMenuStatusFunc (P1 : Glut_proc_21);
   pragma Import (C, 
                  glutMenuStatusFunc, 
                  "glutMenuStatusFunc", 
                  "glutMenuStatusFunc");

   type Glut_proc_22 is access procedure;

   procedure glutOverlayDisplayFunc (P1 : Glut_proc_22);
   pragma Import (C, 
                  glutOverlayDisplayFunc, 
                  "glutOverlayDisplayFunc", 
                  "glutOverlayDisplayFunc");

   type Glut_proc_23 is access procedure (state : Integer);

   procedure glutWindowStatusFunc (P1 : Glut_proc_23);
   pragma Import (C, 
                  glutWindowStatusFunc, 
                  "glutWindowStatusFunc", 
                  "glutWindowStatusFunc");

   --  GLUT color index sub-API.

   procedure glutSetColor (P1    : Integer;
                           red   : GL.GLfloat;
                           green : GL.GLfloat;
                           blue  : GL.GLfloat);
   pragma Import (C, glutSetColor, "glutSetColor", "glutSetColor");

   function glutGetColor
      (ndx : Integer; component : Integer) return GL.GLfloat;
   pragma Import (C, glutGetColor, "glutGetColor", "glutGetColor");

   procedure glutCopyColormap (win : Integer);
   pragma Import (C, glutCopyColormap, "glutCopyColormap", "glutCopyColormap");

   --  GLUT state retrieval sub-API.

   function glutGet (type_Id : GL.GLenum) return Integer;
   pragma Import (C, glutGet, "glutGet", "glutGet");

   function glutDeviceGet (type_Id : GL.GLenum) return Integer;
   pragma Import (C, glutDeviceGet, "glutDeviceGet", "glutDeviceGet");

   --  GLUT extension support sub-API

   function glutExtensionSupported
      (name : Interfaces.C.Strings.Chars_Ptr) return Integer;
   pragma Import (C, 
                  glutExtensionSupported, 
                  "glutExtensionSupported", 
                  "glutExtensionSupported");

   function glutExtensionSupported (name : String) return Integer;

   function glutGetModifiers return Integer;
   pragma Import (C, glutGetModifiers, "glutGetModifiers", "glutGetModifiers");

   function glutLayerGet (type_Id : GL.GLenum) return Integer;
   pragma Import (C, glutLayerGet, "glutLayerGet", "glutLayerGet");

   --  GLUT font sub-API

   procedure glutBitmapCharacter (font      : access Interfaces.C.Extensions.Void_Ptr;
                                  character : Integer);
   pragma Import (C, 
                  glutBitmapCharacter, 
                  "glutBitmapCharacter", 
                  "glutBitmapCharacter");

   function glutBitmapWidth
      (font      : access Interfaces.C.Extensions.Void_Ptr;
       character : Integer) return Integer;
   pragma Import (C, glutBitmapWidth, "glutBitmapWidth", "glutBitmapWidth");

   procedure glutStrokeCharacter
      (font      : access Interfaces.C.Extensions.Void_Ptr;
       character : Integer);
   pragma Import (C, 
                  glutStrokeCharacter, 
                  "glutStrokeCharacter", 
                  "glutStrokeCharacter");

   function glutStrokeWidth
      (font      : access Interfaces.C.Extensions.Void_Ptr;
       character : Integer) return Integer;
   pragma Import (C, glutStrokeWidth, "glutStrokeWidth", "glutStrokeWidth");

   function glutStrokeLength
      (font      : access Interfaces.C.Extensions.Void_Ptr;
       string    : Interfaces.C.Strings.Chars_Ptr) return Integer;
   pragma Import (C, glutStrokeLength, "glutStrokeLength", "glutStrokeLength");

   function glutBitmapLength
      (font      : access Interfaces.C.Extensions.Void_Ptr;
       string    : Interfaces.C.Strings.Chars_Ptr) return Integer;
   pragma Import (C, glutBitmapLength, "glutBitmapLength", "glutBitmapLength");

   --  GLUT pre-built models sub-API

   procedure glutWireSphere (radius : GL.GLdouble;
                             slices : GL.GLint;
                             stacks : GL.GLint);
   pragma Import (C, glutWireSphere, "glutWireSphere", "glutWireSphere");

   procedure glutSolidSphere (radius : GL.GLdouble;
                              slices : GL.GLint;
                              stacks : GL.GLint);
   pragma Import (C, glutSolidSphere, "glutSolidSphere", "glutSolidSphere");

   procedure glutWireCone (base   : GL.GLdouble;
                           height : GL.GLdouble;
                           slices : GL.GLint;
                           stacks : GL.GLint);
   pragma Import (C, glutWireCone, "glutWireCone", "glutWireCone");

   procedure glutSolidCone (base   : GL.GLdouble;
                            height : GL.GLdouble;
                            slices : GL.GLint;
                            stacks : GL.GLint);
   pragma Import (C, glutSolidCone, "glutSolidCone", "glutSolidCone");

   procedure glutWireCube (size : GL.GLdouble);
   pragma Import (C, glutWireCube, "glutWireCube", "glutWireCube");

   procedure glutSolidCube (size : GL.GLdouble);
   pragma Import (C, glutSolidCube, "glutSolidCube", "glutSolidCube");

   procedure glutWireTorus (innerRadius : GL.GLdouble;
                            outerRadius : GL.GLdouble;
                            sides       : GL.GLint;
                            rings       : GL.GLint);
   pragma Import (C, glutWireTorus, "glutWireTorus", "glutWireTorus");

   procedure glutSolidTorus (innerRadius : GL.GLdouble;
                             outerRadius : GL.GLdouble;
                             sides       : GL.GLint;
                             rings       : GL.GLint);
   pragma Import (C, glutSolidTorus, "glutSolidTorus", "glutSolidTorus");

   procedure glutWireDodecahedron;
   pragma Import (C, 
                  glutWireDodecahedron, 
                  "glutWireDodecahedron", 
                  "glutWireDodecahedron");

   procedure glutSolidDodecahedron;
   pragma Import (C, 
                  glutSolidDodecahedron, 
                  "glutSolidDodecahedron", 
                  "glutSolidDodecahedron");

   procedure glutWireTeapot (size : GL.GLdouble);
   pragma Import (C, glutWireTeapot, "glutWireTeapot", "glutWireTeapot");

   procedure glutSolidTeapot (size : GL.GLdouble);
   pragma Import (C, glutSolidTeapot, "glutSolidTeapot", "glutSolidTeapot");

   procedure glutWireOctahedron;
   pragma Import (C, 
                  glutWireOctahedron, 
                  "glutWireOctahedron", 
                  "glutWireOctahedron");

   procedure glutSolidOctahedron;
   pragma Import (C, 
                  glutSolidOctahedron, 
                  "glutSolidOctahedron", 
                  "glutSolidOctahedron");

   procedure glutWireTetrahedron;
   pragma Import (C, 
                  glutWireTetrahedron, 
                  "glutWireTetrahedron", 
                  "glutWireTetrahedron");

   procedure glutSolidTetrahedron;
   pragma Import (C, 
                  glutSolidTetrahedron, 
                  "glutSolidTetrahedron", 
                  "glutSolidTetrahedron");

   procedure glutWireIcosahedron;
   pragma Import (C, 
                  glutWireIcosahedron, 
                  "glutWireIcosahedron", 
                  "glutWireIcosahedron");

   procedure glutSolidIcosahedron;
   pragma Import (C, 
                  glutSolidIcosahedron, 
                  "glutSolidIcosahedron", 
                  "glutSolidIcosahedron");

   function glutVideoResizeGet (param : GL.GLenum) return Integer;
   pragma Import (C, 
                  glutVideoResizeGet, 
                  "glutVideoResizeGet", 
                  "glutVideoResizeGet");

   procedure glutSetupVideoResizing;
   pragma Import (C, 
                  glutSetupVideoResizing, 
                  "glutSetupVideoResizing", 
                  "glutSetupVideoResizing");

   procedure glutStopVideoResizing;
   pragma Import (C, 
                  glutStopVideoResizing, 
                  "glutStopVideoResizing", 
                  "glutStopVideoResizing");

   procedure glutVideoResize (x      : Integer;
                              y      : Integer;
                              width  : Integer;
                              height : Integer);
   pragma Import (C, glutVideoResize, "glutVideoResize", "glutVideoResize");

   procedure glutVideoPan (x      : Integer;
                           y      : Integer;
                           width  : Integer;
                           height : Integer);
   pragma Import (C, glutVideoPan, "glutVideoPan", "glutVideoPan");

end Glut;

