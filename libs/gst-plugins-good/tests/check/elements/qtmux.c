/* GStreamer
 *
 * unit test for qtmux
 *
 * Copyright (C) <2008> Mark Nauwelaerts <mnauw@users.sf.net>
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
#include <config.h>
#endif

#include <glib/gstdio.h>

#include <gst/check/gstcheck.h>
#include <gst/pbutils/encoding-profile.h>

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;

#define VIDEO_RAW_CAPS_STRING "video/x-raw"

#define AUDIO_CAPS_STRING "audio/mpeg, " \
                        "mpegversion = (int) 1, " \
                        "layer = (int) 3, " \
                        "channels = (int) 2, " \
                        "rate = (int) 48000"

#define AUDIO_AAC_TMPL_CAPS_STRING "audio/mpeg, " \
                                   "mpegversion=(int)4, " \
                                   "channels=(int)1, " \
                                   "rate=(int)44100, " \
                                   "stream-format=(string)raw, " \
                                   "level=(string)2, " \
                                   "base-profile=(string)lc, " \
                                   "profile=(string)lc"
/* codec_data shouldn't be in the template caps, only in the actual caps */
#define AUDIO_AAC_CAPS_STRING AUDIO_AAC_TMPL_CAPS_STRING \
                              ", codec_data=(buffer)1208"

#define VIDEO_CAPS_STRING "video/mpeg, " \
                           "mpegversion = (int) 4, " \
                           "systemstream = (boolean) false, " \
                           "width = (int) 384, " \
                           "height = (int) 288, " \
                           "framerate = (fraction) 25/1"

#define VIDEO_TMPL_CAPS_H264_STRING "video/x-h264, " \
                                    "width=(int)320, " \
                                    "height=(int)240, " \
                                    "framerate=(fraction)30/1, " \
                                    "pixel-aspect-ratio=(fraction)1/1, " \
                                    "stream-format=(string)avc, " \
                                    "alignment=(string)au, " \
                                    "level=(string)2, " \
                                    "profile=(string)high"
/* codec_data shouldn't be in the template caps, only in the actual caps */
#define VIDEO_CAPS_H264_STRING VIDEO_TMPL_CAPS_H264_STRING \
                               ", codec_data=(buffer)01640014ffe1001867640014a" \
                                   "cd94141fb0110000003001773594000f14299600" \
                                   "1000568ebecb22c"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/quicktime"));

static GstStaticPadTemplate srcvideotemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (VIDEO_CAPS_STRING));

static GstStaticPadTemplate srcvideoh264template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (VIDEO_TMPL_CAPS_H264_STRING));

static GstStaticPadTemplate srcvideorawtemplate =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (VIDEO_RAW_CAPS_STRING));

static GstStaticPadTemplate srcaudiotemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (AUDIO_CAPS_STRING));

static GstStaticPadTemplate srcaudioaactemplate =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (AUDIO_AAC_TMPL_CAPS_STRING));

/* setup and teardown needs some special handling for muxer */
static GstPad *
setup_src_pad (GstElement * element,
    GstStaticPadTemplate * template, const gchar * sinkname)
{
  GstPad *srcpad, *sinkpad;

  GST_DEBUG_OBJECT (element, "setting up sending pad");
  /* sending pad */
  srcpad = gst_pad_new_from_static_template (template, "src");
  fail_if (srcpad == NULL, "Could not create a srcpad");
  ASSERT_OBJECT_REFCOUNT (srcpad, "srcpad", 1);

  if (!(sinkpad = gst_element_get_static_pad (element, sinkname)))
    sinkpad = gst_element_get_request_pad (element, sinkname);
  fail_if (sinkpad == NULL, "Could not get sink pad from %s",
      GST_ELEMENT_NAME (element));
  /* references are owned by: 1) us, 2) qtmux, 3) collect pads */
  ASSERT_OBJECT_REFCOUNT (sinkpad, "sinkpad", 3);
  fail_unless (gst_pad_link (srcpad, sinkpad) == GST_PAD_LINK_OK,
      "Could not link source and %s sink pads", GST_ELEMENT_NAME (element));
  gst_object_unref (sinkpad);   /* because we got it higher up */

  /* references are owned by: 1) qtmux, 2) collect pads */
  ASSERT_OBJECT_REFCOUNT (sinkpad, "sinkpad", 2);

  return srcpad;
}

static void
teardown_src_pad (GstPad * srcpad)
{
  GstPad *sinkpad;

  /* clean up floating src pad */
  sinkpad = gst_pad_get_peer (srcpad);
  fail_if (sinkpad == NULL);
  /* pad refs held by 1) qtmux 2) collectpads and 3) us (through _get_peer) */
  ASSERT_OBJECT_REFCOUNT (sinkpad, "sinkpad", 3);

  gst_pad_unlink (srcpad, sinkpad);

  /* after unlinking, pad refs still held by
   * 1) qtmux and 2) collectpads and 3) us (through _get_peer) */
  ASSERT_OBJECT_REFCOUNT (sinkpad, "sinkpad", 3);
  gst_object_unref (sinkpad);
  /* one more ref is held by element itself */

  /* pad refs held by creator */
  ASSERT_OBJECT_REFCOUNT (srcpad, "srcpad", 1);
  gst_object_unref (srcpad);
}

gboolean downstream_is_seekable;
static gboolean
qtmux_sinkpad_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean ret = FALSE;

  if (GST_QUERY_TYPE (query) == GST_QUERY_SEEKING) {
    gst_query_set_seeking (query, GST_FORMAT_BYTES, downstream_is_seekable, 0,
        -1);
    ret = TRUE;
  }

  return ret;
}

static GstElement *
setup_qtmux (GstStaticPadTemplate * srctemplate, const gchar * sinkname,
    gboolean seekable)
{
  GstElement *qtmux;

  GST_DEBUG ("setup_qtmux");
  qtmux = gst_check_setup_element ("qtmux");
  mysrcpad = setup_src_pad (qtmux, srctemplate, sinkname);
  mysinkpad = gst_check_setup_sink_pad (qtmux, &sinktemplate);

  downstream_is_seekable = seekable;
  gst_pad_set_query_function (mysinkpad, qtmux_sinkpad_query);

  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  return qtmux;
}

static void
cleanup_qtmux (GstElement * qtmux, const gchar * sinkname)
{
  GST_DEBUG ("cleanup_qtmux");
  gst_element_set_state (qtmux, GST_STATE_NULL);

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  teardown_src_pad (mysrcpad);
  gst_check_teardown_sink_pad (qtmux);
  gst_check_teardown_element (qtmux);
}

