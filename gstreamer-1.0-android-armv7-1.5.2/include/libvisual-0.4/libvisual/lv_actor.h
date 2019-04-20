/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_actor.h,v 1.19 2006/01/27 20:18:26 synap Exp $
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

#ifndef _LV_ACTOR_H
#define _LV_ACTOR_H

#include <libvisual/lv_audio.h>
#include <libvisual/lv_video.h>
#include <libvisual/lv_palette.h>
#include <libvisual/lv_plugin.h>
#include <libvisual/lv_songinfo.h>
#include <libvisual/lv_event.h>

VISUAL_BEGIN_DECLS

#define VISUAL_ACTOR(obj)				(VISUAL_CHECK_CAST ((obj), VisActor))
#define VISUAL_ACTOR_PLUGINENVIRON(obj)			(VISUAL_CHECK_CAST ((obj), VisActorPluginEnviron))
#define VISUAL_ACTOR_PLUGIN(obj)			(VISUAL_CHECK_CAST ((obj), VisActorPlugin))

/**
 * Type defination that should be used in plugins to set the plugin type for an actor plugin.
 */
#define VISUAL_PLUGIN_TYPE_ACTOR	"Libvisual:core:actor"

/**
 * Name defination of the standard VisActorPluginEnviron element for an actor plugin.
 */
#define VISUAL_ACTOR_PLUGIN_ENVIRON	"Libvisual:core:actor:environ"

typedef struct _VisActor VisActor;
typedef struct _VisActorPluginEnviron VisActorPluginEnviron;
typedef struct _VisActorPlugin VisActorPlugin;

/* Actor plugin methods */

/**
 * An actor plugin needs this signature for the requisition function. The requisition function
 * is used to determine the size required by the plugin for a given width/height value.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg width Pointer to an int containing the width requested, will be altered to the nearest
 * 	supported width.
 * @arg height Pointer to an int containing the height requested, will be altered to the nearest
 * 	supported height.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginActorRequisitionFunc)(VisPluginData *plugin, int *width, int *height);

/**
 * An actor plugin needs this signature for the palette function. The palette function
 * is used to retrieve the desired palette from the plugin.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 *
 * @return Pointer to the VisPalette used by the plugin, this should be a VisPalette with 256
 *	VisColor entries, NULL is also allowed to be returned.
 */
typedef VisPalette *(*VisPluginActorPaletteFunc)(VisPluginData *plugin);

/**
 * An actor plugin needs this signature for the render function. The render function
 * is used to render the frame for the visualisation.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg video Pointer to the VisVideo containing all information about the display surface.
 *	Params like height and width won't suddenly change, this is always notified as an event
 *	so the plugin can adjust to the new dimension.
 * @arg audio Pointer to the VisAudio containing all the data regarding the current audio sample.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginActorRenderFunc)(VisPluginData *plugin, VisVideo *video, VisAudio *audio);

/**
 * The VisActor structure encapsulates the actor plugin and provides
 * abstract interfaces to the actor. The VisActor system 
 * it's methods are also capable of doing automatic size fitting 
 * and depth transformations, and it keeps track on songinfo and events.
 *
 * Members in the structure shouldn't be accessed directly but instead
 * it's adviced to use the methods provided.
 *
 * @see visual_actor_new
 */
struct _VisActor {
	VisObject	 object;		/**< The VisObject data. */

	VisPluginData	*plugin;		/**< Pointer to the plugin itself. */

	/* Video management and fake environments when needed */
	VisVideo	*video;			/**< Pointer to the target display video. 
						 * @see visual_actor_set_video */
	VisVideo	*transform;		/**< Private member which is used for depth transformation. */
	VisVideo	*fitting;		/**< Private member which is used to fit the plugin. */
	VisPalette	*ditherpal;		/**< Private member in which a palette is set when transforming
						 * depth from true color to indexed.
						 * @see visual_actor_get_palette */

	/* Songinfo management */
	VisSongInfo	 songcompare;		/**< Private member which is used to compare with new songinfo
						  * to check if a new song event should be emitted. */
};

/**
 * The VisActorPluginEnviron structure is the main environmental element for a VisActorPlugin. The environmental name
 * is stored in the VISUAL_ACTOR_PLUGIN_ENVIRON define. The structure is used to set environmental data like, 
 * desired frames per second. The VisActorPluginEnviron element should be polled by either libvisual-display 
 * or a custom target to check for changes.
 */
struct _VisActorPluginEnviron {
	VisObject			object;		/**< The VisObject data. */

	int				fps;		/**< The desired fps, set by the plugin, optionally read by
							 * the display target. */
};

/**
 * The VisActorPlugin structure is the main data structure
 * for the actor (visualisation) plugin.
 *
 * The actor plugin is the visualisation plugin.
 */
struct _VisActorPlugin {
	VisObject			 object;	/**< The VisObject data. */
	VisPluginActorRequisitionFunc	 requisition;	/**< The requisition function. This is used to
							 * get the desired VisVideo surface size of the plugin. */
	VisPluginActorPaletteFunc	 palette;	/**< Used to retrieve the desired palette from the plugin. */
	VisPluginActorRenderFunc	 render;	/**< The main render loop. This is called to draw a frame. */

	VisSongInfo			 songinfo;	/**< Pointer to VisSongInfo that contains information about
							 *the current playing song. This can be NULL. */

	VisVideoAttributeOptions	 vidoptions;
};

/* prototypes */
VisPluginData *visual_actor_get_plugin (VisActor *actor);

VisList *visual_actor_get_list (void);
const char *visual_actor_get_next_by_name_gl (const char *name);
const char *visual_actor_get_prev_by_name_gl (const char *name);
const char *visual_actor_get_next_by_name_nogl (const char *name);
const char *visual_actor_get_prev_by_name_nogl (const char *name);
const char *visual_actor_get_next_by_name (const char *name);
const char *visual_actor_get_prev_by_name (const char *name);
int visual_actor_valid_by_name (const char *name);

VisActor *visual_actor_new (const char *actorname);
int visual_actor_init (VisActor *actor, const char *actorname);

int visual_actor_realize (VisActor *actor);

VisSongInfo *visual_actor_get_songinfo (VisActor *actor);
VisPalette *visual_actor_get_palette (VisActor *actor);

int visual_actor_video_negotiate (VisActor *actor, int rundepth, int noevent, int forced);

int visual_actor_get_supported_depth (VisActor *actor);
VisVideoAttributeOptions *visual_actor_get_video_attribute_options (VisActor *actor);

int visual_actor_set_video (VisActor *actor, VisVideo *video);

int visual_actor_run (VisActor *actor, VisAudio *audio);

VISUAL_END_DECLS

#endif /* _LV_ACTOR_H */
