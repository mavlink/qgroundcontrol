/* GStreamer RTP H.265 unit test
 *
 * Copyright (C) 2017 Centricular Ltd
 *   @author: Tim-Philipp Müller <tim@centricular.com>
 * Copyright (C) 2018 Collabora Ltd
 *   @author: Nicolas Dufresne <nicolas.dufresne@collabora.com>
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

#include <gst/check/check.h>
#include <gst/app/app.h>
#include <gst/rtp/gstrtpbuffer.h>

#define ALLOCATOR_CUSTOM_SYSMEM "CustomSysMem"

static GstAllocator *custom_sysmem_allocator;   /* NULL */

/* Custom memory */

typedef struct
{
  GstMemory mem;
  guint8 *data;
  guint8 *allocdata;
} CustomSysmem;

static CustomSysmem *
custom_sysmem_new (GstMemoryFlags flags, gsize maxsize, gsize align,
    gsize offset, gsize size)
{
  gsize aoffset, padding;
  CustomSysmem *mem;

  /* ensure configured alignment */
  align |= gst_memory_alignment;
  /* allocate more to compensate for alignment */
  maxsize += align;

  mem = g_new0 (CustomSysmem, 1);

  mem->allocdata = g_malloc (maxsize);

  mem->data = mem->allocdata;

  /* do alignment */
  if ((aoffset = ((guintptr) mem->data & align))) {
    aoffset = (align + 1) - aoffset;
    mem->data += aoffset;
    maxsize -= aoffset;
  }

  if (offset && (flags & GST_MEMORY_FLAG_ZERO_PREFIXED))
    memset (mem->data, 0, offset);

  padding = maxsize - (offset + size);
  if (padding && (flags & GST_MEMORY_FLAG_ZERO_PADDED))
    memset (mem->data + offset + size, 0, padding);

  gst_memory_init (GST_MEMORY_CAST (mem), flags, custom_sysmem_allocator,
      NULL, maxsize, align, offset, size);

  return mem;
}

static gpointer
custom_sysmem_map (CustomSysmem * mem, gsize maxsize, GstMapFlags flags)
{
  return mem->data;
}

static gboolean
custom_sysmem_unmap (CustomSysmem * mem)
{
  return TRUE;
}

static CustomSysmem *
custom_sysmem_copy (CustomSysmem * mem, gssize offset, gsize size)
{
  g_return_val_if_reached (NULL);
}

static CustomSysmem *
custom_sysmem_share (CustomSysmem * mem, gssize offset, gsize size)
{
  g_return_val_if_reached (NULL);
}

static gboolean
custom_sysmem_is_span (CustomSysmem * mem1, CustomSysmem * mem2, gsize * offset)
{
  g_return_val_if_reached (FALSE);
}

/* Custom allocator */

typedef struct
{
  GstAllocator allocator;
} CustomSysmemAllocator;

typedef struct
{
  GstAllocatorClass allocator_class;
} CustomSysmemAllocatorClass;

GType custom_sysmem_allocator_get_type (void);
G_DEFINE_TYPE (CustomSysmemAllocator, custom_sysmem_allocator,
    GST_TYPE_ALLOCATOR);

static GstMemory *
custom_sysmem_allocator_alloc (GstAllocator * allocator, gsize size,
    GstAllocationParams * params)
{
  gsize maxsize = size + params->prefix + params->padding;

  return (GstMemory *) custom_sysmem_new (params->flags,
      maxsize, params->align, params->prefix, size);
}

static void
custom_sysmem_allocator_free (GstAllocator * allocator, GstMemory * mem)
{
  CustomSysmem *csmem = (CustomSysmem *) mem;

  g_free (csmem->allocdata);
  g_free (csmem);
}

static void
custom_sysmem_allocator_class_init (CustomSysmemAllocatorClass * klass)
{
  GstAllocatorClass *allocator_class = (GstAllocatorClass *) klass;

  allocator_class->alloc = custom_sysmem_allocator_alloc;
  allocator_class->free = custom_sysmem_allocator_free;
}