static void
check_qtmux_pad (GstStaticPadTemplate * srctemplate, const gchar * sinkname,
    guint32 dts_method)
{
  GstElement *qtmux;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  int num_buffers;
  int i;
  guint8 data0[12] = "\000\000\000\024ftypqt  ";
  guint8 data1[16] = "\000\000\000\010free\000\000\000\000mdat";
  guint8 data2[4] = "moov";
  GstSegment segment;

  qtmux = setup_qtmux (srctemplate, sinkname, TRUE);
  g_object_set (qtmux, "dts-method", dts_method, NULL);
  fail_unless (gst_element_set_state (qtmux,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  gst_pad_push_event (mysrcpad, gst_event_new_stream_start ("test"));

  caps = gst_pad_get_pad_template_caps (mysrcpad);
  gst_pad_set_caps (mysrcpad, caps);
  gst_caps_unref (caps);

  /* ensure segment (format) properly setup */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  inbuffer = gst_buffer_new_and_alloc (1);
  gst_buffer_memset (inbuffer, 0, 0, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  GST_BUFFER_DURATION (inbuffer) = 40 * GST_MSECOND;
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);

  /* send eos to have moov written */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()) == TRUE);

  num_buffers = g_list_length (buffers);
  /* at least expect ftyp, mdat header, buffer chunk and moov */
  fail_unless (num_buffers >= 4);

  /* clean up first to clear any pending refs in sticky caps */
  cleanup_qtmux (qtmux, sinkname);

  for (i = 0; i < num_buffers; ++i) {
    outbuffer = GST_BUFFER (buffers->data);
    fail_if (outbuffer == NULL);
    buffers = g_list_remove (buffers, outbuffer);

    switch (i) {
      case 0:
      {
        /* ftyp header */
        fail_unless (gst_buffer_get_size (outbuffer) >= 20);
        fail_unless (gst_buffer_memcmp (outbuffer, 0, data0,
                sizeof (data0)) == 0);
        fail_unless (gst_buffer_memcmp (outbuffer, 16, data0 + 8, 4) == 0);
        break;
      }
      case 1:                  /* mdat header */
        fail_unless (gst_buffer_get_size (outbuffer) == 16);
        fail_unless (gst_buffer_memcmp (outbuffer, 0, data1, sizeof (data1))
            == 0);
        break;
      case 2:                  /* buffer we put in */
        fail_unless (gst_buffer_get_size (outbuffer) == 1);
        break;
      case 3:                  /* moov */
        fail_unless (gst_buffer_get_size (outbuffer) > 8);
        fail_unless (gst_buffer_memcmp (outbuffer, 4, data2,
                sizeof (data2)) == 0);
        break;
      default:
        break;
    }

    ASSERT_BUFFER_REFCOUNT (outbuffer, "outbuffer", 1);
    gst_buffer_unref (outbuffer);
    outbuffer = NULL;
  }

  g_list_free (buffers);
  buffers = NULL;
}

static void
check_qtmux_pad_fragmented (GstStaticPadTemplate * srctemplate,
    const gchar * sinkname, guint32 dts_method, gboolean streamable)
{
  GstElement *qtmux;
  GstBuffer *inbuffer, *outbuffer;
  GstCaps *caps;
  int num_buffers;
  int i;
  guint8 data0[12] = "\000\000\000\024ftypqt  ";
  guint8 data1[4] = "mdat";
  guint8 data2[4] = "moov";
  guint8 data3[4] = "moof";
  guint8 data4[4] = "mfra";
  GstSegment segment;

  qtmux = setup_qtmux (srctemplate, sinkname, !streamable);
  g_object_set (qtmux, "dts-method", dts_method, NULL);
  g_object_set (qtmux, "fragment-duration", 2000, NULL);
  g_object_set (qtmux, "streamable", streamable, NULL);
  fail_unless (gst_element_set_state (qtmux,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  gst_pad_push_event (mysrcpad, gst_event_new_stream_start ("test"));

  caps = gst_pad_get_pad_template_caps (mysrcpad);
  gst_pad_set_caps (mysrcpad, caps);
  gst_caps_unref (caps);

  /* ensure segment (format) properly setup */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  inbuffer = gst_buffer_new_and_alloc (1);
  gst_buffer_memset (inbuffer, 0, 0, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  GST_BUFFER_DURATION (inbuffer) = 40 * GST_MSECOND;
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);

  /* send eos to have all written */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()) == TRUE);

  num_buffers = g_list_length (buffers);
  /* at least expect ftyp, moov, moof, mdat header, buffer chunk
   * and optionally mfra */
  fail_unless (num_buffers >= 5);

  /* clean up first to clear any pending refs in sticky caps */
  cleanup_qtmux (qtmux, sinkname);

  for (i = 0; i < num_buffers; ++i) {
    outbuffer = GST_BUFFER (buffers->data);
    fail_if (outbuffer == NULL);
    buffers = g_list_remove (buffers, outbuffer);

    switch (i) {
      case 0:
      {
        /* ftyp header */
        fail_unless (gst_buffer_get_size (outbuffer) >= 20);
        fail_unless (gst_buffer_memcmp (outbuffer, 0, data0,
                sizeof (data0)) == 0);
        fail_unless (gst_buffer_memcmp (outbuffer, 16, data0 + 8, 4) == 0);
        break;
      }
      case 1:                  /* moov */
        fail_unless (gst_buffer_get_size (outbuffer) > 8);
        fail_unless (gst_buffer_memcmp (outbuffer, 4, data2,
                sizeof (data2)) == 0);
        break;
      case 2:                  /* moof */
        fail_unless (gst_buffer_get_size (outbuffer) > 8);
        fail_unless (gst_buffer_memcmp (outbuffer, 4, data3,
                sizeof (data3)) == 0);
        break;
      case 3:                  /* mdat header */
        fail_unless (gst_buffer_get_size (outbuffer) == 8);
        fail_unless (gst_buffer_memcmp (outbuffer, 4, data1,
                sizeof (data1)) == 0);
        break;
      case 4:                  /* buffer we put in */
        fail_unless (gst_buffer_get_size (outbuffer) == 1);
        break;
      case 5:                  /* mfra */
        fail_unless (gst_buffer_get_size (outbuffer) > 8);
        fail_unless (gst_buffer_memcmp (outbuffer, 4, data4,
                sizeof (data4)) == 0);
        break;
      default:
        break;
    }

    ASSERT_BUFFER_REFCOUNT (outbuffer, "outbuffer", 1);
    gst_buffer_unref (outbuffer);
    outbuffer = NULL;
  }

  g_list_free (buffers);
  buffers = NULL;
}

/* dts-method dd */

GST_START_TEST (test_video_pad_dd)
{
  check_qtmux_pad (&srcvideotemplate, "video_%u", 0);
}

GST_END_TEST;

GST_START_TEST (test_audio_pad_dd)
{
  check_qtmux_pad (&srcaudiotemplate, "audio_%u", 0);
}

GST_END_TEST;


GST_START_TEST (test_video_pad_frag_dd)
{
  check_qtmux_pad_fragmented (&srcvideotemplate, "video_%u", 0, FALSE);
}

GST_END_TEST;

GST_START_TEST (test_audio_pad_frag_dd)
{
  check_qtmux_pad_fragmented (&srcaudiotemplate, "audio_%u", 0, FALSE);
}

GST_END_TEST;


