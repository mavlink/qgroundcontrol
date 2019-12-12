/* GStreamer
 *
 * Copyright (C) 2001-2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *               2009 Texas Instruments, Inc - http://www.ti.com/
 *
 * gstv4l2bufferpool.c V4L2 buffer pool class
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
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef _GNU_SOURCE
# define _GNU_SOURCE            /* O_CLOEXEC */
#endif
#include <fcntl.h>

#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include "gst/video/video.h"
#include "gst/video/gstvideometa.h"
#include "gst/video/gstvideopool.h"
#include "gst/allocators/gstdmabuf.h"

#include <gstv4l2bufferpool.h>

#include "gstv4l2object.h"
#include "gst/gst-i18n-plugin.h"
#include <gst/glib-compat-private.h>

GST_DEBUG_CATEGORY_STATIC (v4l2bufferpool_debug);
GST_DEBUG_CATEGORY_STATIC (CAT_PERFORMANCE);
#define GST_CAT_DEFAULT v4l2bufferpool_debug

#define GST_V4L2_IMPORT_QUARK gst_v4l2_buffer_pool_import_quark ()


/*
 * GstV4l2BufferPool:
 */
#define gst_v4l2_buffer_pool_parent_class parent_class
G_DEFINE_TYPE (GstV4l2BufferPool, gst_v4l2_buffer_pool, GST_TYPE_BUFFER_POOL);

enum _GstV4l2BufferPoolAcquireFlags
{
  GST_V4L2_BUFFER_POOL_ACQUIRE_FLAG_RESURRECT =
      GST_BUFFER_POOL_ACQUIRE_FLAG_LAST,
  GST_V4L2_BUFFER_POOL_ACQUIRE_FLAG_LAST
};

static void gst_v4l2_buffer_pool_release_buffer (GstBufferPool * bpool,
    GstBuffer * buffer);

static gboolean
gst_v4l2_is_buffer_valid (GstBuffer * buffer, GstV4l2MemoryGroup ** out_group)
{
  GstMemory *mem = gst_buffer_peek_memory (buffer, 0);
  gboolean valid = FALSE;

  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_TAG_MEMORY))
    goto done;

  if (gst_is_dmabuf_memory (mem))
    mem = gst_mini_object_get_qdata (GST_MINI_OBJECT (mem),
        GST_V4L2_MEMORY_QUARK);

  if (mem && gst_is_v4l2_memory (mem)) {
    GstV4l2Memory *vmem = (GstV4l2Memory *) mem;
    GstV4l2MemoryGroup *group = vmem->group;
    gint i;

    if (group->n_mem != gst_buffer_n_memory (buffer))
      goto done;

    for (i = 0; i < group->n_mem; i++) {
      if (group->mem[i] != gst_buffer_peek_memory (buffer, i))
        goto done;

      if (!gst_memory_is_writable (group->mem[i]))
        goto done;
    }

    valid = TRUE;
    if (out_group)
      *out_group = group;
  }

done:
  return valid;
}

static GstFlowReturn
gst_v4l2_buffer_pool_copy_buffer (GstV4l2BufferPool * pool, GstBuffer * dest,
    GstBuffer * src)
{
  const GstVideoFormatInfo *finfo = pool->caps_info.finfo;

  GST_LOG_OBJECT (pool, "copying buffer");

  if (finfo && (finfo->format != GST_VIDEO_FORMAT_UNKNOWN &&
          finfo->format != GST_VIDEO_FORMAT_ENCODED)) {
    GstVideoFrame src_frame, dest_frame;

    GST_DEBUG_OBJECT (pool, "copy video frame");

    /* we have raw video, use videoframe copy to get strides right */
    if (!gst_video_frame_map (&src_frame, &pool->caps_info, src, GST_MAP_READ))
      goto invalid_buffer;

    if (!gst_video_frame_map (&dest_frame, &pool->caps_info, dest,
            GST_MAP_WRITE)) {
      gst_video_frame_unmap (&src_frame);
      goto invalid_buffer;
    }

    gst_video_frame_copy (&dest_frame, &src_frame);

    gst_video_frame_unmap (&src_frame);
    gst_video_frame_unmap (&dest_frame);
  } else {
    GstMapInfo map;

    GST_DEBUG_OBJECT (pool, "copy raw bytes");

    if (!gst_buffer_map (src, &map, GST_MAP_READ))
      goto invalid_buffer;

    gst_buffer_fill (dest, 0, map.data, gst_buffer_get_size (src));

    gst_buffer_unmap (src, &map);
    gst_buffer_resize (dest, 0, gst_buffer_get_size (src));
  }

  gst_buffer_copy_into (dest, src,
      GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, -1);

  GST_CAT_LOG_OBJECT (CAT_PERFORMANCE, pool, "slow copy into buffer %p", dest);

  return GST_FLOW_OK;

invalid_buffer:
  {
    GST_ERROR_OBJECT (pool, "could not map buffer");
    return GST_FLOW_ERROR;
  }
}

struct UserPtrData
{
  GstBuffer *buffer;
  gboolean is_frame;
  GstVideoFrame frame;
  GstMapInfo map;
};

static GQuark
gst_v4l2_buffer_pool_import_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_string ("GstV4l2BufferPoolUsePtrData");

  return quark;
}

static void
_unmap_userptr_frame (struct UserPtrData *data)
{
  if (data->is_frame)
    gst_video_frame_unmap (&data->frame);
  else
    gst_buffer_unmap (data->buffer, &data->map);

  if (data->buffer)
    gst_buffer_unref (data->buffer);

  g_slice_free (struct UserPtrData, data);
}

static GstFlowReturn
gst_v4l2_buffer_pool_import_userptr (GstV4l2BufferPool * pool,
    GstBuffer * dest, GstBuffer * src)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstV4l2MemoryGroup *group = NULL;
  GstMapFlags flags;
  const GstVideoFormatInfo *finfo = pool->caps_info.finfo;
  struct UserPtrData *data = NULL;

  GST_LOG_OBJECT (pool, "importing userptr");

  /* get the group */
  if (!gst_v4l2_is_buffer_valid (dest, &group))
    goto not_our_buffer;

  if (V4L2_TYPE_IS_OUTPUT (pool->obj->type))
    flags = GST_MAP_READ;
  else
    flags = GST_MAP_WRITE;

  data = g_slice_new0 (struct UserPtrData);

  if (finfo && (finfo->format != GST_VIDEO_FORMAT_UNKNOWN &&
          finfo->format != GST_VIDEO_FORMAT_ENCODED)) {
    gsize size[GST_VIDEO_MAX_PLANES] = { 0, };
    gint i;

    data->is_frame = TRUE;

    if (!gst_video_frame_map (&data->frame, &pool->caps_info, src, flags))
      goto invalid_buffer;

    for (i = 0; i < GST_VIDEO_FORMAT_INFO_N_PLANES (finfo); i++) {
      if (GST_VIDEO_FORMAT_INFO_IS_TILED (finfo)) {
        gint tinfo = GST_VIDEO_FRAME_PLANE_STRIDE (&data->frame, i);
        gint pstride;
        guint pheight;

        pstride = GST_VIDEO_TILE_X_TILES (tinfo) <<
            GST_VIDEO_FORMAT_INFO_TILE_WS (finfo);

        pheight = GST_VIDEO_TILE_Y_TILES (tinfo) <<
            GST_VIDEO_FORMAT_INFO_TILE_HS (finfo);

        size[i] = pstride * pheight;
      } else {
        size[i] = GST_VIDEO_FRAME_PLANE_STRIDE (&data->frame, i) *
            GST_VIDEO_FRAME_COMP_HEIGHT (&data->frame, i);
      }
    }

    /* In the single planar API, planes must be contiguous in memory and
     * therefore they must have expected size. ie: no padding.
     * To check these conditions, we check that plane 'i' start address
     * + plane 'i' size equals to plane 'i+1' start address */
    if (!V4L2_TYPE_IS_MULTIPLANAR (pool->obj->type)) {
      for (i = 0; i < (GST_VIDEO_FORMAT_INFO_N_PLANES (finfo) - 1); i++) {
        const struct v4l2_pix_format *pix_fmt = &pool->obj->format.fmt.pix;
        gpointer tmp;
        gint estride = gst_v4l2_object_extrapolate_stride (finfo, i,
            pix_fmt->bytesperline);
        guint eheight = GST_VIDEO_FORMAT_INFO_SCALE_HEIGHT (finfo, i,
            pix_fmt->height);

        tmp = ((guint8 *) data->frame.data[i]) + estride * eheight;
        if (tmp != data->frame.data[i + 1])
          goto non_contiguous_mem;
      }
    }

    if (!gst_v4l2_allocator_import_userptr (pool->vallocator, group,
            data->frame.info.size, finfo->n_planes, data->frame.data, size))
      goto import_failed;
  } else {
    gpointer ptr[1];
    gsize size[1];

    data->is_frame = FALSE;

    if (!gst_buffer_map (src, &data->map, flags))
      goto invalid_buffer;

    ptr[0] = data->map.data;
    size[0] = data->map.size;

    if (!gst_v4l2_allocator_import_userptr (pool->vallocator, group,
            data->map.size, 1, ptr, size))
      goto import_failed;
  }

  data->buffer = gst_buffer_ref (src);

  gst_mini_object_set_qdata (GST_MINI_OBJECT (dest), GST_V4L2_IMPORT_QUARK,
      data, (GDestroyNotify) _unmap_userptr_frame);

  gst_buffer_copy_into (dest, src,
      GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, -1);

  return ret;