static void
custom_sysmem_allocator_init (CustomSysmemAllocator * allocator)
{
  GstAllocator *alloc = GST_ALLOCATOR_CAST (allocator);

  alloc->mem_type = ALLOCATOR_CUSTOM_SYSMEM;
  alloc->mem_map = (GstMemoryMapFunction) custom_sysmem_map;
  alloc->mem_unmap = (GstMemoryUnmapFunction) custom_sysmem_unmap;
  alloc->mem_copy = (GstMemoryCopyFunction) custom_sysmem_copy;
  alloc->mem_share = (GstMemoryShareFunction) custom_sysmem_share;
  alloc->mem_is_span = (GstMemoryIsSpanFunction) custom_sysmem_is_span;
}

/* AppSink subclass proposing our custom allocator to upstream */

typedef struct
{
  GstAppSink appsink;
} CMemAppSink;

typedef struct
{
  GstAppSinkClass appsink;
} CMemAppSinkClass;

GType c_mem_app_sink_get_type (void);

G_DEFINE_TYPE (CMemAppSink, c_mem_app_sink, GST_TYPE_APP_SINK);

static void
c_mem_app_sink_init (CMemAppSink * cmemsink)
{
}

static gboolean
c_mem_app_sink_propose_allocation (GstBaseSink * sink, GstQuery * query)
{
  gst_query_add_allocation_param (query, custom_sysmem_allocator, NULL);
  return TRUE;
}

static void
c_mem_app_sink_class_init (CMemAppSinkClass * klass)
{
  GstBaseSinkClass *basesink_class = (GstBaseSinkClass *) klass;

  basesink_class->propose_allocation = c_mem_app_sink_propose_allocation;
}

#define RTP_H265_FILE GST_TEST_FILES_PATH G_DIR_SEPARATOR_S "h265.rtp"

GST_START_TEST (test_rtph265depay_with_downstream_allocator)
{
  GstElement *pipeline, *src, *depay, *sink;
  GstMemory *mem;
  GstSample *sample;
  GstBuffer *buf;
  GstCaps *caps;

  custom_sysmem_allocator =
      g_object_new (custom_sysmem_allocator_get_type (), NULL);

  pipeline = gst_pipeline_new ("pipeline");

  src = gst_element_factory_make ("appsrc", NULL);

  caps = gst_caps_new_simple ("application/x-rtp",
      "media", G_TYPE_STRING, "video",
      "payload", G_TYPE_INT, 96,
      "clock-rate", G_TYPE_INT, 90000,
      "encoding-name", G_TYPE_STRING, "H265",
      "ssrc", G_TYPE_UINT, 1990683810,
      "timestamp-offset", G_TYPE_UINT, 3697583446UL,
      "seqnum-offset", G_TYPE_UINT, 15568,
      "a-framerate", G_TYPE_STRING, "30", NULL);
  g_object_set (src, "format", GST_FORMAT_TIME, "caps", caps, NULL);
  gst_bin_add (GST_BIN (pipeline), src);
  gst_caps_unref (caps);

  depay = gst_element_factory_make ("rtph265depay", NULL);
  gst_bin_add (GST_BIN (pipeline), depay);

  sink = g_object_new (c_mem_app_sink_get_type (), NULL);
  gst_bin_add (GST_BIN (pipeline), sink);

  gst_element_link_many (src, depay, sink, NULL);

  gst_element_set_state (pipeline, GST_STATE_PAUSED);

  {
    gchar *data, *pdata;
    gsize len;

    fail_unless (g_file_get_contents (RTP_H265_FILE, &data, &len, NULL));
    fail_unless (len > 2);

    pdata = data;
    while (len > 2) {
      GstFlowReturn flow;
      guint16 packet_len;

      packet_len = GST_READ_UINT16_BE (pdata);
      GST_INFO ("rtp packet length: %u (bytes left: %u)", packet_len,
          (guint) len);
      fail_unless (len >= 2 + packet_len);

      flow = gst_app_src_push_buffer (GST_APP_SRC (src),
          gst_buffer_new_wrapped (g_memdup (pdata + 2, packet_len),
              packet_len));

      fail_unless_equals_int (flow, GST_FLOW_OK);

      pdata += 2 + packet_len;
      len -= 2 + packet_len;
    }

    g_free (data);
  }

  gst_app_src_end_of_stream (GST_APP_SRC (src));

  sample = gst_app_sink_pull_preroll (GST_APP_SINK (sink));
  fail_unless (sample != NULL);

  buf = gst_sample_get_buffer (sample);

  GST_LOG ("buffer has %u memories", gst_buffer_n_memory (buf));
  GST_LOG ("buffer size: %u", (guint) gst_buffer_get_size (buf));

  fail_unless (gst_buffer_n_memory (buf) > 0);
  mem = gst_buffer_peek_memory (buf, 0);
  fail_unless (mem != NULL);

  GST_LOG ("buffer memory type: %s", mem->allocator->mem_type);
  fail_unless (gst_memory_is_type (mem, ALLOCATOR_CUSTOM_SYSMEM));

  gst_sample_unref (sample);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);

  g_object_unref (custom_sysmem_allocator);
  custom_sysmem_allocator = NULL;
}

