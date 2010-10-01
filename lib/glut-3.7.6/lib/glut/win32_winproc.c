
/* Copyright (c) Nate Robins, 1997, 2001. */
/* portions Copyright (c) Mark Kilgard, 1997, 1998. */

/* 
This program is freely distributable without licensing fees 
and is provided without guarantee or warrantee expressed or 
implied. This program is -not- in the public domain. 
*/


#include "glutint.h"
#if defined(__CYGWIN32__)
typedef MINMAXINFO* LPMINMAXINFO;
#else
#include <sys/timeb.h>
#endif

#ifdef _WIN32
#include <assert.h>
#include <crtdbg.h>
#include <windowsx.h>
#include <mmsystem.h>  /* Win32 Multimedia API header. */
#endif

extern unsigned __glutMenuButton;
extern GLUTidleCB __glutIdleFunc;
extern GLUTtimer *__glutTimerList;
extern void handleTimeouts(void);
extern GLUTmenuItem *__glutGetUniqueMenuItem(GLUTmenu * menu, int unique);
static HMENU __glutHMenu;

static int __glutOnCreate( HWND hwnd, LPCREATESTRUCT createStruct )
{
	return TRUE;
}

static void __glutOnClose( HWND hwnd )
{
	if ( __glutExitFunc )
	{
		__glutExitFunc( 0 );
	}
	
	DestroyWindow( hwnd );
}

static void __glutOnDestroy( HWND hwnd )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window )
	{
		if ( window->ctx )
		{
			wglMakeCurrent( NULL, NULL );
			wglDeleteContext( window->ctx );
		}
	}
}

static void __glutOnPaint( HWND hwnd )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	
	PAINTSTRUCT ps;
	BeginPaint( hwnd, &ps );
	EndPaint( hwnd, &ps );
	
	if ( window )
	{
		if ( window->win == hwnd ) 
		{
			__glutPostRedisplay(window, GLUT_REPAIR_WORK);
		} 
	}
}

static void __glutOnSize( HWND hwnd, UINT state, int width, int height )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window )
	{
		if ( window->width != width || window->height != height )
		{
			window->width = width;
			window->height = height;
			
			__glutSetWindow( window );
			
			/* Do not execute OpenGL out of sequence with respect to the 
			SetWindowPos request! */
			GdiFlush();
			
			window->reshape( width, height );
			window->forceReshape = FALSE;
			
			/* A reshape should be considered like posting a repair request. */
			__glutPostRedisplay( window, GLUT_REPAIR_WORK );
		}
	}
}

static void updateWindowState( GLUTwindow *window, int visState )
{
	GLUTwindow* child;
	
	/* XXX shownState and visState are the same in Win32. */
	window->shownState = visState;
	if ( visState != window->visState ) 
	{
		if ( window->windowStatus ) 
		{
			window->visState = visState;
			__glutSetWindow( window );
			window->windowStatus( visState );
		}
	}
	/* Since Win32 only sends an activate for the toplevel window,
	update the visibility for all the child windows. */
	child = window->children;
	while ( child ) 
	{
		updateWindowState( child, visState );
		child = child->siblings;
	}
}

static void __glutOnActivate( HWND hwnd, UINT state, HWND hWndPrev, BOOL minimized )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window ) 
	{
		int visState = ! minimized;
		updateWindowState( window, visState );
	}
	
	/* Just in case there is a palette, make sure we re-select it if the 
	window is being activated. */
	if ( state != WA_INACTIVE )
	{
		PostMessage( hwnd, WM_PALETTECHANGED, 0, 0 );
	}
}