not_our_buffer:
  {
    GST_ERROR_OBJECT (pool, "destination buffer invalid or not from our pool");
    return GST_FLOW_ERROR;
  }
invalid_buffer:
  {
    GST_ERROR_OBJECT (pool, "could not map buffer");
    g_slice_free (struct UserPtrData, data);
    return GST_FLOW_ERROR;
  }
non_contiguous_mem:
  {
    GST_ERROR_OBJECT (pool, "memory is not contiguous or plane size mismatch");
    _unmap_userptr_frame (data);
    return GST_FLOW_ERROR;
  }
import_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to import data");
    _unmap_userptr_frame (data);
    return GST_FLOW_ERROR;
  }
}

static GstFlowReturn
gst_v4l2_buffer_pool_import_dmabuf (GstV4l2BufferPool * pool,
    GstBuffer * dest, GstBuffer * src)
{
  GstV4l2MemoryGroup *group = NULL;
  GstMemory *dma_mem[GST_VIDEO_MAX_PLANES] = { 0 };
  guint n_mem = gst_buffer_n_memory (src);
  gint i;

  GST_LOG_OBJECT (pool, "importing dmabuf");

  if (!gst_v4l2_is_buffer_valid (dest, &group))
    goto not_our_buffer;

  if (n_mem > GST_VIDEO_MAX_PLANES)
    goto too_many_mems;

  for (i = 0; i < n_mem; i++)
    dma_mem[i] = gst_buffer_peek_memory (src, i);

  if (!gst_v4l2_allocator_import_dmabuf (pool->vallocator, group, n_mem,
          dma_mem))
    goto import_failed;

  gst_mini_object_set_qdata (GST_MINI_OBJECT (dest), GST_V4L2_IMPORT_QUARK,
      gst_buffer_ref (src), (GDestroyNotify) gst_buffer_unref);

  gst_buffer_copy_into (dest, src,
      GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, -1);

  return GST_FLOW_OK;

not_our_buffer:
  {
    GST_ERROR_OBJECT (pool, "destination buffer invalid or not from our pool");
    return GST_FLOW_ERROR;
  }
too_many_mems:
  {
    GST_ERROR_OBJECT (pool, "could not map buffer");
    return GST_FLOW_ERROR;
  }
import_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to import dmabuf");
    return GST_FLOW_ERROR;
  }
}

static GstFlowReturn
gst_v4l2_buffer_pool_prepare_buffer (GstV4l2BufferPool * pool,
    GstBuffer * dest, GstBuffer * src)
{
  GstFlowReturn ret = GST_FLOW_OK;
  gboolean own_src = FALSE;

  if (src == NULL) {
    if (pool->other_pool == NULL) {
      GST_ERROR_OBJECT (pool, "can't prepare buffer, source buffer missing");
      return GST_FLOW_ERROR;
    }

    ret = gst_buffer_pool_acquire_buffer (pool->other_pool, &src, NULL);
    if (ret != GST_FLOW_OK) {
      GST_ERROR_OBJECT (pool, "failed to acquire buffer from downstream pool");
      goto done;
    }

    own_src = TRUE;
  }

  switch (pool->obj->mode) {
    case GST_V4L2_IO_MMAP:
    case GST_V4L2_IO_DMABUF:
      ret = gst_v4l2_buffer_pool_copy_buffer (pool, dest, src);
      break;
    case GST_V4L2_IO_USERPTR:
      ret = gst_v4l2_buffer_pool_import_userptr (pool, dest, src);
      break;
    case GST_V4L2_IO_DMABUF_IMPORT:
      ret = gst_v4l2_buffer_pool_import_dmabuf (pool, dest, src);
      break;
    default:
      break;
  }

  if (own_src)
    gst_buffer_unref (src);

done:
  return ret;
}

static GstFlowReturn
gst_v4l2_buffer_pool_alloc_buffer (GstBufferPool * bpool, GstBuffer ** buffer,
    GstBufferPoolAcquireParams * params)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);
  GstV4l2MemoryGroup *group = NULL;
  GstBuffer *newbuf = NULL;
  GstV4l2Object *obj;
  GstVideoInfo *info;

  obj = pool->obj;
  info = &obj->info;

  switch (obj->mode) {
    case GST_V4L2_IO_RW:
      newbuf =
          gst_buffer_new_allocate (pool->allocator, pool->size, &pool->params);
      break;
    case GST_V4L2_IO_MMAP:
      group = gst_v4l2_allocator_alloc_mmap (pool->vallocator);
      break;
    case GST_V4L2_IO_DMABUF:
      group = gst_v4l2_allocator_alloc_dmabuf (pool->vallocator,
          pool->allocator);
      break;
    case GST_V4L2_IO_USERPTR:
      group = gst_v4l2_allocator_alloc_userptr (pool->vallocator);
      break;
    case GST_V4L2_IO_DMABUF_IMPORT:
      group = gst_v4l2_allocator_alloc_dmabufin (pool->vallocator);
      break;
    default:
      newbuf = NULL;
      g_assert_not_reached ();
      break;
  }

  if (group != NULL) {
    gint i;
    newbuf = gst_buffer_new ();

    for (i = 0; i < group->n_mem; i++)
      gst_buffer_append_memory (newbuf, group->mem[i]);
  } else if (newbuf == NULL) {
    goto allocation_failed;
  }

  /* add metadata to raw video buffers */
  if (pool->add_videometa)
    gst_buffer_add_video_meta_full (newbuf, GST_VIDEO_FRAME_FLAG_NONE,
        GST_VIDEO_INFO_FORMAT (info), GST_VIDEO_INFO_WIDTH (info),
        GST_VIDEO_INFO_HEIGHT (info), GST_VIDEO_INFO_N_PLANES (info),
        info->offset, info->stride);

  *buffer = newbuf;

  return GST_FLOW_OK;

  /* ERRORS */
allocation_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to allocate buffer");
    return GST_FLOW_ERROR;
  }
}

