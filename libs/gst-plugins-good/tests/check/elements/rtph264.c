/* GStreamer RTP H.264 unit test
 *
 * Copyright (C) 2017 Centricular Ltd
 *   @author: Tim-Philipp Müller <tim@centricular.com>
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

#define RTP_H264_FILE GST_TEST_FILES_PATH G_DIR_SEPARATOR_S "h264.rtp"

GST_START_TEST (test_rtph264depay_with_downstream_allocator)
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
      "encoding-name", G_TYPE_STRING, "H264",
      "ssrc", G_TYPE_UINT, 1990683810,
      "timestamp-offset", G_TYPE_UINT, 3697583446UL,
      "seqnum-offset", G_TYPE_UINT, 15568,
      "a-framerate", G_TYPE_STRING, "30", NULL);
  g_object_set (src, "format", GST_FORMAT_TIME, "caps", caps, NULL);
  gst_bin_add (GST_BIN (pipeline), src);
  gst_caps_unref (caps);

  depay = gst_element_factory_make ("rtph264depay", NULL);
  gst_bin_add (GST_BIN (pipeline), depay);

  sink = g_object_new (c_mem_app_sink_get_type (), NULL);
  gst_bin_add (GST_BIN (pipeline), sink);

  gst_element_link_many (src, depay, sink, NULL);

  gst_element_set_state (pipeline, GST_STATE_PAUSED);

  {
    gchar *data, *pdata;
    gsize len;

    fail_unless (g_file_get_contents (RTP_H264_FILE, &data, &len, NULL));
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
 *     ! video/x-raw,width=16,height=16 \
 *     ! openh264enc ! rtph264pay ! fakesink dump=1
 */
/* RTP h264_idr + marker */
static guint8 rtp_h264_idr[] = {
  0x80, 0xe0, 0x3e, 0xf0, 0xd9, 0xbe, 0x80, 0x28,
  0xf4, 0x2d, 0xe1, 0x70, 0x65, 0xb8, 0x00, 0x04,
  0x00, 0x00, 0x09, 0xff, 0xff, 0xf8, 0x22, 0x8a,
  0x00, 0x1f, 0x1c, 0x00, 0x04, 0x1c, 0xe3, 0x80,
  0x00, 0x84, 0xde
};

