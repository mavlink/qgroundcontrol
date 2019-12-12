/* GStreamer RTP payloader unit tests
 * Copyright (C) 2008 Nokia Corporation and its subsidiary(-ies)
 *               contact: <stefan.kost@nokia.com>
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
#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/audio/audio.h>
#include <gst/base/base.h>
#include <stdlib.h>

#define RELEASE_ELEMENT(x) if(x) {gst_object_unref(x); x = NULL;}

#define LOOP_COUNT 1

/*
 * RTP pipeline structure to store the required elements.
 */
typedef struct
{
  GstElement *pipeline;
  GstElement *appsrc;
  GstElement *rtppay;
  GstElement *rtpdepay;
  GstElement *fakesink;
  const guint8 *frame_data;
  int frame_data_size;
  int frame_count;
  GstEvent *custom_event;
} rtp_pipeline;

/*
 * Number of bytes received in the chain list function when using buffer lists
 */
static guint chain_list_bytes_received;

/*
 * Chain list function for testing buffer lists
 */
static GstFlowReturn
rtp_pipeline_chain_list (GstPad * pad, GstObject * parent, GstBufferList * list)
{
  guint i, len;

  fail_if (!list);
  /*
   * Count the size of the payload in the buffer list.
   */
  len = gst_buffer_list_length (list);
  GST_LOG ("list length %u", len);

  /* Loop through all buffers */
  for (i = 0; i < len; i++) {
    GstBuffer *paybuf;
    GstMemory *mem;
    gint size;

    paybuf = gst_buffer_list_get (list, i);
    /* only count real data which is expected in last memory block */
    GST_LOG ("n_memory %d", gst_buffer_n_memory (paybuf));
    fail_unless (gst_buffer_n_memory (paybuf) > 1);
    mem = gst_buffer_get_memory_range (paybuf, gst_buffer_n_memory (paybuf) - 1,
        1);
    size = gst_memory_get_sizes (mem, NULL, NULL);
    gst_memory_unref (mem);
    chain_list_bytes_received += size;
    GST_LOG ("size %d, total %u", size, chain_list_bytes_received);
  }
  gst_buffer_list_unref (list);

  return GST_FLOW_OK;
}

static GstFlowReturn
rtp_pipeline_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstBufferList *list;

  list = gst_buffer_list_new_sized (1);
  gst_buffer_list_add (list, buf);
  return rtp_pipeline_chain_list (pad, parent, list);
}

/*
 * RTP bus callback.
 */
static gboolean
rtp_bus_callback (GstBus * bus, GstMessage * message, gpointer data)
{
  GMainLoop *mainloop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
    {
      GError *err;

      gchar *debug;

      gchar *element_name;

      element_name = (message->src) ? gst_object_get_name (message->src) : NULL;
      gst_message_parse_error (message, &err, &debug);
      g_print ("\nError from element %s: %s\n%s\n\n",
          GST_STR_NULL (element_name), err->message, (debug) ? debug : "");
      g_error_free (err);
      g_free (debug);
      g_free (element_name);

      fail_if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ERROR);

      g_main_loop_quit (mainloop);
    }
      break;

    case GST_MESSAGE_EOS:
    {
      g_main_loop_quit (mainloop);
    }
      break;
      break;

    default:
    {
    }
      break;
  }

  return TRUE;
}

/*
 * Creates a RTP pipeline for one test.
 * @param frame_data Pointer to the frame data which is used to pass through pay/depayloaders.
 * @param frame_data_size Frame data size in bytes.
 * @param frame_count Frame count.
 * @param filtercaps Caps filters.
 * @param pay Payloader name.
 * @param depay Depayloader name.
 * @return
 * Returns pointer to the RTP pipeline.
 * The user must free the RTP pipeline when it's not used anymore.
 */
static rtp_pipeline *
rtp_pipeline_create (const guint8 * frame_data, int frame_data_size,
    int frame_count, const char *filtercaps, const char *pay, const char *depay)
{
  gchar *pipeline_name;
  rtp_pipeline *p;
  GstCaps *caps;

  /* Check parameters. */
  if (!frame_data || !pay || !depay) {
    return NULL;
  }

  /* Allocate memory for the RTP pipeline. */
  p = (rtp_pipeline *) malloc (sizeof (rtp_pipeline));

  p->frame_data = frame_data;
  p->frame_data_size = frame_data_size;
  p->frame_count = frame_count;
  p->custom_event = NULL;

  /* Create elements. */
  pipeline_name = g_strdup_printf ("%s-%s-pipeline", pay, depay);
  p->pipeline = gst_pipeline_new (pipeline_name);
  g_free (pipeline_name);
  p->appsrc = gst_element_factory_make ("appsrc", NULL);
  p->rtppay = gst_element_factory_make (pay, NULL);
  p->rtpdepay = gst_element_factory_make (depay, NULL);
  p->fakesink = gst_element_factory_make ("fakesink", NULL);

  /* One or more elements are not created successfully or failed to create p? */
  if (!p->pipeline || !p->appsrc || !p->rtppay || !p->rtpdepay || !p->fakesink) {
    /* Release created elements. */
    RELEASE_ELEMENT (p->pipeline);
    RELEASE_ELEMENT (p->appsrc);
    RELEASE_ELEMENT (p->rtppay);
    RELEASE_ELEMENT (p->rtpdepay);
    RELEASE_ELEMENT (p->fakesink);

    /* Release allocated memory. */
    free (p);

    return NULL;
  }

  /* Set src properties. */
  caps = gst_caps_from_string (filtercaps);
  g_object_set (p->appsrc, "do-timestamp", TRUE, "caps", caps,
      "format", GST_FORMAT_TIME, NULL);
  gst_caps_unref (caps);

  /* Add elements to the pipeline. */
  gst_bin_add (GST_BIN (p->pipeline), p->appsrc);
  gst_bin_add (GST_BIN (p->pipeline), p->rtppay);
  gst_bin_add (GST_BIN (p->pipeline), p->rtpdepay);
  gst_bin_add (GST_BIN (p->pipeline), p->fakesink);

  /* Link elements. */
  gst_element_link (p->appsrc, p->rtppay);
  gst_element_link (p->rtppay, p->rtpdepay);
  gst_element_link (p->rtpdepay, p->fakesink);

  return p;
}

/*
 * Destroys the RTP pipeline.
 * @param p Pointer to the RTP pipeline.
 */
static void
rtp_pipeline_destroy (rtp_pipeline * p)
{
  /* Check parameters. */
  if (p == NULL) {
    return;
  }

  /* Release pipeline. */
  RELEASE_ELEMENT (p->pipeline);

  /* Release allocated memory. */
  free (p);
}

static GstPadProbeReturn
pay_event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  rtp_pipeline *p = (rtp_pipeline *) user_data;
  GstEvent *event = GST_PAD_PROBE_INFO_DATA (info);

  if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_DOWNSTREAM) {
    const GstStructure *s0 = gst_event_get_structure (p->custom_event);
    const GstStructure *s1 = gst_event_get_structure (event);
    if (gst_structure_is_equal (s0, s1)) {
      return GST_PAD_PROBE_DROP;
    }
  }

  return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
depay_event_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  rtp_pipeline *p = (rtp_pipeline *) user_data;
  GstEvent *event = GST_PAD_PROBE_INFO_DATA (info);

  if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_DOWNSTREAM) {
    const GstStructure *s0 = gst_event_get_structure (p->custom_event);
    const GstStructure *s1 = gst_event_get_structure (event);
    if (gst_structure_is_equal (s0, s1)) {
      gst_event_unref (p->custom_event);
      p->custom_event = NULL;
    }
  }

  return GST_PAD_PROBE_OK;
}

