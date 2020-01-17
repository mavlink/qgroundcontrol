/* GStreamer
 *
 * unit test for mpg123audiodec
 *
 * Copyright (c) 2012 Carlos Rafael Giani <dv@pseudoterminal.org>
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

#include <unistd.h>

#include <gst/check/gstcheck.h>
#include <gst/audio/audio.h>

#include <gst/fft/gstfft.h>
#include <gst/fft/gstffts16.h>
#include <gst/fft/gstffts32.h>
#include <gst/fft/gstfftf32.h>
#include <gst/fft/gstfftf64.h>

#include <gst/app/gstappsink.h>

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;


#define MP2_STREAM_FILENAME "stream.mp2"
#define MP3_CBR_STREAM_FILENAME "cbr_stream.mp3"
#define MP3_VBR_STREAM_FILENAME "vbr_stream.mp3"


/* mpeg 1 layer 2 stream created with:
 * gst-launch-1.0 -v audiotestsrc wave=sine freq=440 volume=1 num-buffers=32 ! \
 *   "audio/x-raw, format=(string)S16LE, layout=(string)interleaved, rate=(int)44100, channels=(int)1" ! \
 *   avenc_mp2 bitrate=32000 ! tee name=t \
 *   t. ! queue ! fakesink silent=false \
 *   t. ! queue ! filesink location=test.mp2
 *
 * mpeg 1 layer 3 CBR stream created with:
 * gst-launch-1.0 -v audiotestsrc wave=sine freq=440 volume=1 num-buffers=32 ! \
 *   "audio/x-raw, format=(string)S16LE, layout=(string)interleaved, rate=(int)44100, channels=(int)1" ! \
 *   lamemp3enc encoding-engine-quality=high cbr=true target=bitrate bitrate=32 ! \
 *   "audio/mpeg, rate=(int)44100, channels=(int)1" ! tee name=t \
 *   t. ! queue ! fakesink silent=false \
 *   t. ! queue ! filesink location=test.mp3
 *
 * mpeg 1 layer 3 VBR stream created with:
 * gst-launch-1.0 -v audiotestsrc wave=sine freq=440 volume=1 num-buffers=32 ! \
 *   "audio/x-raw, format=(string)S16LE, layout=(string)interleaved, rate=(int)44100, channels=(int)1" ! \
 *   lamemp3enc encoding-engine-quality=high cbr=false target=quality quality=7 ! \
 *   "audio/mpeg, rate=(int)44100, channels=(int)1" ! tee name=t \
 *   t. ! queue ! fakesink silent=false \
 *   t. ! queue ! filesink location=test.mp3
 */


/* FFT test helpers taken from gst-plugins-base tests/check/audioresample.c */

