/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_plugin.h,v 1.51 2006/02/13 20:54:08 synap Exp $
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

#ifndef _LV_PLUGIN_H
#define _LV_PLUGIN_H

#include <libvisual/lv_video.h>
#include <libvisual/lv_audio.h>
#include <libvisual/lv_palette.h>
#include <libvisual/lv_list.h>
#include <libvisual/lv_songinfo.h>
#include <libvisual/lv_event.h>
#include <libvisual/lv_param.h>
#include <libvisual/lv_ui.h>
#include <libvisual/lv_random.h>
#include <libvisual/lv_types.h>

#include "lvconfig.h"

#if defined(VISUAL_OS_WIN32)
#include <windows.h>
#endif

VISUAL_BEGIN_DECLS

#define VISUAL_PLUGINREF(obj)				(VISUAL_CHECK_CAST ((obj), VisPluginRef))
#define VISUAL_PLUGININFO(obj)				(VISUAL_CHECK_CAST ((obj), VisPluginInfo))
#define VISUAL_PLUGINDATA(obj)				(VISUAL_CHECK_CAST ((obj), VisPluginData))
#define VISUAL_PLUGINENVIRON(obj)			(VISUAL_CHECK_CAST ((obj), VisPluginEnviron))

/**
 * Indicates at which version the plugin API is.
 */
#define VISUAL_PLUGIN_API_VERSION	3004

/**
 * Defination that should be used in plugins to set the plugin type for a NULL plugin.
 */
#define VISUAL_PLUGIN_TYPE_NULL		"Libvisual:core:null"

/**
 * Standard defination for GPL plugins, use this for the .license entry in VisPluginInfo
 */
#define VISUAL_PLUGIN_LICENSE_GPL	"GPL"
/**
 * Standard defination for LGPL plugins, use this for the .license entry in VisPluginInfo
 */
#define VISUAL_PLUGIN_LICENSE_LGPL	"LGPL"
/**
 * Standard defination for BSD plugins, use this for the .license entry in VisPluginInfo
 */
#define VISUAL_PLUGIN_LICENSE_BSD	"BSD"

#define VISUAL_PLUGIN_VERSION_TAG		"__lv_plugin_libvisual_api_version"
#define VISUAL_PLUGIN_API_VERSION_VALIDATOR	VISUAL_C_LINKAGE const int __lv_plugin_libvisual_api_version = \
						VISUAL_PLUGIN_API_VERSION;


/**
 * Enumerate to define the plugin flags. Plugin flags can be used to
 * define some of the plugin it's behavior.
 */
typedef enum {
	VISUAL_PLUGIN_FLAG_NONE			= 0,	/**< Used to set no flags. */
	VISUAL_PLUGIN_FLAG_NOT_REENTRANT	= 1,	/**< Used to tell the plugin loader that this plugin
							  * is not reentrant, and can be loaded only once. */
	VISUAL_PLUGIN_FLAG_SPECIAL		= 2	/**< Used to tell the plugin loader that this plugin has
							  * special purpose, like the GdkPixbuf plugin, or a webcam
							  * plugin. */
} VisPluginFlags;

/**
 * Enumerate to check the depth of the type wildcard/defination used, used together with the visual_plugin_type functions.
 */
typedef enum {
	VISUAL_PLUGIN_TYPE_DEPTH_NONE		= 0,	/**< No type found.*/
	VISUAL_PLUGIN_TYPE_DEPTH_DOMAIN		= 1,	/**< Only domain in type. */
	VISUAL_PLUGIN_TYPE_DEPTH_PACKAGE	= 2,	/**< Domain and package in type. */
	VISUAL_PLUGIN_TYPE_DEPTH_TYPE		= 3,	/**< Domain, package and type found in type. */
} VisPluginTypeDepth;

typedef struct _VisPluginRef VisPluginRef;
typedef struct _VisPluginInfo VisPluginInfo;
typedef struct _VisPluginData VisPluginData;
typedef struct _VisPluginEnviron VisPluginEnviron;


/* Plugin standard get_plugin_info method */

/**
 * This is the signature for the 'get_plugin_info' function every libvisual plugin needs to have. The 
 * 'get_plugin_info' function provides libvisual plugin data and all the detailed information regarding
 * the plugin. This function is compulsory without it libvisual won't load the plugin.
 *
 * @arg count An int pointer in which the number of VisPluginData entries within the plugin. Plugins can have
 * 	multiple 'features' and thus the count is needed.
 *
 * @return Pointer to the VisPluginInfo array which contains information about the plugin.
 */
typedef const VisPluginInfo *(*VisPluginGetInfoFunc)(int *count);

/* Standard plugin methods */

/**
 * Every libvisual plugin that is loaded by the libvisual plugin loader needs this signature for it's
 * intialize function.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginInitFunc)(VisPluginData *plugin);

/**
 * Every libvisual plugin that is loaded by the libvisual plugin loader needs this signature for it's
 * cleanup function.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginCleanupFunc)(VisPluginData *plugin);

/**
 * This is the signature for the event handler within libvisual plugins. An event handler is not mandatory because
 * it has no use in some plugin classes but some plugin types require it nonetheless.
 *
 * @arg plugin Pointer to the VisPluginData instance structure.
 * @arg events Pointer to the VisEventQueue that might contain events that need to be handled.
 *
 * @return 0 on succes -1 on error.
 */
typedef int (*VisPluginEventsFunc)(VisPluginData *plugin, VisEventQueue *events);

/**
 * The VisPluginRef data structure contains information about the plugins
 * and does refcounting. It is also used as entries in the plugin registry.
 */
struct _VisPluginRef {
	VisObject		 object;	/**< The VisObject data. */