/*
 * Runs the RTP pipeline.
 * @param p Pointer to the RTP pipeline.
 */
static void
rtp_pipeline_run (rtp_pipeline * p)
{
  GstFlowReturn flow_ret;
  GMainLoop *mainloop = NULL;
  GstBus *bus;
  gint i, j;

  /* Check parameters. */
  if (p == NULL) {
    return;
  }

  /* Create mainloop. */
  mainloop = g_main_loop_new (NULL, FALSE);
  if (!mainloop) {
    return;
  }

  /* Add bus callback. */
  bus = gst_pipeline_get_bus (GST_PIPELINE (p->pipeline));

  gst_bus_add_watch (bus, rtp_bus_callback, (gpointer) mainloop);

  /* Set pipeline to PLAYING. */
  gst_element_set_state (p->pipeline, GST_STATE_PLAYING);

  /* Push custom event into the pipeline */
  if (p->custom_event) {
    GstPad *srcpad;

    /* Install a probe to drop the event after it being serialized */
    srcpad = gst_element_get_static_pad (p->rtppay, "src");
    gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        pay_event_probe_cb, p, NULL);
    gst_object_unref (srcpad);

    /* Install a probe to trace the deserialized event after depayloading */
    srcpad = gst_element_get_static_pad (p->rtpdepay, "src");
    gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        depay_event_probe_cb, p, NULL);
    gst_object_unref (srcpad);
    /* Send the event */
    gst_element_send_event (p->appsrc, gst_event_ref (p->custom_event));
  }

  /* Push data into the pipeline */
  for (i = 0; i < LOOP_COUNT; i++) {
    const guint8 *data = p->frame_data;

    for (j = 0; j < p->frame_count; j++) {
      GstBuffer *buf;

      buf =
          gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY,
          (guint8 *) data, p->frame_data_size, 0, p->frame_data_size, NULL,
          NULL);

      g_signal_emit_by_name (p->appsrc, "push-buffer", buf, &flow_ret);
      fail_unless_equals_int (flow_ret, GST_FLOW_OK);
      data += p->frame_data_size;

      gst_buffer_unref (buf);
    }
  }

  g_signal_emit_by_name (p->appsrc, "end-of-stream", &flow_ret);

  /* Run mainloop. */
  g_main_loop_run (mainloop);

  /* Set pipeline to NULL. */
  gst_element_set_state (p->pipeline, GST_STATE_NULL);

  /* Release mainloop. */
  g_main_loop_unref (mainloop);

  gst_bus_remove_watch (bus);
  gst_object_unref (bus);

  fail_if (p->custom_event);
}

/*
 * Enables buffer lists and adds a chain_list_function to the depayloader.
 * @param p Pointer to the RTP pipeline.
 */
static void
rtp_pipeline_enable_lists (rtp_pipeline * p)
{
  GstPad *pad;

  /* Add chain list function for the buffer list tests */
  pad = gst_element_get_static_pad (p->rtpdepay, "sink");
  gst_pad_set_chain_list_function (pad,
      GST_DEBUG_FUNCPTR (rtp_pipeline_chain_list));
  /* .. to satisfy this silly test code in case someone dares push a buffer */
  gst_pad_set_chain_function (pad, GST_DEBUG_FUNCPTR (rtp_pipeline_chain));
  gst_object_unref (pad);
}

/*
 * Creates the RTP pipeline and runs the test using the pipeline.
 * @param frame_data Pointer to the frame data which is used to pass through pay/depayloaders.
 * @param frame_data_size Frame data size in bytes.
 * @param frame_count Frame count.
 * @param filtercaps Caps filters.
 * @param pay Payloader name.
 * @param depay Depayloader name.
 * @bytes_sent bytes that will be sent, used when testing buffer lists
 * @mtu_size set mtu size when testing lists
 * @use_lists enable buffer lists
 */
static void
rtp_pipeline_test (const guint8 * frame_data, int frame_data_size,
    int frame_count, const char *filtercaps, const char *pay, const char *depay,
    guint bytes_sent, guint mtu_size, gboolean use_lists)
{
  /* Create RTP pipeline. */
  rtp_pipeline *p =
      rtp_pipeline_create (frame_data, frame_data_size, frame_count, filtercaps,
      pay, depay);

  if (p == NULL) {
    return;
  }

  /* set mtu size if needed */
  if (mtu_size > 0) {
    g_object_set (p->rtppay, "mtu", mtu_size, NULL);
  }

  if (use_lists) {
    rtp_pipeline_enable_lists (p);
    chain_list_bytes_received = 0;
  }

  /* Run RTP pipeline. */
  rtp_pipeline_run (p);

  /* Destroy RTP pipeline. */
  rtp_pipeline_destroy (p);

  if (use_lists) {
    /* 'next NAL' indicator is 4 bytes */
    fail_unless_equals_int (chain_list_bytes_received, bytes_sent * LOOP_COUNT);
  }
}

static const guint8 rtp_ilbc_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_ilbc_frame_data_size = 20;

static int rtp_ilbc_frame_count = 1;