GST_END_TEST;


static GstBuffer *
wrap_static_buffer_with_pts (guint8 * buf, gsize size, GstClockTime pts)
{
  GstBuffer *buffer;

  buffer = gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
      buf, size, 0, size, NULL, NULL);
  GST_BUFFER_PTS (buffer) = pts;

  return buffer;
}

static GstBuffer *
wrap_static_buffer (guint8 * buf, gsize size)
{
  return wrap_static_buffer_with_pts (buf, size, GST_CLOCK_TIME_NONE);
}

/* This was generated using pipeline:
 * gst-launch-1.0 videotestsrc num-buffers=1 pattern=green \
 *     ! video/x-raw,width=64,height=64 ! x265enc ! h265parse \
 *     ! rtph265pay ! fakesink dump=1
 */
/* RTP h265_idr + marker */
static guint8 rtp_h265_idr[] = {
  0x80, 0xe0, 0x2c, 0x6a, 0xab, 0x7f, 0x71, 0xc0,
  0x8d, 0x11, 0x33, 0x07, 0x28, 0x01, 0xaf, 0x05,
  0x38, 0x4a, 0x03, 0x06, 0x7c, 0x7a, 0xb1, 0x8b,
  0xff, 0xfe, 0xfd, 0xb7, 0xff, 0xff, 0xd1, 0xff,
  0x40, 0x06, 0xd8, 0xd3, 0xb2, 0xf8
};

GST_START_TEST (test_rtph265depay_eos)
{
  GstHarness *h = gst_harness_new ("rtph265depay");
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret;

  gst_harness_set_caps_str (h,
      "application/x-rtp,media=video,clock-rate=90000,encoding-name=H265",
      "video/x-h265,alignment=au,stream-format=byte-stream");

  buffer = wrap_static_buffer (rtp_h265_idr, sizeof (rtp_h265_idr));
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_WRITE, &rtp));
  gst_rtp_buffer_set_marker (&rtp, FALSE);
  gst_rtp_buffer_unmap (&rtp);

  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 0);

  fail_unless (gst_harness_push_event (h, gst_event_new_eos ()));
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  gst_harness_teardown (h);
}

GST_END_TEST;


GST_START_TEST (test_rtph265depay_marker_to_flag)
{
  GstHarness *h = gst_harness_new ("rtph265depay");
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret;
  guint16 seq;

  gst_harness_set_caps_str (h,
      "application/x-rtp,media=video,clock-rate=90000,encoding-name=H265",
      "video/x-h265,alignment=au,stream-format=byte-stream");

  buffer = wrap_static_buffer (rtp_h265_idr, sizeof (rtp_h265_idr));
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  seq = gst_rtp_buffer_get_seq (&rtp);
  gst_rtp_buffer_unmap (&rtp);

  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  buffer = wrap_static_buffer (rtp_h265_idr, sizeof (rtp_h265_idr));
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_WRITE, &rtp));
  gst_rtp_buffer_set_marker (&rtp, FALSE);
  gst_rtp_buffer_set_seq (&rtp, ++seq);
  gst_rtp_buffer_unmap (&rtp);

  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  /* the second NAL is blocked as there is no marker to let the payloader
   * know it's a complete AU, we'll use an EOS to unblock it */
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);
  fail_unless (gst_harness_push_event (h, gst_event_new_eos ()));
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  buffer = gst_harness_pull (h);
  fail_unless (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_MARKER));
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_MARKER));
  gst_buffer_unref (buffer);

  gst_harness_teardown (h);
}