GST_START_TEST (test_video_pad_frag_dd_streamable)
{
  check_qtmux_pad_fragmented (&srcvideotemplate, "video_%u", 0, TRUE);
}

GST_END_TEST;


GST_START_TEST (test_audio_pad_frag_dd_streamable)
{
  check_qtmux_pad_fragmented (&srcaudiotemplate, "audio_%u", 0, TRUE);
}

GST_END_TEST;

/* dts-method reorder */

GST_START_TEST (test_video_pad_reorder)
{
  check_qtmux_pad (&srcvideotemplate, "video_%u", 1);
}

GST_END_TEST;

GST_START_TEST (test_audio_pad_reorder)
{
  check_qtmux_pad (&srcaudiotemplate, "audio_%u", 1);
}

GST_END_TEST;


GST_START_TEST (test_video_pad_frag_reorder)
{
  check_qtmux_pad_fragmented (&srcvideotemplate, "video_%u", 1, FALSE);
}

GST_END_TEST;

GST_START_TEST (test_audio_pad_frag_reorder)
{
  check_qtmux_pad_fragmented (&srcaudiotemplate, "audio_%u", 1, FALSE);
}

GST_END_TEST;


GST_START_TEST (test_video_pad_frag_reorder_streamable)
{
  check_qtmux_pad_fragmented (&srcvideotemplate, "video_%u", 1, TRUE);
}

GST_END_TEST;


GST_START_TEST (test_audio_pad_frag_reorder_streamable)
{
  check_qtmux_pad_fragmented (&srcaudiotemplate, "audio_%u", 1, TRUE);
}

GST_END_TEST;

/* dts-method asc */

GST_START_TEST (test_video_pad_asc)
{
  check_qtmux_pad (&srcvideotemplate, "video_%u", 2);
}

GST_END_TEST;

GST_START_TEST (test_audio_pad_asc)
{
  check_qtmux_pad (&srcaudiotemplate, "audio_%u", 2);
}

GST_END_TEST;


GST_START_TEST (test_video_pad_frag_asc)
{
  check_qtmux_pad_fragmented (&srcvideotemplate, "video_%u", 2, FALSE);
}

GST_END_TEST;

GST_START_TEST (test_audio_pad_frag_asc)
{
  check_qtmux_pad_fragmented (&srcaudiotemplate, "audio_%u", 2, FALSE);
}

GST_END_TEST;


GST_START_TEST (test_video_pad_frag_asc_streamable)
{
  check_qtmux_pad_fragmented (&srcvideotemplate, "video_%u", 2, TRUE);
}

GST_END_TEST;


GST_START_TEST (test_audio_pad_frag_asc_streamable)
{
  check_qtmux_pad_fragmented (&srcaudiotemplate, "audio_%u", 2, TRUE);
}

GST_END_TEST;

GST_START_TEST (test_reuse)
{
  GstElement *qtmux = setup_qtmux (&srcvideotemplate, "video_%u", TRUE);
  GstBuffer *inbuffer;
  GstCaps *caps;
  GstSegment segment;

  gst_element_set_state (qtmux, GST_STATE_PLAYING);
  gst_element_set_state (qtmux, GST_STATE_NULL);
  gst_element_set_state (qtmux, GST_STATE_PLAYING);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  gst_pad_push_event (mysrcpad, gst_event_new_stream_start ("test"));

  caps = gst_pad_get_pad_template_caps (mysrcpad);
  gst_pad_set_caps (mysrcpad, caps);
  gst_caps_unref (caps);

  /* ensure segment (format) properly setup */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  inbuffer = gst_buffer_new_and_alloc (1);
  fail_unless (inbuffer != NULL);
  gst_buffer_memset (inbuffer, 0, 0, 1);
  GST_BUFFER_TIMESTAMP (inbuffer) = 0;
  GST_BUFFER_DURATION (inbuffer) = 40 * GST_MSECOND;
  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);
  fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);

  /* send eos to have all written */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()) == TRUE);

  cleanup_qtmux (qtmux, "video_%u");
  gst_check_drop_buffers ();
}

GST_END_TEST;

static GstEncodingContainerProfile *
create_qtmux_profile (const gchar * variant)
{
  GstEncodingContainerProfile *cprof;
  GstCaps *caps;

  if (variant == NULL) {
    caps = gst_caps_new_empty_simple ("video/quicktime");
  } else {
    caps = gst_caps_new_simple ("video/quicktime",
        "variant", G_TYPE_STRING, variant, NULL);
  }

  cprof = gst_encoding_container_profile_new ("Name", "blah", caps, NULL);
  gst_caps_unref (caps);

  caps = gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, "S16BE",
      "channels", G_TYPE_INT, 2, "rate", G_TYPE_INT, 44100, NULL);
  gst_encoding_container_profile_add_profile (cprof,
      GST_ENCODING_PROFILE (gst_encoding_audio_profile_new (caps, NULL, NULL,
              1)));
  gst_caps_unref (caps);

  return cprof;
}

GST_START_TEST (test_encodebin_qtmux)
{
  GstEncodingContainerProfile *cprof;
  GstElement *enc;
  GstPad *pad;

  enc = gst_element_factory_make ("encodebin", NULL);
  if (enc == NULL)
    return;

  /* Make sure encodebin finds a muxer for a profile with a variant field .. */
  cprof = create_qtmux_profile ("apple");
  g_object_set (enc, "profile", cprof, NULL);
  gst_encoding_profile_unref (cprof);

  /* should have created a pad after setting the profile */
  pad = gst_element_get_static_pad (enc, "audio_0");
  fail_unless (pad != NULL);
  gst_object_unref (pad);
  gst_object_unref (enc);

  /* ... and for a profile without a variant field */
  enc = gst_element_factory_make ("encodebin", NULL);
  cprof = create_qtmux_profile (NULL);
  g_object_set (enc, "profile", cprof, NULL);
  gst_encoding_profile_unref (cprof);

  /* should have created a pad after setting the profile */
  pad = gst_element_get_static_pad (enc, "audio_0");
  fail_unless (pad != NULL);
  gst_object_unref (pad);
  gst_object_unref (enc);
}

GST_END_TEST;

/* Fake mp3 encoder for test */
typedef GstElement TestMp3Enc;
typedef GstElementClass TestMp3EncClass;

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, mpegversion=1, layer=[1,3]")
    );

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw")
    );

static GType test_mp3_enc_get_type (void);
static void test_input_push_segment_start (gpointer user_data,
    GstClockTime start);

G_DEFINE_TYPE (TestMp3Enc, test_mp3_enc, GST_TYPE_ELEMENT);

static void
test_mp3_enc_class_init (TestMp3EncClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class, &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);

  gst_element_class_set_metadata (element_class, "MPEG1 Audio Encoder",
      "Codec/Encoder/Audio", "Pretends to encode mp3", "Foo Bar <foo@bar.com>");
}