GST_START_TEST (test_rtph264depay_eos)
{
  GstHarness *h = gst_harness_new ("rtph264depay");
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret;

  gst_harness_set_caps_str (h,
      "application/x-rtp,media=video,clock-rate=90000,encoding-name=H264",
      "video/x-h264,alignment=au,stream-format=byte-stream");

  buffer = wrap_static_buffer (rtp_h264_idr, sizeof (rtp_h264_idr));
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


GST_START_TEST (test_rtph264depay_marker_to_flag)
{
  GstHarness *h = gst_harness_new ("rtph264depay");
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret;
  guint16 seq;

  gst_harness_set_caps_str (h,
      "application/x-rtp,media=video,clock-rate=90000,encoding-name=H264",
      "video/x-h264,alignment=au,stream-format=byte-stream");

  buffer = wrap_static_buffer (rtp_h264_idr, sizeof (rtp_h264_idr));
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless (gst_rtp_buffer_get_marker (&rtp));
  seq = gst_rtp_buffer_get_seq (&rtp);
  gst_rtp_buffer_unmap (&rtp);

  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  buffer = wrap_static_buffer (rtp_h264_idr, sizeof (rtp_h264_idr));
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


/* As GStreamer does not have STAP-A yet, this was extracted from
 * issue #557 provided sample */
static guint8 rtp_stapa_pps_sps[] = {
  0x80, 0x60, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
  0xd8, 0xd9, 0x06, 0x7f, 0x38, 0x00, 0x1c, 0x27,
  0x64, 0x00, 0x29, 0xac, 0x13, 0x16, 0x50, 0x28,
  0x0f, 0x6c, 0x00, 0x30, 0x30, 0x00, 0x5d, 0xc0,
  0x00, 0x17, 0x70, 0x5e, 0xf7, 0xc1, 0xda, 0x08,
  0x84, 0x65, 0x80, 0x00, 0x04, 0x28, 0xef, 0x1f,
  0x2c
};

static guint8 rtp_stapa_slices_marker[] = {
  0x80, 0xe0, 0x00, 0x0c, 0x00, 0x00, 0x57, 0xe4,
  0xd8, 0xd9, 0x06, 0x7f, 0x38, 0x00, 0x28, 0x21,
  0xe0, 0x02, 0x00, 0x10, 0x73, 0x84, 0x12, 0xff,
  0x00, 0x1f, 0x0c, 0xbf, 0x0c, 0xb7, 0xb8, 0x73,
  0xd3, 0xf2, 0xfc, 0xba, 0x7b, 0xce, 0x3c, 0xaf,
  0x90, 0x11, 0xb3, 0x15, 0xa2, 0x53, 0x96, 0xc1,
  0xa8, 0x27, 0x01, 0xdb, 0xde, 0xc8, 0x80, 0x00,
  0x35, 0x21, 0x00, 0xa0, 0xe0, 0x02, 0x00, 0x10,
  0x73, 0x84, 0x12, 0xff, 0x79, 0x9d, 0x3b, 0x38,
  0xc6, 0x26, 0x47, 0x01, 0x02, 0x6b, 0xd6, 0x40,
  0x17, 0x06, 0x0b, 0x8c, 0x22, 0xd0, 0x04, 0xe4,
  0xea, 0x5e, 0x3f, 0xb8, 0x6a, 0x03, 0x4e, 0x15,
  0x1f, 0x74, 0x62, 0x51, 0x78, 0xef, 0x5a, 0xd1,
  0xf0, 0x3e, 0xb8, 0x56, 0xbb, 0x08, 0x00, 0x2e,
  0x21, 0x00, 0x50, 0x38, 0x00, 0x80, 0x04, 0x1c,
  0xe1, 0x04, 0xbf, 0x00, 0xa8, 0x40, 0x67, 0x09,
  0xa8, 0x4d, 0x95, 0x5b, 0x0e, 0x2b, 0xba, 0x34,
  0xc7, 0xa6, 0x78, 0x27, 0xe4, 0x5c, 0x74, 0xa0,
  0xa2, 0xce, 0x30, 0x51, 0x78, 0x30, 0x56, 0xd0,
  0x7a, 0xcd, 0x12, 0xbc, 0xba, 0xe0, 0x00, 0x1e,
  0x21, 0x00, 0x78, 0x38, 0x00, 0x80, 0x04, 0x1c,
  0xe1, 0x04, 0xbf, 0x67, 0x37, 0xc0, 0x86, 0x26,
  0x7d, 0x4c, 0x52, 0x4b, 0x80, 0x9a, 0x3c, 0xa3,
  0x02, 0xcd, 0x8d, 0xcd, 0x18, 0xd8
};

GST_START_TEST (test_rtph264depay_stap_a_marker)
{
  GstHarness *h = gst_harness_new ("rtph264depay");
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstFlowReturn ret;

  gst_harness_set_caps_str (h,
      "application/x-rtp,media=video,clock-rate=90000,encoding-name=H264",
      "video/x-h264,alignment=au,stream-format=byte-stream");

  ret = gst_harness_push (h,
      wrap_static_buffer (rtp_stapa_pps_sps, sizeof (rtp_stapa_pps_sps)));
  fail_unless_equals_int (ret, GST_FLOW_OK);
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 0);

  buffer = wrap_static_buffer (rtp_stapa_slices_marker,
      sizeof (rtp_stapa_slices_marker));
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_WRITE, &rtp));
  gst_rtp_buffer_set_seq (&rtp, 2);
  gst_rtp_buffer_unmap (&rtp);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  /* Only one AU should have been pushed */
  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  gst_harness_teardown (h);
}

GST_END_TEST;


/* AUD */
static guint8 h264_aud[] = {
  0x00, 0x00, 0x00, 0x01, 0x09, 0xf0
};

/* These were generated using pipeline:
 * gst-launch-1.0 videotestsrc num-buffers=1 pattern=green \
 *     ! video/x-raw,width=128,height=128 \
 *     ! openh264enc num-slices=2 \
 *     ! fakesink dump=1
 */

/* SPS */
static guint8 h264_sps[] = {
  0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xc0, 0x29,
  0x8c, 0x8d, 0x41, 0x02, 0x24, 0x03, 0xc2, 0x21,
  0x1a, 0x80
};

