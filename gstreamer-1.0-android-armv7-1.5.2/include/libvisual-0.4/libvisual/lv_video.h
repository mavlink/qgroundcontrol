/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *	    Duilio J. Protti <dprotti@users.sourceforge.net>
 *	    Chong Kai Xiong <descender@phreaker.net>
 *	    Jean-Christophe Hoelt <jeko@ios-software.com>
 *
 * $Id: lv_video.h,v 1.34.2.1 2006/03/04 12:32:48 descender Exp $
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

#ifndef _LV_VIDEO_H
#define _LV_VIDEO_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_palette.h>
#include <libvisual/lv_rectangle.h>
#include <libvisual/lv_buffer.h>
#include <libvisual/lv_gl.h>

VISUAL_BEGIN_DECLS

#define VISUAL_VIDEO(obj)				(VISUAL_CHECK_CAST ((obj), VisVideo))
#define VISUAL_VIDEO_ATTRIBUTE_OPTIONS(obj)		(VISUAL_CHECK_CAST ((obj), VisVideoAttributeOptions))

#define VISUAL_VIDEO_ATTRIBUTE_OPTIONS_GL_ENTRY(options, attr, val)	\
	options.gl_attributes[attr].attribute = attr;			\
	options.gl_attributes[attr].value = val;			\
	options.gl_attributes[attr].mutated = TRUE;


/* NOTE: The depth find helper code in lv_actor depends on an arrangment from low to high */
/**
 * Enumerate that defines video depths for use within plugins, libvisual functions, etc.
 */
typedef enum {
	VISUAL_VIDEO_DEPTH_NONE		= 0,	/**< No video surface flag. */
	VISUAL_VIDEO_DEPTH_8BIT		= 1,	/**< 8 bits indexed surface flag. */
	VISUAL_VIDEO_DEPTH_16BIT	= 2,	/**< 16 bits 5-6-5 surface flag. */
	VISUAL_VIDEO_DEPTH_24BIT	= 4,	/**< 24 bits surface flag. */
	VISUAL_VIDEO_DEPTH_32BIT	= 8,	/**< 32 bits surface flag. */
	VISUAL_VIDEO_DEPTH_GL		= 16,	/**< openGL surface flag. */
	VISUAL_VIDEO_DEPTH_ENDLIST	= 32,	/**< Used to mark the end of the depth list. */
	VISUAL_VIDEO_DEPTH_ERROR	= -1,	/**< Used when there is an error. */
	VISUAL_VIDEO_DEPTH_ALL		= VISUAL_VIDEO_DEPTH_8BIT |  \
					  VISUAL_VIDEO_DEPTH_16BIT | \
					  VISUAL_VIDEO_DEPTH_24BIT | \
					  VISUAL_VIDEO_DEPTH_32BIT | \
					  VISUAL_VIDEO_DEPTH_GL /**< All graphical depths. */
} VisVideoDepth;

/**
 * Enumerate that defines video rotate types, used with the visual_video_rotate_*() functions.
 */
typedef enum {
	VISUAL_VIDEO_ROTATE_NONE	= 0,	/**< No rotating. */
	VISUAL_VIDEO_ROTATE_90		= 1,	/**< 90 degrees rotate. */
	VISUAL_VIDEO_ROTATE_180		= 2,	/**< 180 degrees rotate. */
	VISUAL_VIDEO_ROTATE_270		= 3	/**< 270 degrees rotate. */
} VisVideoRotateDegrees;

/**
 * Enumerate that defines the video mirror types, used with the visual_video_mirror_*() functions.
 */
typedef enum {
	VISUAL_VIDEO_MIRROR_NONE	= 0,	/**< No mirroring. */
	VISUAL_VIDEO_MIRROR_X		= 1,	/**< Mirror on the X ax. */
	VISUAL_VIDEO_MIRROR_Y		= 2	/**< Mirror on the Y ax. */
} VisVideoMirrorOrient;

/**
 * Enumerate that defines the different methods of scaling within VisVideo.
 */
typedef enum {
	VISUAL_VIDEO_SCALE_NEAREST  = 0,    /**< Nearest neighbour. */
	VISUAL_VIDEO_SCALE_BILINEAR = 1	    /**< Bilinearly interpolated. */
} VisVideoScaleMethod;

/**
 * Enumerate that defines the different blitting methods for a VisVideo.
 */
typedef enum {
	VISUAL_VIDEO_COMPOSITE_TYPE_NONE = 0,		/**< No composite set, use default. */
	VISUAL_VIDEO_COMPOSITE_TYPE_SRC,		/**< Source alpha channel. */
	VISUAL_VIDEO_COMPOSITE_TYPE_COLORKEY,		/**< Colorkey alpha. */
	VISUAL_VIDEO_COMPOSITE_TYPE_SURFACE,		/**< One alpha channel for the complete surface. */
	VISUAL_VIDEO_COMPOSITE_TYPE_SURFACECOLORKEY,	/**< Use surface alpha on colorkey. */
	VISUAL_VIDEO_COMPOSITE_TYPE_CUSTOM		/**< Custom composite function (looks up on the source VisVideo. */
} VisVideoCompositeType;