static gboolean
gst_v4l2_buffer_pool_set_config (GstBufferPool * bpool, GstStructure * config)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);
  GstV4l2Object *obj = pool->obj;
  GstCaps *caps;
  guint size, min_buffers, max_buffers;
  GstAllocator *allocator;
  GstAllocationParams params;
  gboolean can_allocate = FALSE;
  gboolean updated = FALSE;
  gboolean ret;

  pool->add_videometa =
      gst_buffer_pool_config_has_option (config,
      GST_BUFFER_POOL_OPTION_VIDEO_META);

  /* parse the config and keep around */
  if (!gst_buffer_pool_config_get_params (config, &caps, &size, &min_buffers,
          &max_buffers))
    goto wrong_config;

  if (!gst_buffer_pool_config_get_allocator (config, &allocator, &params))
    goto wrong_config;

  GST_DEBUG_OBJECT (pool, "config %" GST_PTR_FORMAT, config);

  if (pool->allocator)
    gst_object_unref (pool->allocator);
  pool->allocator = NULL;

  switch (obj->mode) {
    case GST_V4L2_IO_DMABUF:
      pool->allocator = gst_dmabuf_allocator_new ();
      can_allocate = GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, MMAP);
      break;
    case GST_V4L2_IO_MMAP:
      can_allocate = GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, MMAP);
      break;
    case GST_V4L2_IO_USERPTR:
      can_allocate =
          GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, USERPTR);
      break;
    case GST_V4L2_IO_DMABUF_IMPORT:
      can_allocate = GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, DMABUF);
      break;
    case GST_V4L2_IO_RW:
      if (allocator)
        pool->allocator = g_object_ref (allocator);
      pool->params = params;
      /* No need to change the configuration */
      goto done;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  /* libv4l2 conversion code does not handle CREATE_BUFS, and may lead to
   * instability and crash, disable it for now */
  if (can_allocate && obj->fmtdesc->flags & V4L2_FMT_FLAG_EMULATED) {
    GST_WARNING_OBJECT (pool,
        "libv4l2 converter detected, disabling CREATE_BUFS");
    can_allocate = FALSE;
    GST_OBJECT_FLAG_UNSET (pool->vallocator,
        GST_V4L2_ALLOCATOR_FLAG_MMAP_CREATE_BUFS
        | GST_V4L2_ALLOCATOR_FLAG_USERPTR_CREATE_BUFS
        | GST_V4L2_ALLOCATOR_FLAG_DMABUF_CREATE_BUFS);
  }

  if (min_buffers < GST_V4L2_MIN_BUFFERS) {
    updated = TRUE;
    min_buffers = GST_V4L2_MIN_BUFFERS;
    GST_INFO_OBJECT (pool, "increasing minimum buffers to %u", min_buffers);
  }

  /* respect driver requirements */
  if (min_buffers < obj->min_buffers) {
    updated = TRUE;
    min_buffers = obj->min_buffers;
    GST_INFO_OBJECT (pool, "increasing minimum buffers to %u", min_buffers);
  }

  if (max_buffers > VIDEO_MAX_FRAME || max_buffers == 0) {
    updated = TRUE;
    max_buffers = VIDEO_MAX_FRAME;
    GST_INFO_OBJECT (pool, "reducing maximum buffers to %u", max_buffers);
  }

  if (min_buffers > max_buffers) {
    updated = TRUE;
    min_buffers = max_buffers;
    GST_INFO_OBJECT (pool, "reducing minimum buffers to %u", min_buffers);
  } else if (min_buffers != max_buffers) {
    if (!can_allocate) {
      updated = TRUE;
      max_buffers = min_buffers;
      GST_INFO_OBJECT (pool, "can't allocate, setting maximum to minimum");
    }
  }

  if (!pool->add_videometa && obj->need_video_meta) {
    GST_INFO_OBJECT (pool, "adding needed video meta");
    updated = TRUE;
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }

  /* Always update the config to ensure the configured size matches */
  gst_buffer_pool_config_set_params (config, caps, obj->info.size, min_buffers,
      max_buffers);

  /* keep a GstVideoInfo with defaults for the when we need to copy */
  gst_video_info_from_caps (&pool->caps_info, caps);

done:
  ret = GST_BUFFER_POOL_CLASS (parent_class)->set_config (bpool, config);

  /* If anything was changed documentation recommend to return FALSE */
  return !updated && ret;

  /* ERRORS */
wrong_config:
  {
    GST_ERROR_OBJECT (pool, "invalid config %" GST_PTR_FORMAT, config);
    return FALSE;
  }
}

static GstFlowReturn
gst_v4l2_buffer_pool_resurrect_buffer (GstV4l2BufferPool * pool)
{
  GstBufferPoolAcquireParams params = { 0 };
  GstBuffer *buffer = NULL;
  GstFlowReturn ret;

  GST_DEBUG_OBJECT (pool, "A buffer was lost, reallocating it");

  /* block recursive calls to this function */
  g_signal_handler_block (pool->vallocator, pool->group_released_handler);

  params.flags =
      (GstBufferPoolAcquireFlags) GST_V4L2_BUFFER_POOL_ACQUIRE_FLAG_RESURRECT |
      GST_BUFFER_POOL_ACQUIRE_FLAG_DONTWAIT;
  ret =
      gst_buffer_pool_acquire_buffer (GST_BUFFER_POOL (pool), &buffer, &params);

  if (ret == GST_FLOW_OK)
    gst_buffer_unref (buffer);

  g_signal_handler_unblock (pool->vallocator, pool->group_released_handler);

  return ret;
}

static gboolean
gst_v4l2_buffer_pool_streamon (GstV4l2BufferPool * pool)
{
  GstV4l2Object *obj = pool->obj;

  if (pool->streaming)
    return TRUE;

  switch (obj->mode) {
    case GST_V4L2_IO_MMAP:
    case GST_V4L2_IO_USERPTR:
    case GST_V4L2_IO_DMABUF:
    case GST_V4L2_IO_DMABUF_IMPORT:
      if (!V4L2_TYPE_IS_OUTPUT (pool->obj->type)) {
        guint i;

        /* For captures, we need to enqueue buffers before we start streaming,
         * so the driver don't underflow immediately. As we have put then back
         * into the base class queue, resurrect them, then releasing will queue
         * them back. */
        for (i = 0; i < pool->num_allocated; i++)
          gst_v4l2_buffer_pool_resurrect_buffer (pool);
      }

      if (obj->ioctl (pool->video_fd, VIDIOC_STREAMON, &obj->type) < 0)
        goto streamon_failed;

      pool->streaming = TRUE;

      GST_DEBUG_OBJECT (pool, "Started streaming");
      break;
    default:
      break;
  }

  return TRUE;

streamon_failed:
  {
    GST_ERROR_OBJECT (pool, "error with STREAMON %d (%s)", errno,
        g_strerror (errno));
    return FALSE;
  }
}

/* Call with streamlock held, or when streaming threads are down */
static void
gst_v4l2_buffer_pool_streamoff (GstV4l2BufferPool * pool)
{
  GstBufferPoolClass *pclass = GST_BUFFER_POOL_CLASS (parent_class);
  GstV4l2Object *obj = pool->obj;
  gint i;

  if (!pool->streaming)
    return;

  switch (obj->mode) {
    case GST_V4L2_IO_MMAP:
    case GST_V4L2_IO_USERPTR:
    case GST_V4L2_IO_DMABUF:
    case GST_V4L2_IO_DMABUF_IMPORT:

      if (obj->ioctl (pool->video_fd, VIDIOC_STREAMOFF, &obj->type) < 0)
        GST_WARNING_OBJECT (pool, "STREAMOFF failed with errno %d (%s)",
            errno, g_strerror (errno));

      pool->streaming = FALSE;

      GST_DEBUG_OBJECT (pool, "Stopped streaming");

      if (pool->vallocator)
        gst_v4l2_allocator_flush (pool->vallocator);
      break;
    default:
      break;
  }

  for (i = 0; i < VIDEO_MAX_FRAME; i++) {
    if (pool->buffers[i]) {
      GstBuffer *buffer = pool->buffers[i];
      GstBufferPool *bpool = GST_BUFFER_POOL (pool);

      pool->buffers[i] = NULL;

      if (V4L2_TYPE_IS_OUTPUT (pool->obj->type))
        gst_v4l2_buffer_pool_release_buffer (bpool, buffer);
      else                      /* Don't re-enqueue capture buffer on stop */
        pclass->release_buffer (bpool, buffer);

      g_atomic_int_add (&pool->num_queued, -1);
    }
  }
}