static void
test_mp3_enc_init (TestMp3Enc * mp3enc)
{
  GstPad *pad;

  pad = gst_pad_new_from_static_template (&sink_template, "sink");
  gst_element_add_pad (mp3enc, pad);

  pad = gst_pad_new_from_static_template (&src_template, "src");
  gst_element_add_pad (mp3enc, pad);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "testmp3enc", GST_RANK_NONE,
      test_mp3_enc_get_type ());
}

static GstEncodingContainerProfile *
create_mp4mux_profile (void)
{
  GstEncodingContainerProfile *cprof;
  GstCaps *caps;

  caps = gst_caps_new_simple ("video/quicktime",
      "variant", G_TYPE_STRING, "iso", NULL);

  cprof = gst_encoding_container_profile_new ("Name", "blah", caps, NULL);
  gst_caps_unref (caps);

  caps = gst_caps_new_simple ("audio/mpeg", "mpegversion", G_TYPE_INT, 1,
      "layer", G_TYPE_INT, 3, "channels", G_TYPE_INT, 2, "rate", G_TYPE_INT,
      44100, NULL);
  gst_encoding_container_profile_add_profile (cprof,
      GST_ENCODING_PROFILE (gst_encoding_audio_profile_new (caps, NULL, NULL,
              1)));
  gst_caps_unref (caps);

  return cprof;
}

GST_START_TEST (test_encodebin_mp4mux)
{
  GstEncodingContainerProfile *cprof;
  GstPluginFeature *feature;
  GstElement *enc, *mux;
  GstPad *pad;

  /* need a fake mp3 encoder because mp4 only accepts encoded formats */
  gst_plugin_register_static (GST_VERSION_MAJOR, GST_VERSION_MINOR,
      "fakemp3enc", "fakemp3enc", plugin_init, VERSION, "LGPL",
      "gst-plugins-good", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);

  feature = gst_registry_find_feature (gst_registry_get (), "testmp3enc",
      GST_TYPE_ELEMENT_FACTORY);
  gst_plugin_feature_set_rank (feature, GST_RANK_PRIMARY + 100);

  enc = gst_element_factory_make ("encodebin", NULL);
  if (enc == NULL)
    return;

  /* Make sure encodebin finds mp4mux even though qtmux outputs a superset */
  cprof = create_mp4mux_profile ();
  g_object_set (enc, "profile", cprof, NULL);
  gst_encoding_profile_unref (cprof);

  /* should have created a pad after setting the profile */
  pad = gst_element_get_static_pad (enc, "audio_0");
  fail_unless (pad != NULL);
  gst_object_unref (pad);

  mux = gst_bin_get_by_interface (GST_BIN (enc), GST_TYPE_TAG_SETTER);
  fail_unless (mux != NULL);
  {
    GstElementFactory *f = gst_element_get_factory (mux);

    /* make sure we got mp4mux for variant=iso */
    GST_INFO ("muxer: %s", G_OBJECT_TYPE_NAME (mux));
    fail_unless_equals_string (GST_OBJECT_NAME (f), "mp4mux");
  }
  gst_object_unref (mux);
  gst_object_unref (enc);

  gst_plugin_feature_set_rank (feature, GST_RANK_NONE);
  gst_object_unref (feature);
}

GST_END_TEST;

static gboolean
extract_tags (const gchar * location, GstTagList ** taglist)
{
  gboolean ret = TRUE;
  GstElement *src;
  GstBus *bus;
  GstElement *pipeline =
      gst_parse_launch ("filesrc name=src ! qtdemux ! fakesink", NULL);

  src = gst_bin_get_by_name (GST_BIN (pipeline), "src");
  g_object_set (src, "location", location, NULL);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  fail_unless (gst_element_set_state (pipeline, GST_STATE_PLAYING)
      != GST_STATE_CHANGE_FAILURE);

  if (*taglist == NULL) {
    *taglist = gst_tag_list_new_empty ();
  }

  while (1) {
    GstMessage *msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_TAG | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS) {
      gst_message_unref (msg);
      break;
    } else if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
      ret = FALSE;
      gst_message_unref (msg);
      break;
    } else if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_TAG) {
      GstTagList *tags;

      gst_message_parse_tag (msg, &tags);
      gst_tag_list_insert (*taglist, tags, GST_TAG_MERGE_REPLACE);
      gst_tag_list_unref (tags);
    }
    gst_message_unref (msg);
  }

  gst_object_unref (bus);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (src);
  gst_object_unref (pipeline);
  return ret;
}