typedef struct _VisVideo VisVideo;
typedef struct _VisVideoAttributeOptions VisVideoAttributeOptions;

/* VisVideo custom composite method */

/**
 * A custom composite function needs this signature. Custom composite functions can be
 * used to overload the normal libvisual overlay functions, these are used by the different
 * blit methods. The following template should be used for custom composite functions:
 *
 * int custom_composite (VisVideo *dest, VisVideo *src)
 * {
 *         int i
 *         uint8_t *destbuf = dest->pixels;
 *         uint8_t *srcbuf = src->pixels;
 * 
 *         for (i = 0; i < src->height; i++) {
 *                 for (j = 0; j < src->width; j++) {
 *	                   
 *	                   
 *	                   destbuf += dest->bpp;
 *	                   srcbuf += src->bpp;
 *                 }
 *
 *                 destbuf += dest->pitch - (dest->width * dest->bpp);
 *                 srcbuf += src->pitch - (src->width * src->bpp);
 *         }
 * }
 *
 * It's very important that the function is pitch (rowstride, bytes per line) aware, also
 * for width and height, it's compulsory to look at the source, and not the dest. Also be aware
 * that the custom composite function is correct for the depth you're using, if you want to add
 * depth awareness to the function, you could do this by checking dest->depth.
 *
 * @see visual_video_blit_overlay_rectangle_custom
 * @see visual_video_blit_overlay_rectangle_scale_custom
 * @see visual_video_blit_overlay_custom
 *
 * @arg dest a pointer to the dest visvideo source.
 * @arg src A pointer to the source VisVideo source.
 *
 * @return VISUAL_OK on succes -VISUAL_ERROR_GENERAL on error.
 */
typedef int (*VisVideoCustomCompositeFunc)(VisVideo *dest, VisVideo *src);

/**
 * Data structure that contains all the information about a screen surface.
 * Contains all the information regarding a screen surface like the current depth it's in,
 * width, height, bpp, the size in bytes it's pixel buffer is and the screen pitch.
 *
 * It also contains a pointer to the pixels and an optional pointer to the palette.
 *
 * Elements within the structure should be set using the VisVideo system it's methods.
 */
struct _VisVideo {
	VisObject			 object;	/**< The VisObject data. */

	VisVideoDepth			 depth;		/**< Surface it's depth. */
	int				 width;		/**< Surface it's width. */
	int				 height;	/**< Surface it's height. */
	int				 bpp;		/**< Surface it's bytes per pixel. */
	int				 pitch;		/**< Surface it's pitch value. Value contains
							 * the number of bytes per line. */
	VisBuffer			*buffer;	/**< The video buffer. */
	void				**pixel_rows;	/**< Pixel row start pointer table. */
	VisPalette			*pal;		/**< Optional pointer to the palette. */

	/* Sub region */
	VisVideo			*parent;	/**< The surface it's parent, ONLY when it is a subregion. */
	VisRectangle			 rect;		/**< The rectangle over the parent surface. */

	/* Composite control */
	VisVideoCompositeType		 compositetype;	/**< The surface it's composite type. */
	VisVideoCustomCompositeFunc	 compfunc;	/**< The surface it's custom composite function. */
	VisColor			 colorkey;	/**< The surface it's alpha colorkey. */
	uint8_t				 density;	/**< The surface it's global alpha density. */
};

struct _VisVideoAttributeOptions {
	VisObject			 object;

	int				 depth;

	VisGLAttributeEntry		 gl_attributes[VISUAL_GL_ATTRIBUTE_LAST];
};

/* prototypes */
VisVideo *visual_video_new (void);
int visual_video_init (VisVideo *video);
VisVideo *visual_video_new_with_buffer (int width, int height, VisVideoDepth depth);
int visual_video_free_buffer (VisVideo *video);
int visual_video_allocate_buffer (VisVideo *video);
int visual_video_have_allocated_buffer (VisVideo *video);
int visual_video_clone (VisVideo *dest, VisVideo *src);
int visual_video_compare (VisVideo *src1, VisVideo *src2);
int visual_video_compare_ignore_pitch (VisVideo *src1, VisVideo *src2);

int visual_video_set_palette (VisVideo *video, VisPalette *pal);
int visual_video_set_buffer (VisVideo *video, void *buffer);
int visual_video_set_dimension (VisVideo *video, int width, int height);
int visual_video_set_pitch (VisVideo *video, int pitch);
int visual_video_set_depth (VisVideo *video, VisVideoDepth depth);
int visual_video_set_attributes (VisVideo *video, int width, int height, int pitch, VisVideoDepth depth);

int visual_video_get_size (VisVideo *video);

void *visual_video_get_pixels (VisVideo *video);
VisBuffer *visual_video_get_buffer (VisVideo *video);

