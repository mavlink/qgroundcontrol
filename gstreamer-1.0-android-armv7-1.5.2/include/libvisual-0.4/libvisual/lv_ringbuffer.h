/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_ringbuffer.h,v 1.6 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_RINGBUFFER_H
#define _LV_RINGBUFFER_H

#include <libvisual/lv_common.h>
#include <libvisual/lv_list.h>
#include <libvisual/lv_buffer.h>

VISUAL_BEGIN_DECLS

#define VISUAL_RINGBUFFER(obj)				(VISUAL_CHECK_CAST ((obj), VisRingBuffer))
#define VISUAL_RINGBUFFER_ENTRY(obj)			(VISUAL_CHECK_CAST ((obj), VisRingBufferEntry))

/**
 * Enum defining the VisRingBufferEntryTypes.
 */
typedef enum {
	VISUAL_RINGBUFFER_ENTRY_TYPE_NONE	= 0,	/**< State less entry. */
	VISUAL_RINGBUFFER_ENTRY_TYPE_BUFFER	= 1,	/**< Normal byte buffer. */
	VISUAL_RINGBUFFER_ENTRY_TYPE_FUNCTION	= 2	/**< Data retrieval using a callback. */
} VisRingBufferEntryType;

typedef struct _VisRingBufferEntry VisRingBufferEntry;
typedef struct _VisRingBuffer VisRingBuffer;

/**
 * A VisRingBuffer data provider function needs this signature. It can be used to provide
 * ringbuffer entry data on runtime through a callback.
 *
 * @arg ringbuffer The VisRingBuffer data structure.
 * @arg entry The VisRingBufferEntry to which the callback entry is connected.
 */
typedef VisBuffer *(*VisRingBufferDataFunc)(VisRingBuffer *ringbuffer, VisRingBufferEntry *entry);

typedef void (*VisRingBufferDestroyFunc)(VisRingBufferEntry *entry);

typedef int (*VisRingBufferSizeFunc)(VisRingBuffer *ringbuffer, VisRingBufferEntry *entry);


/**
 * The VisRingBufferEntry data structure is an entry within the ringbuffer.
 */
struct _VisRingBufferEntry {
	VisObject			 object;

	VisRingBufferEntryType		 type;
	VisRingBufferDataFunc		 datafunc;
	VisRingBufferDestroyFunc	 destroyfunc;
	VisRingBufferSizeFunc		 sizefunc;

	VisBuffer			*buffer;

	void				*functiondata;
};

/**
 * The VisRingBuffer data structure holding the ringbuffer.
 */
struct _VisRingBuffer {
	VisObject		 object;	/**< The VisObject data. */

	VisList			*entries;	/**< The ring buffer entries list. */
};

/* prototypes */
VisRingBuffer *visual_ringbuffer_new (void);
int visual_ringbuffer_init (VisRingBuffer *ringbuffer);

int visual_ringbuffer_add_entry (VisRingBuffer *ringbuffer, VisRingBufferEntry *entry);
int visual_ringbuffer_add_buffer (VisRingBuffer *ringbuffer, VisBuffer *buffer);
int visual_ringbuffer_add_buffer_by_data (VisRingBuffer *ringbuffer, void *data, int nbytes);
int visual_ringbuffer_add_function (VisRingBuffer *ringbuffer,
		VisRingBufferDataFunc datafunc,
		VisRingBufferDestroyFunc destroyfunc,
		VisRingBufferSizeFunc sizefunc,
		void *functiondata);

int visual_ringbuffer_get_size (VisRingBuffer *ringbuffer);

VisList *visual_ringbuffer_get_list (VisRingBuffer *ringbuffer);

int visual_ringbuffer_get_data (VisRingBuffer *ringbuffer, VisBuffer *data, int nbytes);
int visual_ringbuffer_get_data_offset (VisRingBuffer *ringbuffer, VisBuffer *data, int offset, int nbytes);
int visual_ringbuffer_get_data_from_end (VisRingBuffer *ringbuffer, VisBuffer *data, int nbytes);

int visual_ringbuffer_get_data_without_wrap (VisRingBuffer *ringbuffer, VisBuffer *data, int nbytes);

VisBuffer *visual_ringbuffer_get_data_new (VisRingBuffer *ringbuffer, int nbytes);
VisBuffer *visual_ringbuffer_get_data_new_without_wrap (VisRingBuffer *ringbuffer, int nbytes);

VisRingBufferEntry *visual_ringbuffer_entry_new (VisBuffer *buffer);
int visual_ringbuffer_entry_init (VisRingBufferEntry *entry, VisBuffer *buffer);
VisRingBufferEntry *visual_ringbuffer_entry_new_function (
		VisRingBufferDataFunc datafunc,
		VisRingBufferDestroyFunc destroyfunc,
		VisRingBufferSizeFunc sizefunc,
		void *functiondata);
int visual_ringbuffer_entry_init_function (VisRingBufferEntry *entry,
		VisRingBufferDataFunc datafunc,
		VisRingBufferDestroyFunc destroyfunc,
		VisRingBufferSizeFunc sizefunc,
		void *functiondata);
void *visual_ringbuffer_entry_get_functiondata (VisRingBufferEntry *entry);

VISUAL_END_DECLS

#endif /* _LV_RINGBUFFER_H */
