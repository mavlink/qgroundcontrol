/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_object.h,v 1.14 2006/01/23 21:06:24 synap Exp $
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

#ifndef _LV_OBJECT_H
#define _LV_OBJECT_H

#include <libvisual/lv_defines.h>
#include <libvisual/lv_types.h>

VISUAL_BEGIN_DECLS

#define VISUAL_OBJECT(obj)				(VISUAL_CHECK_CAST ((obj), VisObject))

typedef struct _VisObject VisObject;

/**
 * The function defination for an object destructor. This can be assigned to any VisObject
 * and is mostly used for internal usage or by support libraries. Keep in mind that this should not free
 * the VisObject itself. This is done in the visual_object_destroy function itself.
 *
 * The destructor function should be safe to enter more than once, the object always contains the object
 * however make sure that freed members are set to NULL and that it's checked.
 *
 * @arg object The VisObject that is passed to the destructor.
 * 
 * @return VISUAL_OK on succes, -VISUAL_ERROR_OBJECT_DTOR_FAILED on failure.
 */
typedef int (*VisObjectDtorFunc)(VisObject *object);

/**
 * The VisObject structure contains all the VisObject housekeeping data like refcounting and a pointer
 * to the VisObjectDtorFunc. Also it's possible to set private data on a VisObject.
 *
 * Nearly all libvisual structures inherent from a VisObject.
 */
struct _VisObject {
	int			 allocated;	/**< Set to TRUE if this object is allocated and should be freed completely.
						  * if set to FALSE, it will run the VisObjectDtorFunc but won't free the VisObject
						  * itself when refcount reaches 0. */
	int			 refcount;	/**< Contains the number of references to this object. */
	VisObjectDtorFunc	 dtor;		/**< Pointer to the object destruction function. */

	void			*priv;		/**< Private which can be used by application or plugin developers
						 * depending on the sub class object. */
};

int visual_object_collection_destroyer (void *data);

VisObject *visual_object_new (void);
int visual_object_free (VisObject *object);
int visual_object_destroy (VisObject *object);

int visual_object_initialize (VisObject *object, int allocated, VisObjectDtorFunc dtor);
int visual_object_clear (VisObject *object);
int visual_object_set_dtor (VisObject *object, VisObjectDtorFunc dtor);
int visual_object_set_allocated (VisObject *object, int allocated);
int visual_object_set_refcount (VisObject *object, int refcount);

int visual_object_ref (VisObject *object);
int visual_object_unref (VisObject *object);

int visual_object_set_private (VisObject *object, void *priv);
void *visual_object_get_private (VisObject *object);

/**
 * @ingroup VisObject
 *
 * Macro that will set the data after the VisObject data to 0.
 */
#define visual_object_clean(object, struct_type)	\
	visual_mem_set ((uint8_t *) (object) + sizeof (VisObject), 0, sizeof (struct_type) - sizeof (VisObject))

/**
 * @ingroup VisObject
 *
 * Macro that will copy the data after the VisObject data to another VisObject.
 */
#define visual_object_copy_data(dest, src, struct_type)			\
	visual_mem_copy ((uint8_t *) (dest) + sizeof (VisObject),	\
			(uint8_t *) (src) + sizeof (VisObject),		\
			sizeof (struct_type) - sizeof (VisObject))


VISUAL_END_DECLS

#endif /* _LV_OBJECT_H */
