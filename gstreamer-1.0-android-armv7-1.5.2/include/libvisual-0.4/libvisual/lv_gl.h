/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_gl.h,v 1.5.2.1 2006/03/04 12:32:47 descender Exp $
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

#ifndef _LV_GL_H
#define _LV_GL_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_hashmap.h>

VISUAL_BEGIN_DECLS

#define VISUAL_GL_ATTRIBUTE_ENTRY(obj)			(VISUAL_CHECK_CAST ((obj), VisGLAttributeEntry))

/**
 * Enumerate with GL attributes.
 */
typedef enum {
	VISUAL_GL_ATTRIBUTE_NONE = 0,		/**< No attribute. */
	VISUAL_GL_ATTRIBUTE_BUFFER_SIZE,	/**< Depth of the color buffer. */
	VISUAL_GL_ATTRIBUTE_LEVEL,		/**< Level in plane stacking. */
	VISUAL_GL_ATTRIBUTE_RGBA,		/**< True if RGBA mode. */
	VISUAL_GL_ATTRIBUTE_DOUBLEBUFFER,	/**< Double buffering supported. */
	VISUAL_GL_ATTRIBUTE_STEREO,		/**< Stereo buffering supported. */
	VISUAL_GL_ATTRIBUTE_AUX_BUFFERS,	/**< Number of aux buffers. */
	VISUAL_GL_ATTRIBUTE_RED_SIZE,		/**< Number of red component bits. */
	VISUAL_GL_ATTRIBUTE_GREEN_SIZE,		/**< Number of green component bits. */
	VISUAL_GL_ATTRIBUTE_BLUE_SIZE,		/**< Number of blue component bits. */
	VISUAL_GL_ATTRIBUTE_ALPHA_SIZE,		/**< Number of alpha component bits. */
	VISUAL_GL_ATTRIBUTE_DEPTH_SIZE,		/**< Number of depth bits. */
	VISUAL_GL_ATTRIBUTE_STENCIL_SIZE,	/**< Number of stencil bits. */
	VISUAL_GL_ATTRIBUTE_ACCUM_RED_SIZE,	/**< Number of red accum bits. */
	VISUAL_GL_ATTRIBUTE_ACCUM_GREEN_SIZE,	/**< Number of green accum bits. */
	VISUAL_GL_ATTRIBUTE_ACCUM_BLUE_SIZE,	/**< Number of blue accum bits. */
	VISUAL_GL_ATTRIBUTE_ACCUM_ALPHA_SIZE,	/**< Number of alpha accum bits. */
	VISUAL_GL_ATTRIBUTE_LAST
} VisGLAttribute;


typedef struct _VisGLAttributeEntry VisGLAttributeEntry;

struct _VisGLAttributeEntry {
	VisGLAttribute	attribute;
	int		value;
	int		mutated;
};

/* prototypes */
void *visual_gl_get_proc_address (char *procname);

VISUAL_END_DECLS

#endif /* _LV_GL_H */
