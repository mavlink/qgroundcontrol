/*
 * Copyright (C) 2014 Collabora Ltd.
 *     Author: Nicolas Dufresne <nicolas.dufresne@collabora.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */


#ifndef __GST_V4L2_ALLOCATOR_H__
#define __GST_V4L2_ALLOCATOR_H__

#include "ext/videodev2.h"
#include <gst/gst.h>
#include <gst/gstatomicqueue.h>

G_BEGIN_DECLS

#define GST_TYPE_V4L2_ALLOCATOR                 (gst_v4l2_allocator_get_type())
#define GST_IS_V4L2_ALLOCATOR(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_V4L2_ALLOCATOR))
#define GST_IS_V4L2_ALLOCATOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_V4L2_ALLOCATOR))
#define GST_V4L2_ALLOCATOR_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_V4L2_ALLOCATOR, GstV4l2AllocatorClass))
#define GST_V4L2_ALLOCATOR(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_V4L2_ALLOCATOR, GstV4l2Allocator))
#define GST_V4L2_ALLOCATOR_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_V4L2_ALLOCATOR, GstV4l2AllocatorClass))
#define GST_V4L2_ALLOCATOR_CAST(obj)            ((GstV4l2Allocator *)(obj))

#define GST_V4L2_ALLOCATOR_CAN_REQUEST(obj,type) \
        (GST_OBJECT_FLAG_IS_SET (obj, GST_V4L2_ALLOCATOR_FLAG_ ## type ## _REQBUFS))
#define GST_V4L2_ALLOCATOR_CAN_ALLOCATE(obj,type) \
        (GST_OBJECT_FLAG_IS_SET (obj, GST_V4L2_ALLOCATOR_FLAG_ ## type ## _CREATE_BUFS))
#define GST_V4L2_ALLOCATOR_CAN_ORPHAN_BUFS(obj) \
        (GST_OBJECT_FLAG_IS_SET (obj, GST_V4L2_ALLOCATOR_FLAG_SUPPORTS_ORPHANED_BUFS))
#define GST_V4L2_ALLOCATOR_IS_ORPHANED(obj) \
        (GST_OBJECT_FLAG_IS_SET (obj, GST_V4L2_ALLOCATOR_FLAG_ORPHANED))

#define GST_V4L2_MEMORY_QUARK gst_v4l2_memory_quark ()

typedef struct _GstV4l2Allocator GstV4l2Allocator;
typedef struct _GstV4l2AllocatorClass GstV4l2AllocatorClass;
typedef struct _GstV4l2MemoryGroup GstV4l2MemoryGroup;
typedef struct _GstV4l2Memory GstV4l2Memory;
typedef enum _GstV4l2Capabilities GstV4l2Capabilities;
typedef enum _GstV4l2Return GstV4l2Return;

enum _GstV4l2AllocatorFlags
{
  GST_V4L2_ALLOCATOR_FLAG_MMAP_REQBUFS        = (GST_ALLOCATOR_FLAG_LAST << 0),
  GST_V4L2_ALLOCATOR_FLAG_MMAP_CREATE_BUFS    = (GST_ALLOCATOR_FLAG_LAST << 1),
  GST_V4L2_ALLOCATOR_FLAG_USERPTR_REQBUFS     = (GST_ALLOCATOR_FLAG_LAST << 2),
  GST_V4L2_ALLOCATOR_FLAG_USERPTR_CREATE_BUFS = (GST_ALLOCATOR_FLAG_LAST << 3),
  GST_V4L2_ALLOCATOR_FLAG_DMABUF_REQBUFS      = (GST_ALLOCATOR_FLAG_LAST << 4),
  GST_V4L2_ALLOCATOR_FLAG_DMABUF_CREATE_BUFS  = (GST_ALLOCATOR_FLAG_LAST << 5),
  GST_V4L2_ALLOCATOR_FLAG_SUPPORTS_ORPHANED_BUFS = (GST_ALLOCATOR_FLAG_LAST << 6),
  GST_V4L2_ALLOCATOR_FLAG_ORPHANED            = (GST_ALLOCATOR_FLAG_LAST << 7),
};

enum _GstV4l2Return
{
  GST_V4L2_OK = 0,
  GST_V4L2_ERROR = -1,
  GST_V4L2_BUSY = -2
};

struct _GstV4l2Memory
{
  GstMemory mem;
  gint plane;
  GstV4l2MemoryGroup *group;
  gpointer data;
  gint dmafd;
};

struct _GstV4l2MemoryGroup
{
  gint n_mem;
  GstMemory * mem[VIDEO_MAX_PLANES];
  gint mems_allocated;
  struct v4l2_buffer buffer;
  struct v4l2_plane planes[VIDEO_MAX_PLANES];
};

struct _GstV4l2Allocator
{
  GstAllocator parent;
  GstV4l2Object *obj;
  guint32 count;
  guint32 memory;
  gboolean can_allocate;
  gboolean active;

  GstV4l2MemoryGroup * groups[VIDEO_MAX_FRAME];
  GstAtomicQueue *free_queue;
  GstAtomicQueue *pending_queue;

};

struct _GstV4l2AllocatorClass {
  GstAllocatorClass parent_class;
};

GType gst_v4l2_allocator_get_type(void);

gboolean             gst_is_v4l2_memory                (GstMemory * mem);

GQuark               gst_v4l2_memory_quark             (void);

gboolean             gst_v4l2_allocator_is_active      (GstV4l2Allocator * allocator);

guint                gst_v4l2_allocator_get_size       (GstV4l2Allocator * allocator);

GstV4l2Allocator*    gst_v4l2_allocator_new            (GstObject *parent, GstV4l2Object * obj);

guint                gst_v4l2_allocator_start          (GstV4l2Allocator * allocator,
                                                        guint32 count, guint32 memory);

GstV4l2Return        gst_v4l2_allocator_stop           (GstV4l2Allocator * allocator);

gboolean             gst_v4l2_allocator_orphan         (GstV4l2Allocator * allocator);

GstV4l2MemoryGroup*  gst_v4l2_allocator_alloc_mmap     (GstV4l2Allocator * allocator);

GstV4l2MemoryGroup*  gst_v4l2_allocator_alloc_dmabuf   (GstV4l2Allocator * allocator,
                                                        GstAllocator * dmabuf_allocator);

GstV4l2MemoryGroup * gst_v4l2_allocator_alloc_dmabufin (GstV4l2Allocator * allocator);

GstV4l2MemoryGroup * gst_v4l2_allocator_alloc_userptr  (GstV4l2Allocator * allocator);

gboolean             gst_v4l2_allocator_import_dmabuf  (GstV4l2Allocator * allocator,
                                                        GstV4l2MemoryGroup *group,
                                                        gint n_mem, GstMemory ** dma_mem);

gboolean             gst_v4l2_allocator_import_userptr (GstV4l2Allocator * allocator,
                                                        GstV4l2MemoryGroup *group,
                                                        gsize img_size, int n_planes,
                                                        gpointer * data, gsize * size);

void                 gst_v4l2_allocator_flush          (GstV4l2Allocator * allocator);

gboolean             gst_v4l2_allocator_qbuf           (GstV4l2Allocator * allocator,
                                                        GstV4l2MemoryGroup * group);

GstFlowReturn        gst_v4l2_allocator_dqbuf          (GstV4l2Allocator * allocator,
                                                        GstV4l2MemoryGroup ** group);

void                 gst_v4l2_allocator_reset_group    (GstV4l2Allocator * allocator,
                                                        GstV4l2MemoryGroup * group);

G_END_DECLS

#endif /* __GST_V4L2_ALLOCATOR_H__ */
