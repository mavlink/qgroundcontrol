/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_cache.h,v 1.6 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_CACHE_H
#define _LV_CACHE_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_time.h>
#include <libvisual/lv_list.h>
#include <libvisual/lv_hashmap.h>

VISUAL_BEGIN_DECLS

#define VISUAL_CACHE(obj)				(VISUAL_CHECK_CAST ((obj), VisCache))
#define VISUAL_CACHEENTRY(obj)				(VISUAL_CHECK_CAST ((obj), VisCacheEntry))

typedef struct _VisCache VisCache;
typedef struct _VisCacheEntry VisCacheEntry;

/**
 * Using the VisCache structure
 */
struct _VisCache {
	VisObject			 object;

	VisCollectionDestroyerFunc	 destroyer;

	int				 size;

	int				 withmaxage;
	VisTime				 maxage;

	int				 reqreset;

	/* The reason that we index the cache items two times is because we both need
	 * fast access to a sorted version and fast access by name, access as a sorted list
	 * is needed so we can quickly identify old cache items that should be disposed when
	 * the cache reaches it's limit */
	VisList				*list;
	VisHashmap			*index;
};

/**
 */
struct _VisCacheEntry {
	VisTimer	 timer;

	char		*key;

	void		*data;
};

/* prototypes */
VisCache *visual_cache_new (VisCollectionDestroyerFunc destroyer, int size, VisTime *maxage, int reqreset);
int visual_cache_init (VisCache *cache, VisCollectionDestroyerFunc destroyer, int size, VisTime *maxage, int reqreset);

int visual_cache_clear (VisCache *cache);
int visual_cache_flush_outdated (VisCache *cache);

int visual_cache_put (VisCache *cache, char *key, void *data);
int visual_cache_remove (VisCache *cache, char *key);

void *visual_cache_get (VisCache *cache, char *key);

int visual_cache_size (VisCache *cache);

int visual_cache_set_limits (VisCache *cache, int size, VisTime *maxage);

VisList *visual_cache_get_list (VisCache *cache);

VISUAL_END_DECLS

#endif /* _LV_CACHE_H */
