/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_collection.h,v 1.7 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_COLLECTION_H
#define _LV_COLLECTION_H

#include <libvisual/lv_common.h>

VISUAL_BEGIN_DECLS

#define VISUAL_COLLECTION(obj)				(VISUAL_CHECK_CAST ((obj), VisCollection))
#define VISUAL_COLLECTIONITER(obj)			(VISUAL_CHECK_CAST ((obj), VisCollectionIter))

typedef struct _VisCollection VisCollection;
typedef struct _VisCollectionIter VisCollectionIter;

/**
 * A VisCollection destroyer function needs this signature, these functions are used
 * to destroy data entries within collections.
 *
 * @arg data The data that was stored in a collection entry and thus can be destroyed.
 *
 * @return FIXME blah blah blah
 */
typedef int (*VisCollectionDestroyerFunc)(void *data);

/**
 */
typedef int (*VisCollectionDestroyFunc)(VisCollection *collection);

/**
 */
typedef int (*VisCollectionSizeFunc)(VisCollection *collection);

/**
 */
typedef VisCollectionIter *(*VisCollectionIterFunc)(VisCollection *collection);

/**
 */
typedef void (*VisCollectionIterAssignFunc)(VisCollectionIter *iter, VisCollection *collection, VisObject *context,
		int index);

/**
 */
typedef void (*VisCollectionIterNextFunc)(VisCollectionIter *iter, VisCollection *collection, VisObject *context);

/**
 */
typedef int (*VisCollectionIterHasMoreFunc)(VisCollectionIter *iter, VisCollection *collection, VisObject *context);

/**
 */
typedef void *(*VisCollectionIterGetDataFunc)(VisCollectionIter *iter, VisCollection *collection, VisObject *context);

/**
 */
struct _VisCollection {
	VisObject			 object;	/**< The VisObject data. */

	VisCollectionDestroyerFunc	 destroyer;
	VisCollectionDestroyFunc	 destroyfunc;
	VisCollectionSizeFunc		 sizefunc;
	VisCollectionIterFunc		 iterfunc;
};

/**
 */
struct _VisCollectionIter {
	VisObject			 object;

	VisCollectionIterAssignFunc	 assignfunc;
	VisCollectionIterNextFunc	 nextfunc;
	VisCollectionIterHasMoreFunc	 hasmorefunc;
	VisCollectionIterGetDataFunc	 getdatafunc;

	VisCollection			*collection;

	VisObject			*context;
};

/* prototypes */
int visual_collection_set_destroyer (VisCollection *collection, VisCollectionDestroyerFunc destroyer);
VisCollectionDestroyerFunc visual_collection_get_destroyer (VisCollection *collection);

int visual_collection_set_destroy_func (VisCollection *collection, VisCollectionDestroyFunc destroyfunc);
VisCollectionDestroyFunc visual_collection_get_destroy_func (VisCollection *collection);

int visual_collection_set_size_func (VisCollection *collection, VisCollectionSizeFunc sizefunc);
VisCollectionSizeFunc visual_collection_get_size_func (VisCollection *collection);

int visual_collection_set_iter_func (VisCollection *collection, VisCollectionIterFunc iterfunc);
VisCollectionIterFunc visual_collection_get_iter_func (VisCollection *collection);

int visual_collection_dtor (VisObject *object);

int visual_collection_destroy (VisCollection *collection);
int visual_collection_size (VisCollection *collection);
VisCollectionIter *visual_collection_get_iter (VisCollection *collection);


VisCollectionIter *visual_collection_iter_new (
		VisCollectionIterAssignFunc assignfunc, VisCollectionIterNextFunc nextfunc,
		VisCollectionIterHasMoreFunc hasmorefunc, VisCollectionIterGetDataFunc getdatafunc,
		VisCollection *collection, VisObject *context);
int visual_collection_iter_init (VisCollectionIter *iter,
		VisCollectionIterAssignFunc assignfunc, VisCollectionIterNextFunc nextfunc,
		VisCollectionIterHasMoreFunc hasmorefunc, VisCollectionIterGetDataFunc getdatafunc,
		VisCollection *collection, VisObject *context);

void visual_collection_iter_assign (VisCollectionIter *iter, int index);
void visual_collection_iter_next (VisCollectionIter *iter);
int visual_collection_iter_has_more (VisCollectionIter *iter);
void *visual_collection_iter_get_data (VisCollectionIter *iter);

VISUAL_END_DECLS

#endif /* _LV_COLLECTION_H */
