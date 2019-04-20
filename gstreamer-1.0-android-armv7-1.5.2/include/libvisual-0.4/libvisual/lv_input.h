/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_input.h,v 1.17 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_INPUT_H
#define _LV_INPUT_H

#include <libvisual/lv_audio.h>
#include <libvisual/lv_plugin.h>
#include <libvisual/lv_common.h>

VISUAL_BEGIN_DECLS

#define VISUAL_INPUT(obj)				(VISUAL_CHECK_CAST ((obj), VisInput))
#define VISUAL_INPUT_PLUGIN(obj)			(VISUAL_CHECK_CAST ((obj), VisInputPlugin))

/**
 * Type defination that should be used in plugins to set the plugin type for an input  plugin.
 */
#define VISUAL_PLUGIN_TYPE_INPUT	"Libvisual:core:input"

typedef struct _VisInput VisInput;
typedef struct _VisInputPlugin VisInputPlugin;

/**
 * Callback function that is set using visual_input_set_callback should use this signature.
 *
 * @see visual_input_set_callback
 *
 * @arg input Pointer to the VisInput structure.
 *
 * @arg audio Pointer to the VisAudio containing all the audio information, and in which
 * data needs to be set within the callback.
 *
 * @arg priv Private field to be used by the client. The library will never touch this.
 */
typedef int (*VisInputUploadCallbackFunc)(VisInput *input, VisAudio *audio, void *priv);

/* Input plugin methods */

/**
 * An input plugin needs this signature for the sample upload function. The sample upload function
 * is used to retrieve sample information when a input is being used to retrieve the
 * audio sample.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg audio Pointer to the VisAudio in which the new sample data is set.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginInputUploadFunc)(VisPluginData *plugin, VisAudio *audio);

/**
 * The VisInput structure encapsulates the input plugin and provides
 * abstract interfaces to the input. The VisInput system provides
 * PCM data to the visualisation elements of libvisual. This can be done
 * through both plugins and callback functions.
 *
 * Members in the structure shouldn't be accessed directly but instead
 * it's adviced to use the methods provided.
 *
 * @see visual_input_new
 */ 
struct _VisInput {
	VisObject			 object;	/**< The VisObject data. */

	VisPluginData			*plugin;	/**< Pointer to the plugin itself. */
	VisAudio			*audio;		/**< Pointer to the VisAudio structure
							  * that contains the audio analyse
							  * results.
							  * @see visual_audio_analyse */
	VisInputUploadCallbackFunc	 callback;	/**< Callback function when a callback
							  * is used instead of a plugin. */
};

/**
 * The VisInputPlugin structure is the main data structure
 * for the input plugin.
 *
 * The input plugin is used to retrieve PCM samples from
 * certain sources.
 */
struct _VisInputPlugin {
	VisObject			 object;	/**< The VisObject data. */
	VisPluginInputUploadFunc	 upload;	/**< The sample upload function. This is the main function
							  * of the plugin which uploads sample data into
							  * libvisual. */
};

/* prototypes */
VisPluginData *visual_input_get_plugin (VisInput *input);

VisList *visual_input_get_list (void);
const char *visual_input_get_next_by_name (const char *name);
const char *visual_input_get_prev_by_name (const char *name);
int visual_input_valid_by_name (const char *name);

VisInput *visual_input_new (const char *inputname);
int visual_input_init (VisInput *input, const char *inputname);

int visual_input_realize (VisInput *input);

int visual_input_set_callback (VisInput *input, VisInputUploadCallbackFunc callback, void *priv);

int visual_input_run (VisInput *input);

VISUAL_END_DECLS

#endif /* _LV_INPUT_H */