static void __glutOnSetFocus( HWND hwnd, HWND hwndOldFocus )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window )
	{
		window->entryState = WM_SETFOCUS;
		if ( window->entry )
		{
			__glutSetWindow( window );
			window->entry( GLUT_ENTERED );
			/* XXX Generation of fake passive notify?  See how much
			work the X11 code does to support fake passive notify
			callbacks. */
		}
		
		if ( window->joystick && __glutCurrentWindow )
		{
			if ( __glutCurrentWindow->joyPollInterval > 0 ) 
			{
				/* Because Win32 will only let one window capture the
				joystick at a time, we must capture it when we get the
				focus and release it when we lose the focus. */
				MMRESULT result = joySetCapture( __glutCurrentWindow->win, JOYSTICKID1, 0, TRUE );
				if ( result == JOYERR_NOERROR )
				{
					joySetThreshold( JOYSTICKID1, __glutCurrentWindow->joyPollInterval );
				}
			}
		}
	}
}

static void __glutOnKillFocus( HWND hwnd, HWND hwndNewFocus )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window )
	{
		window->entryState = WM_KILLFOCUS;
		if ( window->entry ) 
		{
			__glutSetWindow( window );
			window->entry( GLUT_LEFT );
		}
		
		if ( window->joystick && __glutCurrentWindow ) 
		{
			if ( __glutCurrentWindow->joyPollInterval > 0 )
			{
				joyReleaseCapture( JOYSTICKID1 );
			}
		}
	}
}

static void __glutOnGetMinMaxInfo( HWND hwnd, LPMINMAXINFO mmi )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window )
	{
		RECT r;
		
		/* set it to as small as possible, although it doesn't seem to allow 
		the decorations to be munged. */
		r.top    = 0;
		r.left   = 0;
		r.bottom = 1;
		r.right  = 1;
		
		/* get window coordinates from the client coordinates. */
		AdjustWindowRect( &r, GetWindowLong( hwnd, GWL_STYLE ), FALSE );
		mmi->ptMinTrackSize.x = r.right - r.left;
		mmi->ptMinTrackSize.y = r.bottom - r.top;
		mmi->ptMaxTrackSize.x = __glutScreenWidth;
		mmi->ptMaxTrackSize.y = __glutScreenHeight;
	}
}

static GLUTwindow* getWindowUnderCursor( HWND hwndParent, GLUTwindow* windowParent )
{
	/* It seems that some messages are sent to the parent window only.  Since 
	GLUT wants to send information to the "current" window, descend the 
	heirarchy until the window with the cursor in it is found. */
	
	assert( windowParent );
	
	if ( windowParent->children ) 
	{
		HWND hwndChild;
		
		POINT p;
		GetCursorPos( &p );
		ScreenToClient( hwndParent, &p );
		
		hwndChild = ChildWindowFromPoint( hwndParent, p );
		if ( hwndChild && hwndChild != hwndParent )
		{
			GLUTwindow* windowChild = __glutGetWindow( hwndChild );
			if ( windowChild )
			{
				windowParent = getWindowUnderCursor( hwndChild, windowChild );
			}
		}
	}
	
	return windowParent;
}

static unsigned int getModifierMask( void )
{
	unsigned int mask = 0;
	
	if ( ( GetKeyState( VK_SHIFT ) & 0xFF00 ) )
	{
		mask |= ShiftMask;
	}
	if ( ( GetKeyState( VK_CONTROL ) & 0xFF00 ) )
	{
		mask |= ControlMask;
	}
	if ( ( GetKeyState( VK_MENU ) & 0xFF00 ) )
	{
		mask |= Mod1Mask;
	}
	
	return mask;
}

