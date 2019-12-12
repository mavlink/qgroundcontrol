/* GStreamer interactive test for accurate seeking
 * Copyright (C) 2014 Tim-Philipp Müller <tim centricular com>
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
 * Based on python script by Kibeom Kim <kkb110@gmail.com>
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>

#include <gst/gst.h>
#include <gst/base/base.h>
#include <gst/audio/audio.h>
#include <gst/app/app.h>

#define SAMPLE_FREQ 44100

static const guint8 *
_memmem (const guint8 * haystack, gsize hlen, const guint8 * needle, gsize nlen)
{
  const guint8 *p = haystack;
  int needle_first;
  gsize plen = hlen;

  if (!nlen)
    return NULL;

  needle_first = *(unsigned char *) needle;

  while (plen >= nlen && (p = memchr (p, needle_first, plen - nlen + 1))) {
    if (!memcmp (p, needle, nlen))
      return (guint8 *) p;

    p++;
    plen = hlen - (p - haystack);
  }

  return NULL;
}

static GstClockTime
sample_to_nanotime (guint sample)
{
  return (guint64) ((1.0 * sample * GST_SECOND / SAMPLE_FREQ) + 0.5);
}

static guint
nanotime_to_sample (GstClockTime nanotime)
{
  return gst_util_uint64_scale_round (nanotime, SAMPLE_FREQ, GST_SECOND);
}

static GstBuffer *
generate_test_data (guint N)
{
  gint16 *left, *right, *stereo;
  guint largeN, i, j;

  /* 32767 = (2 ** 15) - 1 */
  /* 32768 = (2 ** 15) */
  largeN = ((N + 32767) / 32768) * 32768;
  left = g_new0 (gint16, largeN);
  right = g_new0 (gint16, largeN);
  stereo = g_new0 (gint16, 2 * largeN);

  for (i = 0; i < (largeN / 32768); ++i) {
    gint c = 0;

    for (j = i * 32768; j < ((i + 1) * 32768); ++j) {
      left[j] = i;

      if (i % 2 == 0) {
        right[j] = c;
      } else {
        right[j] = 32767 - c;
      }
      ++c;
    }
  }

  /* could just fill stereo directly from the start, but keeping original code for now */
  for (i = 0; i < largeN; ++i) {
    stereo[(2 * i) + 0] = left[i];
    stereo[(2 * i) + 1] = right[i];
  }
  g_free (left);
  g_free (right);

  return gst_buffer_new_wrapped (stereo, 2 * largeN * sizeof (gint16));
}

static void
generate_test_sound (const gchar * fn, const gchar * launch_string,
    guint num_samples)
{
  GstElement *pipeline, *src, *parse, *enc_bin, *sink;
  GstFlowReturn flow;
  GstMessage *msg;
  GstBuffer *buf;
  GstCaps *caps;

  pipeline = gst_pipeline_new (NULL);

  src = gst_element_factory_make ("appsrc", NULL);

  caps = gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, GST_AUDIO_NE (S16),
      "rate", G_TYPE_INT, SAMPLE_FREQ, "channels", G_TYPE_INT, 2,
      "layout", G_TYPE_STRING, "interleaved",
      "channel-mask", GST_TYPE_BITMASK, (guint64) 3, NULL);
  g_object_set (src, "caps", caps, "format", GST_FORMAT_TIME, NULL);
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_caps_unref (caps);

  /* audioparse to put proper timestamps on buffers for us, without which
   * vorbisenc in particular is unhappy (or oggmux, rather) */
  parse = gst_element_factory_make ("audioparse", NULL);
  if (parse != NULL) {
    g_object_set (parse, "use-sink-caps", TRUE, NULL);
  } else {
    parse = gst_element_factory_make ("identity", NULL);
    g_warning ("audioparse element not available, vorbis/ogg might not work\n");
  }

  enc_bin = gst_parse_bin_from_description (launch_string, TRUE, NULL);

  sink = gst_element_factory_make ("filesink", NULL);
  g_object_set (sink, "location", fn, NULL);

  gst_bin_add_many (GST_BIN (pipeline), src, parse, enc_bin, sink, NULL);

  gst_element_link_many (src, parse, enc_bin, sink, NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  buf = generate_test_data (num_samples);
  flow = gst_app_src_push_buffer (GST_APP_SRC (src), buf);
  g_assert (flow == GST_FLOW_OK);

  gst_app_src_end_of_stream (GST_APP_SRC (src));

  /*g_print ("generating test sound %s, waiting for EOS..\n", fn); */

  msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipeline),
      GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS | GST_MESSAGE_ERROR);

  g_assert (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS);
  gst_message_unref (msg);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  /* g_print ("Done %s\n", fn); */
}