GST_START_TEST (rtp_ilbc)
{
  rtp_pipeline_test (rtp_ilbc_frame_data, rtp_ilbc_frame_data_size,
      rtp_ilbc_frame_count, "audio/x-iLBC,mode=20", "rtpilbcpay",
      "rtpilbcdepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_gsm_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_gsm_frame_data_size = 20;

static int rtp_gsm_frame_count = 1;

GST_START_TEST (rtp_gsm)
{
  rtp_pipeline_test (rtp_gsm_frame_data, rtp_gsm_frame_data_size,
      rtp_gsm_frame_count, "audio/x-gsm,rate=8000,channels=1", "rtpgsmpay",
      "rtpgsmdepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_amr_frame_data[] =
    { 0x3c, 0x24, 0x03, 0xb3, 0x48, 0x10, 0x68, 0x46, 0x6c, 0xec, 0x03,
  0x7a, 0x37, 0x16, 0x41, 0x41, 0xc0, 0x00, 0x0d, 0xcd, 0x12, 0xed,
  0xad, 0x80, 0x00, 0x00, 0x11, 0x31, 0x00, 0x00, 0x0d, 0xa0
};

static int rtp_amr_frame_data_size = 32;

static int rtp_amr_frame_count = 1;

GST_START_TEST (rtp_amr)
{
  rtp_pipeline_test (rtp_amr_frame_data, rtp_amr_frame_data_size,
      rtp_amr_frame_count, "audio/AMR,channels=1,rate=8000", "rtpamrpay",
      "rtpamrdepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_pcma_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_pcma_frame_data_size = 20;

static int rtp_pcma_frame_count = 1;

GST_START_TEST (rtp_pcma)
{
  rtp_pipeline_test (rtp_pcma_frame_data, rtp_pcma_frame_data_size,
      rtp_pcma_frame_count, "audio/x-alaw,channels=1,rate=8000", "rtppcmapay",
      "rtppcmadepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_pcmu_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_pcmu_frame_data_size = 20;

static int rtp_pcmu_frame_count = 1;

GST_START_TEST (rtp_pcmu)
{
  rtp_pipeline_test (rtp_pcmu_frame_data, rtp_pcmu_frame_data_size,
      rtp_pcmu_frame_count, "audio/x-mulaw,channels=1,rate=8000", "rtppcmupay",
      "rtppcmudepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_mpa_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_mpa_frame_data_size = 20;

static int rtp_mpa_frame_count = 1;

GST_START_TEST (rtp_mpa)
{
  rtp_pipeline_test (rtp_mpa_frame_data, rtp_mpa_frame_data_size,
      rtp_mpa_frame_count, "audio/mpeg,mpegversion=1", "rtpmpapay",
      "rtpmpadepay", 0, 0, FALSE);
}

GST_END_TEST;

static const guint8 rtp_h261_frame_data[] = {
  0x00, 0x01, 0x00, 0x06, 0x00, 0x01, 0x11, 0x00, 0x00, 0x4c, 0x40, 0x00,
  0x15, 0x10,
};

static int rtp_h261_frame_data_size = 14;
static int rtp_h261_frame_count = 1;

GST_START_TEST (rtp_h261)
{
  rtp_pipeline_test (rtp_h261_frame_data, rtp_h261_frame_data_size,
      rtp_h261_frame_count, "video/x-h261", "rtph261pay", "rtph261depay",
      0, 0, FALSE);
}

GST_END_TEST;

static const guint8 rtp_h263_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_h263_frame_data_size = 20;

static int rtp_h263_frame_count = 1;

GST_START_TEST (rtp_h263)
{
  rtp_pipeline_test (rtp_h263_frame_data, rtp_h263_frame_data_size,
      rtp_h263_frame_count,
      "video/x-h263,variant=(string)itu,h263version=h263",
      "rtph263pay", "rtph263depay", 0, 0, FALSE);
  rtp_pipeline_test (rtp_h263_frame_data, rtp_h263_frame_data_size,
      rtp_h263_frame_count,
      "video/x-h263,variant=(string)itu,h263version=h263,width=10,height=20",
      "rtph263pay", "rtph263depay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_h263p_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_h263p_frame_data_size = 20;

static int rtp_h263p_frame_count = 1;

GST_START_TEST (rtp_h263p)
{
  rtp_pipeline_test (rtp_h263p_frame_data, rtp_h263p_frame_data_size,
      rtp_h263p_frame_count, "video/x-h263,variant=(string)itu,"
      "h263version=(string)h263", "rtph263ppay", "rtph263pdepay", 0, 0, FALSE);

  /* payloader should accept any input that matches the template caps
   * if there's just a udpsink or fakesink downstream */
  rtp_pipeline_test (rtp_h263p_frame_data, rtp_h263p_frame_data_size,
      rtp_h263p_frame_count, "video/x-h263,variant=(string)itu,"
      "h263version=(string)h263", "rtph263ppay", "identity", 0, 0, FALSE);

  /* default output of avenc_h263p */
  rtp_pipeline_test (rtp_h263p_frame_data, rtp_h263p_frame_data_size,
      rtp_h263p_frame_count, "video/x-h263,variant=(string)itu,"
      "h263version=(string)h263p, annex-f=(boolean)true, "
      "annex-j=(boolean)true, annex-i=(boolean)true, annex-t=(boolean)true",
      "rtph263ppay", "identity", 0, 0, FALSE);

  /* pay ! depay should also work with any input */
  rtp_pipeline_test (rtp_h263p_frame_data, rtp_h263p_frame_data_size,
      rtp_h263p_frame_count, "video/x-h263,variant=(string)itu,"
      "h263version=(string)h263p, annex-f=(boolean)true, "
      "annex-j=(boolean)true, annex-i=(boolean)true, annex-t=(boolean)true",
      "rtph263ppay", "rtph263pdepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_h264_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_h264_frame_data_size = 20;

static int rtp_h264_frame_count = 1;

GST_START_TEST (rtp_h264)
{
  /* FIXME 0.11: fully specify h264 caps (and make payloader check) */
  rtp_pipeline_test (rtp_h264_frame_data, rtp_h264_frame_data_size,
      rtp_h264_frame_count,
      "video/x-h264,stream-format=(string)byte-stream,alignment=(string)nal",
      "rtph264pay", "rtph264depay", 0, 0, FALSE);

  /* config-interval property used to be of uint type, was changed to int,
   * make sure old GValue stuff still works */
  {
    GValue val = G_VALUE_INIT;
    GstElement *rtph264pay;
    GParamSpec *pspec;


    rtph264pay = gst_element_factory_make ("rtph264pay", NULL);
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (rtph264pay),
        "config-interval");
    fail_unless (pspec->value_type == G_TYPE_INT);
    g_value_init (&val, G_TYPE_UINT);
    g_value_set_uint (&val, 10);
    g_object_set_property (G_OBJECT (rtph264pay), "config-interval", &val);
    g_value_set_uint (&val, 0);
    g_object_get_property (G_OBJECT (rtph264pay), "config-interval", &val);
    fail_unless_equals_int (10, g_value_get_uint (&val));
    g_object_set (G_OBJECT (rtph264pay), "config-interval", -1, NULL);
    g_object_get_property (G_OBJECT (rtph264pay), "config-interval", &val);
    fail_unless (g_value_get_uint (&val) == G_MAXUINT);
    g_value_unset (&val);
    gst_object_unref (rtph264pay);
  }
}

GST_END_TEST;

/* H264 data generated with:
 * videotestsrc pattern=black ! video/x-raw,width=16,height=16 ! openh264enc */
static const guint8 h264_16x16_black_bs[] = {
  0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xd0, 0x0b,
  0x8c, 0x8d, 0x4e, 0x40, 0x3c, 0x22, 0x11, 0xa8,
  0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80,
  0x00, 0x00, 0x00, 0x01, 0x65, 0xb8, 0x00, 0x04,
  0x00, 0x00, 0x09, 0xe4, 0xc5, 0x00, 0x01, 0x19,
  0xfc
};

static GstSample *
rtp_h264depay_run (const gchar * stream_format)
{
  GstHarness *h;
  GstSample *sample;
  GstBuffer *buf;
  GstEvent *e;
  GstCaps *out_caps;
  GstCaps *in_caps;
  gboolean seen_caps = FALSE;
  gsize size;

  h = gst_harness_new_parse ("rtph264pay ! rtph264depay");

  /* Our input data is in byte-stream format (not that it matters) */
  in_caps = gst_caps_new_simple ("video/x-h264",
      "stream-format", G_TYPE_STRING, "byte-stream",
      "alignment", G_TYPE_STRING, "au",
      "profile", G_TYPE_STRING, "baseline",
      "width", G_TYPE_INT, 16,
      "height", G_TYPE_INT, 16, "framerate", GST_TYPE_FRACTION, 30, 1, NULL);

  /* Force rtph264depay to output format as requested */
  out_caps = gst_caps_new_simple ("video/x-h264",
      "stream-format", G_TYPE_STRING, stream_format,
      "alignment", G_TYPE_STRING, "au", NULL);

  gst_harness_set_caps (h, in_caps, out_caps);
  in_caps = NULL;
  out_caps = NULL;

  gst_harness_play (h);

  size = sizeof (h264_16x16_black_bs);
  buf = gst_buffer_new_wrapped (g_memdup (h264_16x16_black_bs, size), size);
  fail_unless_equals_int (gst_harness_push (h, buf), GST_FLOW_OK);
  fail_unless (gst_harness_push_event (h, gst_event_new_eos ()));

  while ((e = gst_harness_try_pull_event (h))) {
    if (GST_EVENT_TYPE (e) == GST_EVENT_CAPS) {
      GstCaps *caps = NULL;

      gst_event_parse_caps (e, &caps);
      gst_caps_replace (&out_caps, caps);
      seen_caps = TRUE;
    }
    gst_event_unref (e);
  }
  fail_unless (seen_caps);

  buf = gst_harness_pull (h);
  sample = gst_sample_new (buf, out_caps, NULL, NULL);
  gst_buffer_unref (buf);
  gst_caps_replace (&out_caps, NULL);

  gst_harness_teardown (h);
  return sample;
}

GST_START_TEST (rtp_h264depay_avc)
{
  const GValue *val;
  GstStructure *st;
  GstMapInfo map = GST_MAP_INFO_INIT;
  GstBuffer *buf;
  GstSample *s;
  GstCaps *caps;

  s = rtp_h264depay_run ("avc");

  /* must have codec_data in output caps */
  caps = gst_sample_get_caps (s);
  st = gst_caps_get_structure (caps, 0);
  GST_LOG ("caps: %" GST_PTR_FORMAT, caps);
  fail_unless (gst_structure_has_field (st, "stream-format"));
  fail_unless (gst_structure_has_field (st, "alignment"));
  fail_unless (gst_structure_has_field (st, "level"));
  fail_unless (gst_structure_has_field (st, "profile"));
  val = gst_structure_get_value (st, "codec_data");
  fail_unless (val != NULL);
  fail_unless (GST_VALUE_HOLDS_BUFFER (val));
  /* check codec_data, shouldn't contain trailing zeros */
  buf = gst_value_get_buffer (val);
  fail_unless (gst_buffer_map (buf, &map, GST_MAP_READ));
  {
    guint num_sps, num_pps, len;
    guint8 *data;

    GST_MEMDUMP ("H.264 codec_data", map.data, map.size);
    fail_unless_equals_int (map.data[0], 1);
    num_sps = map.data[5] & 0x1f;
    data = map.data + 6;
    fail_unless_equals_int (num_sps, 1);
    len = GST_READ_UINT16_BE (data);
    data += 2;
    /* make sure there are no trailing zeros in the SPS */
    fail_unless (data[len - 1] != 0);
    data += len;
    num_pps = *data++;
    fail_unless_equals_int (num_pps, 1);
    len = GST_READ_UINT16_BE (data);
    data += 2;
    /* make sure there are no trailing zeros in the PPS */
    fail_unless (data[len - 1] != 0);
  }
  gst_buffer_unmap (buf, &map);

  buf = gst_sample_get_buffer (s);
  fail_unless (gst_buffer_map (buf, &map, GST_MAP_READ));
  GST_MEMDUMP ("H.264 AVC frame", map.data, map.size);
  fail_unless (map.size >= 4 + 13);
  /* Want IDR slice as very first thing.
   * We assume nal size markers are 4 bytes here. */
  fail_unless_equals_int (map.data[4] & 0x1f, 5);
  gst_buffer_unmap (buf, &map);

  gst_sample_unref (s);
}

GST_END_TEST;

GST_START_TEST (rtp_h264depay_bytestream)
{
  GstByteReader br;
  GstStructure *st;
  GstMapInfo map = GST_MAP_INFO_INIT;
  GstBuffer *buf;
  GstSample *s;
  GstCaps *caps;
  guint32 dw = 0;
  guint8 b = 0;
  guint off, left;

  s = rtp_h264depay_run ("byte-stream");

  /* must not have codec_data in output caps */
  caps = gst_sample_get_caps (s);
  st = gst_caps_get_structure (caps, 0);
  GST_LOG ("caps: %" GST_PTR_FORMAT, caps);
  fail_if (gst_structure_has_field (st, "codec_data"));

  buf = gst_sample_get_buffer (s);
  fail_unless (gst_buffer_map (buf, &map, GST_MAP_READ));
  GST_MEMDUMP ("H.264 byte-stream frame", map.data, map.size);
  fail_unless (map.size > 40);
  gst_byte_reader_init (&br, map.data, map.size);
  /* We assume nal sync markers are 4 bytes... */
  fail_unless (gst_byte_reader_get_uint32_be (&br, &dw));
  fail_unless_equals_int (dw, 0x00000001);
  /* Want SPS as very first thing */
  fail_unless (gst_byte_reader_get_uint8 (&br, &b));
  fail_unless_equals_int (b & 0x1f, 7);
  /* Then, we want the PPS */
  left = gst_byte_reader_get_remaining (&br);
  off = gst_byte_reader_masked_scan_uint32 (&br, 0xffffffff, 1, 0, left);
  fail_if (off == (guint) - 1);
  gst_byte_reader_skip (&br, off + 4);
  fail_unless (gst_byte_reader_get_uint8 (&br, &b));
  fail_unless_equals_int (b & 0x1f, 8);
  /* FIXME: looks like we get two sets of SPS/PPS ?! */
  left = gst_byte_reader_get_remaining (&br);
  off = gst_byte_reader_masked_scan_uint32 (&br, 0xffffffff, 1, 0, left);
  fail_if (off == (guint) - 1);
  gst_byte_reader_skip (&br, off + 4);
  left = gst_byte_reader_get_remaining (&br);
  off = gst_byte_reader_masked_scan_uint32 (&br, 0xffffffff, 1, 0, left);
  fail_if (off == (guint) - 1);
  gst_byte_reader_skip (&br, off + 4);
  /* Finally, we want an IDR slice */
  left = gst_byte_reader_get_remaining (&br);
  off = gst_byte_reader_masked_scan_uint32 (&br, 0xffffffff, 1, 0, left);
  fail_if (off == (guint) - 1);
  gst_byte_reader_skip (&br, off + 4);
  fail_unless (gst_byte_reader_get_uint8 (&br, &b));
  fail_unless_equals_int (b & 0x1f, 5);
  gst_buffer_unmap (buf, &map);

  gst_sample_unref (s);
}

GST_END_TEST;

static const guint8 rtp_h264_list_lt_mtu_frame_data[] =
    /* not packetized, next NAL starts with 0001 */
{ 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0xad, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x10
};

static int rtp_h264_list_lt_mtu_frame_data_size = 16;

static int rtp_h264_list_lt_mtu_frame_count = 2;

/* NAL = 4 bytes */
/* also 2 bytes FU-A header each time */
static int rtp_h264_list_lt_mtu_bytes_sent = (16 - 4);

static int rtp_h264_list_lt_mtu_mtu_size = 1024;

GST_START_TEST (rtp_h264_list_lt_mtu)
{
  /* FIXME 0.11: fully specify h264 caps (and make payloader check) */
  rtp_pipeline_test (rtp_h264_list_lt_mtu_frame_data,
      rtp_h264_list_lt_mtu_frame_data_size, rtp_h264_list_lt_mtu_frame_count,
      "video/x-h264,stream-format=(string)byte-stream,alignment=(string)nal",
      "rtph264pay", "rtph264depay",
      rtp_h264_list_lt_mtu_bytes_sent, rtp_h264_list_lt_mtu_mtu_size, TRUE);
}

GST_END_TEST;
static const guint8 rtp_h264_list_lt_mtu_frame_data_avc[] =
    /* packetized data */
{ 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
  0xad, 0x80, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x0d, 0x00
};

/* Only the last NAL of each packet is computed by the strange algorithm in
 * rtp_pipeline_chain_list()
 */
static int rtp_h264_list_lt_mtu_bytes_sent_avc = 7 + 3;

//static int rtp_h264_list_lt_mtu_mtu_size = 1024;

GST_START_TEST (rtp_h264_list_lt_mtu_avc)
{
  /* FIXME 0.11: fully specify h264 caps (and make payloader check) */
  rtp_pipeline_test (rtp_h264_list_lt_mtu_frame_data_avc,
      rtp_h264_list_lt_mtu_frame_data_size, rtp_h264_list_lt_mtu_frame_count,
      "video/x-h264,stream-format=(string)avc,alignment=(string)au,"
      "codec_data=(buffer)01640014ffe1001867640014acd94141fb0110000003001773594000f142996001000568ebecb22c",
      "rtph264pay", "rtph264depay",
      rtp_h264_list_lt_mtu_bytes_sent_avc, rtp_h264_list_lt_mtu_mtu_size, TRUE);
}

GST_END_TEST;
static const guint8 rtp_h264_list_gt_mtu_frame_data[] =
    /* not packetized, next NAL starts with 0001 */
{ 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
};

static int rtp_h264_list_gt_mtu_frame_data_size = 64;

static int rtp_h264_list_gt_mtu_frame_count = 1;

/* NAL = 4 bytes. When data does not fit into 1 mtu, 1 byte will be skipped */
static int rtp_h264_list_gt_mtu_bytes_sent = 1 * (64 - 4) - 1;

static int rtp_h264_list_gt_mtu_mty_size = 28;

GST_START_TEST (rtp_h264_list_gt_mtu)
{
  /* FIXME 0.11: fully specify h264 caps (and make payloader check) */
  rtp_pipeline_test (rtp_h264_list_gt_mtu_frame_data,
      rtp_h264_list_gt_mtu_frame_data_size, rtp_h264_list_gt_mtu_frame_count,
      "video/x-h264,stream-format=(string)byte-stream,alignment=(string)nal",
      "rtph264pay", "rtph264depay",
      rtp_h264_list_gt_mtu_bytes_sent, rtp_h264_list_gt_mtu_mty_size, TRUE);
}

GST_END_TEST;
static const guint8 rtp_h264_list_gt_mtu_frame_data_avc[] =
    /* packetized data */
{ 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* NAL = 4 bytes. When data does not fit into 1 mtu, 1 byte will be skipped */
static int rtp_h264_list_gt_mtu_bytes_sent_avc = 1 * (64 - 2 * 4 - 2 * 1);

GST_START_TEST (rtp_h264_list_gt_mtu_avc)
{
  /* FIXME 0.11: fully specify h264 caps (and make payloader check) */
  rtp_pipeline_test (rtp_h264_list_gt_mtu_frame_data_avc,
      rtp_h264_list_gt_mtu_frame_data_size, rtp_h264_list_gt_mtu_frame_count,
      "video/x-h264,stream-format=(string)avc,alignment=(string)au,"
      "codec_data=(buffer)01640014ffe1001867640014acd94141fb0110000003001773594000f142996001000568ebecb22c",
      "rtph264pay", "rtph264depay",
      rtp_h264_list_gt_mtu_bytes_sent_avc, rtp_h264_list_gt_mtu_mty_size, TRUE);
}

GST_END_TEST;

static const guint8 rtp_h265_frame_data[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_h265_frame_data_size = 20;

static int rtp_h265_frame_count = 1;

GST_START_TEST (rtp_h265)
{
  rtp_pipeline_test (rtp_h265_frame_data, rtp_h265_frame_data_size,
      rtp_h265_frame_count,
      "video/x-h265,stream-format=(string)byte-stream,alignment=(string)nal",
      "rtph265pay", "rtph265depay", 0, 0, FALSE);

  /* config-interval property used to be of uint type, was changed to int,
   * make sure old GValue stuff still works */
  {
    GValue val = G_VALUE_INIT;
    GstElement *rtph265pay;
    GParamSpec *pspec;


    rtph265pay = gst_element_factory_make ("rtph265pay", NULL);
    pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (rtph265pay),
        "config-interval");
    fail_unless (pspec->value_type == G_TYPE_INT);
    g_value_init (&val, G_TYPE_UINT);
    g_value_set_uint (&val, 10);
    g_object_set_property (G_OBJECT (rtph265pay), "config-interval", &val);
    g_value_set_uint (&val, 0);
    g_object_get_property (G_OBJECT (rtph265pay), "config-interval", &val);
    fail_unless_equals_int (10, g_value_get_uint (&val));
    g_object_set (G_OBJECT (rtph265pay), "config-interval", -1, NULL);
    g_object_get_property (G_OBJECT (rtph265pay), "config-interval", &val);
    fail_unless (g_value_get_uint (&val) == G_MAXUINT);
    g_value_unset (&val);
    gst_object_unref (rtph265pay);
  }
}

GST_END_TEST;
static const guint8 rtp_h265_list_lt_mtu_frame_data[] = {
  /* not packetized, next NALU starts with 0x00000001 */
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10
};

static int rtp_h265_list_lt_mtu_frame_data_size = 16;

static int rtp_h265_list_lt_mtu_frame_count = 2;

/* 3 bytes start code prefixed with one zero byte, NALU header is in payload */
static int rtp_h265_list_lt_mtu_bytes_sent = 2 * (16 - 3 - 1);

static int rtp_h265_list_lt_mtu_mtu_size = 1024;

GST_START_TEST (rtp_h265_list_lt_mtu)
{
  rtp_pipeline_test (rtp_h265_list_lt_mtu_frame_data,
      rtp_h265_list_lt_mtu_frame_data_size, rtp_h265_list_lt_mtu_frame_count,
      "video/x-h265,stream-format=(string)byte-stream,alignment=(string)nal",
      "rtph265pay", "rtph265depay", rtp_h265_list_lt_mtu_bytes_sent,
      rtp_h265_list_lt_mtu_mtu_size, TRUE);
}

GST_END_TEST;
static const guint8 rtp_h265_list_lt_mtu_frame_data_hvc1[] = {
  /* packetized data */
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
};

/* length size is 3 bytes */
static int rtp_h265_list_lt_mtu_bytes_sent_hvc1 = 4 + 9;


GST_START_TEST (rtp_h265_list_lt_mtu_hvc1)
{
  rtp_pipeline_test (rtp_h265_list_lt_mtu_frame_data_hvc1,
      rtp_h265_list_lt_mtu_frame_data_size, rtp_h265_list_lt_mtu_frame_count,
      "video/x-h265,stream-format=(string)hvc1,alignment=(string)au,"
      "codec_data=(buffer)0101c000000080000000000099f000fcfdf8f800000203a000010"
      "01840010c01ffff01c000000300800000030000030099ac0900a10001003042010101c00"
      "0000300800000030000030099a00a080f1fe36bbb5377725d602dc040404100000300010"
      "00003000a0800a2000100074401c172b02240",
      "rtph265pay", "rtph265depay", rtp_h265_list_lt_mtu_bytes_sent_hvc1,
      rtp_h265_list_lt_mtu_mtu_size, TRUE);
}

GST_END_TEST;
static const guint8 rtp_h265_list_gt_mtu_frame_data[] = {
  /* not packetized, next NAL starts with 0x000001 */
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x10
};

static const int rtp_h265_list_gt_mtu_frame_data_size = 62;

static const int rtp_h265_list_gt_mtu_frame_count = 1;

/* start code is 3 bytes, NALU header is 2 bytes */
static int rtp_h265_list_gt_mtu_bytes_sent = 1 * (62 - 3 - 2);

static int rtp_h265_list_gt_mtu_mtu_size = 28;

GST_START_TEST (rtp_h265_list_gt_mtu)
{
  rtp_pipeline_test (rtp_h265_list_gt_mtu_frame_data,
      rtp_h265_list_gt_mtu_frame_data_size, rtp_h265_list_gt_mtu_frame_count,
      "video/x-h265,stream-format=(string)byte-stream,alignment=(string)nal",
      "rtph265pay", "rtph265depay", rtp_h265_list_gt_mtu_bytes_sent,
      rtp_h265_list_gt_mtu_mtu_size, TRUE);
}

GST_END_TEST;
static const guint8 rtp_h265_list_gt_mtu_frame_data_hvc1[] = {
  /* packetized data */
  0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* length size is 3 bytes, NALU header is 2 bytes */
static int rtp_h265_list_gt_mtu_bytes_sent_hvc1 = 1 * (62 - 2 * 3 - 2 * 2);

GST_START_TEST (rtp_h265_list_gt_mtu_hvc1)
{
  rtp_pipeline_test (rtp_h265_list_gt_mtu_frame_data_hvc1,
      rtp_h265_list_gt_mtu_frame_data_size, rtp_h265_list_gt_mtu_frame_count,
      "video/x-h265,stream-format=(string)hvc1,alignment=(string)au,"
      "codec_data=(buffer)0101c000000080000000000099f000fcfdf8f800000203a000010"
      "01840010c01ffff01c000000300800000030000030099ac0900a10001003042010101c00"
      "0000300800000030000030099a00a080f1fe36bbb5377725d602dc040404100000300010"
      "00003000a0800a2000100074401c172b02240",
      "rtph265pay", "rtph265depay", rtp_h265_list_gt_mtu_bytes_sent_hvc1,
      rtp_h265_list_gt_mtu_mtu_size, TRUE);
}

GST_END_TEST;

/* KLV data from Day_Flight.mpg */
static const guint8 rtp_KLV_frame_data[] = {
  0x06, 0x0e, 0x2b, 0x34, 0x02, 0x0b, 0x01, 0x01,
  0x0e, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00,
  0x81, 0x91, 0x02, 0x08, 0x00, 0x04, 0x6c, 0x8e,
  0x20, 0x03, 0x83, 0x85, 0x41, 0x01, 0x01, 0x05,
  0x02, 0x3d, 0x3b, 0x06, 0x02, 0x15, 0x80, 0x07,
  0x02, 0x01, 0x52, 0x0b, 0x03, 0x45, 0x4f, 0x4e,
  0x0c, 0x0e, 0x47, 0x65, 0x6f, 0x64, 0x65, 0x74,
  0x69, 0x63, 0x20, 0x57, 0x47, 0x53, 0x38, 0x34,
  0x0d, 0x04, 0x4d, 0xc4, 0xdc, 0xbb, 0x0e, 0x04,
  0xb1, 0xa8, 0x6c, 0xfe, 0x0f, 0x02, 0x1f, 0x4a,
  0x10, 0x02, 0x00, 0x85, 0x11, 0x02, 0x00, 0x4b,
  0x12, 0x04, 0x20, 0xc8, 0xd2, 0x7d, 0x13, 0x04,
  0xfc, 0xdd, 0x02, 0xd8, 0x14, 0x04, 0xfe, 0xb8,
  0xcb, 0x61, 0x15, 0x04, 0x00, 0x8f, 0x3e, 0x61,
  0x16, 0x04, 0x00, 0x00, 0x01, 0xc9, 0x17, 0x04,
  0x4d, 0xdd, 0x8c, 0x2a, 0x18, 0x04, 0xb1, 0xbe,
  0x9e, 0xf4, 0x19, 0x02, 0x0b, 0x85, 0x28, 0x04,
  0x4d, 0xdd, 0x8c, 0x2a, 0x29, 0x04, 0xb1, 0xbe,
  0x9e, 0xf4, 0x2a, 0x02, 0x0b, 0x85, 0x38, 0x01,
  0x2e, 0x39, 0x04, 0x00, 0x8d, 0xd4, 0x29, 0x01,
  0x02, 0x1c, 0x5f
};

GST_START_TEST (rtp_klv)
{
  rtp_pipeline_test (rtp_KLV_frame_data, G_N_ELEMENTS (rtp_KLV_frame_data), 1,
      "meta/x-klv, parsed=(bool)true", "rtpklvpay", "rtpklvdepay", 0, 0, FALSE);
}

GST_END_TEST;

GST_START_TEST (rtp_klv_fragmented)
{
  /* force super-small mtu of 60 to fragment KLV unit */
  rtp_pipeline_test (rtp_KLV_frame_data, sizeof (rtp_KLV_frame_data), 1,
      "meta/x-klv, parsed=(bool)true", "rtpklvpay", "rtpklvdepay",
      sizeof (rtp_KLV_frame_data), 60, FALSE);
}

GST_END_TEST;

static const guint8 rtp_L16_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_L16_frame_data_size = 20;

static int rtp_L16_frame_count = 1;

GST_START_TEST (rtp_L16)
{
  rtp_pipeline_test (rtp_L16_frame_data, rtp_L16_frame_data_size,
      rtp_L16_frame_count,
      "audio/x-raw,format=S16BE,rate=1,channels=1,layout=(string)interleaved",
      "rtpL16pay", "rtpL16depay", 0, 0, FALSE);
}

GST_END_TEST;

static const guint8 rtp_L24_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_L24_frame_data_size = 24;

static int rtp_L24_frame_count = 1;

GST_START_TEST (rtp_L24)
{
  rtp_pipeline_test (rtp_L24_frame_data, rtp_L24_frame_data_size,
      rtp_L24_frame_count,
      "audio/x-raw,format=S24BE,rate=1,channels=1,layout=(string)interleaved",
      "rtpL24pay", "rtpL24depay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_mp2t_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_mp2t_frame_data_size = 20;

static int rtp_mp2t_frame_count = 1;

GST_START_TEST (rtp_mp2t)
{
  rtp_pipeline_test (rtp_mp2t_frame_data, rtp_mp2t_frame_data_size,
      rtp_mp2t_frame_count, "video/mpegts,packetsize=188,systemstream=true",
      "rtpmp2tpay", "rtpmp2tdepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_mp4v_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_mp4v_frame_data_size = 20;

static int rtp_mp4v_frame_count = 1;

GST_START_TEST (rtp_mp4v)
{
  rtp_pipeline_test (rtp_mp4v_frame_data, rtp_mp4v_frame_data_size,
      rtp_mp4v_frame_count, "video/mpeg,mpegversion=4,systemstream=false",
      "rtpmp4vpay", "rtpmp4vdepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_mp4v_list_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_mp4v_list_frame_data_size = 20;

static int rtp_mp4v_list_frame_count = 1;

static int rtp_mp4v_list_bytes_sent = 1 * 20;

GST_START_TEST (rtp_mp4v_list)
{
  rtp_pipeline_test (rtp_mp4v_list_frame_data, rtp_mp4v_list_frame_data_size,
      rtp_mp4v_list_frame_count,
      "video/mpeg,mpegversion=4,systemstream=false,codec_data=(buffer)000001b001",
      "rtpmp4vpay", "rtpmp4vdepay", rtp_mp4v_list_bytes_sent, 0, TRUE);
}

GST_END_TEST;
static const guint8 rtp_mp4g_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_mp4g_frame_data_size = 20;

static int rtp_mp4g_frame_count = 1;

GST_START_TEST (rtp_mp4g)
{
  rtp_pipeline_test (rtp_mp4g_frame_data, rtp_mp4g_frame_data_size,
      rtp_mp4g_frame_count,
      "video/mpeg,mpegversion=4,systemstream=false,codec_data=(buffer)000001b001",
      "rtpmp4gpay", "rtpmp4gdepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_theora_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_theora_frame_data_size = 20;

static int rtp_theora_frame_count = 1;

GST_START_TEST (rtp_theora)
{
  rtp_pipeline_test (rtp_theora_frame_data, rtp_theora_frame_data_size,
      rtp_theora_frame_count, "video/x-theora", "rtptheorapay",
      "rtptheoradepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_vorbis_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_vorbis_frame_data_size = 20;

static int rtp_vorbis_frame_count = 1;

GST_START_TEST (rtp_vorbis)
{
  rtp_pipeline_test (rtp_vorbis_frame_data, rtp_vorbis_frame_data_size,
      rtp_vorbis_frame_count, "audio/x-vorbis", "rtpvorbispay",
      "rtpvorbisdepay", 0, 0, FALSE);
}

GST_END_TEST;
static const guint8 rtp_jpeg_frame_data[] =
    { /* SOF */ 0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x08, 0x00, 0x08,
  0x03, 0x00, 0x21, 0x08, 0x01, 0x11, 0x08, 0x02, 0x11, 0x08,
  /* DQT */ 0xFF, 0xDB, 0x00, 0x43, 0x08,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* DATA */ 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_jpeg_frame_data_size = sizeof (rtp_jpeg_frame_data);

static int rtp_jpeg_frame_count = 1;

GST_START_TEST (rtp_jpeg)
{
  rtp_pipeline_test (rtp_jpeg_frame_data, rtp_jpeg_frame_data_size,
      rtp_jpeg_frame_count, "video/x-jpeg,height=640,width=480", "rtpjpegpay",
      "rtpjpegdepay", 0, 0, FALSE);
}

GST_END_TEST;

GST_START_TEST (rtp_jpeg_width_greater_than_2040)
{
  rtp_pipeline_test (rtp_jpeg_frame_data, rtp_jpeg_frame_data_size,
      rtp_jpeg_frame_count, "video/x-jpeg,height=2048,width=480", "rtpjpegpay",
      "rtpjpegdepay", 0, 0, FALSE);
}

GST_END_TEST;

GST_START_TEST (rtp_jpeg_height_greater_than_2040)
{
  rtp_pipeline_test (rtp_jpeg_frame_data, rtp_jpeg_frame_data_size,
      rtp_jpeg_frame_count, "video/x-jpeg,height=640,width=2048", "rtpjpegpay",
      "rtpjpegdepay", 0, 0, FALSE);
}

GST_END_TEST;

GST_START_TEST (rtp_jpeg_width_and_height_greater_than_2040)
{
  rtp_pipeline_test (rtp_jpeg_frame_data, rtp_jpeg_frame_data_size,
      rtp_jpeg_frame_count, "video/x-jpeg,height=2048,width=2048", "rtpjpegpay",
      "rtpjpegdepay", 0, 0, FALSE);
}

GST_END_TEST;

static const guint8 rtp_jpeg_list_frame_data[] =
    { /* SOF */ 0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x08, 0x00, 0x08,
  0x03, 0x00, 0x21, 0x08, 0x01, 0x11, 0x08, 0x02, 0x11, 0x08,
  /* DQT */ 0xFF, 0xDB, 0x00, 0x43, 0x08,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* DATA */ 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_jpeg_list_frame_data_size = sizeof (rtp_jpeg_list_frame_data);

static int rtp_jpeg_list_frame_count = 1;

static int rtp_jpeg_list_bytes_sent = 1 * sizeof (rtp_jpeg_list_frame_data);

GST_START_TEST (rtp_jpeg_list)
{
  rtp_pipeline_test (rtp_jpeg_list_frame_data, rtp_jpeg_list_frame_data_size,
      rtp_jpeg_list_frame_count, "video/x-jpeg,height=640,width=480",
      "rtpjpegpay", "rtpjpegdepay", rtp_jpeg_list_bytes_sent, 0, TRUE);
}

GST_END_TEST;

GST_START_TEST (rtp_jpeg_list_width_greater_than_2040)
{
  rtp_pipeline_test (rtp_jpeg_list_frame_data, rtp_jpeg_list_frame_data_size,
      rtp_jpeg_list_frame_count, "video/x-jpeg,height=2048,width=480",
      "rtpjpegpay", "rtpjpegdepay", rtp_jpeg_list_bytes_sent, 0, TRUE);
}

GST_END_TEST;

GST_START_TEST (rtp_jpeg_list_height_greater_than_2040)
{
  rtp_pipeline_test (rtp_jpeg_list_frame_data, rtp_jpeg_list_frame_data_size,
      rtp_jpeg_list_frame_count, "video/x-jpeg,height=640,width=2048",
      "rtpjpegpay", "rtpjpegdepay", rtp_jpeg_list_bytes_sent, 0, TRUE);
}

GST_END_TEST;

GST_START_TEST (rtp_jpeg_list_width_and_height_greater_than_2040)
{
  rtp_pipeline_test (rtp_jpeg_list_frame_data, rtp_jpeg_list_frame_data_size,
      rtp_jpeg_list_frame_count, "video/x-jpeg,height=2048,width=2048",
      "rtpjpegpay", "rtpjpegdepay", rtp_jpeg_list_bytes_sent, 0, TRUE);
}

GST_END_TEST;

static void
rtp_jpeg_do_packet_loss (gdouble prob, gint num_expected)
{
  GstHarness *h;
  gboolean eos = FALSE;
  gchar *s;
  guint i, buffer_count;

  s = g_strdup_printf ("videotestsrc pattern=ball num-buffers=100 ! "
      "jpegenc quality=50 ! rtpjpegpay ! identity drop-probability=%g ! "
      "rtpjpegdepay", prob);
  GST_INFO ("running pipeline %s", s);
  h = gst_harness_new_parse (s);
  g_free (s);

  gst_harness_play (h);

  do {
    GstEvent *event;

    event = gst_harness_pull_event (h);
    eos = (GST_EVENT_TYPE (event) == GST_EVENT_EOS);
    gst_event_unref (event);
  } while (!eos);

  buffer_count = gst_harness_buffers_received (h);
  GST_INFO ("Got %u buffers", buffer_count);

  if (num_expected >= 0) {
    fail_unless_equals_int (num_expected, buffer_count);
  }

  for (i = 0; i < buffer_count; ++i) {
    GstBuffer *buf;
    GstMapInfo map;
    guint16 soi, eoi;

    buf = gst_harness_pull (h);
    fail_unless (buf != NULL);

    fail_unless (gst_buffer_map (buf, &map, GST_MAP_READ));
    GST_MEMDUMP ("jpeg frame", map.data, map.size);
    fail_unless (map.size > 4);
    soi = GST_READ_UINT16_BE (map.data);
    fail_unless (soi == 0xffd8, "expected JPEG frame start FFD8 not %02X", soi);
    eoi = GST_READ_UINT16_BE (map.data + map.size - 2);
    fail_unless (eoi == 0xffd9, "expected JPEG frame end FFD9 not %02X", eoi);
    gst_buffer_unmap (buf, &map);
    gst_buffer_unref (buf);
  }

  gst_harness_teardown (h);
}

GST_START_TEST (rtp_jpeg_packet_loss)
{
  gdouble probabilities[] = { 0.0, 0.001, 0.01, 0.1, 0.2, 0.5, 1.0 };
  gint num_expected[] = { 100, -1, -1, -1, -1, -1, 0 };

  GST_INFO ("Start iteration %d", __i__);
  fail_unless (__i__ < G_N_ELEMENTS (probabilities));
  rtp_jpeg_do_packet_loss (probabilities[__i__], num_expected[__i__]);
  GST_INFO ("Done with iteration %d", __i__);
}

GST_END_TEST;

static const guint8 rtp_g729_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_g729_frame_data_size = 22;

static int rtp_g729_frame_count = 1;

GST_START_TEST (rtp_g729)
{
  rtp_pipeline_test (rtp_g729_frame_data, rtp_g729_frame_data_size,
      rtp_g729_frame_count, "audio/G729,rate=8000,channels=1", "rtpg729pay",
      "rtpg729depay", 0, 0, FALSE);
}

GST_END_TEST;

static const guint8 rtp_gst_frame_data[] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int rtp_gst_frame_data_size = 22;

static int rtp_gst_frame_count = 1;

GST_START_TEST (rtp_gst_custom_event)
{
  /* Create RTP pipeline. */
  rtp_pipeline *p =
      rtp_pipeline_create (rtp_gst_frame_data, rtp_gst_frame_data_size,
      rtp_gst_frame_count, "application/x-test",
      "rtpgstpay", "rtpgstdepay");

  if (p == NULL) {
    return;
  }

  p->custom_event =
      gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
      gst_structure_new ("test", "foo", G_TYPE_INT, 1, NULL));

  /* Run RTP pipeline. */
  rtp_pipeline_run (p);

  /* Destroy RTP pipeline. */
  rtp_pipeline_destroy (p);
}

GST_END_TEST;

GST_START_TEST (rtp_vorbis_renegotiate)
{
  GstElement *pipeline;
  GstElement *enc, *pay, *depay, *dec, *sink;
  GstPad *sinkpad, *srcpad;
  GstCaps *templcaps, *caps, *filter, *srccaps;
  GstSegment segment;
  GstBuffer *buffer;
  GstMapInfo map;
  GstAudioInfo info;

  pipeline = gst_pipeline_new (NULL);
  enc = gst_element_factory_make ("vorbisenc", NULL);
  pay = gst_element_factory_make ("rtpvorbispay", NULL);
  depay = gst_element_factory_make ("rtpvorbisdepay", NULL);
  dec = gst_element_factory_make ("vorbisdec", NULL);
  sink = gst_element_factory_make ("fakesink", NULL);
  g_object_set (sink, "async", FALSE, NULL);
  gst_bin_add_many (GST_BIN (pipeline), enc, pay, depay, dec, sink, NULL);
  fail_unless (gst_element_link_many (enc, pay, depay, dec, sink, NULL));
  fail_unless_equals_int (gst_element_set_state (pipeline, GST_STATE_PLAYING),
      GST_STATE_CHANGE_SUCCESS);

  sinkpad = gst_element_get_static_pad (enc, "sink");
  srcpad = gst_element_get_static_pad (dec, "src");

  templcaps = gst_pad_get_pad_template_caps (sinkpad);
  filter =
      gst_caps_new_simple ("audio/x-raw", "channels", G_TYPE_INT, 2, "rate",
      G_TYPE_INT, 44100, NULL);
  caps = gst_caps_intersect (templcaps, filter);
  caps = gst_caps_fixate (caps);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_send_event (sinkpad,
          gst_event_new_stream_start ("test")));
  fail_unless (gst_pad_send_event (sinkpad, gst_event_new_caps (caps)));
  fail_unless (gst_pad_send_event (sinkpad, gst_event_new_segment (&segment)));

  gst_audio_info_from_caps (&info, caps);
  buffer = gst_buffer_new_and_alloc (44100 * info.bpf);
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  gst_audio_format_fill_silence (info.finfo, map.data, map.size);
  gst_buffer_unmap (buffer, &map);
  GST_BUFFER_PTS (buffer) = 0;
  GST_BUFFER_DURATION (buffer) = 1 * GST_SECOND;

  fail_unless_equals_int (gst_pad_chain (sinkpad, buffer), GST_FLOW_OK);

  srccaps = gst_pad_get_current_caps (srcpad);
  fail_unless (gst_caps_can_intersect (srccaps, caps));
  gst_caps_unref (srccaps);

  gst_caps_unref (caps);
  gst_caps_unref (filter);
  filter =
      gst_caps_new_simple ("audio/x-raw", "channels", G_TYPE_INT, 2, "rate",
      G_TYPE_INT, 48000, NULL);
  caps = gst_caps_intersect (templcaps, filter);
  caps = gst_caps_fixate (caps);

  fail_unless (gst_pad_send_event (sinkpad, gst_event_new_caps (caps)));

  gst_audio_info_from_caps (&info, caps);
  buffer = gst_buffer_new_and_alloc (48000 * info.bpf);
  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  gst_audio_format_fill_silence (info.finfo, map.data, map.size);
  gst_buffer_unmap (buffer, &map);
  GST_BUFFER_PTS (buffer) = 0;
  GST_BUFFER_DURATION (buffer) = 1 * GST_SECOND;
  GST_BUFFER_FLAG_SET (buffer, GST_BUFFER_FLAG_DISCONT);

  fail_unless_equals_int (gst_pad_chain (sinkpad, buffer), GST_FLOW_OK);

  srccaps = gst_pad_get_current_caps (srcpad);
  fail_unless (gst_caps_can_intersect (srccaps, caps));
  gst_caps_unref (srccaps);

  gst_caps_unref (caps);
  gst_caps_unref (filter);
  gst_caps_unref (templcaps);
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
}

GST_END_TEST;

/*
 * Creates the test suite.
 *
 * Returns: pointer to the test suite.
 */
static Suite *
rtp_payloading_suite (void)
{
  GstRegistry *registry = gst_registry_get ();
  Suite *s = suite_create ("rtp_data_test");

  TCase *tc_chain = tcase_create ("linear");

  /* Set timeout to 60 seconds. */
  tcase_set_timeout (tc_chain, 60);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, rtp_ilbc);
  tcase_add_test (tc_chain, rtp_gsm);
  tcase_add_test (tc_chain, rtp_amr);
  tcase_add_test (tc_chain, rtp_pcma);
  tcase_add_test (tc_chain, rtp_pcmu);
  tcase_add_test (tc_chain, rtp_mpa);
  tcase_add_test (tc_chain, rtp_h261);
  tcase_add_test (tc_chain, rtp_h263);
  tcase_add_test (tc_chain, rtp_h263p);
  tcase_add_test (tc_chain, rtp_h264);
  tcase_add_test (tc_chain, rtp_h264depay_avc);
  tcase_add_test (tc_chain, rtp_h264depay_bytestream);
  tcase_add_test (tc_chain, rtp_h264_list_lt_mtu);
  tcase_add_test (tc_chain, rtp_h264_list_lt_mtu_avc);
  tcase_add_test (tc_chain, rtp_h264_list_gt_mtu);
  tcase_add_test (tc_chain, rtp_h264_list_gt_mtu_avc);
  tcase_add_test (tc_chain, rtp_h265);
  tcase_add_test (tc_chain, rtp_h265_list_lt_mtu);
  tcase_add_test (tc_chain, rtp_h265_list_lt_mtu_hvc1);
  tcase_add_test (tc_chain, rtp_h265_list_gt_mtu);
  tcase_add_test (tc_chain, rtp_h265_list_gt_mtu_hvc1);
  tcase_add_test (tc_chain, rtp_klv);
  tcase_add_test (tc_chain, rtp_klv_fragmented);
  tcase_add_test (tc_chain, rtp_L16);
  tcase_add_test (tc_chain, rtp_L24);
  tcase_add_test (tc_chain, rtp_mp2t);
  tcase_add_test (tc_chain, rtp_mp4v);
  tcase_add_test (tc_chain, rtp_mp4v_list);
  tcase_add_test (tc_chain, rtp_mp4g);
  tcase_add_test (tc_chain, rtp_theora);
  tcase_add_test (tc_chain, rtp_vorbis);
  tcase_add_test (tc_chain, rtp_jpeg);
  tcase_add_test (tc_chain, rtp_jpeg_width_greater_than_2040);
  tcase_add_test (tc_chain, rtp_jpeg_height_greater_than_2040);
  tcase_add_test (tc_chain, rtp_jpeg_width_and_height_greater_than_2040);
  tcase_add_test (tc_chain, rtp_jpeg_list);
  tcase_add_test (tc_chain, rtp_jpeg_list_width_greater_than_2040);
  tcase_add_test (tc_chain, rtp_jpeg_list_height_greater_than_2040);
  tcase_add_test (tc_chain, rtp_jpeg_list_width_and_height_greater_than_2040);
  if (gst_registry_check_feature_version (registry, "jpegenc", 1, 0, 0)
      && gst_registry_check_feature_version (registry, "videotestsrc", 1, 0, 0))
    tcase_add_loop_test (tc_chain, rtp_jpeg_packet_loss, 0, 7);
  tcase_add_test (tc_chain, rtp_g729);
  tcase_add_test (tc_chain, rtp_gst_custom_event);
  tcase_add_test (tc_chain, rtp_vorbis_renegotiate);
  return s;
}

GST_CHECK_MAIN (rtp_payloading)
