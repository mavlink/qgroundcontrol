/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_keysym.h,v 1.6 2006/02/13 20:54:08 synap Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_KEYSYM_H
#define _LV_KEYSYM_H

VISUAL_BEGIN_DECLS

/**
 * Enumerate values used within the libvisual event system for keyboard events.
 *
 * The table is closely modelled after that of SDL and the SDL1.2 
 * SDLK defines can be directly translated to those of libvisual, however
 * some keys are left out, but these are rarely or never used.
 *
 * The basic keys are also mapped as in the ASCII table so basic
 * keyboard support is easy to implement within a libvisual client.
 *
 * @see visual_event_queue_add_keyboard
 */
typedef enum {
	VKEY_UNKNOWN		= 0,
	VKEY_FIRST		= 0,
	VKEY_BACKSPACE		= 8,
	VKEY_TAB		= 9,
	VKEY_CLEAR		= 12,
	VKEY_RETURN		= 13,
	VKEY_PAUSE		= 19,
	VKEY_ESCAPE		= 27,
	VKEY_SPACE		= 32,
	VKEY_EXCLAIM		= 33,
	VKEY_QUOTEDBL		= 34,
	VKEY_HASH		= 35,
	VKEY_DOLLAR		= 36,
	VKEY_AMPERSAND		= 38,
	VKEY_QUOTE		= 39,
	VKEY_LEFTPAREN		= 40,
	VKEY_RIGHTPAREN		= 41,
	VKEY_ASTERISK		= 42,
	VKEY_PLUS		= 43,
	VKEY_COMMA		= 44,
	VKEY_MINUS		= 45,
	VKEY_PERIOD		= 46,
	VKEY_SLASH		= 47,
	VKEY_0			= 48,
	VKEY_1			= 49,
	VKEY_2			= 50,
	VKEY_3			= 51,
	VKEY_4			= 52,
	VKEY_5			= 53,
	VKEY_6			= 54,
	VKEY_7			= 55,
	VKEY_8			= 56,
	VKEY_9			= 57,
	VKEY_COLON		= 58,
	VKEY_SEMICOLON		= 59,
	VKEY_LESS		= 60,
	VKEY_EQUALS		= 61,
	VKEY_GREATER		= 62,
	VKEY_QUESTION		= 63,
	VKEY_AT			= 64,

	/* Skip uppercase here because it's done via the VisKeyMod */
	VKEY_LEFTBRACKET	= 91,
	VKEY_BACKSLASH		= 92,
	VKEY_RIGHTBRACKET	= 93,
	VKEY_CARET		= 94,
	VKEY_UNDERSCORE		= 95,
	VKEY_BACKQUOTE		= 96,
	VKEY_a			= 97,
	VKEY_b			= 98,
	VKEY_c			= 99,
	VKEY_d			= 100,
	VKEY_e			= 101,
	VKEY_f			= 102,
	VKEY_g			= 103,
	VKEY_h			= 104,
	VKEY_i			= 105,
	VKEY_j			= 106,
	VKEY_k			= 107,
	VKEY_l			= 108,
	VKEY_m			= 109,
	VKEY_n			= 110,
	VKEY_o			= 111,
	VKEY_p			= 112,
	VKEY_q			= 113,
	VKEY_r			= 114,
	VKEY_s			= 115,
	VKEY_t			= 116,
	VKEY_u			= 117,
	VKEY_v			= 118,
	VKEY_w			= 119,
	VKEY_x			= 120,
	VKEY_y			= 121,
	VKEY_z			= 122,
	VKEY_DELETE		= 127,

	/* Numeric keypad */
	VKEY_KP0		= 256,
	VKEY_KP1		= 257,
	VKEY_KP2		= 258,
	VKEY_KP3		= 259,
	VKEY_KP4		= 260,
	VKEY_KP5		= 261,
	VKEY_KP6		= 262,
	VKEY_KP7		= 263,
	VKEY_KP8		= 264,
	VKEY_KP9		= 265,
	VKEY_KP_PERIOD		= 266,
	VKEY_KP_DIVIDE		= 267,
	VKEY_KP_MULTIPLY	= 268,
	VKEY_KP_MINUS		= 269,
	VKEY_KP_PLUS		= 270,
	VKEY_KP_ENTER		= 271,
	VKEY_KP_EQUALS		= 272,

	/* Arrows + Home/End pad */
	VKEY_UP			= 273,
	VKEY_DOWN		= 274,
	VKEY_RIGHT		= 275,
	VKEY_LEFT		= 276,
	VKEY_INSERT		= 277,
	VKEY_HOME		= 278,
	VKEY_END		= 279,
	VKEY_PAGEUP		= 280,
	VKEY_PAGEDOWN		= 281,

	/* Function keys */
	VKEY_F1			= 282,
	VKEY_F2			= 283,
	VKEY_F3			= 284,
	VKEY_F4			= 285,
	VKEY_F5			= 286,
	VKEY_F6			= 287,
	VKEY_F7			= 288,
	VKEY_F8			= 289,
	VKEY_F9			= 290,
	VKEY_F10		= 291,
	VKEY_F11		= 292,
	VKEY_F12		= 293,
	VKEY_F13		= 294,
	VKEY_F14		= 295,
	VKEY_F15		= 296,

	/* Key state modifier keys */
	VKEY_NUMLOCK		= 300,
	VKEY_CAPSLOCK		= 301,
	VKEY_SCROLLOCK		= 302,
	VKEY_RSHIFT		= 303,
	VKEY_LSHIFT		= 304,
	VKEY_RCTRL		= 305,
	VKEY_LCTRL		= 306,
	VKEY_RALT		= 307,
	VKEY_LALT		= 308,
	VKEY_RMETA		= 309,
	VKEY_LMETA		= 310,
	VKEY_LSUPER		= 311,		/* Left "Windows" key */
	VKEY_RSUPER		= 312,		/* Right "Windows" key */
	VKEY_MODE		= 313,		/* "Alt Gr" key */
	VKEY_COMPOSE		= 314,		/* Multi-key compose key */

	/* Miscellaneous function keys */
	VKEY_HELP		= 315,
	VKEY_PRINT		= 316,
	VKEY_SYSREQ		= 317,
	VKEY_BREAK		= 318,
	VKEY_MENU		= 319,

	VKEY_LAST
} VisKey;