GST_END_TEST;

/* These were generated using pipeline:
 * gst-launch-1.0 videotestsrc num-buffers=1 pattern=green \
 *     ! video/x-raw,width=256,height=256 \
 *     ! x265enc option-string="slices=2" \
 *     ! fakesink dump=1
 */

static guint8 h265_vps[] = {
  0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01,
  0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00,
  0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00,
  0x3f, 0x95, 0x98, 0x09
};

static guint8 h265_sps[] = {
  0x00, 0x00, 0x00, 0x01, 0x42, 0x01, 0x01, 0x01,
  0x60, 0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x03, 0x00, 0x3f, 0xa0, 0x08,
  0x08, 0x04, 0x05, 0x96, 0x56, 0x69, 0x24, 0xca,
  0xff, 0xf0, 0x00, 0x10, 0x00, 0x10, 0x10, 0x00,
  0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x03, 0x01,
  0xe0, 0x80
};

static guint8 h265_pps[] = {
  0x00, 0x00, 0x00, 0x01, 0x44, 0x01, 0xc1, 0x72,
  0xb4, 0x42, 0x40
};

/* IDR Slice 1 */
static guint8 h265_idr_slice_1[] = {
  0x00, 0x00, 0x00, 0x01, 0x28, 0x01, 0xaf, 0x08,
  0xa2, 0xe6, 0xa3, 0xc6, 0x53, 0x90, 0xea, 0xc8,
  0x3f, 0xfe, 0xfa, 0xf9, 0x3f, 0xf2, 0x61, 0x98,
  0xef, 0xf4, 0xe9, 0x97, 0xe7, 0xc2, 0x74, 0x78,
  0x98, 0x10, 0x01, 0x21, 0xa4, 0x3c, 0x4c, 0x08,
  0x00, 0x3e, 0x40, 0x92, 0x0c, 0x78
};

/* IDR Slice 2 */
static guint8 h265_idr_slice_2[] = {
  0x00, 0x00, 0x01, 0x28, 0x01, 0x30, 0xf0, 0x8a,
  0x2e, 0x60, 0xa3, 0xc6, 0x53, 0x90, 0xea, 0xc8,
  0x3f, 0xfe, 0xfa, 0xf9, 0x3f, 0xf2, 0x61, 0x98,
  0xef, 0xf4, 0xe9, 0x97, 0xe7, 0xc2, 0x74, 0x78,
  0x98, 0x10, 0x01, 0x21, 0xa4, 0x3c, 0x4c, 0x08,
  0x00, 0x3e, 0x40, 0x92, 0x0c, 0x78
};


GST_START_TEST (test_rtph265pay_two_slices_timestamp)
{
  GstHarness *h = gst_harness_new_parse ("rtph265pay timestamp-offset=123");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=nal,stream-format=byte-stream");

  ret = gst_harness_push (h, wrap_static_buffer_with_pts (h265_idr_slice_1,
          sizeof (h265_idr_slice_1), 0));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  ret = gst_harness_push (h, wrap_static_buffer_with_pts (h265_idr_slice_2,
          sizeof (h265_idr_slice_2), 0));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  ret = gst_harness_push (h, wrap_static_buffer_with_pts (h265_idr_slice_1,
          sizeof (h265_idr_slice_1), GST_SECOND));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  ret = gst_harness_push (h, wrap_static_buffer_with_pts (h265_idr_slice_2,
          sizeof (h265_idr_slice_2), GST_SECOND));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 4);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), GST_SECOND);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123 + 90000);
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), GST_SECOND);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123 + 90000);
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtph265pay_marker_for_flag)
{
  GstHarness *h = gst_harness_new_parse ("rtph265pay timestamp-offset=123");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=nal,stream-format=byte-stream");

  ret = gst_harness_push (h, wrap_static_buffer (h265_idr_slice_1,
          sizeof (h265_idr_slice_1)));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer (h265_idr_slice_2, sizeof (h265_idr_slice_2));
  GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_MARKER);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_if (gst_rtp_buffer_get_marker (&rtp));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  gst_harness_teardown (h);
}