static gboolean
gst_v4l2_buffer_pool_start (GstBufferPool * bpool)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);
  GstBufferPoolClass *pclass = GST_BUFFER_POOL_CLASS (parent_class);
  GstV4l2Object *obj = pool->obj;
  GstStructure *config;
  GstCaps *caps;
  guint size, min_buffers, max_buffers;
  guint max_latency, min_latency, copy_threshold = 0;
  gboolean can_allocate = FALSE, ret = TRUE;

  GST_DEBUG_OBJECT (pool, "activating pool");

  if (pool->other_pool) {
    GstBuffer *buffer;

    if (!gst_buffer_pool_set_active (pool->other_pool, TRUE))
      goto other_pool_failed;

    if (gst_buffer_pool_acquire_buffer (pool->other_pool, &buffer, NULL) !=
        GST_FLOW_OK)
      goto other_pool_failed;

    if (!gst_v4l2_object_try_import (obj, buffer)) {
      gst_buffer_unref (buffer);
      goto cannot_import;
    }
    gst_buffer_unref (buffer);
  }

  config = gst_buffer_pool_get_config (bpool);
  if (!gst_buffer_pool_config_get_params (config, &caps, &size, &min_buffers,
          &max_buffers))
    goto wrong_config;

  min_latency = MAX (GST_V4L2_MIN_BUFFERS, obj->min_buffers);

  switch (obj->mode) {
    case GST_V4L2_IO_RW:
      can_allocate = TRUE;
#ifdef HAVE_LIBV4L2
      /* This workaround a unfixable bug in libv4l2 when RW is emulated on top
       * of MMAP. In this case, the first read initialize the queues, but the
       * poll before that will always fail. Doing an empty read, forces the
       * queue to be initialized now. We only do this if we have a streaming
       * driver. */
      if (obj->device_caps & V4L2_CAP_STREAMING)
        obj->read (obj->video_fd, NULL, 0);
#endif
      break;
    case GST_V4L2_IO_DMABUF:
    case GST_V4L2_IO_MMAP:
    {
      guint count;

      can_allocate = GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, MMAP);

      /* first, lets request buffers, and see how many we can get: */
      GST_DEBUG_OBJECT (pool, "requesting %d MMAP buffers", min_buffers);

      count = gst_v4l2_allocator_start (pool->vallocator, min_buffers,
          V4L2_MEMORY_MMAP);
      pool->num_allocated = count;

      if (count < GST_V4L2_MIN_BUFFERS) {
        min_buffers = count;
        goto no_buffers;
      }

      /* V4L2 buffer pool are often very limited in the amount of buffers it
       * can offer. The copy_threshold will workaround this limitation by
       * falling back to copy if the pipeline needed more buffers. This also
       * prevent having to do REQBUFS(N)/REQBUFS(0) every time configure is
       * called. */
      if (count != min_buffers || pool->enable_copy_threshold) {
        GST_WARNING_OBJECT (pool,
            "Uncertain or not enough buffers, enabling copy threshold");
        min_buffers = count;
        copy_threshold = min_latency;
      }

      break;
    }
    case GST_V4L2_IO_USERPTR:
    {
      guint count;

      can_allocate =
          GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, USERPTR);

      GST_DEBUG_OBJECT (pool, "requesting %d USERPTR buffers", min_buffers);

      count = gst_v4l2_allocator_start (pool->vallocator, min_buffers,
          V4L2_MEMORY_USERPTR);

      /* There is no rational to not get what we asked */
      if (count < min_buffers) {
        min_buffers = count;
        goto no_buffers;
      }

      min_buffers = count;
      break;
    }
    case GST_V4L2_IO_DMABUF_IMPORT:
    {
      guint count;

      can_allocate = GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, DMABUF);

      GST_DEBUG_OBJECT (pool, "requesting %d DMABUF buffers", min_buffers);

      count = gst_v4l2_allocator_start (pool->vallocator, min_buffers,
          V4L2_MEMORY_DMABUF);

      /* There is no rational to not get what we asked */
      if (count < min_buffers) {
        min_buffers = count;
        goto no_buffers;
      }

      min_buffers = count;
      break;
    }
    default:
      min_buffers = 0;
      copy_threshold = 0;
      g_assert_not_reached ();
      break;
  }

  if (can_allocate)
    max_latency = max_buffers;
  else
    max_latency = min_buffers;

  pool->size = size;
  pool->copy_threshold = copy_threshold;
  pool->max_latency = max_latency;
  pool->min_latency = min_latency;
  pool->num_queued = 0;

  if (max_buffers != 0 && max_buffers < min_buffers)
    max_buffers = min_buffers;

  gst_buffer_pool_config_set_params (config, caps, size, min_buffers,
      max_buffers);
  pclass->set_config (bpool, config);
  gst_structure_free (config);

  /* now, allocate the buffers: */
  if (!pclass->start (bpool))
    goto start_failed;

  if (!V4L2_TYPE_IS_OUTPUT (obj->type)) {
    if (g_atomic_int_get (&pool->num_queued) < min_buffers)
      goto queue_failed;

    pool->group_released_handler =
        g_signal_connect_swapped (pool->vallocator, "group-released",
        G_CALLBACK (gst_v4l2_buffer_pool_resurrect_buffer), pool);
    ret = gst_v4l2_buffer_pool_streamon (pool);
  }

  return ret;

  /* ERRORS */
wrong_config:
  {
    GST_ERROR_OBJECT (pool, "invalid config %" GST_PTR_FORMAT, config);
    gst_structure_free (config);
    return FALSE;
  }
no_buffers:
  {
    GST_ERROR_OBJECT (pool,
        "we received %d buffer from device '%s', we want at least %d",
        min_buffers, obj->videodev, GST_V4L2_MIN_BUFFERS);
    gst_structure_free (config);
    return FALSE;
  }
start_failed:
  {
    GST_ERROR_OBJECT (pool, "allocate failed");
    return FALSE;
  }
other_pool_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to activate the other pool %"
        GST_PTR_FORMAT, pool->other_pool);
    return FALSE;
  }
queue_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to queue buffers into the capture queue");
    return FALSE;
  }
cannot_import:
  {
    GST_ERROR_OBJECT (pool, "cannot import buffers from downstream pool");
    return FALSE;
  }
}

static gboolean
gst_v4l2_buffer_pool_vallocator_stop (GstV4l2BufferPool * pool)
{
  GstV4l2Return vret;

  if (!pool->vallocator)
    return TRUE;

  vret = gst_v4l2_allocator_stop (pool->vallocator);

  if (vret == GST_V4L2_BUSY)
    GST_WARNING_OBJECT (pool, "some buffers are still outstanding");

  return (vret == GST_V4L2_OK);
}

static gboolean
gst_v4l2_buffer_pool_stop (GstBufferPool * bpool)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);
  gboolean ret;

  if (pool->orphaned)
    return gst_v4l2_buffer_pool_vallocator_stop (pool);

  GST_DEBUG_OBJECT (pool, "stopping pool");

  if (pool->group_released_handler > 0) {
    g_signal_handler_disconnect (pool->vallocator,
        pool->group_released_handler);
    pool->group_released_handler = 0;
  }

  if (pool->other_pool) {
    gst_buffer_pool_set_active (pool->other_pool, FALSE);
    gst_object_unref (pool->other_pool);
    pool->other_pool = NULL;
  }

  gst_v4l2_buffer_pool_streamoff (pool);

  ret = GST_BUFFER_POOL_CLASS (parent_class)->stop (bpool);

  if (ret)
    ret = gst_v4l2_buffer_pool_vallocator_stop (pool);

  return ret;
}

gboolean
gst_v4l2_buffer_pool_orphan (GstBufferPool ** bpool)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (*bpool);
  gboolean ret;

  if (!GST_V4L2_ALLOCATOR_CAN_ORPHAN_BUFS (pool->vallocator))
    return FALSE;

  if (g_getenv ("GST_V4L2_FORCE_DRAIN"))
    return FALSE;

  GST_DEBUG_OBJECT (pool, "orphaning pool");

  gst_buffer_pool_set_active (*bpool, FALSE);
  /*
   * If the buffer pool has outstanding buffers, it will not be stopped
   * by the base class when set inactive. Stop it manually and mark it
   * as orphaned
   */
  ret = gst_v4l2_buffer_pool_stop (*bpool);
  if (!ret)
    ret = gst_v4l2_allocator_orphan (pool->vallocator);

  if (!ret)
    goto orphan_failed;

  pool->orphaned = TRUE;
  gst_object_unref (*bpool);
  *bpool = NULL;

orphan_failed:
  return ret;
}

static void
gst_v4l2_buffer_pool_flush_start (GstBufferPool * bpool)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);

  GST_DEBUG_OBJECT (pool, "start flushing");

  gst_poll_set_flushing (pool->poll, TRUE);

  GST_OBJECT_LOCK (pool);
  pool->empty = FALSE;
  g_cond_broadcast (&pool->empty_cond);
  GST_OBJECT_UNLOCK (pool);

  if (pool->other_pool)
    gst_buffer_pool_set_flushing (pool->other_pool, TRUE);
}

static void
gst_v4l2_buffer_pool_flush_stop (GstBufferPool * bpool)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);

  GST_DEBUG_OBJECT (pool, "stop flushing");

  if (pool->other_pool)
    gst_buffer_pool_set_flushing (pool->other_pool, FALSE);

  gst_poll_set_flushing (pool->poll, FALSE);
}

static GstFlowReturn
gst_v4l2_buffer_pool_poll (GstV4l2BufferPool * pool, gboolean wait)
{
  gint ret;
  GstClockTime timeout;

  if (wait)
    timeout = GST_CLOCK_TIME_NONE;
  else
    timeout = 0;

  /* In RW mode there is no queue, hence no need to wait while the queue is
   * empty */
  if (pool->obj->mode != GST_V4L2_IO_RW) {
    GST_OBJECT_LOCK (pool);

    if (!wait && pool->empty) {
      GST_OBJECT_UNLOCK (pool);
      goto no_buffers;
    }

    while (pool->empty)
      g_cond_wait (&pool->empty_cond, GST_OBJECT_GET_LOCK (pool));

    GST_OBJECT_UNLOCK (pool);
  }

  if (!pool->can_poll_device) {
    if (wait)
      goto done;
    else
      goto no_buffers;
  }

  GST_LOG_OBJECT (pool, "polling device");

again:
  ret = gst_poll_wait (pool->poll, timeout);
  if (G_UNLIKELY (ret < 0)) {
    switch (errno) {
      case EBUSY:
        goto stopped;
      case EAGAIN:
      case EINTR:
        goto again;
      case ENXIO:
        GST_WARNING_OBJECT (pool,
            "v4l2 device doesn't support polling. Disabling"
            " using libv4l2 in this case may cause deadlocks");
        pool->can_poll_device = FALSE;
        goto done;
      default:
        goto select_error;
    }
  }

  if (gst_poll_fd_has_error (pool->poll, &pool->pollfd))
    goto select_error;

  if (ret == 0)
    goto no_buffers;

done:
  return GST_FLOW_OK;

  /* ERRORS */
stopped:
  {
    GST_DEBUG_OBJECT (pool, "stop called");
    return GST_FLOW_FLUSHING;
  }
select_error:
  {
    GST_ELEMENT_ERROR (pool->obj->element, RESOURCE, READ, (NULL),
        ("poll error %d: %s (%d)", ret, g_strerror (errno), errno));
    return GST_FLOW_ERROR;
  }
no_buffers:
  return GST_FLOW_CUSTOM_SUCCESS;
}