static void
test_seek_FORMAT_TIME_by_sample (const gchar * fn, GList * seek_positions)
{
  GstElement *pipeline, *src, *sink;
  GstAdapter *adapter;
  GstSample *sample;
  GstCaps *caps;
  gconstpointer answer;
  guint answer_size;

  pipeline = gst_parse_launch ("filesrc name=src ! decodebin ! "
      "audioconvert dithering=0 ! appsink name=sink", NULL);

  src = gst_bin_get_by_name (GST_BIN (pipeline), "src");
  g_object_set (src, "location", fn, NULL);
  gst_object_unref (src);

  sink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");
  caps = gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, GST_AUDIO_NE (S16),
      "rate", G_TYPE_INT, SAMPLE_FREQ, "channels", G_TYPE_INT, 2, NULL);
  g_object_set (sink, "caps", caps, "sync", FALSE, NULL);
  gst_caps_unref (caps);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* wait for preroll, so we can seek */
  gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (pipeline), GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ASYNC_DONE);

  /* first, read entire file to end */
  adapter = gst_adapter_new ();
  while ((sample = gst_app_sink_pull_sample (GST_APP_SINK (sink)))) {
    gst_adapter_push (adapter, gst_buffer_ref (gst_sample_get_buffer (sample)));
    gst_sample_unref (sample);
  }
  answer_size = gst_adapter_available (adapter);
  answer = gst_adapter_map (adapter, answer_size);
  /* g_print ("%s: read %u bytes\n", fn, answer_size); */

  g_print ("%10s\t%10s\t%10s\n", "requested", "sample per ts", "actual(data)");

  while (seek_positions != NULL) {
    gconstpointer found;
    GstMapInfo map;
    GstBuffer *buf;
    gboolean ret;
    guint actual_position, buffer_timestamp_position;
    guint seek_sample;

    seek_sample = GPOINTER_TO_UINT (seek_positions->data);

    ret = gst_element_seek_simple (pipeline, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
        sample_to_nanotime (seek_sample));

    g_assert (ret);

    sample = gst_app_sink_pull_sample (GST_APP_SINK (sink));

    buf = gst_sample_get_buffer (sample);
    gst_buffer_map (buf, &map, GST_MAP_READ);
    GST_MEMDUMP ("answer", answer, answer_size);
    GST_MEMDUMP ("buffer", map.data, map.size);
    found = _memmem (answer, answer_size, map.data, map.size);
    gst_buffer_unmap (buf, &map);

    g_assert (found != NULL);
    actual_position = ((goffset) ((guint8 *) found - (guint8 *) answer)) / 4;
    buffer_timestamp_position = nanotime_to_sample (GST_BUFFER_PTS (buf));
    g_print ("%10u\t%10u\t%10u\n", seek_sample, buffer_timestamp_position,
        actual_position);
    gst_sample_unref (sample);

    seek_positions = seek_positions->next;
  }

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (sink);
  gst_object_unref (pipeline);
  g_object_unref (adapter);
}

static GList *
create_test_samples (guint from, guint to, guint step)
{
  GQueue q = G_QUEUE_INIT;
  guint i;

  for (i = from; i < to; i += step)
    g_queue_push_tail (&q, GUINT_TO_POINTER (i));

  return q.head;
}

#define SECS 10

int
main (int argc, char **argv)
{
  GList *test_samples;

  gst_init (&argc, &argv);

  test_samples = create_test_samples (SAMPLE_FREQ, SAMPLE_FREQ * 2, 5000);

  g_print ("\nwav:\n");
  generate_test_sound ("test.wav", "wavenc", SAMPLE_FREQ * SECS);
  test_seek_FORMAT_TIME_by_sample ("test.wav", test_samples);

  g_print ("\nflac:\n");
  generate_test_sound ("test.flac", "flacenc", SAMPLE_FREQ * SECS);
  test_seek_FORMAT_TIME_by_sample ("test.flac", test_samples);

  g_print ("\nogg:\n");
  generate_test_sound ("test.ogg",
      "audioconvert dithering=0 ! vorbisenc quality=1 ! oggmux",
      SAMPLE_FREQ * SECS);
  test_seek_FORMAT_TIME_by_sample ("test.ogg", test_samples);

  g_print ("\nmp3:\n");
  generate_test_sound ("test.mp3", "lamemp3enc bitrate=320",
      SAMPLE_FREQ * SECS);
  test_seek_FORMAT_TIME_by_sample ("test.mp3", test_samples);

  g_list_free (test_samples);
  return 0;
}
