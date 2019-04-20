/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_hashmap.h,v 1.8 2006/02/17 22:00:17 synap Exp $
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

#ifndef _LV_HASHMAP_H
#define _LV_HASHMAP_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_list.h>
#include <libvisual/lv_collection.h>

VISUAL_BEGIN_DECLS

#define VISUAL_HASHMAP_START_SIZE	1024

#define VISUAL_HASHMAP(obj)				(VISUAL_CHECK_CAST ((obj), VisHashmap))
#define VISUAL_HASHMAPENTRY(obj)			(VISUAL_CHECK_CAST ((obj), VisHashmapEntry))
#define VISUAL_HASHMAPCHAINENTRY(obj)			(VISUAL_CHECK_CAST ((obj), VisHashmapChainEntry))

typedef struct _VisHashmap VisHashmap;
typedef struct _VisHashmapEntry VisHashmapEntry;
typedef struct _VisHashmapChainEntry VisHashmapChainEntry;

typedef enum {
	VISUAL_HASHMAP_KEY_TYPE_NONE		= 0,
	VISUAL_HASHMAP_KEY_TYPE_INTEGER		= 1,
	VISUAL_HASHMAP_KEY_TYPE_STRING		= 2
} VisHashmapKeyType;

/**
 * Using the VisHashmap structure you can store a collection of data within a hashmap.
 */
struct _VisHashmap {
	VisCollection		 collection;	/**< The VisCollection data. */

	int			 tablesize;	/**< Size of the table array. */
	int			 size;		/**< Number of entries stored in the VisHashmap. */

	VisHashmapEntry		*table;		/**< The VisHashmap array. */
};

/**
 * Private VisHashmap array entry.
 */
struct _VisHashmapEntry {
	VisList			 list;
};

/**
 * Private VisHashmap chain entry.
 */
struct _VisHashmapChainEntry {
	VisHashmapKeyType	 keytype;

	void			*data;

	union {
		uint32_t	 integer;
		char		*string;
	} key;
};

/* prototypes */
VisHashmap *visual_hashmap_new (VisCollectionDestroyerFunc destroyer);
int visual_hashmap_init (VisHashmap *hashmap, VisCollectionDestroyerFunc destroyer);

int visual_hashmap_put (VisHashmap *hashmap, void *key, VisHashmapKeyType keytype, void *data);
int visual_hashmap_put_integer (VisHashmap *hashmap, uint32_t key, void *data);
int visual_hashmap_put_string (VisHashmap *hashmap, char *key, void *data);

int visual_hashmap_remove (VisHashmap *hashmap, void *key, VisHashmapKeyType keytype, int destroy);
int visual_hashmap_remove_integer (VisHashmap *hashmap, uint32_t key, int destroy);
int visual_hashmap_remove_string (VisHashmap *hashmap, char *key, int destroy);

void *visual_hashmap_get (VisHashmap *hashmap, void *key, VisHashmapKeyType keytype);
void *visual_hashmap_get_integer (VisHashmap *hashmap, uint32_t key);
void *visual_hashmap_get_string (VisHashmap *hashmap, char *key);

int visual_hashmap_set_table_size (VisHashmap *hashmap, int tablesize);
int visual_hashmap_get_table_size (VisHashmap *hashmap);

VISUAL_END_DECLS

#endif /* _LV_HASHMAP_H */