static void
test_average_bitrate_custom (const gchar * elementname,
    GstStaticPadTemplate * tmpl, const gchar * caps_str,
    const gchar * sinkpadname)
{
  gchar *location;
  GstElement *qtmux;
  GstElement *filesink;
  GstBuffer *inbuffer;
  GstCaps *caps;
  int i;
  gint bytes[] = { 16, 22, 12 };
  gint64 durations[] = { GST_SECOND * 3, GST_SECOND * 5, GST_SECOND * 2 };
  gint64 total_bytes = 0;
  GstClockTime total_duration = 0;
  GstSegment segment;

  location = g_strdup_printf ("%s/%s-%d", g_get_tmp_dir (), "qtmuxtest",
      g_random_int ());
  GST_INFO ("Using location %s for bitrate test", location);
  qtmux = gst_check_setup_element (elementname);
  filesink = gst_element_factory_make ("filesink", NULL);
  g_object_set (filesink, "location", location, NULL);
  gst_element_link (qtmux, filesink);
  mysrcpad = setup_src_pad (qtmux, tmpl, sinkpadname);
  fail_unless (mysrcpad != NULL);
  gst_pad_set_active (mysrcpad, TRUE);


  fail_unless (gst_element_set_state (filesink,
          GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
      "could not set filesink to playing");
  fail_unless (gst_element_set_state (qtmux,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  gst_pad_push_event (mysrcpad, gst_event_new_stream_start ("test"));

  caps = gst_caps_from_string (caps_str);
  gst_pad_set_caps (mysrcpad, caps);
  gst_caps_unref (caps);

  /* ensure segment (format) properly setup */
  gst_segment_init (&segment, GST_FORMAT_TIME);
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_segment (&segment)));

  for (i = 0; i < 3; i++) {
    inbuffer = gst_buffer_new_and_alloc (bytes[i]);
    gst_buffer_memset (inbuffer, 0, 0, bytes[i]);
    GST_BUFFER_TIMESTAMP (inbuffer) = total_duration;
    GST_BUFFER_DURATION (inbuffer) = (GstClockTime) durations[i];
    ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

    total_bytes += gst_buffer_get_size (inbuffer);
    total_duration += GST_BUFFER_DURATION (inbuffer);
    fail_unless (gst_pad_push (mysrcpad, inbuffer) == GST_FLOW_OK);
  }

  /* send eos to have moov written */
  fail_unless (gst_pad_push_event (mysrcpad, gst_event_new_eos ()) == TRUE);

  gst_element_set_state (qtmux, GST_STATE_NULL);
  gst_element_set_state (filesink, GST_STATE_NULL);

  gst_check_drop_buffers ();
  gst_pad_set_active (mysrcpad, FALSE);
  teardown_src_pad (mysrcpad);
  gst_object_unref (filesink);
  gst_check_teardown_element (qtmux);

  /* check the bitrate tag */
  {
    GstTagList *taglist = NULL;
    guint bitrate = 0;
    guint expected;

    fail_unless (extract_tags (location, &taglist));

    fail_unless (gst_tag_list_get_uint (taglist, GST_TAG_BITRATE, &bitrate));

    expected =
        (guint) gst_util_uint64_scale_round ((guint64) total_bytes,
        (guint64) 8 * GST_SECOND, (guint64) total_duration);
    fail_unless (bitrate == expected);
    gst_tag_list_unref (taglist);
  }

  /* delete file */
  g_unlink (location);
  g_free (location);
}

GST_START_TEST (test_average_bitrate)
{
  test_average_bitrate_custom ("mp4mux", &srcaudioaactemplate,
      AUDIO_AAC_CAPS_STRING, "audio_%u");
  test_average_bitrate_custom ("mp4mux", &srcvideoh264template,
      VIDEO_CAPS_H264_STRING, "video_%u");

  test_average_bitrate_custom ("qtmux", &srcaudioaactemplate,
      AUDIO_AAC_CAPS_STRING, "audio_%u");
  test_average_bitrate_custom ("qtmux", &srcvideoh264template,
      VIDEO_CAPS_H264_STRING, "video_%u");
}

GST_END_TEST;

struct TestInputData
{
  GstPad *srcpad;
  GstSegment segment;
  GList *input;
  GThread *thread;

  /* When comparing ts, the input will be subtracted from this */
  gint64 ts_offset;
  /* Due to DTS, the segment start might be shifted so this list
   * is used to vefity each received segments */
  GList *expected_segment_start;

  GstClockTime expected_gap_ts;
  GstClockTime expected_gap_duration;
  gboolean gap_received;

  GstPad *sinkpad;

  GList *output_iter;
};

static void
test_input_data_init (struct TestInputData *data)
{
  data->ts_offset = 0;
  data->expected_segment_start = NULL;
  data->expected_gap_ts = 0;
  data->expected_gap_duration = 0;
  data->gap_received = FALSE;
  data->srcpad = NULL;
  data->sinkpad = NULL;
  data->input = NULL;
  data->thread = NULL;

  test_input_push_segment_start (data, 0);
}

static void
test_input_data_clean (struct TestInputData *data)
{
  g_list_free_full (data->input, (GDestroyNotify) gst_mini_object_unref);

  if (data->sinkpad) {
    gst_pad_set_active (data->sinkpad, FALSE);
    gst_object_unref (data->sinkpad);
  }

  gst_pad_set_active (data->srcpad, FALSE);
  teardown_src_pad (data->srcpad);
}

static gpointer
test_input_push_data (gpointer user_data)
{
  struct TestInputData *data = user_data;
  GList *iter;
  GstFlowReturn flow;

  for (iter = data->input; iter; iter = g_list_next (iter)) {
    if (GST_IS_BUFFER (iter->data)) {
      GST_INFO ("Pushing buffer %" GST_PTR_FORMAT " on pad: %s:%s", iter->data,
          GST_DEBUG_PAD_NAME (data->srcpad));
      flow =
          gst_pad_push (data->srcpad,
          gst_buffer_ref ((GstBuffer *) iter->data));
      fail_unless (flow == GST_FLOW_OK);
    } else {
      GST_INFO_OBJECT (data->srcpad, "Pushing event: %"
          GST_PTR_FORMAT, iter->data);
      fail_unless (gst_pad_push_event (data->srcpad,
              gst_event_ref ((GstEvent *) iter->data)) == TRUE);
    }
  }
  return NULL;
}

static void
test_input_push_segment_start (gpointer user_data, GstClockTime start)
{
  struct TestInputData *data = user_data;
  GstClockTime *start_data = g_malloc (sizeof (GstClockTime));

  *start_data = start;
  data->expected_segment_start = g_list_append (data->expected_segment_start,
      start_data);
}

static GstClockTime
test_input_pop_segment_start (gpointer user_data)
{
  struct TestInputData *data = user_data;
  GstClockTime start = GST_CLOCK_TIME_NONE;
  GstClockTime *start_data;

  if (data->expected_segment_start) {
    start_data = data->expected_segment_start->data;
    data->expected_segment_start =
        g_list_delete_link (data->expected_segment_start,
        data->expected_segment_start);
    start = *start_data;
    g_free (start_data);
  }

  return start;
}

static GstBuffer *
create_buffer (GstClockTime pts, GstClockTime dts, GstClockTime duration,
    guint bytes)
{
  GstBuffer *buf;
  guint8 *data;

  data = g_malloc0 (bytes);
  buf = gst_buffer_new_wrapped (data, bytes);
  GST_BUFFER_PTS (buf) = pts;
  GST_BUFFER_DTS (buf) = dts;
  GST_BUFFER_DURATION (buf) = duration;
  return buf;
}

static GstFlowReturn
_test_sink_pad_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  struct TestInputData *test_data = g_object_get_qdata (G_OBJECT (pad),
      g_quark_from_static_string ("test-mux-pad"));
  GstBuffer *expected_buffer;

  fail_unless (test_data->output_iter);
  fail_unless (GST_IS_BUFFER (test_data->output_iter->data));
  expected_buffer = test_data->output_iter->data;

  fail_unless (GST_BUFFER_PTS (buffer) ==
      (GST_BUFFER_PTS_IS_VALID (expected_buffer) ?
          GST_BUFFER_PTS (expected_buffer) -
          test_data->ts_offset : GST_BUFFER_PTS (expected_buffer)));
  fail_unless (GST_BUFFER_DTS (buffer) ==
      (GST_BUFFER_DTS_IS_VALID (expected_buffer) ?
          GST_BUFFER_DTS (expected_buffer) -
          test_data->ts_offset : GST_BUFFER_DTS (buffer)));
  fail_unless (GST_BUFFER_DURATION (buffer) ==
      GST_BUFFER_DURATION (expected_buffer));

  test_data->output_iter = g_list_next (test_data->output_iter);

  gst_buffer_unref (buffer);
  return GST_FLOW_OK;
}

static void
compare_event (GstEvent * event, GstEvent * expected)
{
  fail_unless (GST_EVENT_TYPE (event) == GST_EVENT_TYPE (expected));
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:{
      GstCaps *caps, *expected_caps;

      gst_event_parse_caps (event, &caps);
      gst_event_parse_caps (expected, &expected_caps);
      fail_unless (gst_caps_can_intersect (caps, expected_caps));
    }
      break;
    default:
      break;
  }
}