	char			*file;		/**< The file location of the plugin. */
	int			 index;		/**< Contains the index number for the entry in the VisPluginInfo table. */
	int			 usecount;	/**< The use count, this indicates how many instances are loaded. */
	VisPluginInfo		*info;		/**< A copy of the VisPluginInfo structure. */
};

/**
 * The VisPluginInfo data structure contains information about a plugin
 * and is filled within the plugin itself.
 */
struct _VisPluginInfo {
	VisObject		 object;	/**< The VisObject data. */

	char			*type;		/**< Plugin type, in the format of "domain:package:type", as example,
						 * this could be "Libvisual:core:actor". It's adviced to use the defination macros here
						 * instead of filling in the string yourself. */
	char			*plugname;	/**< The plugin name as it's saved in the registry. */

	char			*name;		/**< Long name */
	char			*author;	/**< Author */
	char			*version;	/**< Version */
	char			*about;		/**< About */
	char			*help;		/**< Help */
	char			*license;	/**< License */

	VisPluginInitFunc	 init;		/**< The standard init function, every plugin has to implement this. */
	VisPluginCleanupFunc	 cleanup;	/**< The standard cleanup function, every plugin has to implement this. */
	VisPluginEventsFunc	 events;	/**< The standard event function, implementation is optional. */

	int			 flags;		/**< Plugin flags from the VisPluginFlags enumerate. */

	VisObject		*plugin;	/**< Pointer to the plugin specific data structures. */
};

/**
 * The VisPluginData structure is the main plugin structure, every plugin
 * is encapsulated in this.
 */
struct _VisPluginData {
	VisObject		 object;	/**< The VisObject data. */

	VisPluginRef		*ref;		/**< Pointer to the plugin references corresponding to this VisPluginData. */

	VisPluginInfo		*info;		/**< Pointer to the VisPluginInfo that is obtained from the plugin. */

	VisEventQueue		 eventqueue;	/**< The plugin it's VisEventQueue for queueing events. */
	VisParamContainer	*params;	/**< The plugin it's VisParamContainer in which VisParamEntries can be placed. */
	VisUIWidget		*userinterface;	/**< The plugin it's top level VisUIWidget, this acts as the container for the
						  * rest of the user interface. */

	int			 plugflags;	/**< Plugin flags, currently unused but will be used in the future. */

	VisRandomContext	 random;	/**< Pointer to the plugin it's private random context. It's highly adviced to use
						  * the plugin it's randomize functions. The reason is so more advanced apps can
						  * semi reproduce visuals. */

	int			 realized;	/**< Flag that indicates if the plugin is realized. */
#if defined(VISUAL_OS_WIN32)
	HMODULE			 handle;	/**< The LoadLibrary handle for windows32 */
#else /* !VISUAL_OS_WIN32 */
	void			*handle;	/**< The dlopen handle */
#endif

	VisList			 environment;	/**< Misc environment specific data. */
};

/**
 * The VisPluginEnviron is used to setup a pre realize/init environment for plugins.
 * Some types of plugins might need this internally and thus this system provides this function.
 */
struct _VisPluginEnviron {
	VisObject		 object;	/**< The VisObject data. */

	const char		*type;		/**< Almost the same as _VisPluginInfo.type. */

	VisObject		*environment;	/**< VisObject that contains environ specific data. */
};

/* prototypes */
VisPluginInfo *visual_plugin_info_new (void);
int visual_plugin_info_copy (VisPluginInfo *dest, VisPluginInfo *src);

int visual_plugin_events_pump (VisPluginData *plugin);
VisEventQueue *visual_plugin_get_eventqueue (VisPluginData *plugin);
int visual_plugin_set_userinterface (VisPluginData *plugin, VisUIWidget *widget);
VisUIWidget *visual_plugin_get_userinterface (VisPluginData *plugin);

VisPluginInfo *visual_plugin_get_info (VisPluginData *plugin);

VisParamContainer *visual_plugin_get_params (VisPluginData *plugin);

VisRandomContext *visual_plugin_get_random_context (VisPluginData *plugin);

void *visual_plugin_get_specific (VisPluginData *plugin);

VisPluginRef *visual_plugin_ref_new (void);

VisPluginData *visual_plugin_new (void);

VisList *visual_plugin_get_registry (void);
VisList *visual_plugin_registry_filter (VisList *pluglist, const char *domain);

const char *visual_plugin_get_next_by_name (VisList *list, const char *name);
const char *visual_plugin_get_prev_by_name (VisList *list, const char *name);

int visual_plugin_unload (VisPluginData *plugin);
VisPluginData *visual_plugin_load (VisPluginRef *ref);
int visual_plugin_realize (VisPluginData *plugin);

VisPluginRef **visual_plugin_get_references (const char *pluginpath, int *count);
VisList *visual_plugin_get_list (const char **paths, int ignore_non_existing);

VisPluginRef *visual_plugin_find (VisList *list, const char *name);

int visual_plugin_get_api_version (void);

const char *visual_plugin_type_get_domain (const char *type);
const char *visual_plugin_type_get_package (const char *type);
const char *visual_plugin_type_get_type (const char *type);
VisPluginTypeDepth visual_plugin_type_get_depth (const char *type);
int visual_plugin_type_member_of (const char *domain, const char *type);
const char *visual_plugin_type_get_flags (const char *type);
int visual_plugin_type_has_flag (const char *type, const char *flag);

VisPluginEnviron *visual_plugin_environ_new (const char *type, VisObject *envobj);
int visual_plugin_environ_add (VisPluginData *plugin, VisPluginEnviron *enve);
int visual_plugin_environ_remove (VisPluginData *plugin, const char *type);
VisObject *visual_plugin_environ_get (VisPluginData *plugin, const char *type);

VISUAL_END_DECLS

#endif /* _LV_PLUGIN_H */