#define FFT_HELPERS(type,ffttag,ffttag2,scale)                                \
static gdouble magnitude##ffttag (const GstFFT##ffttag##Complex *c)           \
{                                                                             \
  gdouble mag = (gdouble) c->r * (gdouble) c->r;                              \
  mag += (gdouble) c->i * (gdouble) c->i;                                     \
  mag /= scale * scale;                                                       \
  mag = 10.0 * log10 (mag);                                                   \
  return mag;                                                                 \
}                                                                             \
static gdouble find_main_frequency_spot_##ffttag (                            \
    const GstFFT##ffttag##Complex *v, int elements)                           \
{                                                                             \
  int i;                                                                      \
  gdouble maxmag = -9999;                                                     \
  int maxidx = 0;                                                             \
  for (i=0; i<elements; ++i) {                                                \
    gdouble mag = magnitude##ffttag (v+i);                                    \
    if (mag > maxmag) {                                                       \
      maxmag = mag;                                                           \
      maxidx = i;                                                             \
    }                                                                         \
  }                                                                           \
  return maxidx / (gdouble) elements;                                         \
}                                                                             \
static gboolean is_zero_except_##ffttag (const GstFFT##ffttag##Complex *v,    \
    int elements, gdouble spot)                                               \
{                                                                             \
  int i;                                                                      \
  for (i=0; i<elements; ++i) {                                                \
    gdouble pos = i / (gdouble) elements;                                     \
    gdouble mag = magnitude##ffttag (v+i);                                    \
    if (fabs (pos - spot) > 0.01) {                                           \
      if (mag > -35.0) {                                                      \
        GST_LOG("Found magnitude at %f : %f (peak at %f)\n", pos, mag, spot); \
        return FALSE;                                                         \
      }                                                                       \
    }                                                                         \
  }                                                                           \
  return TRUE;                                                                \
}                                                                             \
static void check_main_frequency_spot_##ffttag (GstBuffer *buffer, gdouble    \
    expected_spot)                                                            \
{                                                                             \
  GstMapInfo map;                                                             \
  int num_samples;                                                            \
  gdouble actual_spot;                                                        \
  GstFFT##ffttag *ctx;                                                        \
  GstFFT##ffttag##Complex *fftdata;                                           \
                                                                              \
  gst_buffer_map (buffer, &map, GST_MAP_READ);                                \
                                                                              \
  num_samples = map.size / sizeof(type) & ~1;                                 \
  ctx = gst_fft_##ffttag2##_new (num_samples, FALSE);                         \
  fftdata = g_new (GstFFT##ffttag##Complex, num_samples / 2 + 1);             \
                                                                              \
  gst_fft_##ffttag2##_window (ctx, (type*)map.data,                           \
    GST_FFT_WINDOW_HAMMING);                                                  \
  gst_fft_##ffttag2##_fft (ctx, (type*)map.data, fftdata);                    \
                                                                              \
  actual_spot = find_main_frequency_spot_##ffttag (fftdata,                   \
    num_samples / 2 + 1);                                                     \
  GST_LOG ("Expected spot: %.3f actual: %.3f %f", expected_spot, actual_spot, \
    fabs (expected_spot - actual_spot));                                      \
  fail_unless (fabs (expected_spot - actual_spot) < 0.05,                     \
    "Actual main frequency spot is too far away from expected one");          \
  fail_unless (is_zero_except_##ffttag (fftdata, num_samples / 2 + 1,         \
    actual_spot), "One secondary peak in spectrum exceeds threshold");        \
                                                                              \
  gst_buffer_unmap (buffer, &map);                                            \
                                                                              \
  gst_fft_##ffttag2##_free (ctx);                                             \
  g_free (fftdata);                                                           \
}
FFT_HELPERS (gint32, S32, s32, 2147483647.0);


static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, format = (string) " GST_AUDIO_NE (S32))
    );
static GstStaticPadTemplate layer2_srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);
static GstStaticPadTemplate layer3_srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);


static void
setup_input_pipeline (gchar const *stream_filename, GstElement ** pipeline,
    GstElement ** appsink)
{
  GstElement *source, *parser;

  *pipeline = gst_pipeline_new (NULL);
  source = gst_element_factory_make ("filesrc", NULL);
  parser = gst_element_factory_make ("mpegaudioparse", NULL);
  *appsink = gst_element_factory_make ("appsink", NULL);

  gst_bin_add_many (GST_BIN (*pipeline), source, parser, *appsink, NULL);
  gst_element_link_many (source, parser, *appsink, NULL);

  {
    char *full_filename =
        g_build_filename (GST_TEST_FILES_PATH, stream_filename, NULL);
    g_object_set (G_OBJECT (source), "location", full_filename, NULL);
    g_free (full_filename);
  }

  gst_element_set_state (*pipeline, GST_STATE_PLAYING);
}

static void
cleanup_input_pipeline (GstElement * pipeline)
{
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);
}