GST_END_TEST;


GST_START_TEST (test_rtph265pay_marker_for_au)
{
  GstHarness *h = gst_harness_new_parse
      ("rtph265pay timestamp-offset=123 aggregate-mode=none");
  GstFlowReturn ret;
  GstBuffer *slice1, *slice2, *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=au,stream-format=byte-stream");

  slice1 = wrap_static_buffer (h265_idr_slice_1, sizeof (h265_idr_slice_1));
  slice2 = wrap_static_buffer (h265_idr_slice_2, sizeof (h265_idr_slice_2));
  buffer = gst_buffer_append (slice1, slice2);

  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_if (gst_rtp_buffer_get_marker (&rtp));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  gst_harness_teardown (h);
}

GST_END_TEST;


GST_START_TEST (test_rtph265pay_marker_for_fragmented_au)
{
  GstHarness *h =
      gst_harness_new_parse ("rtph265pay timestamp-offset=123 mtu=40");
  GstFlowReturn ret;
  GstBuffer *slice1, *slice2, *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  gint i;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=au,stream-format=byte-stream");

  slice1 = wrap_static_buffer (h265_idr_slice_1, sizeof (h265_idr_slice_1));
  slice2 = wrap_static_buffer (h265_idr_slice_2, sizeof (h265_idr_slice_2));
  buffer = gst_buffer_append (slice1, slice2);

  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 4);

  for (i = 0; i < 3; i++) {
    buffer = gst_harness_pull (h);
    fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
    fail_if (gst_rtp_buffer_get_marker (&rtp));
    gst_rtp_buffer_unmap (&rtp);
    gst_buffer_unref (buffer);
  }

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtph265pay_aggregate_two_slices_per_buffer)
{
  GstHarness *h = gst_harness_new_parse ("rtph265pay timestamp-offset=123"
      " name=p");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstElement *e = gst_bin_get_by_name (GST_BIN (h->element), "p");
  gint i;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=nal,stream-format=byte-stream");

  /* No aggregation latency mode */

  g_object_set (e, "aggregate-mode", 0, NULL);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 4);

  for (i = 0; i < 4; i++) {
    buffer = gst_harness_pull (h);
    fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
    fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
    fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
    fail_unless_equals_int (gst_buffer_get_size (buffer), 12 +
        ((i % 2) ? (sizeof (h265_idr_slice_2) - 3) :
            (sizeof (h265_idr_slice_1)) - 4));
    gst_rtp_buffer_unmap (&rtp);
    gst_buffer_unref (buffer);
  }

  /* Zero latency mode */
  g_object_set (e, "aggregate-mode", 1, NULL);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  for (i = 0; i < 2; i++) {
    buffer = gst_harness_pull (h);
    fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
    fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
    fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
    /* RTP header = 12,  AP header = 2, 2 bytes length per NAL */
    fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 2 +
        (2 + sizeof (h265_idr_slice_2) - 3) +
        (2 + sizeof (h265_idr_slice_1) - 4));
    gst_rtp_buffer_unmap (&rtp);
    gst_buffer_unref (buffer);
  }

  /* Max aggregation */
  g_object_set (e, "aggregate-mode", 2, NULL);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 0);

  /* Push EOS to send it out */
  gst_harness_push_event (h, gst_event_new_eos ());

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 2 +
      2 * ((2 + sizeof (h265_idr_slice_2) - 3) +
          (2 + sizeof (h265_idr_slice_1) - 4)));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);


  g_object_unref (e);
  gst_harness_teardown (h);
}

GST_END_TEST;

/* AUD */
static guint8 h265_aud[] = {
  0x00, 0x00, 0x00, 0x01, (35 << 1), 0x00, 0x80
};