static int vkToSpecial( UINT vk )
{
	switch ( vk )
	{
	case VK_F1:			return -GLUT_KEY_F1;
	case VK_F2:			return -GLUT_KEY_F2;
	case VK_F3:			return -GLUT_KEY_F3;
	case VK_F4:			return -GLUT_KEY_F4;
	case VK_F5:			return -GLUT_KEY_F5;
	case VK_F6:			return -GLUT_KEY_F6;
	case VK_F7:			return -GLUT_KEY_F7;
	case VK_F8:			return -GLUT_KEY_F8;
	case VK_F9:			return -GLUT_KEY_F9;
	case VK_F10:		return -GLUT_KEY_F10;
	case VK_F11:		return -GLUT_KEY_F11;
	case VK_F12:		return -GLUT_KEY_F12;
	case VK_LEFT:		return -GLUT_KEY_LEFT;
	case VK_UP:			return -GLUT_KEY_UP;
	case VK_RIGHT:		return -GLUT_KEY_RIGHT;
	case VK_DOWN:		return -GLUT_KEY_DOWN;
	case VK_PRIOR:		return -GLUT_KEY_PAGE_UP;
	case VK_NEXT:		return -GLUT_KEY_PAGE_DOWN;
	case VK_HOME:		return -GLUT_KEY_HOME;
	case VK_END:		return -GLUT_KEY_END;
	case VK_INSERT:		return -GLUT_KEY_INSERT;
	default:			return 0;
	}
}

static int getKey( UINT vk )
{
	BYTE keyState[ 256 ];
	WORD c[ 2 ];
	
	GetKeyboardState( keyState );
	
	if ( ToAscii( vk, 0, keyState, c, 0 ) == 1 )
	{
		return c[ 0 ];
	}
	else
	{
		if ( vk == VK_DELETE )
		{
			return 127;	/* 127 = DEL in ascii */
		}
		else
		{
			return vkToSpecial( vk );
		}
	}
}

static void __glutOnKey( HWND hwnd, UINT vk, BOOL down, int repeats, UINT flags )
{
	int key;
	POINT point;
	
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window )
	{
		window = getWindowUnderCursor( hwnd, window );
		
		/* If we are ignoring auto repeated key strokes for the window, 
		and this keystroke is an autorepeat generated one, bail. */ 
		if ( down )
		{
			BOOL autorepeat = ( ( flags >> 14 ) & 0x1 ) == 1;
		
			if ( window->ignoreKeyRepeat && autorepeat )
			{
				return;
			}
		}
		
		GetCursorPos( &point );
		ScreenToClient( window->win, &point );
		
		__glutModifierMask = getModifierMask();
		
		key = getKey( vk );
		if ( key < 0 ) 
		{
			/* special */
			
			if ( down )
			{
				if ( window->special )
				{
					__glutSetWindow( window );
					window->special( -key, point.x, point.y );
				}
			}
			else
			{
				if ( window->specialUp )
				{
					__glutSetWindow( window );
					window->specialUp( -key, point.x, point.y );
				}
			}
		}
		else if ( key > 0 )
		{
			/* ascii */
			
			if ( down )
			{
				if ( window->keyboard )
				{
					__glutSetWindow( window );
					window->keyboard( ( unsigned char )key, point.x, point.y );
				}
			}
			else
			{
				if ( window->keyboardUp )
				{
					__glutSetWindow( window );
					window->keyboardUp( ( unsigned char )key, point.x, point.y );
				}
			}
		}
		
		__glutModifierMask = ( unsigned int )~0;
	}
}

static void __glutOnButtonDn( HWND hwnd, int x, int y, int button )
{
	GLUTmenu* menu;
	GLUTwindow* window = __glutGetWindow( hwnd );
	
	// !!! - look at this more closely sometime
	// !!! - look at this more closely sometime
	// !!! - look at this more closely sometime
	
	/* finish the menu if we get a button down message (user must have
	cancelled the menu). */
	if ( __glutMappedMenu ) 
	{
		POINT point;
		
		/* TODO: take this out once the menu on middle mouse stuff works
		properly. */
		if (button == GLUT_MIDDLE_BUTTON)
			return;
		GetCursorPos(&point);
		ScreenToClient(hwnd, &point);
		__glutItemSelected = NULL;
		__glutFinishMenu(hwnd, point.x, point.y);
		return;
	}
	
	// !!!
	// !!!
	// !!!
	
	/* Set the capture so we can get mouse events outside the window. */
	SetCapture( hwnd );
	
	if ( window )
	{
		menu = __glutGetMenuByNum( window->menu[ button ] );
		if ( menu ) 
		{
			POINT point;
			point.x = x; point.y = y;
			ClientToScreen(window->win, &point);
			__glutMenuButton = button == GLUT_RIGHT_BUTTON ? TPM_RIGHTBUTTON :
			button == GLUT_LEFT_BUTTON  ? TPM_LEFTBUTTON :
			0x0001;
			__glutStartMenu(menu, window, point.x, point.y, x, y);
		} 
		else if ( window->mouse ) 
		{
			__glutModifierMask = getModifierMask();
			
			__glutSetWindow( window );
			window->mouse( button, GLUT_DOWN, x, y );
			
			__glutModifierMask = ( unsigned int )~0;
		}
	}
}