static gboolean
_test_sink_pad_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  struct TestInputData *test_data = g_object_get_qdata (G_OBJECT (pad),
      g_quark_from_static_string ("test-mux-pad"));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_STREAM_START:
    case GST_EVENT_CAPS:
    case GST_EVENT_EOS:
      fail_unless (test_data->output_iter);
      fail_unless (GST_IS_EVENT (test_data->output_iter->data));
      compare_event (event, test_data->output_iter->data);
      test_data->output_iter = g_list_next (test_data->output_iter);
      break;
    case GST_EVENT_SEGMENT:{
      const GstSegment *segment;

      fail_unless (test_data->output_iter);
      fail_unless (GST_IS_EVENT (test_data->output_iter->data));
      gst_event_parse_segment (event, &segment);
      fail_unless (segment->start == test_input_pop_segment_start (test_data));
      test_data->output_iter = g_list_next (test_data->output_iter);
      break;
    }
    case GST_EVENT_GAP:{
      GstClockTime timestamp;
      GstClockTime duration;
      gst_event_parse_gap (event, &timestamp, &duration);
      fail_unless (timestamp == test_data->expected_gap_ts);
      fail_unless (duration == test_data->expected_gap_duration);
      test_data->gap_received = TRUE;
      break;
    }
    case GST_EVENT_TAG:
      /* ignore this event */
      break;
    default:
      GST_ERROR_OBJECT (pad, "Unexpected event: %" GST_PTR_FORMAT, event);
      fail ("Unexpected event received %s", GST_EVENT_TYPE_NAME (event));
      break;
  }

  gst_event_unref (event);
  return TRUE;
}

static void
_test_pad_added_cb (GstElement * element, GstPad * pad, gpointer udata)
{
  GstCaps *caps;
  struct TestInputData **inputs = udata;
  gint i = -1;
  const gchar *name;
  const gchar *strname;

  caps = gst_pad_get_current_caps (pad);
  strname = gst_structure_get_name (gst_caps_get_structure (caps, 0));
  if (g_str_has_prefix (strname, "video/")) {
    i = 0;                      /* video is 0, audio is 1 */
    name = "videosink";
  } else {
    i = 1;
    name = "audiosink";
  }
  gst_caps_unref (caps);

  fail_unless (i != -1);
  fail_unless (inputs[i]->sinkpad == NULL);
  inputs[i]->sinkpad = gst_pad_new (name, GST_PAD_SINK);
  inputs[i]->output_iter = inputs[i]->input;
  g_object_set_qdata (G_OBJECT (inputs[i]->sinkpad),
      g_quark_from_static_string ("test-mux-pad"), inputs[i]);
  gst_pad_set_chain_function (inputs[i]->sinkpad, _test_sink_pad_chain);
  gst_pad_set_event_function (inputs[i]->sinkpad, _test_sink_pad_event);
  gst_pad_set_active (inputs[i]->sinkpad, TRUE);
  fail_unless (gst_pad_link (pad, inputs[i]->sinkpad) == GST_PAD_LINK_OK);
}

static void
check_output (const gchar * location, struct TestInputData *input1,
    struct TestInputData *input2)
{
  GstElement *filesrc;
  GstElement *demux;
  struct TestInputData *inputs[2] = { input1, input2 };

  filesrc = gst_element_factory_make ("filesrc", NULL);
  demux = gst_element_factory_make ("qtdemux", NULL);

  fail_unless (gst_element_link (filesrc, demux));

  g_object_set (filesrc, "location", location, NULL);
  g_signal_connect (demux, "pad-added", (GCallback) _test_pad_added_cb, inputs);