/* PPS */
static guint8 h264_pps[] = {
  0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80
};

/* IDR Slice 1 */
static guint8 h264_idr_slice_1[] = {
  0x00, 0x00, 0x00, 0x01, 0x65, 0xb8, 0x00, 0x04,
  0x00, 0x00, 0x11, 0xff, 0xff, 0xf8, 0x22, 0x8a,
  0x1f, 0x1c, 0x00, 0x04, 0x0a, 0x63, 0x80, 0x00,
  0x81, 0xec, 0x9a, 0x93, 0x93, 0x93, 0x93, 0x93,
  0x93, 0xad, 0x57, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
  0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d,
  0x75, 0xd7, 0x5d, 0x78
};

/* IDR Slice 2 */
static guint8 h264_idr_slice_2[] = {
  0x00, 0x00, 0x00, 0x01, 0x65, 0x04, 0x2e, 0x00,
  0x01, 0x00, 0x00, 0x04, 0x7f, 0xff, 0xfe, 0x08,
  0xa2, 0x87, 0xc7, 0x00, 0x01, 0x02, 0x98, 0xe0,
  0x00, 0x20, 0x7b, 0x26, 0xa4, 0xe4, 0xe4, 0xe4,
  0xe4, 0xe4, 0xeb, 0x55, 0xd7, 0x5d, 0x75, 0xd7,
  0x5d, 0x75, 0xd7, 0x5d, 0x75, 0xd7, 0x5d, 0x75,
  0xd7, 0x5d, 0x75, 0xd7, 0x5e
};

/* The RFC makes special use of NAL type 24 to 27, this test makes sure that
 * such a NAL from the outside gets ignored properly. */
GST_START_TEST (test_rtph264pay_reserved_nals)
{
  GstHarness *h = gst_harness_new_parse ("rtph264pay aggregate-mode=none");
  /* we simply hack an AUD with the reserved nal types */
  guint8 nal_24[sizeof (h264_aud)];
  guint8 nal_25[sizeof (h264_aud)];
  guint8 nal_26[sizeof (h264_aud)];
  guint8 nal_27[sizeof (h264_aud)];
  GstFlowReturn ret;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=nal,stream-format=byte-stream");

  ret = gst_harness_push (h, wrap_static_buffer (h264_sps, sizeof (h264_sps)));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  ret = gst_harness_push (h, wrap_static_buffer (h264_pps, sizeof (h264_pps)));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  memcpy (nal_24, h264_aud, sizeof (h264_aud));
  nal_24[4] = 24;
  ret = gst_harness_push (h, wrap_static_buffer (nal_24, sizeof (nal_24)));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  memcpy (nal_25, h264_aud, sizeof (h264_aud));
  nal_25[4] = 25;
  ret = gst_harness_push (h, wrap_static_buffer (nal_25, sizeof (nal_25)));
  fail_unless_equals_int (ret, GST_FLOW_OK);


  memcpy (nal_26, h264_aud, sizeof (h264_aud));
  nal_26[4] = 26;
  ret = gst_harness_push (h, wrap_static_buffer (nal_26, sizeof (nal_26)));
  fail_unless_equals_int (ret, GST_FLOW_OK);


  memcpy (nal_27, h264_aud, sizeof (h264_aud));
  nal_27[4] = 27;
  ret = gst_harness_push (h, wrap_static_buffer (nal_27, sizeof (nal_27)));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);
  gst_harness_teardown (h);
}

GST_END_TEST;