/**
 * Enumerate values used within the libvisual event system to set modifier keys.
 * 
 * Values can ben ORred together.
 *
 * @see visual_event_queue_add_keyboard
 */
typedef enum {
	VKMOD_NONE	= 0x0000,
	VKMOD_LSHIFT	= 0x0001,
	VKMOD_RSHIFT	= 0x0002,
	VKMOD_LCTRL	= 0x0040,
	VKMOD_RCTRL	= 0x0080,
	VKMOD_LALT	= 0x0100,
	VKMOD_RALT	= 0x0200,
	VKMOD_LMETA	= 0x0400,
	VKMOD_RMETA	= 0x0800,
	VKMOD_NUM	= 0x1000,
	VKMOD_CAPS	= 0x2000,
	VKMOD_MODE	= 0x4000,
} VisKeyMod;

#define VKMOD_CTRL	(VKMOD_LCTRL  | VKMOD_RCTRL)
#define VKMOD_SHIFT	(VKMOD_LSHIFT | VKMOD_RSHIFT)
#define VKMOD_ALT	(VKMOD_LALT   | VKMOD_RALT)
#define VKMOD_META	(VKMOD_LMETA  | VKMOD_RMETA)

typedef struct _VisKeySym VisKeySym;

/**
 * Cantains data about the current keyboard state.
 */
struct _VisKeySym {
	VisKey		sym;	/**< Keyboard key to which everything relates. */
	int		mod;	/**< Modifier vlags, Using key modifiers from the VisKeyMod enumerate. */
};

VISUAL_END_DECLS

#endif /* _LV_KEYSYM_H */