static void __glutOnButtonUp( HWND hwnd, int x, int y, int button )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	
	// !!! - look at this more closely sometime
	// !!! - look at this more closely sometime
	// !!! - look at this more closely sometime
	
	/* Bail out if we're processing a menu. */
	if (__glutMappedMenu) {
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(hwnd, &point);
		/* if we're getting the middle button up signal, then something
		on the menu was selected. */
		if (button == GLUT_MIDDLE_BUTTON) {
			return;
			
			/* For some reason, the code below always returns -1 even
			though the point IS IN THE ITEM!  Therefore, just bail out if
			we get a middle mouse up.  The user must select using the
			left mouse button.  Stupid Win32. */
			#if 0
			int item = MenuItemFromPoint(hwnd, __glutHMenu, point);
			if (item != -1)
				__glutItemSelected = (GLUTmenuItem*)GetMenuItemID(__glutHMenu, item);
			else
				__glutItemSelected = NULL;
			__glutFinishMenu(hwnd, point.x, point.y);
			#endif

		} else {
			__glutItemSelected = NULL;
			__glutFinishMenu(hwnd, point.x, point.y);
		}
		return;
	}
	
	// !!!
	// !!!
	// !!!
	
	/* Release the mouse capture. */
	ReleaseCapture();
	
	if ( window )
	{
		if ( window->mouse ) 
		{
			__glutModifierMask = getModifierMask();
			
			__glutSetWindow( window );
			window->mouse( button, GLUT_UP, x, y );
			
			__glutModifierMask = ( unsigned int )~0;
		}
	}
}

static void __glutOnLButtonDn( HWND hwnd, BOOL doubleClick, int x, int y, UINT flags )
{
	__glutOnButtonDn( hwnd, x, y, GLUT_LEFT_BUTTON );
}

static void __glutOnRButtonDn( HWND hwnd, BOOL doubleClick, int x, int y, UINT flags )
{
	__glutOnButtonDn( hwnd, x, y, GLUT_RIGHT_BUTTON );
}

static void __glutOnMButtonDn( HWND hwnd, BOOL doubleClick, int x, int y, UINT flags )
{
	__glutOnButtonDn( hwnd, x, y, GLUT_MIDDLE_BUTTON );
}

static void __glutOnLButtonUp( HWND hwnd, int x, int y, UINT flags )
{
	__glutOnButtonUp( hwnd, x, y, GLUT_LEFT_BUTTON );
}

static void __glutOnRButtonUp( HWND hwnd, int x, int y, UINT flags )
{
	__glutOnButtonUp( hwnd, x, y, GLUT_RIGHT_BUTTON );
}

static void __glutOnMButtonUp( HWND hwnd, int x, int y, UINT flags )
{
	__glutOnButtonUp( hwnd, x, y, GLUT_MIDDLE_BUTTON );
}

static void __glutOnMouseMove( HWND hwnd, int x, int y, UINT flags )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	
	if ( __glutMappedMenu )
	{
		return;
	}
	
	if ( window )
	{
		BOOL down = ( flags & MK_LBUTTON ) || ( flags & MK_RBUTTON ) || ( flags & MK_MBUTTON );
		
		if ( window->motion && down )
		{
			__glutSetWindow( window );
			window->motion( x, y );
		}
		if ( window->passive && ! down )
		{
			__glutSetWindow( window );
			window->passive( x, y );
		}
	}
}

