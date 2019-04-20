/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_hashlist.h,v 1.3 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_HASHLIST_H
#define _LV_HASHLIST_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_time.h>
#include <libvisual/lv_list.h>
#include <libvisual/lv_hashmap.h>

VISUAL_BEGIN_DECLS

#define VISUAL_HASHLIST(obj)				(VISUAL_CHECK_CAST ((obj), VisHashlist))
#define VISUAL_HASHLISTENTRY(obj)			(VISUAL_CHECK_CAST ((obj), VisHashlistEntry))

typedef struct _VisHashlist VisHashlist;
typedef struct _VisHashlistEntry VisHashlistEntry;

/**
 * Using the VisHashlist structure
 */
struct _VisHashlist {
	VisCollection			 collection;

	int				 size;

	VisList				*list;
	VisHashmap			*index;
};

/**
 */
struct _VisHashlistEntry {
	char		*key;

	void		*data;
};

/* prototypes */
VisHashlist *visual_hashlist_new (VisCollectionDestroyerFunc destroyer, int size);
int visual_hashlist_init (VisHashlist *hashlist, VisCollectionDestroyerFunc destroyer, int size);

int visual_hashlist_clear (VisHashlist *hashlist);

int visual_hashlist_put (VisHashlist *hashlist, char *key, void *data);
int visual_hashlist_remove (VisHashlist *hashlist, char *key);
int visual_hashlist_remove_list_entry (VisHashlist *hashlist, VisListEntry *le);

void *visual_hashlist_get (VisHashlist *hashlist, char *key);

int visual_hashlist_size (VisHashlist *hashlist);

int visual_hashlist_set_size (VisHashlist *hashlist, int size);

VisList *visual_hashlist_get_list (VisHashlist *hashlist);

VISUAL_END_DECLS

#endif /* _LV_HASHLIST_H */
