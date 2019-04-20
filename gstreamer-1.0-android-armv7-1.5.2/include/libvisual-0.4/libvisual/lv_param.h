/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_param.h,v 1.32 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_PARAM_H
#define _LV_PARAM_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_color.h>
#include <libvisual/lv_palette.h>
#include <libvisual/lv_list.h>
#include <libvisual/lv_event.h>

VISUAL_BEGIN_DECLS

#define VISUAL_PARAMCONTAINER(obj)			(VISUAL_CHECK_CAST ((obj), VisParamContainer))
#define VISUAL_PARAMENTRY_CALLBACK(obj)			(VISUAL_CHECK_CAST ((obj), VisParamEntryCallback))
#define VISUAL_PARAMENTRY(obj)				(VISUAL_CHECK_CAST ((obj), VisParamEntry))

/* Use 0 for pointers instead of NULL because of C++ programs shocking on ((void *) 0) */
#define VISUAL_PARAM_LIST_ENTRY(name)			{ {}, 0, name, VISUAL_PARAM_ENTRY_TYPE_NULL }
#define VISUAL_PARAM_LIST_ENTRY_STRING(name, string)	{ {}, 0, name, VISUAL_PARAM_ENTRY_TYPE_STRING, string, {0, 0, 0}}
#define VISUAL_PARAM_LIST_ENTRY_INTEGER(name, val)	{ {}, 0, name, VISUAL_PARAM_ENTRY_TYPE_INTEGER, 0, {val, 0, 0}}
#define VISUAL_PARAM_LIST_ENTRY_FLOAT(name, val)	{ {}, 0, name, VISUAL_PARAM_ENTRY_TYPE_FLOAT, 0, {0, val, 0}}
#define VISUAL_PARAM_LIST_ENTRY_DOUBLE(name, val)	{ {}, 0, name, VISUAL_PARAM_ENTRY_TYPE_DOUBLE, 0, {0, 0, val}}
#define VISUAL_PARAM_LIST_ENTRY_COLOR(name, r, g, b)	{ {}, 0, name, VISUAL_PARAM_ENTRY_TYPE_COLOR, 0, {0, 0, 0}, {{}, r, g, b, 0}}
#define VISUAL_PARAM_LIST_END				{ {}, 0, 0, VISUAL_PARAM_ENTRY_TYPE_END }

#define VISUAL_PARAM_CALLBACK_ID_MAX	2147483647

/**
 * Different types of parameters that can be used.
 */
typedef enum {
	VISUAL_PARAM_ENTRY_TYPE_NULL,		/**< No parameter. */
	VISUAL_PARAM_ENTRY_TYPE_STRING,		/**< String parameter. */
	VISUAL_PARAM_ENTRY_TYPE_INTEGER,	/**< Integer parameter. */
	VISUAL_PARAM_ENTRY_TYPE_FLOAT,		/**< Floating point parameter. */
	VISUAL_PARAM_ENTRY_TYPE_DOUBLE,		/**< Double floating point parameter. */
	VISUAL_PARAM_ENTRY_TYPE_COLOR,		/**< VisColor parameter. */
	VISUAL_PARAM_ENTRY_TYPE_PALETTE,	/**< VisPalette parameter. */
	VISUAL_PARAM_ENTRY_TYPE_OBJECT,		/**< VisObject parameter. */
	VISUAL_PARAM_ENTRY_TYPE_END		/**< List end, and used as terminator for VisParamEntry lists. */
} VisParamEntryType;

typedef struct _VisParamContainer VisParamContainer;
typedef struct _VisParamEntryCallback VisParamEntryCallback;
typedef struct _VisParamEntry VisParamEntry;

/**
 * The param changed callback is used to be able to notify of changes within parameters. This should
 * not be used within the plugin itself, instead use the event queue there. This is so it's possible to
 * notify of changes outside the plugin. For example, this is needed by VisUI.
 * 
 * @arg param Pointer to the param that has been changed, and to which the callback was set.
 * @arg priv Private argument, that can be set when adding the callback to the callback list.
 */
typedef void (*VisParamChangedCallbackFunc)(VisParamEntry *param, void *priv);


/**
 * Parameter container, is the container for a set of parameters.
 *
 * All members should never be accessed directly, instead methods should be used.
 */