GST_START_TEST (test_rtph264pay_two_slices_timestamp)
{
  GstHarness *h = gst_harness_new_parse ("rtph264pay timestamp-offset=123");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=nal,stream-format=byte-stream");

  ret = gst_harness_push (h, wrap_static_buffer_with_pts (h264_idr_slice_1,
          sizeof (h264_idr_slice_1), 0));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  ret = gst_harness_push (h, wrap_static_buffer_with_pts (h264_idr_slice_2,
          sizeof (h264_idr_slice_2), 0));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  ret = gst_harness_push (h, wrap_static_buffer_with_pts (h264_idr_slice_1,
          sizeof (h264_idr_slice_1), GST_SECOND));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  ret = gst_harness_push (h, wrap_static_buffer_with_pts (h264_idr_slice_2,
          sizeof (h264_idr_slice_2), GST_SECOND));
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

GST_START_TEST (test_rtph264pay_marker_for_flag)
{
  GstHarness *h = gst_harness_new_parse ("rtph264pay timestamp-offset=123");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=nal,stream-format=byte-stream");

  ret = gst_harness_push (h, wrap_static_buffer (h264_idr_slice_1,
          sizeof (h264_idr_slice_1)));
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer (h264_idr_slice_2, sizeof (h264_idr_slice_2));
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


GST_START_TEST (test_rtph264pay_marker_for_au)
{
  GstHarness *h =
      gst_harness_new_parse
      ("rtph264pay timestamp-offset=123 aggregate-mode=none");
  GstFlowReturn ret;
  GstBuffer *slice1, *slice2, *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=au,stream-format=byte-stream");

  slice1 = wrap_static_buffer (h264_idr_slice_1, sizeof (h264_idr_slice_1));
  slice2 = wrap_static_buffer (h264_idr_slice_2, sizeof (h264_idr_slice_2));
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


GST_START_TEST (test_rtph264pay_marker_for_fragmented_au)
{
  GstHarness *h =
      gst_harness_new_parse ("rtph264pay timestamp-offset=123 mtu=40");
  GstFlowReturn ret;
  GstBuffer *slice1, *slice2, *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  gint i;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=au,stream-format=byte-stream");

  slice1 = wrap_static_buffer (h264_idr_slice_1, sizeof (h264_idr_slice_1));
  slice2 = wrap_static_buffer (h264_idr_slice_2, sizeof (h264_idr_slice_2));
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

GST_START_TEST (test_rtph264pay_aggregate_two_slices_per_buffer)
{
  GstHarness *h = gst_harness_new_parse ("rtph264pay timestamp-offset=123"
      " name=p");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
  GstElement *e = gst_bin_get_by_name (GST_BIN (h->element), "p");
  gint i;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=nal,stream-format=byte-stream");

  /* No aggregation latency mode */

  g_object_set (e, "aggregate-mode", 0, NULL);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
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
        ((i % 2) ? sizeof (h264_idr_slice_2) : sizeof (h264_idr_slice_1)) - 4);
    gst_rtp_buffer_unmap (&rtp);
    gst_buffer_unref (buffer);
  }

  /* Zero latency mode */
  g_object_set (e, "aggregate-mode", 1, NULL);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  for (i = 0; i < 2; i++) {
    buffer = gst_harness_pull (h);
    fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
    fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
    fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
    /* RTP header = 12,  STAP header = 1, 2 bytes length per NAL */
    fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 1 +
        (2 + sizeof (h264_idr_slice_2) - 4) +
        (2 + sizeof (h264_idr_slice_1) - 4));
    gst_rtp_buffer_unmap (&rtp);
    gst_buffer_unref (buffer);
  }

  /* Max aggregation */
  g_object_set (e, "aggregate-mode", 2, NULL);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
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
  /* RTP header = 12,  STAP header = 1, 2 bytes length per NAL */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 1 +
      2 * (2 + sizeof (h264_idr_slice_2) - 4) +
      2 * (2 + sizeof (h264_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);


  g_object_unref (e);
  gst_harness_teardown (h);
}

GST_END_TEST;


GST_START_TEST (test_rtph264pay_aggregate_with_aud)
{
  GstHarness *h = gst_harness_new_parse ("rtph264pay timestamp-offset=123");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=nal,stream-format=byte-stream");

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_aud, sizeof (h264_aud), 0));
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_1, sizeof (h264_idr_slice_1),
          0));

  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  /* RTP header = 12 */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 +
      (sizeof (h264_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  /* RTP header = 12,  STAP header = 1, 2 bytes length per NAL */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 1 +
      (2 + sizeof (h264_aud) - 4) + (2 + sizeof (h264_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);


  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtph264pay_aggregate_with_ts_change)
{
  GstHarness *h = gst_harness_new_parse ("rtph264pay timestamp-offset=123 "
      "aggregate-mode=max-stap");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=nal,stream-format=byte-stream");

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), GST_SECOND);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
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
  /* RTP header = 12,  STAP header = 1, 2 bytes length per NAL */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 1 +
      (2 + sizeof (h264_idr_slice_1) - 4) +
      (2 + sizeof (h264_idr_slice_2) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), GST_SECOND);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123 + 90000);
  /* RTP header = 12,  STAP header = 1, 2 bytes length per NAL */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 1 +
      (2 + sizeof (h264_idr_slice_1) - 4) +
      (2 + sizeof (h264_idr_slice_2) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);


  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtph264pay_aggregate_with_discont)
{
  GstHarness *h = gst_harness_new_parse ("rtph264pay timestamp-offset=123");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=nal,stream-format=byte-stream");

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
          0));
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  buffer = gst_buffer_append (buffer,
      wrap_static_buffer_with_pts (h264_idr_slice_2, sizeof (h264_idr_slice_2),
          0));
  GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DISCONT);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 2);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  /* RTP header = 12,  STAP header = 1, 2 bytes length per NAL */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 1 +
      (2 + sizeof (h264_idr_slice_1) - 4) +
      (2 + sizeof (h264_idr_slice_2) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123 + 0);
  /* RTP header = 12,  STAP header = 1, 2 bytes length per NAL */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 1 +
      (2 + sizeof (h264_idr_slice_1) - 4) +
      (2 + sizeof (h264_idr_slice_2) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);


  gst_harness_teardown (h);
}