static GstFlowReturn
gst_v4l2_buffer_pool_qbuf (GstV4l2BufferPool * pool, GstBuffer * buf,
    GstV4l2MemoryGroup * group)
{
  const GstV4l2Object *obj = pool->obj;
  GstClockTime timestamp;
  gint index;

  index = group->buffer.index;

  if (pool->buffers[index] != NULL)
    goto already_queued;

  GST_LOG_OBJECT (pool, "queuing buffer %i", index);

  if (V4L2_TYPE_IS_OUTPUT (obj->type)) {
    enum v4l2_field field;

    /* Except when field is set to alternate, buffer field is the same as
     * the one defined in format */
    if (V4L2_TYPE_IS_MULTIPLANAR (obj->type))
      field = obj->format.fmt.pix_mp.field;
    else
      field = obj->format.fmt.pix.field;

    /* NB: At this moment, we can't have alternate mode because it not handled
     * yet */
    if (field == V4L2_FIELD_ALTERNATE) {
      if (GST_BUFFER_FLAG_IS_SET (buf, GST_VIDEO_FRAME_FLAG_TFF))
        field = V4L2_FIELD_TOP;
      else
        field = V4L2_FIELD_BOTTOM;
    }

    group->buffer.field = field;
  }

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
    timestamp = GST_BUFFER_TIMESTAMP (buf);
    GST_TIME_TO_TIMEVAL (timestamp, group->buffer.timestamp);
  }

  GST_OBJECT_LOCK (pool);
  g_atomic_int_inc (&pool->num_queued);
  pool->buffers[index] = buf;

  if (!gst_v4l2_allocator_qbuf (pool->vallocator, group))
    goto queue_failed;

  pool->empty = FALSE;
  g_cond_signal (&pool->empty_cond);
  GST_OBJECT_UNLOCK (pool);

  return GST_FLOW_OK;

already_queued:
  {
    GST_ERROR_OBJECT (pool, "the buffer %i was already queued", index);
    return GST_FLOW_ERROR;
  }
queue_failed:
  {
    GST_ERROR_OBJECT (pool, "could not queue a buffer %i", index);
    /* Mark broken buffer to the allocator */
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_TAG_MEMORY);
    g_atomic_int_add (&pool->num_queued, -1);
    pool->buffers[index] = NULL;
    GST_OBJECT_UNLOCK (pool);
    return GST_FLOW_ERROR;
  }
}

static GstFlowReturn
gst_v4l2_buffer_pool_dqbuf (GstV4l2BufferPool * pool, GstBuffer ** buffer,
    gboolean wait)
{
  GstFlowReturn res;
  GstBuffer *outbuf = NULL;
  GstV4l2Object *obj = pool->obj;
  GstClockTime timestamp;
  GstV4l2MemoryGroup *group;
  GstVideoMeta *vmeta;
  gsize size;
  gint i;

  if ((res = gst_v4l2_buffer_pool_poll (pool, wait)) < GST_FLOW_OK)
    goto poll_failed;

  if (res == GST_FLOW_CUSTOM_SUCCESS) {
    GST_LOG_OBJECT (pool, "nothing to dequeue");
    goto done;
  }

  GST_LOG_OBJECT (pool, "dequeueing a buffer");

  res = gst_v4l2_allocator_dqbuf (pool->vallocator, &group);
  if (res == GST_FLOW_EOS)
    goto eos;
  if (res != GST_FLOW_OK)
    goto dqbuf_failed;

  /* get our GstBuffer with that index from the pool, if the buffer was
   * outstanding we have a serious problem.
   */
  outbuf = pool->buffers[group->buffer.index];
  if (outbuf == NULL)
    goto no_buffer;

  /* mark the buffer outstanding */
  pool->buffers[group->buffer.index] = NULL;
  if (g_atomic_int_dec_and_test (&pool->num_queued)) {
    GST_OBJECT_LOCK (pool);
    pool->empty = TRUE;
    GST_OBJECT_UNLOCK (pool);
  }

  timestamp = GST_TIMEVAL_TO_TIME (group->buffer.timestamp);

  size = 0;
  vmeta = gst_buffer_get_video_meta (outbuf);
  for (i = 0; i < group->n_mem; i++) {
    GST_LOG_OBJECT (pool,
        "dequeued buffer %p seq:%d (ix=%d), mem %p used %d, plane=%d, flags %08x, ts %"
        GST_TIME_FORMAT ", pool-queued=%d, buffer=%p", outbuf,
        group->buffer.sequence, group->buffer.index, group->mem[i],
        group->planes[i].bytesused, i, group->buffer.flags,
        GST_TIME_ARGS (timestamp), pool->num_queued, outbuf);

    if (vmeta) {
      vmeta->offset[i] = size;
      size += gst_memory_get_sizes (group->mem[i], NULL, NULL);
    }
  }

  /* Ignore timestamp and field for OUTPUT device */
  if (V4L2_TYPE_IS_OUTPUT (obj->type))
    goto done;

  /* Check for driver bug in reporting feild */
  if (group->buffer.field == V4L2_FIELD_ANY) {
    /* Only warn once to avoid the spamming */
#ifndef GST_DISABLE_GST_DEBUG
    if (!pool->has_warned_on_buggy_field) {
      pool->has_warned_on_buggy_field = TRUE;
      GST_WARNING_OBJECT (pool,
          "Driver should never set v4l2_buffer.field to ANY");
    }
#endif

    /* Use the value from the format (works for UVC bug) */
    group->buffer.field = obj->format.fmt.pix.field;

    /* If driver also has buggy S_FMT, assume progressive */
    if (group->buffer.field == V4L2_FIELD_ANY) {
#ifndef GST_DISABLE_GST_DEBUG
      if (!pool->has_warned_on_buggy_field) {
        pool->has_warned_on_buggy_field = TRUE;
        GST_WARNING_OBJECT (pool,
            "Driver should never set v4l2_format.pix.field to ANY");
      }
#endif

      group->buffer.field = V4L2_FIELD_NONE;
    }
  }

  /* set top/bottom field first if v4l2_buffer has the information */
  switch (group->buffer.field) {
    case V4L2_FIELD_NONE:
      GST_BUFFER_FLAG_UNSET (outbuf, GST_VIDEO_BUFFER_FLAG_INTERLACED);
      GST_BUFFER_FLAG_UNSET (outbuf, GST_VIDEO_BUFFER_FLAG_TFF);
      break;
    case V4L2_FIELD_INTERLACED_TB:
      GST_BUFFER_FLAG_SET (outbuf, GST_VIDEO_BUFFER_FLAG_INTERLACED);
      GST_BUFFER_FLAG_SET (outbuf, GST_VIDEO_BUFFER_FLAG_TFF);
      break;
    case V4L2_FIELD_INTERLACED_BT:
      GST_BUFFER_FLAG_SET (outbuf, GST_VIDEO_BUFFER_FLAG_INTERLACED);
      GST_BUFFER_FLAG_UNSET (outbuf, GST_VIDEO_BUFFER_FLAG_TFF);
      break;
    case V4L2_FIELD_INTERLACED:
      GST_BUFFER_FLAG_SET (outbuf, GST_VIDEO_BUFFER_FLAG_INTERLACED);
      if (obj->tv_norm == V4L2_STD_NTSC_M ||
          obj->tv_norm == V4L2_STD_NTSC_M_JP ||
          obj->tv_norm == V4L2_STD_NTSC_M_KR) {
        GST_BUFFER_FLAG_UNSET (outbuf, GST_VIDEO_BUFFER_FLAG_TFF);
      } else {
        GST_BUFFER_FLAG_SET (outbuf, GST_VIDEO_BUFFER_FLAG_TFF);
      }
      break;
    default:
      GST_BUFFER_FLAG_UNSET (outbuf, GST_VIDEO_BUFFER_FLAG_INTERLACED);
      GST_BUFFER_FLAG_UNSET (outbuf, GST_VIDEO_BUFFER_FLAG_TFF);
      GST_FIXME_OBJECT (pool,
          "Unhandled enum v4l2_field %d - treating as progressive",
          group->buffer.field);
      break;
  }

  if (GST_VIDEO_INFO_FORMAT (&obj->info) == GST_VIDEO_FORMAT_ENCODED) {
    if ((group->buffer.flags & V4L2_BUF_FLAG_KEYFRAME) ||
        GST_V4L2_PIXELFORMAT (obj) == V4L2_PIX_FMT_MJPEG ||
        GST_V4L2_PIXELFORMAT (obj) == V4L2_PIX_FMT_JPEG ||
        GST_V4L2_PIXELFORMAT (obj) == V4L2_PIX_FMT_PJPG)
      GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);
    else
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);
  }

  if (group->buffer.flags & V4L2_BUF_FLAG_ERROR)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_CORRUPTED);

  GST_BUFFER_TIMESTAMP (outbuf) = timestamp;
  GST_BUFFER_OFFSET (outbuf) = group->buffer.sequence;
  GST_BUFFER_OFFSET_END (outbuf) = group->buffer.sequence + 1;