  fail_unless (gst_element_set_state (demux,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS);
  fail_unless (gst_element_set_state (filesrc,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS);

  /* FIXME use a main loop */
  g_usleep (2 * G_USEC_PER_SEC);

  fail_unless (gst_element_set_state (demux,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);
  fail_unless (gst_element_set_state (filesrc,
          GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);
  gst_object_unref (filesrc);
  gst_object_unref (demux);
}

/* Muxes a file with qtmux using the inputs provided and
 * then verifies that the generated file corresponds to the
 * data in the inputs */
static void
run_muxing_test (struct TestInputData *input1, struct TestInputData *input2)
{
  gchar *location;
  GstElement *qtmux;
  GstElement *filesink;

  location = g_strdup_printf ("%s/%s-%d", g_get_tmp_dir (), "qtmuxtest",
      g_random_int ());
  qtmux = gst_check_setup_element ("qtmux");
  filesink = gst_element_factory_make ("filesink", NULL);
  g_object_set (filesink, "location", location, NULL);
  gst_element_link (qtmux, filesink);

  input1->srcpad = setup_src_pad (qtmux, &srcvideorawtemplate, "video_%u");
  fail_unless (input1->srcpad != NULL);
  gst_pad_set_active (input1->srcpad, TRUE);

  input2->srcpad = setup_src_pad (qtmux, &srcaudioaactemplate, "audio_%u");
  fail_unless (input2->srcpad != NULL);
  gst_pad_set_active (input2->srcpad, TRUE);

  fail_unless (gst_element_set_state (filesink,
          GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE,
      "could not set filesink to playing");
  fail_unless (gst_element_set_state (qtmux,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");

  input1->thread =
      g_thread_new ("test-push-data-1", test_input_push_data, input1);
  input2->thread =
      g_thread_new ("test-push-data-2", test_input_push_data, input2);

  /* FIXME set a mainloop and wait for EOS */

  g_thread_join (input1->thread);
  g_thread_join (input2->thread);
  input1->thread = NULL;
  input2->thread = NULL;

  gst_element_set_state (qtmux, GST_STATE_NULL);
  gst_element_set_state (filesink, GST_STATE_NULL);

  check_output (location, input1, input2);

  gst_object_unref (filesink);
  test_input_data_clean (input1);
  test_input_data_clean (input2);
  gst_check_teardown_element (qtmux);

  /* delete file */
  g_unlink (location);
  g_free (location);
}

GST_START_TEST (test_muxing)
{
  struct TestInputData input1, input2;
  GstCaps *caps;

  test_input_data_init (&input1);
  test_input_data_init (&input2);

  /* Create the inputs, after calling the run below, all this data is
   * transferred to it and we have no need to clean up */
  input1.input = NULL;
  input1.input =
      g_list_append (input1.input, gst_event_new_stream_start ("test-1"));
  caps = gst_caps_from_string
      ("video/x-raw, width=(int)800, height=(int)600, "
      "framerate=(fraction)1/1, format=(string)RGB");
  input1.input = g_list_append (input1.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input1.segment, GST_FORMAT_TIME);
  input1.input =
      g_list_append (input1.input, gst_event_new_segment (&input1.segment));
  input1.input =
      g_list_append (input1.input, create_buffer (0, GST_CLOCK_TIME_NONE,
          GST_SECOND, 800 * 600 * 3));
  input1.input =
      g_list_append (input1.input, create_buffer (1 * GST_SECOND,
          GST_CLOCK_TIME_NONE, GST_SECOND, 800 * 600 * 3));
  input1.input =
      g_list_append (input1.input, create_buffer (2 * GST_SECOND,
          GST_CLOCK_TIME_NONE, GST_SECOND, 800 * 600 * 3));
  input1.input = g_list_append (input1.input, gst_event_new_eos ());

  input2.input = NULL;
  input2.input =
      g_list_append (input2.input, gst_event_new_stream_start ("test-2"));
  caps = gst_caps_from_string (AUDIO_AAC_CAPS_STRING);
  input2.input = g_list_append (input2.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input2.segment, GST_FORMAT_TIME);
  input2.input =
      g_list_append (input2.input, gst_event_new_segment (&input2.segment));
  input2.input =
      g_list_append (input2.input, create_buffer (0, 0, GST_SECOND, 4096));
  input2.input =
      g_list_append (input2.input, create_buffer (1 * GST_SECOND,
          1 * GST_SECOND, GST_SECOND, 4096));
  input2.input =
      g_list_append (input2.input, create_buffer (2 * GST_SECOND,
          2 * GST_SECOND, GST_SECOND, 4096));
  input2.input = g_list_append (input2.input, gst_event_new_eos ());

  run_muxing_test (&input1, &input2);
}

GST_END_TEST;


GST_START_TEST (test_muxing_non_zero_segment)
{
  struct TestInputData input1, input2;
  GstCaps *caps;

  test_input_data_init (&input1);
  test_input_data_init (&input2);

  /* Create the inputs, after calling the run below, all this data is
   * transferred to it and we have no need to clean up */
  input1.input = NULL;
  input1.input =
      g_list_append (input1.input, gst_event_new_stream_start ("test-1"));
  caps = gst_caps_from_string
      ("video/x-raw, width=(int)800, height=(int)600, "
      "framerate=(fraction)1/1, format=(string)RGB");
  input1.input = g_list_append (input1.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input1.segment, GST_FORMAT_TIME);
  input1.segment.start = 10 * GST_SECOND;
  input1.input =
      g_list_append (input1.input, gst_event_new_segment (&input1.segment));
  input1.input =
      g_list_append (input1.input, create_buffer (10 * GST_SECOND,
          GST_CLOCK_TIME_NONE, GST_SECOND, 800 * 600 * 3));
  input1.input =
      g_list_append (input1.input, create_buffer (11 * GST_SECOND,
          GST_CLOCK_TIME_NONE, GST_SECOND, 800 * 600 * 3));
  input1.input =
      g_list_append (input1.input, create_buffer (12 * GST_SECOND,
          GST_CLOCK_TIME_NONE, GST_SECOND, 800 * 600 * 3));
  input1.input = g_list_append (input1.input, gst_event_new_eos ());
  input1.ts_offset = GST_SECOND * 10;

  input2.input = NULL;
  input2.input =
      g_list_append (input2.input, gst_event_new_stream_start ("test-2"));
  caps = gst_caps_from_string (AUDIO_AAC_CAPS_STRING);
  input2.input = g_list_append (input2.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input2.segment, GST_FORMAT_TIME);
  input2.segment.start = 10 * GST_SECOND;
  input2.input =
      g_list_append (input2.input, gst_event_new_segment (&input2.segment));
  input2.input =
      g_list_append (input2.input, create_buffer (10 * GST_SECOND,
          10 * GST_SECOND, GST_SECOND, 4096));
  input2.input =
      g_list_append (input2.input, create_buffer (11 * GST_SECOND,
          11 * GST_SECOND, GST_SECOND, 4096));
  input2.input =
      g_list_append (input2.input, create_buffer (12 * GST_SECOND,
          12 * GST_SECOND, GST_SECOND, 4096));
  input2.input = g_list_append (input2.input, gst_event_new_eos ());
  input2.ts_offset = GST_SECOND * 10;

  run_muxing_test (&input1, &input2);
}

GST_END_TEST;


GST_START_TEST (test_muxing_non_zero_segment_different)
{
  struct TestInputData input1, input2;
  GstCaps *caps;

  test_input_data_init (&input1);
  test_input_data_init (&input2);

  /* Create the inputs, after calling the run below, all this data is
   * transferred to it and we have no need to clean up */
  input1.input = NULL;
  input1.input =
      g_list_append (input1.input, gst_event_new_stream_start ("test-1"));
  caps = gst_caps_from_string
      ("video/x-raw, width=(int)800, height=(int)600, "
      "framerate=(fraction)1/1, format=(string)RGB");
  input1.input = g_list_append (input1.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input1.segment, GST_FORMAT_TIME);
  input1.segment.start = 5 * GST_SECOND;
  input1.input =
      g_list_append (input1.input, gst_event_new_segment (&input1.segment));
  input1.input =
      g_list_append (input1.input, create_buffer (5 * GST_SECOND,
          GST_CLOCK_TIME_NONE, GST_SECOND, 800 * 600 * 3));
  input1.input =
      g_list_append (input1.input, create_buffer (6 * GST_SECOND,
          GST_CLOCK_TIME_NONE, GST_SECOND, 800 * 600 * 3));
  input1.input =
      g_list_append (input1.input, create_buffer (7 * GST_SECOND,
          GST_CLOCK_TIME_NONE, GST_SECOND, 800 * 600 * 3));
  input1.input = g_list_append (input1.input, gst_event_new_eos ());
  input1.ts_offset = GST_SECOND * 5;

  input2.input = NULL;
  input2.input =
      g_list_append (input2.input, gst_event_new_stream_start ("test-2"));
  caps = gst_caps_from_string (AUDIO_AAC_CAPS_STRING);
  input2.input = g_list_append (input2.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input2.segment, GST_FORMAT_TIME);
  input2.segment.start = 10 * GST_SECOND;
  input2.input =
      g_list_append (input2.input, gst_event_new_segment (&input2.segment));
  input2.input =
      g_list_append (input2.input, create_buffer (10 * GST_SECOND,
          10 * GST_SECOND, GST_SECOND, 4096));
  input2.input =
      g_list_append (input2.input, create_buffer (11 * GST_SECOND,
          11 * GST_SECOND, GST_SECOND, 4096));
  input2.input =
      g_list_append (input2.input, create_buffer (12 * GST_SECOND,
          12 * GST_SECOND, GST_SECOND, 4096));
  input2.input = g_list_append (input2.input, gst_event_new_eos ());
  input2.ts_offset = GST_SECOND * 10;

  run_muxing_test (&input1, &input2);
}

GST_END_TEST;

GST_START_TEST (test_muxing_dts_outside_segment)
{
  struct TestInputData input1, input2;
  GstCaps *caps;

  test_input_data_init (&input1);
  test_input_data_init (&input2);

  /* Create the inputs, after calling the run below, all this data is
   * transferred to it and we have no need to clean up */
  input1.input = NULL;
  input1.input =
      g_list_append (input1.input, gst_event_new_stream_start ("test-1"));
  caps = gst_caps_from_string
      ("video/x-h264, width=(int)800, height=(int)600, "
      "framerate=(fraction)1/1, stream-format=(string)avc, codec_data=(buffer)0000,"
      " alignment=(string)au, level=(int)2, profile=(string)high");
  input1.input = g_list_append (input1.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input1.segment, GST_FORMAT_TIME);
  input1.segment.start = 1 * GST_SECOND;
  input1.input =
      g_list_append (input1.input, gst_event_new_segment (&input1.segment));
  input1.input =
      g_list_append (input1.input, create_buffer (1 * GST_SECOND,
          0, GST_SECOND, 4096));
  input1.input =
      g_list_append (input1.input, create_buffer (2 * GST_SECOND,
          1 * GST_SECOND, GST_SECOND, 4096));
  input1.input =
      g_list_append (input1.input, create_buffer (3 * GST_SECOND,
          2 * GST_SECOND, GST_SECOND, 4096));
  input1.input = g_list_append (input1.input, gst_event_new_eos ());
  /* First DTS is 0, first PTS is 1s. The segment start being 1, this means
   * running time -1s and 0. So the output segment should start from 1s to keep
   * the same running time */
  test_input_pop_segment_start (&input1);
  test_input_push_segment_start (&input1, GST_SECOND);

  input2.input = NULL;
  input2.input =
      g_list_append (input2.input, gst_event_new_stream_start ("test-2"));
  caps = gst_caps_from_string (AUDIO_AAC_CAPS_STRING);
  input2.input = g_list_append (input2.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input2.segment, GST_FORMAT_TIME);
  input2.input =
      g_list_append (input2.input, gst_event_new_segment (&input2.segment));
  input2.input =
      g_list_append (input2.input, create_buffer (0, 0, GST_SECOND,
          44100 * 4 * 2));
  input2.input =
      g_list_append (input2.input, create_buffer (GST_SECOND, GST_SECOND,
          GST_SECOND, 44100 * 4 * 2));
  input2.input =
      g_list_append (input2.input, create_buffer (2 * GST_SECOND,
          2 * GST_SECOND, GST_SECOND, 44100 * 4 * 2));
  input2.input = g_list_append (input2.input, gst_event_new_eos ());

  run_muxing_test (&input1, &input2);
}

GST_END_TEST;

GST_START_TEST (test_muxing_initial_gap)
{
  struct TestInputData input1, input2;
  GstCaps *caps;

  test_input_data_init (&input1);
  test_input_data_init (&input2);

  /* Create the inputs, after calling the run below, all this data is
   * transferred to it and we have no need to clean up */
  input1.input = NULL;
  input1.input =
      g_list_append (input1.input, gst_event_new_stream_start ("test-1"));
  caps = gst_caps_from_string
      ("video/x-h264, width=(int)800, height=(int)600, "
      "framerate=(fraction)1/1, stream-format=(string)avc, codec_data=(buffer)0000,"
      " alignment=(string)au, level=(int)2, profile=(string)high");
  input1.input = g_list_append (input1.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input1.segment, GST_FORMAT_TIME);
  input1.input =
      g_list_append (input1.input, gst_event_new_segment (&input1.segment));
  /* Duplicate the segment to please the harness */
  input1.input =
      g_list_append (input1.input, gst_event_new_segment (&input1.segment));
  input1.input =
      g_list_append (input1.input, create_buffer (1 * GST_SECOND,
          0, GST_SECOND, 4096));
  input1.input =
      g_list_append (input1.input, create_buffer (2 * GST_SECOND,
          1 * GST_SECOND, GST_SECOND, 4096));
  input1.input =
      g_list_append (input1.input, create_buffer (3 * GST_SECOND,
          2 * GST_SECOND, GST_SECOND, 4096));
  input1.input = g_list_append (input1.input, gst_event_new_eos ());

  /* We expect a 1s gap at the start */
  input1.expected_gap_duration = GST_SECOND;
  /* There will be two segments, first is 0, so leave it there, second should
   * match the first CTTS (PTS - DTS) */
  test_input_push_segment_start (&input1, GST_SECOND);

  input2.input = NULL;
  input2.input =
      g_list_append (input2.input, gst_event_new_stream_start ("test-2"));
  caps = gst_caps_from_string (AUDIO_AAC_CAPS_STRING);
  input2.input = g_list_append (input2.input, gst_event_new_caps (caps));
  gst_caps_unref (caps);
  gst_segment_init (&input2.segment, GST_FORMAT_TIME);
  input2.input =
      g_list_append (input2.input, gst_event_new_segment (&input2.segment));
  input2.input =
      g_list_append (input2.input, create_buffer (0, 0, GST_SECOND,
          44100 * 4 * 2));
  input2.input =
      g_list_append (input2.input, create_buffer (GST_SECOND, GST_SECOND,
          GST_SECOND, 44100 * 4 * 2));
  input2.input =
      g_list_append (input2.input, create_buffer (2 * GST_SECOND,
          2 * GST_SECOND, GST_SECOND, 44100 * 4 * 2));
  input2.input = g_list_append (input2.input, gst_event_new_eos ());

  run_muxing_test (&input1, &input2);

  fail_unless (input1.gap_received);
}

GST_END_TEST;

static Suite *
qtmux_suite (void)
{
  Suite *s = suite_create ("qtmux");
  TCase *tc_chain = tcase_create ("general");

  /* avoid glib warnings when setting deprecated dts-method property */
  g_setenv ("G_ENABLE_DIAGNOSTIC", "0", TRUE);

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_video_pad_dd);
  tcase_add_test (tc_chain, test_audio_pad_dd);
  tcase_add_test (tc_chain, test_video_pad_frag_dd);
  tcase_add_test (tc_chain, test_audio_pad_frag_dd);
  tcase_add_test (tc_chain, test_video_pad_frag_dd_streamable);
  tcase_add_test (tc_chain, test_audio_pad_frag_dd_streamable);

  tcase_add_test (tc_chain, test_video_pad_reorder);
  tcase_add_test (tc_chain, test_audio_pad_reorder);
  tcase_add_test (tc_chain, test_video_pad_frag_reorder);
  tcase_add_test (tc_chain, test_audio_pad_frag_reorder);
  tcase_add_test (tc_chain, test_video_pad_frag_reorder_streamable);
  tcase_add_test (tc_chain, test_audio_pad_frag_reorder_streamable);

  tcase_add_test (tc_chain, test_video_pad_asc);
  tcase_add_test (tc_chain, test_audio_pad_asc);
  tcase_add_test (tc_chain, test_video_pad_frag_asc);
  tcase_add_test (tc_chain, test_audio_pad_frag_asc);
  tcase_add_test (tc_chain, test_video_pad_frag_asc_streamable);
  tcase_add_test (tc_chain, test_audio_pad_frag_asc_streamable);

  tcase_add_test (tc_chain, test_average_bitrate);

  tcase_add_test (tc_chain, test_reuse);
  tcase_add_test (tc_chain, test_encodebin_qtmux);
  tcase_add_test (tc_chain, test_encodebin_mp4mux);

  tcase_add_test (tc_chain, test_muxing);
  tcase_add_test (tc_chain, test_muxing_non_zero_segment);
  tcase_add_test (tc_chain, test_muxing_non_zero_segment_different);
  tcase_add_test (tc_chain, test_muxing_dts_outside_segment);
  tcase_add_test (tc_chain, test_muxing_initial_gap);

  return s;
}

GST_CHECK_MAIN (qtmux)