static BOOL __glutOnEnterMenuLoop( HWND hwnd )
{
	/* KLUDGE: create a timer that fires every 100 ms when we start a
	menu so that we can still process the idle & timer events (that
	way, the timers will fire during a menu pick and so will the
	idle func. */
	SetTimer( hwnd, 'MENU', 1, NULL );
	
	return FALSE;
}

static BOOL __glutOnExitMenuLoop( HWND hwnd )
{
	/* nuke the above created timer...we don't need it anymore, since
	the menu is gone now. */
	KillTimer( hwnd, 'MENU' );
	
	return FALSE;
}

static void __glutOnMenuSelect( HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags )
{
	if ( hmenu != 0 )
	{
		__glutHMenu = hmenu;
	}
}

static void __glutOnCommand( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify )
{
	if ( __glutMappedMenu ) 
	{
		POINT point;

		#if 0
		if ( GetSubMenu( __glutHMenu, id ) )
		{
			__glutItemSelected = NULL;
		}
		else
		#endif
		{
			__glutItemSelected = __glutGetUniqueMenuItem( __glutMappedMenu, id );
		}
		
		GetCursorPos( &point );
		ScreenToClient( hwnd, &point );
		
		__glutFinishMenu( hwnd, point.x, point.y );
	} 
}

static void __glutOnTimer( HWND hwnd, UINT id )
{
	/* only worry about the idle function and the timeouts, since
	these are the only events we expect to process during
	processing of a menu. */
	/* we no longer process the idle functions (as outlined in the
	README), since drawing can't be done until the menu has
	finished...it's pretty lame when the animation goes on, but
	doesn't update, so you get this weird jerkiness. */
	#if 0
	if ( __glutIdleFunc )
	{
		__glutIdleFunc();
	}
	#endif

	if ( __glutTimerList )
	{
		handleTimeouts();
	}
}

static BOOL __glutOnSetCursor( HWND hwnd, HWND hWndCursor, UINT codeHitTest, UINT msg )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window )
	{
		__glutSetCursor( window );
	}
	
	return FALSE;
}

static BOOL __glutOnPalette( HWND hwnd )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window && window->colormap ) 
	{
		UnrealizeObject( window->colormap->cmap );
		SelectPalette( window->hdc, window->colormap->cmap, FALSE );
		RealizePalette( window->hdc );
		
		return TRUE;
	}
	
	return FALSE;
}

static BOOL __glutOnJoystick( HWND hwnd )
{
	GLUTwindow* window = __glutGetWindow( hwnd );
	if ( window->joystick ) 
	{
		JOYINFOEX jix;
		int x, y, z;
		
		/* Because WIN32 only supports messages for X, Y, and Z
		translations, we must poll for the rest */
		jix.dwSize = sizeof( jix );
		jix.dwFlags = JOY_RETURNALL;
		joyGetPosEx( JOYSTICKID1, &jix );
		
		#define SCALE( v )  ( ( int )( ( v - 32767 ) / 32.768 ) )
		
		/* Convert to integer for scaling. */
		x = jix.dwXpos;
		y = jix.dwYpos;
		z = jix.dwZpos;
		window->joystick( jix.dwButtons, SCALE( x ), SCALE( y ), SCALE( z ) );
		
		return TRUE;
	}
	
	return FALSE;
}