done:
  *buffer = outbuf;

  return res;

  /* ERRORS */
poll_failed:
  {
    GST_DEBUG_OBJECT (pool, "poll error %s", gst_flow_get_name (res));
    return res;
  }
eos:
  {
    return GST_FLOW_EOS;
  }
dqbuf_failed:
  {
    return GST_FLOW_ERROR;
  }
no_buffer:
  {
    GST_ERROR_OBJECT (pool, "No free buffer found in the pool at index %d.",
        group->buffer.index);
    return GST_FLOW_ERROR;
  }
}

static GstFlowReturn
gst_v4l2_buffer_pool_acquire_buffer (GstBufferPool * bpool, GstBuffer ** buffer,
    GstBufferPoolAcquireParams * params)
{
  GstFlowReturn ret;
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);
  GstBufferPoolClass *pclass = GST_BUFFER_POOL_CLASS (parent_class);
  GstV4l2Object *obj = pool->obj;

  GST_DEBUG_OBJECT (pool, "acquire");

  /* If this is being called to resurrect a lost buffer */
  if (params && params->flags & GST_V4L2_BUFFER_POOL_ACQUIRE_FLAG_RESURRECT) {
    ret = pclass->acquire_buffer (bpool, buffer, params);
    goto done;
  }

  switch (obj->type) {
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
      /* capture, This function should return a buffer with new captured data */
      switch (obj->mode) {
        case GST_V4L2_IO_RW:
        {
          /* take empty buffer from the pool */
          ret = pclass->acquire_buffer (bpool, buffer, params);
          break;
        }
        case GST_V4L2_IO_DMABUF:
        case GST_V4L2_IO_MMAP:
        case GST_V4L2_IO_USERPTR:
        case GST_V4L2_IO_DMABUF_IMPORT:
        {
          /* just dequeue a buffer, we basically use the queue of v4l2 as the
           * storage for our buffers. This function does poll first so we can
           * interrupt it fine. */
          ret = gst_v4l2_buffer_pool_dqbuf (pool, buffer, TRUE);
          break;
        }
        default:
          ret = GST_FLOW_ERROR;
          g_assert_not_reached ();
          break;
      }
      break;


    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
      /* playback, This function should return an empty buffer */
      switch (obj->mode) {
        case GST_V4L2_IO_RW:
          /* get an empty buffer */
          ret = pclass->acquire_buffer (bpool, buffer, params);
          break;

        case GST_V4L2_IO_MMAP:
        case GST_V4L2_IO_DMABUF:
        case GST_V4L2_IO_USERPTR:
        case GST_V4L2_IO_DMABUF_IMPORT:
          /* get a free unqueued buffer */
          ret = pclass->acquire_buffer (bpool, buffer, params);
          break;

        default:
          ret = GST_FLOW_ERROR;
          g_assert_not_reached ();
          break;
      }
      break;

    default:
      ret = GST_FLOW_ERROR;
      g_assert_not_reached ();
      break;
  }
done:
  return ret;
}

static void
gst_v4l2_buffer_pool_release_buffer (GstBufferPool * bpool, GstBuffer * buffer)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);
  GstBufferPoolClass *pclass = GST_BUFFER_POOL_CLASS (parent_class);
  GstV4l2Object *obj = pool->obj;

  GST_DEBUG_OBJECT (pool, "release buffer %p", buffer);

  /* If the buffer's pool has been orphaned, dispose of it so that
   * the pool resources can be freed */
  if (pool->orphaned) {
    GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_TAG_MEMORY);
    pclass->release_buffer (bpool, buffer);
    return;
  }

  switch (obj->type) {
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
      /* capture, put the buffer back in the queue so that we can refill it
       * later. */
      switch (obj->mode) {
        case GST_V4L2_IO_RW:
          /* release back in the pool */
          pclass->release_buffer (bpool, buffer);
          break;

        case GST_V4L2_IO_DMABUF:
        case GST_V4L2_IO_MMAP:
        case GST_V4L2_IO_USERPTR:
        case GST_V4L2_IO_DMABUF_IMPORT:
        {
          GstV4l2MemoryGroup *group;
          if (gst_v4l2_is_buffer_valid (buffer, &group)) {
            GstFlowReturn ret = GST_FLOW_OK;

            gst_v4l2_allocator_reset_group (pool->vallocator, group);
            /* queue back in the device */
            if (pool->other_pool)
              ret = gst_v4l2_buffer_pool_prepare_buffer (pool, buffer, NULL);
            if (ret != GST_FLOW_OK ||
                gst_v4l2_buffer_pool_qbuf (pool, buffer, group) != GST_FLOW_OK)
              pclass->release_buffer (bpool, buffer);
          } else {
            /* Simply release invalid/modified buffer, the allocator will
             * give it back later */
            GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_TAG_MEMORY);
            pclass->release_buffer (bpool, buffer);
          }
          break;
        }
        default:
          g_assert_not_reached ();
          break;
      }
      break;

    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
      switch (obj->mode) {
        case GST_V4L2_IO_RW:
          /* release back in the pool */
          pclass->release_buffer (bpool, buffer);
          break;

        case GST_V4L2_IO_MMAP:
        case GST_V4L2_IO_DMABUF:
        case GST_V4L2_IO_USERPTR:
        case GST_V4L2_IO_DMABUF_IMPORT:
        {
          GstV4l2MemoryGroup *group;
          guint index;

          if (!gst_v4l2_is_buffer_valid (buffer, &group)) {
            /* Simply release invalid/modified buffer, the allocator will
             * give it back later */
            GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_TAG_MEMORY);
            pclass->release_buffer (bpool, buffer);
            break;
          }

          index = group->buffer.index;

          if (pool->buffers[index] == NULL) {
            GST_LOG_OBJECT (pool, "buffer %u not queued, putting on free list",
                index);

            /* Remove qdata, this will unmap any map data in userptr */
            gst_mini_object_set_qdata (GST_MINI_OBJECT (buffer),
                GST_V4L2_IMPORT_QUARK, NULL, NULL);

            /* reset to default size */
            gst_v4l2_allocator_reset_group (pool->vallocator, group);

            /* playback, put the buffer back in the queue to refill later. */
            pclass->release_buffer (bpool, buffer);
          } else {
            /* the buffer is queued in the device but maybe not played yet. We just
             * leave it there and not make it available for future calls to acquire
             * for now. The buffer will be dequeued and reused later. */
            GST_LOG_OBJECT (pool, "buffer %u is queued", index);
          }
          break;
        }

        default:
          g_assert_not_reached ();
          break;
      }
      break;

    default:
      g_assert_not_reached ();
      break;
  }
}

static void
gst_v4l2_buffer_pool_dispose (GObject * object)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (object);

  if (pool->vallocator)
    gst_object_unref (pool->vallocator);
  pool->vallocator = NULL;

  if (pool->allocator)
    gst_object_unref (pool->allocator);
  pool->allocator = NULL;

  if (pool->other_pool)
    gst_object_unref (pool->other_pool);
  pool->other_pool = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_v4l2_buffer_pool_finalize (GObject * object)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (object);

  if (pool->video_fd >= 0)
    pool->obj->close (pool->video_fd);

  gst_poll_free (pool->poll);

  /* This can't be done in dispose method because we must not set pointer
   * to NULL as it is part of the v4l2object and dispose could be called
   * multiple times */
  gst_object_unref (pool->obj->element);

  g_cond_clear (&pool->empty_cond);

  /* FIXME have we done enough here ? */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_v4l2_buffer_pool_init (GstV4l2BufferPool * pool)
{
  pool->poll = gst_poll_new (TRUE);
  pool->can_poll_device = TRUE;
  g_cond_init (&pool->empty_cond);
  pool->empty = TRUE;
  pool->orphaned = FALSE;
}