int visual_video_depth_is_supported (int depthflag, VisVideoDepth depth);
VisVideoDepth visual_video_depth_get_next (int depthflag, VisVideoDepth depth);
VisVideoDepth visual_video_depth_get_prev (int depthflag, VisVideoDepth depth);
VisVideoDepth visual_video_depth_get_lowest (int depthflag);
VisVideoDepth visual_video_depth_get_highest (int depthflag);
VisVideoDepth visual_video_depth_get_highest_nogl (int depthflag);
int visual_video_depth_is_sane (VisVideoDepth depth);
int visual_video_depth_value_from_enum (VisVideoDepth depth);
VisVideoDepth visual_video_depth_enum_from_value (int depthvalue);

int visual_video_bpp_from_depth (VisVideoDepth depth);

int visual_video_get_boundary (VisVideo *video, VisRectangle *rect);

int visual_video_region_sub (VisVideo *dest, VisVideo *src, VisRectangle *rect);
int visual_video_region_sub_by_values (VisVideo *dest, VisVideo *src, int x, int y, int width, int height);
int visual_video_region_sub_all (VisVideo *dest, VisVideo *src);
int visual_video_region_sub_with_boundary (VisVideo *dest, VisRectangle *drect, VisVideo *src, VisRectangle *srect);

int visual_video_composite_set_type (VisVideo *video, VisVideoCompositeType type);
int visual_video_composite_set_colorkey (VisVideo *video, VisColor *color);
int visual_video_composite_set_surface (VisVideo *video, uint8_t alpha);
VisVideoCustomCompositeFunc visual_video_composite_get_function (VisVideo *dest, VisVideo *src, int alpha);
int visual_video_composite_set_function (VisVideo *video, VisVideoCustomCompositeFunc compfunc);

int visual_video_blit_overlay_rectangle (VisVideo *dest, VisRectangle *drect, VisVideo *src, VisRectangle *srect, int alpha);
int visual_video_blit_overlay_rectangle_custom (VisVideo *dest, VisRectangle *drect, VisVideo *src, VisRectangle *srect,
		VisVideoCustomCompositeFunc compfunc);
int visual_video_blit_overlay_rectangle_scale (VisVideo *dest, VisRectangle *drect, VisVideo *src, VisRectangle *srect,
		int alpha, VisVideoScaleMethod scale_method);
int visual_video_blit_overlay_rectangle_scale_custom (VisVideo *dest, VisRectangle *drect, VisVideo *src, VisRectangle *srect,
		VisVideoScaleMethod scale_method, VisVideoCustomCompositeFunc compfunc);
int visual_video_blit_overlay (VisVideo *dest, VisVideo *src, int x, int y, int alpha);
int visual_video_blit_overlay_custom (VisVideo *dest, VisVideo *src, int x, int y, VisVideoCustomCompositeFunc compfunc);

int visual_video_fill_alpha_color (VisVideo *video, VisColor *color, uint8_t density);
int visual_video_fill_alpha (VisVideo *video, uint8_t density);
int visual_video_fill_alpha_rectangle (VisVideo *video, uint8_t density, VisRectangle *rect);
int visual_video_fill_color (VisVideo *video, VisColor *color);
int visual_video_fill_color_rectangle (VisVideo *video, VisColor *color, VisRectangle *rect);

int visual_video_color_bgr_to_rgb (VisVideo *dest, VisVideo *src);

int visual_video_rotate (VisVideo *dest, VisVideo *src, VisVideoRotateDegrees degrees);
VisVideo *visual_video_rotate_new (VisVideo *src, VisVideoRotateDegrees degrees);

int visual_video_mirror (VisVideo *dest, VisVideo *src, VisVideoMirrorOrient orient);
VisVideo *visual_video_mirror_new (VisVideo *src, VisVideoMirrorOrient orient);

int visual_video_depth_transform (VisVideo *viddest, VisVideo *vidsrc);

VisVideo *visual_video_zoom_new (VisVideo *src, VisVideoScaleMethod scale_method, float zoom_factor);
int visual_video_zoom_double (VisVideo *dest, VisVideo *src);

int visual_video_scale (VisVideo *dest, VisVideo *src, VisVideoScaleMethod scale_method);
VisVideo *visual_video_scale_new (VisVideo *src, int width, int height, VisVideoScaleMethod scale_method);
int visual_video_scale_depth (VisVideo *dest, VisVideo *src, VisVideoScaleMethod scale_method);
VisVideo *visual_video_scale_depth_new (VisVideo *src, int width, int height, VisVideoDepth depth,
		VisVideoScaleMethod scale_method);

/* Optimized versions of performance sensitive routines */
/* mmx from lv_video_simd.c */ /* FIXME can we do this nicer ? */
int _lv_blit_overlay_alphasrc_mmx (VisVideo *dest, VisVideo *src);
int _lv_scale_bilinear_32_mmx (VisVideo *dest, VisVideo *src);

VISUAL_END_DECLS

#endif /* _LV_VIDEO_H */
