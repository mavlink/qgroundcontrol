/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_morph.h,v 1.17 2006/01/27 20:18:26 synap Exp $
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

#ifndef _LV_MORPH_H
#define _LV_MORPH_H

#include <libvisual/lv_palette.h>
#include <libvisual/lv_plugin.h>
#include <libvisual/lv_list.h>
#include <libvisual/lv_video.h>
#include <libvisual/lv_time.h>

VISUAL_BEGIN_DECLS

#define VISUAL_MORPH(obj)				(VISUAL_CHECK_CAST ((obj), VisMorph))
#define VISUAL_MORPH_PLUGIN(obj)			(VISUAL_CHECK_CAST ((obj), VisMorphPlugin))

/**
 * Type defination that should be used in plugins to set the plugin type for a morph plugin.
 */
#define VISUAL_PLUGIN_TYPE_MORPH	"Libvisual:core:morph"

/**
 * Morph morphing methods.
 */
typedef enum {
	VISUAL_MORPH_MODE_SET,		/**< Morphing is done by a rate set,
					  * nothing is automated here. */
	VISUAL_MORPH_MODE_STEPS,	/**< Morphing is done by setting a number of steps,
					  * the morph will be automated. */
	VISUAL_MORPH_MODE_TIME		/**< Morphing is done by setting a target time when the morph should be done,
					  * This is as well automated. */
} VisMorphMode;

typedef struct _VisMorph VisMorph;
typedef struct _VisMorphPlugin VisMorphPlugin;

/* Morph plugin methods */

/**
 * A morph plugin needs this signature for the palette function. The palette function
 * is used to give a palette for the morph. The palette function isn't mandatory and the
 * VisMorph system will interpolate between the two palettes in VISUAL_VIDEO_DEPTH_8BIT when
 * a palette function isn't set.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg rate A float between 0.0 and 1.0 that tells how far the morph has proceeded.
 * @arg audio Pointer to the VisAudio containing all the data regarding the current audio sample.
 * @arg pal A pointer to the target VisPalette in which the morph between the two palettes is saved. Should have
 * 	256 VisColor entries.
 * @arg src1 A pointer to the first VisVideo source.
 * @arg src2 A pointer to the second VisVideo source.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginMorphPaletteFunc)(VisPluginData *plugin, float rate, VisAudio *audio, VisPalette *pal,
		VisVideo *src1, VisVideo *src2);

/**
 * A morph plugin needs this signature for the apply function. The apply function
 * is used to execute a morph between two VisVideo sources. It's the 'render' function of
 * the morph plugin and here is the morphing done.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg rate A float between 0.0 and 1.0 that tells how far the morph has proceeded.
 * @arg audio Pointer to the VisAudio containing all the data regarding the current audio sample.
 * @arg src1 A pointer to the first VisVideo source.
 * @arg src2 A pointer to the second VisVideo source.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginMorphApplyFunc)(VisPluginData *plugin, float rate, VisAudio *audio, VisVideo *dest,
		VisVideo *src1, VisVideo *src2);

/**
 * The VisMorph structure encapsulates the morph plugin and provides 
 * abstract interfaces for morphing between actors, or rather between
 * two video sources.
 *
 * Members in the structure shouldn't be accessed directly but instead
 * it's adviced to use the methods provided.
 *
 * @see visual_morph_new
 */
struct _VisMorph {
	VisObject	 object;	/**< The VisObject data. */

	VisPluginData	*plugin;	/**< Pointer to the plugin itself. */
	VisVideo	*dest;		/**< Destination video, this is where
					 * the result of the morph gets drawn. */
	float		 rate;		/**< The rate of morph, 0 draws the first video source
					 * 1 the second video source, 0.5 is a 50/50, final
					 * content depends on the plugin being used. */
	VisPalette	 morphpal;	/**< Morph plugins can also set a palette for indexed
					 * color depths. */
	VisTime		 morphtime;	/**< Amount of time which the morphing should take. */
	VisTimer	 timer;		/**< Private entry that holds the time elapsed from 
					 * the beginning of the switch. */
	int		 steps;		/**< Private entry that contains the number of steps
					 * a morph suppose to take. */
	int		 stepsdone;	/**< Private entry that contains the number of steps done. */

	VisMorphMode	 mode;		/**< Private entry that holds the mode of morphing. */
};

/**
 * The VisMorphPlugin structure is the main data structure
 * for the morph plugin.
 *
 * The morph plugin is capable of morphing between two VisVideo
 * sources, and thus is capable of morphing between two
 * VisActors.
 */
struct _VisMorphPlugin {
	VisObject			 object;	/**< The VisObject data. */
	VisPluginMorphPaletteFunc	 palette;	/**< The plugin's palette function. This can be used
							  * to obtain a palette for VISUAL_VIDEO_DEPTH_8BIT surfaces.
							  * However the function may be set to NULL. In this case the
							  * VisMorph system morphs between palettes itself. */
	VisPluginMorphApplyFunc		 apply;		/**< The plugin it's main function. This is used to morph
							  * between two VisVideo sources. */
	int				 requests_audio;/**< When set on TRUE this will indicate that the Morph plugin
							  * requires an VisAudio context in order to render properly. */
	VisVideoAttributeOptions	 vidoptions;
};


/* prototypes */
VisPluginData *visual_morph_get_plugin (VisMorph *morph);

VisList *visual_morph_get_list (void);
const char *visual_morph_get_next_by_name (const char *name);
const char *visual_morph_get_prev_by_name (const char *name);
int visual_morph_valid_by_name (const char *name);

VisMorph *visual_morph_new (const char *morphname);
int visual_morph_init (VisMorph *morph, const char *morphname);

int visual_morph_realize (VisMorph *morph);

int visual_morph_get_supported_depth (VisMorph *morph);
VisVideoAttributeOptions *visual_morph_get_video_attribute_options (VisMorph *morph);

int visual_morph_set_video (VisMorph *morph, VisVideo *video);
int visual_morph_set_time (VisMorph *morph, VisTime *time);
int visual_morph_set_rate (VisMorph *morph, float rate);
int visual_morph_set_steps (VisMorph *morph, int steps);
int visual_morph_set_mode (VisMorph *morph, VisMorphMode mode);

VisPalette *visual_morph_get_palette (VisMorph *morph);

int visual_morph_is_done (VisMorph *morph);

int visual_morph_requests_audio (VisMorph *morph);

int visual_morph_run (VisMorph *morph, VisAudio *audio, VisVideo *src1, VisVideo *src2);

VISUAL_END_DECLS

#endif /* _LV_MORPH_H */