static void
gst_v4l2_buffer_pool_class_init (GstV4l2BufferPoolClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GstBufferPoolClass *bufferpool_class = GST_BUFFER_POOL_CLASS (klass);

  object_class->dispose = gst_v4l2_buffer_pool_dispose;
  object_class->finalize = gst_v4l2_buffer_pool_finalize;

  bufferpool_class->start = gst_v4l2_buffer_pool_start;
  bufferpool_class->stop = gst_v4l2_buffer_pool_stop;
  bufferpool_class->set_config = gst_v4l2_buffer_pool_set_config;
  bufferpool_class->alloc_buffer = gst_v4l2_buffer_pool_alloc_buffer;
  bufferpool_class->acquire_buffer = gst_v4l2_buffer_pool_acquire_buffer;
  bufferpool_class->release_buffer = gst_v4l2_buffer_pool_release_buffer;
  bufferpool_class->flush_start = gst_v4l2_buffer_pool_flush_start;
  bufferpool_class->flush_stop = gst_v4l2_buffer_pool_flush_stop;

  GST_DEBUG_CATEGORY_INIT (v4l2bufferpool_debug, "v4l2bufferpool", 0,
      "V4L2 Buffer Pool");
  GST_DEBUG_CATEGORY_GET (CAT_PERFORMANCE, "GST_PERFORMANCE");
}

/**
 * gst_v4l2_buffer_pool_new:
 * @obj:  the v4l2 object owning the pool
 *
 * Construct a new buffer pool.
 *
 * Returns: the new pool, use gst_object_unref() to free resources
 */
GstBufferPool *
gst_v4l2_buffer_pool_new (GstV4l2Object * obj, GstCaps * caps)
{
  GstV4l2BufferPool *pool;
  GstStructure *config;
  gchar *name, *parent_name;
  gint fd;

  fd = obj->dup (obj->video_fd);
  if (fd < 0)
    goto dup_failed;

  /* setting a significant unique name */
  parent_name = gst_object_get_name (GST_OBJECT (obj->element));
  name = g_strconcat (parent_name, ":", "pool:",
      V4L2_TYPE_IS_OUTPUT (obj->type) ? "sink" : "src", NULL);
  g_free (parent_name);

  pool = (GstV4l2BufferPool *) g_object_new (GST_TYPE_V4L2_BUFFER_POOL,
      "name", name, NULL);
  g_object_ref_sink (pool);
  g_free (name);

  gst_poll_fd_init (&pool->pollfd);
  pool->pollfd.fd = fd;
  gst_poll_add_fd (pool->poll, &pool->pollfd);
  if (V4L2_TYPE_IS_OUTPUT (obj->type))
    gst_poll_fd_ctl_write (pool->poll, &pool->pollfd, TRUE);
  else
    gst_poll_fd_ctl_read (pool->poll, &pool->pollfd, TRUE);

  pool->video_fd = fd;
  pool->obj = obj;
  pool->can_poll_device = TRUE;

  pool->vallocator = gst_v4l2_allocator_new (GST_OBJECT (pool), obj);
  if (pool->vallocator == NULL)
    goto allocator_failed;

  gst_object_ref (obj->element);

  config = gst_buffer_pool_get_config (GST_BUFFER_POOL_CAST (pool));
  gst_buffer_pool_config_set_params (config, caps, obj->info.size, 0, 0);
  /* This will simply set a default config, but will not configure the pool
   * because min and max are not valid */
  gst_buffer_pool_set_config (GST_BUFFER_POOL_CAST (pool), config);

  return GST_BUFFER_POOL (pool);

  /* ERRORS */
dup_failed:
  {
    GST_ERROR ("failed to dup fd %d (%s)", errno, g_strerror (errno));
    return NULL;
  }
allocator_failed:
  {
    GST_ERROR_OBJECT (pool, "Failed to create V4L2 allocator");
    gst_object_unref (pool);
    return NULL;
  }
}

static GstFlowReturn
gst_v4l2_do_read (GstV4l2BufferPool * pool, GstBuffer * buf)
{
  GstFlowReturn res;
  GstV4l2Object *obj = pool->obj;
  gint amount;
  GstMapInfo map;
  gint toread;

  toread = obj->info.size;

  GST_LOG_OBJECT (pool, "reading %d bytes into buffer %p", toread, buf);

  gst_buffer_map (buf, &map, GST_MAP_WRITE);

  do {
    if ((res = gst_v4l2_buffer_pool_poll (pool, TRUE)) != GST_FLOW_OK)
      goto poll_error;

    amount = obj->read (obj->video_fd, map.data, toread);

    if (amount == toread) {
      break;
    } else if (amount == -1) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      } else
        goto read_error;
    } else {
      /* short reads can happen if a signal interrupts the read */
      continue;
    }
  } while (TRUE);

  GST_LOG_OBJECT (pool, "read %d bytes", amount);
  gst_buffer_unmap (buf, &map);
  gst_buffer_resize (buf, 0, amount);

  return GST_FLOW_OK;

  /* ERRORS */
poll_error:
  {
    GST_DEBUG ("poll error %s", gst_flow_get_name (res));
    goto cleanup;
  }
read_error:
  {
    GST_ELEMENT_ERROR (obj->element, RESOURCE, READ,
        (_("Error reading %d bytes from device '%s'."),
            toread, obj->videodev), GST_ERROR_SYSTEM);
    res = GST_FLOW_ERROR;
    goto cleanup;
  }
cleanup:
  {
    gst_buffer_unmap (buf, &map);
    gst_buffer_resize (buf, 0, 0);
    return res;
  }
}

/**
 * gst_v4l2_buffer_pool_process:
 * @bpool: a #GstBufferPool
 * @buf: a #GstBuffer, maybe be replaced
 *
 * Process @buf in @bpool. For capture devices, this functions fills @buf with
 * data from the device. For output devices, this functions send the contents of
 * @buf to the device for playback.
 *
 * Returns: %GST_FLOW_OK on success.
 */