GST_START_TEST (test_rtph265pay_aggregate_with_aud)
{
  GstHarness *h = gst_harness_new_parse ("rtph265pay timestamp-offset=123");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=nal,stream-format=byte-stream");

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_aud, sizeof (h265_aud), 0));
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_1, sizeof (h265_idr_slice_1),
          0));

  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 +
      (sizeof (h265_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 2 +
      (2 + sizeof (h265_aud) - 4) + (2 + sizeof (h265_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);


  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtph265pay_aggregate_with_ts_change)
{
  GstHarness *h = gst_harness_new_parse ("rtph265pay timestamp-offset=123 "
      "aggregate-mode=max");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=nal,stream-format=byte-stream");

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), GST_SECOND);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          GST_SECOND));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  /* Push EOS to send the second one out */
  gst_harness_push_event (h, gst_event_new_eos ());

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 2 +
      (2 + sizeof (h265_idr_slice_2) - 3) +
      (2 + sizeof (h265_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), GST_SECOND);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123 + 90000);
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 2 +
      (2 + sizeof (h265_idr_slice_2) - 3) +
      (2 + sizeof (h265_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);


  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtph265pay_aggregate_with_discont)
{
  GstHarness *h = gst_harness_new_parse ("rtph265pay timestamp-offset=123");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=nal,stream-format=byte-stream");

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h265_idr_slice_2, sizeof (h265_idr_slice_2),
          0));
  GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DISCONT);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 2 +
      (2 + sizeof (h265_idr_slice_2) - 3) +
      (2 + sizeof (h265_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123 + 0);
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 2 +
      (2 + sizeof (h265_idr_slice_2) - 3) +
      (2 + sizeof (h265_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);


  gst_harness_teardown (h);
}

GST_END_TEST;

/* EOS */
static guint8 h265_eos[] = {
  0x00, 0x00, 0x00, 0x01, (36 << 1), 0x00
};


GST_START_TEST (test_rtph265pay_aggregate_until_vcl)
{
  GstHarness *h = gst_harness_new_parse ("rtph265pay timestamp-offset=123"
      " name=p");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h265,alignment=nal,stream-format=byte-stream");

  buffer = wrap_static_buffer_with_pts (h265_vps, sizeof (h265_vps), 0);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h265_sps, sizeof (h265_sps), 0);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h265_pps, sizeof (h265_pps), 0);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h265_idr_slice_1,
      sizeof (h265_idr_slice_1), 0);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  /* RTP header = 12,  STAP header = 2, 2 bytes length per NAL */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 2 +
      (2 + sizeof (h265_vps) - 4) +
      (2 + sizeof (h265_sps) - 4) +
      (2 + sizeof (h265_pps) - 4) + (2 + sizeof (h265_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  /* Push EOS now */

  buffer = wrap_static_buffer_with_pts (h265_eos, sizeof (h265_eos), 0);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 +
      sizeof (h265_eos) - 4);
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  gst_harness_teardown (h);
}

GST_END_TEST;



static Suite *
rtph265_suite (void)
{
  Suite *s = suite_create ("rtph265");
  TCase *tc_chain;

  tc_chain = tcase_create ("rtph265depay");
  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_rtph265depay_with_downstream_allocator);
  tcase_add_test (tc_chain, test_rtph265depay_eos);
  tcase_add_test (tc_chain, test_rtph265depay_marker_to_flag);
  /* TODO We need a sample to test with */
  /* tcase_add_test (tc_chain, test_rtph265depay_aggregate_marker); */

  tc_chain = tcase_create ("rtph265pay");
  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_rtph265pay_two_slices_timestamp);
  tcase_add_test (tc_chain, test_rtph265pay_marker_for_flag);
  tcase_add_test (tc_chain, test_rtph265pay_marker_for_au);
  tcase_add_test (tc_chain, test_rtph265pay_marker_for_fragmented_au);
  tcase_add_test (tc_chain, test_rtph265pay_aggregate_two_slices_per_buffer);
  tcase_add_test (tc_chain, test_rtph265pay_aggregate_with_aud);
  tcase_add_test (tc_chain, test_rtph265pay_aggregate_with_ts_change);
  tcase_add_test (tc_chain, test_rtph265pay_aggregate_with_discont);
  tcase_add_test (tc_chain, test_rtph265pay_aggregate_until_vcl);

  return s;
}

GST_CHECK_MAIN (rtph265);