GST_END_TEST;

GST_START_TEST (test_rtph264pay_aggregate_until_vcl)
{
  GstHarness *h = gst_harness_new_parse ("rtph264pay timestamp-offset=123"
      " name=p");
  GstFlowReturn ret;
  GstBuffer *buffer;
  GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;

  gst_harness_set_src_caps_str (h,
      "video/x-h264,alignment=nal,stream-format=byte-stream");

  buffer = wrap_static_buffer_with_pts (h264_sps, sizeof (h264_sps), 0);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h264_pps, sizeof (h264_pps), 0);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);

  buffer = wrap_static_buffer_with_pts (h264_idr_slice_1,
      sizeof (h264_idr_slice_1), 0);
  ret = gst_harness_push (h, buffer);
  fail_unless_equals_int (ret, GST_FLOW_OK);


  fail_unless_equals_int (gst_harness_buffers_in_queue (h), 1);

  buffer = gst_harness_pull (h);
  fail_unless (gst_rtp_buffer_map (buffer, GST_MAP_READ, &rtp));
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buffer), 0);
  fail_unless_equals_uint64 (gst_rtp_buffer_get_timestamp (&rtp), 123);
  /* RTP header = 12,  STAP header = 1, 2 bytes length per NAL */
  fail_unless_equals_int (gst_buffer_get_size (buffer), 12 + 1 +
      (2 + sizeof (h264_sps) - 4) +
      (2 + sizeof (h264_pps) - 4) + (2 + sizeof (h264_idr_slice_1) - 4));
  gst_rtp_buffer_unmap (&rtp);
  gst_buffer_unref (buffer);

  gst_harness_teardown (h);
}

GST_END_TEST;

static Suite *
rtph264_suite (void)
{
  Suite *s = suite_create ("rtph264");
  TCase *tc_chain;

  tc_chain = tcase_create ("rtph264depay");
  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_rtph264depay_with_downstream_allocator);
  tcase_add_test (tc_chain, test_rtph264depay_eos);
  tcase_add_test (tc_chain, test_rtph264depay_marker_to_flag);
  tcase_add_test (tc_chain, test_rtph264depay_stap_a_marker);

  tc_chain = tcase_create ("rtph264pay");
  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_rtph264pay_reserved_nals);
  tcase_add_test (tc_chain, test_rtph264pay_two_slices_timestamp);
  tcase_add_test (tc_chain, test_rtph264pay_marker_for_flag);
  tcase_add_test (tc_chain, test_rtph264pay_marker_for_au);
  tcase_add_test (tc_chain, test_rtph264pay_marker_for_fragmented_au);
  tcase_add_test (tc_chain, test_rtph264pay_aggregate_two_slices_per_buffer);
  tcase_add_test (tc_chain, test_rtph264pay_aggregate_with_aud);
  tcase_add_test (tc_chain, test_rtph264pay_aggregate_with_ts_change);
  tcase_add_test (tc_chain, test_rtph264pay_aggregate_with_discont);
  tcase_add_test (tc_chain, test_rtph264pay_aggregate_until_vcl);

  return s;
}

GST_CHECK_MAIN (rtph264);