GstFlowReturn
gst_v4l2_buffer_pool_process (GstV4l2BufferPool * pool, GstBuffer ** buf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstBufferPool *bpool = GST_BUFFER_POOL_CAST (pool);
  GstV4l2Object *obj = pool->obj;

  GST_DEBUG_OBJECT (pool, "process buffer %p", buf);

  if (GST_BUFFER_POOL_IS_FLUSHING (pool))
    return GST_FLOW_FLUSHING;

  switch (obj->type) {
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
      /* capture */
      switch (obj->mode) {
        case GST_V4L2_IO_RW:
          /* capture into the buffer */
          ret = gst_v4l2_do_read (pool, *buf);
          break;

        case GST_V4L2_IO_MMAP:
        case GST_V4L2_IO_DMABUF:
        {
          GstBuffer *tmp;

          if ((*buf)->pool == bpool) {
            guint num_queued;
            gsize size = gst_buffer_get_size (*buf);

            /* Legacy M2M devices return empty buffer when drained */
            if (size == 0 && GST_V4L2_IS_M2M (obj->device_caps))
              goto eos;

            if (GST_VIDEO_INFO_FORMAT (&pool->caps_info) !=
                GST_VIDEO_FORMAT_ENCODED && size < pool->size)
              goto buffer_truncated;

            num_queued = g_atomic_int_get (&pool->num_queued);
            GST_TRACE_OBJECT (pool, "Only %i buffer left in the capture queue.",
                num_queued);

            /* If we have no more buffer, and can allocate it time to do so */
            if (num_queued == 0) {
              if (GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, MMAP)) {
                ret = gst_v4l2_buffer_pool_resurrect_buffer (pool);
                if (ret == GST_FLOW_OK)
                  goto done;
              }
            }

            /* start copying buffers when we are running low on buffers */
            if (num_queued < pool->copy_threshold) {
              GstBuffer *copy;

              if (GST_V4L2_ALLOCATOR_CAN_ALLOCATE (pool->vallocator, MMAP)) {
                ret = gst_v4l2_buffer_pool_resurrect_buffer (pool);
                if (ret == GST_FLOW_OK)
                  goto done;
              }

              /* copy the buffer */
              copy = gst_buffer_copy_region (*buf,
                  GST_BUFFER_COPY_ALL | GST_BUFFER_COPY_DEEP, 0, -1);
              GST_LOG_OBJECT (pool, "copy buffer %p->%p", *buf, copy);

              /* and requeue so that we can continue capturing */
              gst_buffer_unref (*buf);
              *buf = copy;
            }

            ret = GST_FLOW_OK;
            /* nothing, data was inside the buffer when we did _acquire() */
            goto done;
          }

          /* buffer not from our pool, grab a frame and copy it into the target */
          if ((ret = gst_v4l2_buffer_pool_dqbuf (pool, &tmp, TRUE))
              != GST_FLOW_OK)
            goto done;

          /* An empty buffer on capture indicates the end of stream */
          if (gst_buffer_get_size (tmp) == 0) {
            gst_v4l2_buffer_pool_release_buffer (bpool, tmp);

            /* Legacy M2M devices return empty buffer when drained */
            if (GST_V4L2_IS_M2M (obj->device_caps))
              goto eos;
          }

          ret = gst_v4l2_buffer_pool_copy_buffer (pool, *buf, tmp);

          /* an queue the buffer again after the copy */
          gst_v4l2_buffer_pool_release_buffer (bpool, tmp);

          if (ret != GST_FLOW_OK)
            goto copy_failed;
          break;
        }

        case GST_V4L2_IO_USERPTR:
        {
          struct UserPtrData *data;
          GstBuffer *tmp;

          /* Replace our buffer with downstream allocated buffer */
          data = gst_mini_object_steal_qdata (GST_MINI_OBJECT (*buf),
              GST_V4L2_IMPORT_QUARK);
          tmp = gst_buffer_ref (data->buffer);
          _unmap_userptr_frame (data);

          /* Now tmp is writable, copy the flags and timestamp */
          gst_buffer_copy_into (tmp, *buf,
              GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, -1);

          gst_buffer_replace (buf, tmp);
          gst_buffer_unref (tmp);
          break;
        }

        case GST_V4L2_IO_DMABUF_IMPORT:
        {
          GstBuffer *tmp;

          /* Replace our buffer with downstream allocated buffer */
          tmp = gst_mini_object_steal_qdata (GST_MINI_OBJECT (*buf),
              GST_V4L2_IMPORT_QUARK);

          gst_buffer_copy_into (tmp, *buf,
              GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS, 0, -1);

          gst_buffer_replace (buf, tmp);
          gst_buffer_unref (tmp);
          break;
        }

        default:
          g_assert_not_reached ();
          break;
      }
      break;

    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
      /* playback */
      switch (obj->mode) {
        case GST_V4L2_IO_RW:
          /* FIXME, do write() */
          GST_WARNING_OBJECT (pool, "implement write()");
          break;

        case GST_V4L2_IO_USERPTR:
        case GST_V4L2_IO_DMABUF_IMPORT:
        case GST_V4L2_IO_DMABUF:
        case GST_V4L2_IO_MMAP:
        {
          GstBuffer *to_queue = NULL;
          GstBuffer *buffer;
          GstV4l2MemoryGroup *group;
          gint index;

          if ((*buf)->pool != bpool)
            goto copying;

          if (!gst_v4l2_is_buffer_valid (*buf, &group))
            goto copying;

          index = group->buffer.index;

          GST_LOG_OBJECT (pool, "processing buffer %i from our pool", index);

          if (pool->buffers[index] != NULL) {
            GST_LOG_OBJECT (pool, "buffer %i already queued, copying", index);
            goto copying;
          }

          /* we can queue directly */
          to_queue = gst_buffer_ref (*buf);

        copying:
          if (to_queue == NULL) {
            GstBufferPoolAcquireParams params = { 0 };

            GST_LOG_OBJECT (pool, "alloc buffer from our pool");

            /* this can return EOS if all buffers are outstanding which would
             * be strange because we would expect the upstream element to have
             * allocated them and returned to us.. */
            params.flags = GST_BUFFER_POOL_ACQUIRE_FLAG_DONTWAIT;
            ret = gst_buffer_pool_acquire_buffer (bpool, &to_queue, &params);
            if (ret != GST_FLOW_OK)
              goto acquire_failed;

            ret = gst_v4l2_buffer_pool_prepare_buffer (pool, to_queue, *buf);
            if (ret != GST_FLOW_OK) {
              gst_buffer_unref (to_queue);
              goto prepare_failed;
            }

            /* retrieve the group */
            gst_v4l2_is_buffer_valid (to_queue, &group);
          }

          if ((ret = gst_v4l2_buffer_pool_qbuf (pool, to_queue, group))
              != GST_FLOW_OK)
            goto queue_failed;

          /* if we are not streaming yet (this is the first buffer, start
           * streaming now */
          if (!gst_v4l2_buffer_pool_streamon (pool)) {
            /* don't check return value because qbuf would have failed */
            gst_v4l2_is_buffer_valid (to_queue, &group);

            /* qbuf has stored to_queue buffer but we are not in
             * streaming state, so the flush logic won't be performed.
             * To avoid leaks, flush the allocator and restore the queued
             * buffer as non-queued */
            gst_v4l2_allocator_flush (pool->vallocator);

            pool->buffers[group->buffer.index] = NULL;

            gst_mini_object_set_qdata (GST_MINI_OBJECT (to_queue),
                GST_V4L2_IMPORT_QUARK, NULL, NULL);
            gst_buffer_unref (to_queue);
            g_atomic_int_add (&pool->num_queued, -1);
            goto start_failed;
          }

          /* Remove our ref, we will still hold this buffer in acquire as needed,
           * otherwise the pool will think it is outstanding and will refuse to stop. */
          gst_buffer_unref (to_queue);

          /* release as many buffer as possible */
          while (gst_v4l2_buffer_pool_dqbuf (pool, &buffer, FALSE) ==
              GST_FLOW_OK) {
            if (buffer->pool == NULL)
              gst_v4l2_buffer_pool_release_buffer (bpool, buffer);
          }

          if (g_atomic_int_get (&pool->num_queued) >= pool->min_latency) {
            /* all buffers are queued, try to dequeue one and release it back
             * into the pool so that _acquire can get to it again. */
            ret = gst_v4l2_buffer_pool_dqbuf (pool, &buffer, TRUE);
            if (ret == GST_FLOW_OK && buffer->pool == NULL)
              /* release the rendered buffer back into the pool. This wakes up any
               * thread waiting for a buffer in _acquire(). */
              gst_v4l2_buffer_pool_release_buffer (bpool, buffer);
          }
          break;
        }
        default:
          g_assert_not_reached ();
          break;
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
done:
  return ret;

  /* ERRORS */
copy_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to copy buffer");
    return ret;
  }
buffer_truncated:
  {
    GST_WARNING_OBJECT (pool,
        "Dropping truncated buffer, this is likely a driver bug.");
    gst_buffer_unref (*buf);
    *buf = NULL;
    return GST_V4L2_FLOW_CORRUPTED_BUFFER;
  }
eos:
  {
    GST_DEBUG_OBJECT (pool, "end of stream reached");
    gst_buffer_unref (*buf);
    *buf = NULL;
    return GST_V4L2_FLOW_LAST_BUFFER;
  }
acquire_failed:
  {
    if (ret == GST_FLOW_FLUSHING)
      GST_DEBUG_OBJECT (pool, "flushing");
    else
      GST_WARNING_OBJECT (pool, "failed to acquire a buffer: %s",
          gst_flow_get_name (ret));
    return ret;
  }
prepare_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to prepare data");
    return ret;
  }
queue_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to queue buffer");
    return ret;
  }
start_failed:
  {
    GST_ERROR_OBJECT (pool, "failed to start streaming");
    return GST_FLOW_ERROR;
  }
}

void
gst_v4l2_buffer_pool_set_other_pool (GstV4l2BufferPool * pool,
    GstBufferPool * other_pool)
{
  g_return_if_fail (!gst_buffer_pool_is_active (GST_BUFFER_POOL (pool)));

  if (pool->other_pool)
    gst_object_unref (pool->other_pool);
  pool->other_pool = gst_object_ref (other_pool);
}

void
gst_v4l2_buffer_pool_copy_at_threshold (GstV4l2BufferPool * pool, gboolean copy)
{
  GST_OBJECT_LOCK (pool);
  pool->enable_copy_threshold = copy;
  GST_OBJECT_UNLOCK (pool);
}

gboolean
gst_v4l2_buffer_pool_flush (GstBufferPool * bpool)
{
  GstV4l2BufferPool *pool = GST_V4L2_BUFFER_POOL (bpool);
  gboolean ret = TRUE;

  gst_v4l2_buffer_pool_streamoff (pool);

  if (!V4L2_TYPE_IS_OUTPUT (pool->obj->type))
    ret = gst_v4l2_buffer_pool_streamon (pool);

  return ret;
}