static GstElement *
setup_mpeg1layer2dec (void)
{
  GstElement *mpg123audiodec;
  GstCaps *caps;

  GST_DEBUG ("setup_mpeg1layer2dec");
  mpg123audiodec = gst_check_setup_element ("mpg123audiodec");
  mysrcpad = gst_check_setup_src_pad (mpg123audiodec, &layer2_srctemplate);
  mysinkpad = gst_check_setup_sink_pad (mpg123audiodec, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  /* This is necessary to trigger a set_format call in the decoder;
   * fixed caps don't trigger it */
  caps = gst_caps_new_simple ("audio/mpeg",
      "mpegversion", G_TYPE_INT, 1,
      "layer", G_TYPE_INT, 2,
      "rate", G_TYPE_INT, 44100,
      "channels", G_TYPE_INT, 1, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
  gst_check_setup_events (mysrcpad, mpg123audiodec, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  return mpg123audiodec;
}

static GstElement *
setup_mpeg1layer3dec (void)
{
  GstElement *mpg123audiodec;
  GstCaps *caps;

  GST_DEBUG ("setup_mpeg1layer3dec");
  mpg123audiodec = gst_check_setup_element ("mpg123audiodec");
  mysrcpad = gst_check_setup_src_pad (mpg123audiodec, &layer3_srctemplate);
  mysinkpad = gst_check_setup_sink_pad (mpg123audiodec, &sinktemplate);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  /* This is necessary to trigger a set_format call in the decoder;
   * fixed caps don't trigger it */
  caps = gst_caps_new_simple ("audio/mpeg",
      "mpegversion", G_TYPE_INT, 1,
      "layer", G_TYPE_INT, 3,
      "rate", G_TYPE_INT, 44100,
      "channels", G_TYPE_INT, 1, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
  gst_check_setup_events (mysrcpad, mpg123audiodec, caps, GST_FORMAT_TIME);
  gst_caps_unref (caps);

  return mpg123audiodec;
}

static void
cleanup_mpg123audiodec (GstElement * mpg123audiodec)
{
  GST_DEBUG ("cleanup_mpeg1layer2dec");
  gst_element_set_state (mpg123audiodec, GST_STATE_NULL);

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (mpg123audiodec);
  gst_check_teardown_sink_pad (mpg123audiodec);
  gst_check_teardown_element (mpg123audiodec);
}

static void
run_decoding_test (GstElement * mpg123audiodec, gchar const *filename)
{
  GstBus *bus;
  unsigned int num_input_buffers, num_decoded_buffers;
  gint expected_size;
  GstCaps *out_caps, *caps;
  GstAudioInfo audioinfo;
  GstElement *input_pipeline, *input_appsink;
  int i;
  GstBuffer *outbuffer;

  /* 440 Hz = frequency of sine wave in audio data
   * 44100 Hz = sample rate
   * (44100 / 2) Hz = Nyquist frequency */
  static double const expected_frequency_spot = 440.0 / (44100.0 / 2.0);

  fail_unless (gst_element_set_state (mpg123audiodec,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");
  bus = gst_bus_new ();

  gst_element_set_bus (mpg123audiodec, bus);

  setup_input_pipeline (filename, &input_pipeline, &input_appsink);

  num_input_buffers = 0;
  while (TRUE) {
    GstSample *sample;
    GstBuffer *input_buffer;

    sample = gst_app_sink_pull_sample (GST_APP_SINK (input_appsink));
    if (sample == NULL)
      break;

    fail_unless (GST_IS_SAMPLE (sample));

    input_buffer = gst_sample_get_buffer (sample);
    fail_if (input_buffer == NULL);

    /* This is done to be on the safe side - docs say lifetime of the input buffer
     * depends *solely* on the sample */
    input_buffer = gst_buffer_copy (input_buffer);

    fail_unless_equals_int (gst_pad_push (mysrcpad, input_buffer), GST_FLOW_OK);

    ++num_input_buffers;

    gst_sample_unref (sample);
  }

  num_decoded_buffers = g_list_length (buffers);

  /* check number of decoded buffers */
  fail_unless_equals_int (num_decoded_buffers, num_input_buffers - 2);

  caps = gst_pad_get_current_caps (mysinkpad);
  GST_LOG ("output caps %" GST_PTR_FORMAT, caps);
  fail_unless (gst_audio_info_from_caps (&audioinfo, caps),
      "Getting audio info from caps failed");

  /* check caps */
  out_caps = gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, GST_AUDIO_NE (S32),
      "layout", G_TYPE_STRING, "interleaved",
      "rate", G_TYPE_INT, 44100, "channels", G_TYPE_INT, 1, NULL);

  fail_unless (gst_caps_is_equal_fixed (caps, out_caps), "Incorrect out caps");

  gst_caps_unref (out_caps);
  gst_caps_unref (caps);

  /* here, test if decoded data is a sine tone, and if the sine frequency is at the
   * right spot in the spectrum */
  for (i = 0; i < num_decoded_buffers; ++i) {
    outbuffer = GST_BUFFER (buffers->data);
    fail_if (outbuffer == NULL, "Invalid buffer retrieved");

    /* MPEG 1 layer 2 uses 1152 samples per frame */
    expected_size = 1152 * GST_AUDIO_INFO_BPF (&audioinfo);
    fail_unless_equals_int (gst_buffer_get_size (outbuffer), expected_size);

    check_main_frequency_spot_S32 (outbuffer, expected_frequency_spot);

    buffers = g_list_remove (buffers, outbuffer);
    gst_buffer_unref (outbuffer);
    outbuffer = NULL;
  }

  g_list_free (buffers);
  buffers = NULL;

  cleanup_input_pipeline (input_pipeline);
  gst_bus_set_flushing (bus, TRUE);
  gst_element_set_bus (mpg123audiodec, NULL);
  gst_object_unref (GST_OBJECT (bus));
}


GST_START_TEST (test_decode_mpeg1layer2)
{
  GstElement *mpg123audiodec;
  mpg123audiodec = setup_mpeg1layer2dec ();
  run_decoding_test (mpg123audiodec, MP2_STREAM_FILENAME);
  cleanup_mpg123audiodec (mpg123audiodec);
  mpg123audiodec = NULL;
}

GST_END_TEST;


GST_START_TEST (test_decode_mpeg1layer3_cbr)
{
  GstElement *mpg123audiodec;
  mpg123audiodec = setup_mpeg1layer3dec ();
  run_decoding_test (mpg123audiodec, MP3_CBR_STREAM_FILENAME);
  cleanup_mpg123audiodec (mpg123audiodec);
}

GST_END_TEST;


GST_START_TEST (test_decode_mpeg1layer3_vbr)
{
  GstElement *mpg123audiodec;
  mpg123audiodec = setup_mpeg1layer3dec ();
  run_decoding_test (mpg123audiodec, MP3_VBR_STREAM_FILENAME);
  cleanup_mpg123audiodec (mpg123audiodec);
}

GST_END_TEST;


GST_START_TEST (test_decode_garbage_mpeg1layer2)
{
  GstElement *mpg123audiodec;
  GstBuffer *inbuffer;
  GstBus *bus;
  int i, num_buffers;
  guint32 *tmpbuf;

  mpg123audiodec = setup_mpeg1layer2dec ();

  fail_unless (gst_element_set_state (mpg123audiodec,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");
  bus = gst_bus_new ();

  /* initialize the buffer with something that is no mpeg2 */
  tmpbuf = g_new (guint32, 4096);
  for (i = 0; i < 4096; i++) {
    tmpbuf[i] = i;
  }
  inbuffer = gst_buffer_new_wrapped (tmpbuf, 4096 * sizeof (guint32));

  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  gst_element_set_bus (mpg123audiodec, bus);

  /* should be possible to push without problems but nothing gets decoded */
  fail_unless_equals_int (gst_pad_push (mysrcpad, inbuffer), GST_FLOW_OK);

  num_buffers = g_list_length (buffers);

  /* should be 0 buffers as decoding should've been impossible */
  fail_unless_equals_int (num_buffers, 0);

  g_list_free (buffers);
  buffers = NULL;

  gst_bus_set_flushing (bus, TRUE);
  gst_element_set_bus (mpg123audiodec, NULL);
  gst_object_unref (GST_OBJECT (bus));
  cleanup_mpg123audiodec (mpg123audiodec);
  mpg123audiodec = NULL;
}

GST_END_TEST;


GST_START_TEST (test_decode_garbage_mpeg1layer3)
{
  GstElement *mpg123audiodec;
  GstBuffer *inbuffer;
  GstBus *bus;
  int i, num_buffers;
  guint32 *tmpbuf;

  mpg123audiodec = setup_mpeg1layer3dec ();

  fail_unless (gst_element_set_state (mpg123audiodec,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "could not set to playing");
  bus = gst_bus_new ();

  /* initialize the buffer with something that is no mpeg2 */
  tmpbuf = g_new (guint32, 4096);
  for (i = 0; i < 4096; i++) {
    tmpbuf[i] = i;
  }
  inbuffer = gst_buffer_new_wrapped (tmpbuf, 4096 * sizeof (guint32));

  ASSERT_BUFFER_REFCOUNT (inbuffer, "inbuffer", 1);

  gst_element_set_bus (mpg123audiodec, bus);

  /* should be possible to push without problems but nothing gets decoded */
  fail_unless_equals_int (gst_pad_push (mysrcpad, inbuffer), GST_FLOW_OK);

  num_buffers = g_list_length (buffers);

  /* should be 0 buffers as decoding should've been impossible */
  fail_unless_equals_int (num_buffers, 0);

  g_list_free (buffers);
  buffers = NULL;

  gst_bus_set_flushing (bus, TRUE);
  gst_element_set_bus (mpg123audiodec, NULL);
  gst_object_unref (GST_OBJECT (bus));
  cleanup_mpg123audiodec (mpg123audiodec);
  mpg123audiodec = NULL;
}

GST_END_TEST;


static gboolean
is_test_file_available (gchar const *filename)
{
  gboolean ret;
  gchar *full_filename;
  gchar *cwd;

  cwd = g_get_current_dir ();
  full_filename = g_build_filename (cwd, GST_TEST_FILES_PATH, filename, NULL);
  ret =
      g_file_test (full_filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
  g_free (full_filename);
  g_free (cwd);
  return ret;
}

static Suite *
mpg123audiodec_suite (void)
{
  GstRegistry *registry;
  Suite *s = suite_create ("mpg123audiodec");
  TCase *tc_chain = tcase_create ("general");

  registry = gst_registry_get ();

  suite_add_tcase (s, tc_chain);
  if (gst_registry_check_feature_version (registry, "filesrc",
          GST_VERSION_MAJOR, GST_VERSION_MINOR, 0) &&
      gst_registry_check_feature_version (registry, "mpegaudioparse",
          GST_VERSION_MAJOR, GST_VERSION_MINOR, 0) &&
      gst_registry_check_feature_version (registry, "appsrc",
          GST_VERSION_MAJOR, GST_VERSION_MINOR, 0)) {
    if (is_test_file_available (MP2_STREAM_FILENAME))
      tcase_add_test (tc_chain, test_decode_mpeg1layer2);
    if (is_test_file_available (MP3_CBR_STREAM_FILENAME))
      tcase_add_test (tc_chain, test_decode_mpeg1layer3_cbr);
    if (is_test_file_available (MP3_VBR_STREAM_FILENAME))
      tcase_add_test (tc_chain, test_decode_mpeg1layer3_vbr);
  }
  tcase_add_test (tc_chain, test_decode_garbage_mpeg1layer2);
  tcase_add_test (tc_chain, test_decode_garbage_mpeg1layer3);

  return s;
}


GST_CHECK_MAIN (mpg123audiodec)