LONG WINAPI __glutWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
	switch ( msg ) 
	{
	case WM_CREATE:			return HANDLE_WM_CREATE 		( hwnd, wparam, lparam, __glutOnCreate			);
	case WM_CLOSE: 			return HANDLE_WM_CLOSE			( hwnd, wparam, lparam, __glutOnClose			);
	case WM_DESTROY:		return HANDLE_WM_DESTROY		( hwnd, wparam, lparam, __glutOnDestroy 		);
	case WM_PAINT: 			return HANDLE_WM_PAINT			( hwnd, wparam, lparam, __glutOnPaint			);
	case WM_SIZE:			return HANDLE_WM_SIZE			( hwnd, wparam, lparam, __glutOnSize 			);
	case WM_ACTIVATE: 		return HANDLE_WM_ACTIVATE		( hwnd, wparam, lparam, __glutOnActivate		);
	case WM_SETFOCUS: 		return HANDLE_WM_SETFOCUS		( hwnd, wparam, lparam, __glutOnSetFocus		);
	case WM_KILLFOCUS:		return HANDLE_WM_KILLFOCUS 		( hwnd, wparam, lparam, __glutOnKillFocus		);
	case WM_GETMINMAXINFO:	return HANDLE_WM_GETMINMAXINFO  ( hwnd, wparam, lparam, __glutOnGetMinMaxInfo   );
	case WM_KEYDOWN:		return HANDLE_WM_KEYDOWN		( hwnd, wparam, lparam, __glutOnKey				);
	case WM_KEYUP: 			return HANDLE_WM_KEYUP			( hwnd, wparam, lparam, __glutOnKey				);
	case WM_SYSKEYDOWN:		return HANDLE_WM_KEYDOWN		( hwnd, wparam, lparam, __glutOnKey				);
	case WM_SYSKEYUP: 		return HANDLE_WM_KEYUP			( hwnd, wparam, lparam, __glutOnKey				);
	case WM_LBUTTONDOWN: 	return HANDLE_WM_LBUTTONDOWN	( hwnd, wparam, lparam, __glutOnLButtonDn		);
	case WM_RBUTTONDOWN: 	return HANDLE_WM_RBUTTONDOWN	( hwnd, wparam, lparam, __glutOnRButtonDn		);
	case WM_MBUTTONDOWN: 	return HANDLE_WM_MBUTTONDOWN	( hwnd, wparam, lparam, __glutOnMButtonDn		);
	case WM_LBUTTONUP:		return HANDLE_WM_LBUTTONUP 		( hwnd, wparam, lparam, __glutOnLButtonUp		);
	case WM_RBUTTONUP:		return HANDLE_WM_RBUTTONUP 		( hwnd, wparam, lparam, __glutOnRButtonUp		);
	case WM_MBUTTONUP:		return HANDLE_WM_MBUTTONUP 		( hwnd, wparam, lparam, __glutOnMButtonUp		);
	case WM_MOUSEMOVE:		return HANDLE_WM_MOUSEMOVE 		( hwnd, wparam, lparam, __glutOnMouseMove		);
	case WM_ENTERMENULOOP:	return __glutOnEnterMenuLoop( hwnd );
	case WM_EXITMENULOOP:	return __glutOnExitMenuLoop( hwnd );
	case WM_COMMAND:		return HANDLE_WM_COMMAND        ( hwnd, wparam, lparam, __glutOnCommand         );
	case WM_TIMER:			return HANDLE_WM_TIMER          ( hwnd, wparam, lparam, __glutOnTimer           );
		
	case WM_SETCURSOR:
		if ( LOWORD( lparam ) != HTCLIENT )
		{
			/* Let the default window proc handle cursors outside the client area */
			break;
		}
		else
		{
			return HANDLE_WM_SETCURSOR( hwnd, wparam, lparam, __glutOnSetCursor );
		}
		
	case WM_PALETTECHANGED:
		if ( ( HWND )wparam == hwnd ) 
		{
			/* Don't respond to the message that we sent! */
			break;
		}
		/* Fall through to WM_QUERYNEWPALETTE */
		
	case WM_QUERYNEWPALETTE:
		return __glutOnPalette( hwnd );
		
	case MM_JOY1MOVE:
	case MM_JOY1ZMOVE:
	case MM_JOY1BUTTONDOWN:
	case MM_JOY1BUTTONUP:
		return __glutOnJoystick( hwnd );
		
	default:
		break;
	}
	
	return DefWindowProc( hwnd, msg, wparam, lparam );
}