struct _VisParamContainer {
	VisObject	 object;	/**< The VisObject data. */
	VisList		 entries;	/**< The list that contains all the parameters. */
	VisEventQueue	*eventqueue;	/**< Pointer to an optional eventqueue to which events can be emitted
					  * on parameter changes. */
};

/**
 * A parameter callback entry, used for change notification callbacks.
 */
struct _VisParamEntryCallback {
	VisObject			 object;	/**< The VisObject data. */
	int				 id;		/**< Callback ID. */
	VisParamChangedCallbackFunc	 callback;	/**< The param change callback function. */
};

/**
 * A parameter entry, used for plugin parameters and such.
 *
 * All members should never be accessed directly, instead methods should be used.
 */
struct _VisParamEntry {
	VisObject		 object;	/**< The VisObject data. */
	VisParamContainer	*parent;	/**< Parameter container in which the param entry is encapsulated. */
	char			*name;		/**< Parameter name. */
	VisParamEntryType	 type;		/**< Parameter type. */

	char			*string;	/**< String data. */

	/* No union, we can't choose a member of the union using static initializers */
	struct {
		int		 integer;		/**< Integer data. */
		float		 floating;		/**< Floating point data. */
		double		 doubleflt;		/**< Double floating point data. */
	} numeric;

	VisColor		 color;		/**< VisColor data. */
	VisPalette		 pal;		/**< VisPalette data. */
	VisObject		*objdata;	/**< VisObject data for a VisObject parameter. */

	VisList			 callbacks;	/**< The change notify callbacks. */
};

/* prototypes */
VisParamContainer *visual_param_container_new (void);
int visual_param_container_set_eventqueue (VisParamContainer *paramcontainer, VisEventQueue *eventqueue);
VisEventQueue *visual_param_container_get_eventqueue (VisParamContainer *paramcontainer);

int visual_param_container_add (VisParamContainer *paramcontainer, VisParamEntry *param);
int visual_param_container_add_many (VisParamContainer *paramcontainer, VisParamEntry *params);
int visual_param_container_remove (VisParamContainer *paramcontainer, const char *name);
int visual_param_container_copy (VisParamContainer *destcont, VisParamContainer *srccont);
int visual_param_container_copy_match (VisParamContainer *destcont, VisParamContainer *srccont);
VisParamEntry *visual_param_container_get (VisParamContainer *paramcontainer, const char *name);

VisParamEntry *visual_param_entry_new (char *name);
int visual_param_entry_add_callback (VisParamEntry *param, VisParamChangedCallbackFunc callback, void *priv);
int visual_param_entry_remove_callback (VisParamEntry *param, int id);
int visual_param_entry_notify_callbacks (VisParamEntry *param);
int visual_param_entry_is (VisParamEntry *param, const char *name);
int visual_param_entry_compare (VisParamEntry *src1, VisParamEntry *src2);
int visual_param_entry_changed (VisParamEntry *param);
VisParamEntryType visual_param_entry_get_type (VisParamEntry *param);

int visual_param_entry_set_from_param (VisParamEntry *param, VisParamEntry *src);
int visual_param_entry_set_name (VisParamEntry *param, char *name);
int visual_param_entry_set_string (VisParamEntry *param, char *string);
int visual_param_entry_set_integer (VisParamEntry *param, int integer);
int visual_param_entry_set_float (VisParamEntry *param, float floating);
int visual_param_entry_set_double (VisParamEntry *param, double doubleflt);
int visual_param_entry_set_color (VisParamEntry *param, uint8_t r, uint8_t g, uint8_t b);
int visual_param_entry_set_color_by_color (VisParamEntry *param, VisColor *color);
int visual_param_entry_set_palette (VisParamEntry *param, VisPalette *pal);
int visual_param_entry_set_object (VisParamEntry *param, VisObject *object);

char *visual_param_entry_get_name (VisParamEntry *param);
char *visual_param_entry_get_string (VisParamEntry *param);
int visual_param_entry_get_integer (VisParamEntry *param);
float visual_param_entry_get_float (VisParamEntry *param);
double visual_param_entry_get_double (VisParamEntry *param);
VisColor *visual_param_entry_get_color (VisParamEntry *param);
VisPalette *visual_param_entry_get_palette (VisParamEntry *param);
VisObject *visual_param_entry_get_object (VisParamEntry *param);

VISUAL_END_DECLS

#endif /* _LV_PARAM_H */
