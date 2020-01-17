/* GStreamer ReplayGain analysis
 *
 * Copyright (C) 2006 Rene Stadler <mail@renestadler.de>
 *
 * rganalysis.c: Unit test for the rganalysis element
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

/* Some things to note about the RMS window length of the analysis algorithm and
 * thus the implementation used in the element: Processing divides input data
 * into 50ms windows at some point.  Some details about this that normally do
 * not matter:
 *
 *  1. At the end of a stream, the remainder of data that did not fill up the
 *     last 50ms window is simply discarded.
 *
 *  2. If the sample rate changes during a stream, the currently running window
 *     is discarded and the equal loudness filter gets reset as if a new stream
 *     started.
 *
 *  3. For the album gain, it is not entirely correct to think of obtaining it
 *     like "as if all the tracks are analyzed as one track".  There isn't a
 *     separate window being tracked for album processing, so at stream (track)
 *     end, the remaining unfilled window does not contribute to the album gain
 *     either.
 *
 *  4. If a waveform with a result gain G is concatenated to itself and the
 *     result processed as a track, the gain can be different from G if and only
 *     if the duration of the original waveform is not an integer multiple of
 *     50ms.  If the original waveform gets processed as a single track and then
 *     the same data again as a subsequent track, the album result gain will
 *     always match G (this is implied by 3.).
 *
 *  5. A stream shorter than 50ms cannot be analyzed.  At 8000 and 48000 Hz,
 *     this corresponds to 400 resp. 2400 frames.  If a stream is shorter than
 *     50ms, the element will not generate tags at EOS (only if an album
 *     finished, but only album tags are generated then).  This is not an
 *     erroneous condition, the element should behave normally.
 *
 * The limitations outlined in 1.-4. do not apply to the peak values.  Every
 * single sample is accounted for when looking for the peak.  Thus the album
 * peak is guaranteed to be the maximum value of all track peaks.
 *
 * In normal day-to-day use, these little facts are unlikely to be relevant, but
 * they have to be kept in mind for writing the tests here.
 */

#include <gst/check/gstcheck.h>
#include <gst/audio/audio.h>

/* For ease of programming we use globals to keep refs for our floating src and
 * sink pads we create; otherwise we always have to do get_pad, get_peer, and
 * then remove references in every test function */
static GstPad *mysrcpad, *mysinkpad;
static GstBus *mybus;

/* Mapping from supported sample rates to the correct result gain for the
 * following test waveform: 20 * 512 samples with a quarter-full amplitude of
 * toggling sign, changing every 48 samples and starting with the positive
 * value.
 *
 * Even if we would generate a wave describing a signal with the same frequency
 * at each sampling rate, the results would vary (slightly).  Hence the simple
 * generation method, since we cannot use a constant value as expected result
 * anyways.  For all sample rates, changing the sign every 48 frames gives a
 * sane frequency.  Buffers containing data that forms such a waveform is
 * created using the test_buffer_square_{float,int16}_{mono,stereo} functions
 * below.
 *
 * The results have been checked against what the metaflac and wavegain programs
 * generate for such a stream.  If you want to verify these, be sure that the
 * metaflac program does not produce incorrect results in your environment: I
 * found a strange bug in the (defacto) reference code for the analysis that
 * sometimes leads to incorrect RMS window lengths. */

struct rate_test
{
  guint sample_rate;
  gdouble gain;
};

static const struct rate_test supported_rates[] = {
  {8000, -0.91},
  {11025, -2.80},
  {12000, -3.13},
  {16000, -4.26},
  {22050, -5.64},
  {24000, -5.87},
  {32000, -6.03},
  {44100, -6.20},
  {48000, -6.14}
};

/* Lookup the correct gain adjustment result in above array. */

static gdouble
get_expected_gain (guint sample_rate)
{
  gint i;

  for (i = G_N_ELEMENTS (supported_rates); i--;)
    if (supported_rates[i].sample_rate == sample_rate)
      return supported_rates[i].gain;
  g_return_val_if_reached (0.0);
}

#define SILENCE_GAIN 64.82

#define REPLAY_GAIN_CAPS                                \
  "channels = (int) { 1, 2 }, "                         \
  "rate = (int) { 8000, 11025, 12000, 16000, 22050, "   \
  "24000, 32000, 44100, 48000 }"

#define RG_ANALYSIS_CAPS_TEMPLATE_STRING      \
  "audio/x-raw, "                             \
  "format = (string) "GST_AUDIO_NE (F32) ", " \
  "layout = (string) interleaved, "           \
  REPLAY_GAIN_CAPS                            \
  "; "                                        \
  "audio/x-raw, "                             \
  "format = (string) "GST_AUDIO_NE (S16) ", " \
  "layout = (string) interleaved, "           \
  REPLAY_GAIN_CAPS

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (RG_ANALYSIS_CAPS_TEMPLATE_STRING)
    );
static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (RG_ANALYSIS_CAPS_TEMPLATE_STRING)
    );

static gboolean
mysink_event_func (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GST_LOG_OBJECT (pad, "%s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  /* a sink would post tag events as messages, so do the same here,
   * esp. since we're polling on the bus waiting for TAG messages.. */
  if (GST_EVENT_TYPE (event) == GST_EVENT_TAG) {
    GstTagList *taglist;

    gst_event_parse_tag (event, &taglist);

    gst_bus_post (mybus, gst_message_new_tag (GST_OBJECT (mysinkpad),
            gst_tag_list_copy (taglist)));
  }

  gst_event_unref (event);
  return TRUE;
}

static GstElement *
setup_rganalysis (void)
{
  GstElement *analysis;
  GstBus *bus;

  GST_DEBUG ("setup_rganalysis");
  analysis = gst_check_setup_element ("rganalysis");
  mysrcpad = gst_check_setup_src_pad (analysis, &srctemplate);
  mysinkpad = gst_check_setup_sink_pad (analysis, &sinktemplate);
  gst_pad_set_event_function (mysinkpad, mysink_event_func);
  gst_pad_set_active (mysrcpad, TRUE);
  gst_pad_set_active (mysinkpad, TRUE);

  bus = gst_bus_new ();
  gst_element_set_bus (analysis, bus);

  mybus = bus;                  /* keep ref */

  return analysis;
}

static void
cleanup_rganalysis (GstElement * element)
{
  GST_DEBUG ("cleanup_rganalysis");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_object_unref (mybus);
  mybus = NULL;

  /* The bus owns references to the element: */
  gst_element_set_bus (element, NULL);

  gst_pad_set_active (mysrcpad, FALSE);
  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_src_pad (element);
  gst_check_teardown_sink_pad (element);
  gst_check_teardown_element (element);
}

static void
set_playing_state (GstElement * element)
{
  fail_unless (gst_element_set_state (element,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
      "Could not set state to PLAYING");
}

static void
send_flush_events (GstElement * element)
{
  gboolean res;
  GstPad *pad;

  pad = mysrcpad;
  res = gst_pad_push_event (pad, gst_event_new_flush_start ());
  fail_unless (res, "flush-start even not handledt");
  res = gst_pad_push_event (pad, gst_event_new_flush_stop (TRUE));
  fail_unless (res, "flush-stop event not handled");
}

static void
send_stream_start_event (GstElement * element)
{
  gboolean res;
  GstPad *pad;

  pad = mysrcpad;
  res = gst_pad_push_event (pad, gst_event_new_stream_start ("test"));
  fail_unless (res, "STREAM_START event not handled");
}

static void
send_caps_event (const gchar * format, gint sample_rate, gint channels)
{
  GstCaps *caps;

  caps = gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, format,
      "rate", G_TYPE_INT, sample_rate, "channels", G_TYPE_INT, channels,
      "layout", G_TYPE_STRING, "interleaved", NULL);
  if (channels == 2) {
    gst_caps_set_simple (caps,
        "channel-mask", GST_TYPE_BITMASK,
        G_GUINT64_CONSTANT (0x0000000000000003), NULL);
  }
  gst_pad_set_caps (mysrcpad, caps);
  gst_caps_unref (caps);
}

static void
send_segment_event (GstElement * element)
{
  GstSegment segment;
  gboolean res;
  GstPad *pad;

  pad = mysrcpad;
  gst_segment_init (&segment, GST_FORMAT_TIME);
  res = gst_pad_push_event (pad, gst_event_new_segment (&segment));
  fail_unless (res, "SEGMENT event not handled");
}

static void
send_eos_event (GstElement * element)
{
  GstBus *bus = gst_element_get_bus (element);
  GstPad *pad = mysrcpad;
  gboolean res;

  res = gst_pad_push_event (pad, gst_event_new_eos ());
  fail_unless (res, "EOS event not handled");

  /* There is no sink element, so _we_ post the EOS message on the bus here.  Of
   * course we generate any EOS ourselves, but this allows us to poll for the
   * EOS message in poll_eos if we expect the element to _not_ generate a TAG
   * message.  That's better than waiting for a timeout to lapse. */
  fail_unless (gst_bus_post (bus, gst_message_new_eos (NULL)));

  gst_object_unref (bus);
}

static void
send_tag_event (GstElement * element, GstTagList * tag_list)
{
  GstPad *pad = mysrcpad;
  GstEvent *event = gst_event_new_tag (tag_list);

  fail_unless (gst_pad_push_event (pad, event),
      "Cannot send TAG event: Not handled.");
}

static void
poll_eos (GstElement * element)
{
  GstBus *bus = gst_element_get_bus (element);
  GstMessage *message;

  message = gst_bus_poll (bus, GST_MESSAGE_EOS | GST_MESSAGE_TAG, GST_SECOND);
  fail_unless (message != NULL, "Could not poll for EOS message: Timed out");
  fail_unless (message->type == GST_MESSAGE_EOS,
      "Could not poll for eos message: got message of type %s instead",
      gst_message_type_get_name (message->type));

  gst_message_unref (message);
  gst_object_unref (bus);
}

static GstTagList *
poll_tags_only (GstElement * element)
{
  GstBus *bus = gst_element_get_bus (element);
  GstTagList *tag_list;
  GstMessage *message;

  message = gst_bus_poll (bus, GST_MESSAGE_TAG, GST_SECOND);
  fail_unless (message != NULL, "Could not poll for TAG message: Timed out");

  gst_message_parse_tag (message, &tag_list);
  gst_message_unref (message);
  gst_object_unref (bus);

  return tag_list;
}

/* This also polls for EOS since the TAG message comes right before the end of
 * streams. */

static GstTagList *
poll_tags_followed_by_eos (GstElement * element)
{
  GstTagList *tag_list = poll_tags_only (element);

  poll_eos (element);

  return tag_list;
}

#define MATCH_PEAK(p1, p2) ((p1 < p2 + 1e-6) && (p2 < p1 + 1e-6))
#define MATCH_GAIN(g1, g2) ((g1 < g2 + 1e-13) && (g2 < g1 + 1e-13))

static void
fail_unless_track_gain (const GstTagList * tag_list, gdouble gain)
{
  gdouble result;

  fail_unless (gst_tag_list_get_double (tag_list, GST_TAG_TRACK_GAIN, &result),
      "Tag list contains no track gain value");
  fail_unless (MATCH_GAIN (gain, result),
      "Track gain %+.2f does not match, expected %+.2f", result, gain);
}

static void
fail_unless_track_peak (const GstTagList * tag_list, gdouble peak)
{
  gdouble result;

  fail_unless (gst_tag_list_get_double (tag_list, GST_TAG_TRACK_PEAK, &result),
      "Tag list contains no track peak value");
  fail_unless (MATCH_PEAK (peak, result),
      "Track peak %f does not match, expected %f", result, peak);
}

static void
fail_unless_album_gain (const GstTagList * tag_list, gdouble gain)
{
  gdouble result;

  fail_unless (gst_tag_list_get_double (tag_list, GST_TAG_ALBUM_GAIN, &result),
      "Tag list contains no album gain value");
  fail_unless (MATCH_GAIN (result, gain),
      "Album gain %+.2f does not match, expected %+.2f", result, gain);
}

static void
fail_unless_album_peak (const GstTagList * tag_list, gdouble peak)
{
  gdouble result;

  fail_unless (gst_tag_list_get_double (tag_list, GST_TAG_ALBUM_PEAK, &result),
      "Tag list contains no album peak value");
  fail_unless (MATCH_PEAK (peak, result),
      "Album peak %f does not match, expected %f", result, peak);
}

static void
fail_if_track_tags (const GstTagList * tag_list)
{
  gdouble result;

  fail_if (gst_tag_list_get_double (tag_list, GST_TAG_TRACK_GAIN, &result),
      "Tag list contains track gain value (but should not)");
  fail_if (gst_tag_list_get_double (tag_list, GST_TAG_TRACK_PEAK, &result),
      "Tag list contains track peak value (but should not)");
}

static void
fail_if_album_tags (const GstTagList * tag_list)
{
  gdouble result;

  fail_if (gst_tag_list_get_double (tag_list, GST_TAG_ALBUM_GAIN, &result),
      "Tag list contains album gain value (but should not)");
  fail_if (gst_tag_list_get_double (tag_list, GST_TAG_ALBUM_PEAK, &result),
      "Tag list contains album peak value (but should not)");
}

static void
fail_unless_num_tracks (GstElement * element, guint num_tracks)
{
  guint current;

  g_object_get (element, "num-tracks", &current, NULL);
  fail_unless (current == num_tracks,
      "num-tracks property has incorrect value %u, expected %u",
      current, num_tracks);
}

/* Functions that create buffers with constant sample values, for peak
 * tests. */

static GstBuffer *
test_buffer_const_float_mono (gint sample_rate, gsize n_frames, gfloat value)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (n_frames * sizeof (gfloat));
  GstMapInfo map;
  gfloat *data;
  gint i;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gfloat *) map.data;
  for (i = n_frames; i--;)
    *data++ = value;
  gst_buffer_unmap (buf, &map);

  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

static GstBuffer *
test_buffer_const_float_stereo (gint sample_rate, gsize n_frames,
    gfloat value_l, gfloat value_r)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (n_frames * sizeof (gfloat) * 2);
  GstMapInfo map;
  gfloat *data;
  gint i;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gfloat *) map.data;
  for (i = n_frames; i--;) {
    *data++ = value_l;
    *data++ = value_r;
  }
  gst_buffer_unmap (buf, &map);

  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

static GstBuffer *
test_buffer_const_int16_mono (gint sample_rate, gint depth, gsize n_frames,
    gint16 value)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (n_frames * sizeof (gint16));
  gint16 *data;
  GstMapInfo map;
  gint i;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gint16 *) map.data;
  for (i = n_frames; i--;)
    *data++ = value;
  gst_buffer_unmap (buf, &map);

  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

static GstBuffer *
test_buffer_const_int16_stereo (gint sample_rate, gint depth, gsize n_frames,
    gint16 value_l, gint16 value_r)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (n_frames * sizeof (gint16) * 2);
  gint16 *data;
  GstMapInfo map;
  gint i;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gint16 *) map.data;
  for (i = n_frames; i--;) {
    *data++ = value_l;
    *data++ = value_r;
  }
  gst_buffer_unmap (buf, &map);

  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

/* Functions that create data buffers containing square signal
 * waveforms. */

static GstBuffer *
test_buffer_square_float_mono (gint * accumulator, gint sample_rate,
    gsize n_frames, gfloat value)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (n_frames * sizeof (gfloat));
  gfloat *data;
  GstMapInfo map;
  gint i;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gfloat *) map.data;
  for (i = n_frames; i--;) {
    *accumulator += 1;
    *accumulator %= 96;

    if (*accumulator < 48)
      *data++ = value;
    else
      *data++ = -value;
  }
  gst_buffer_unmap (buf, &map);

  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

static GstBuffer *
test_buffer_square_float_stereo (gint * accumulator, gint sample_rate,
    gsize n_frames, gfloat value_l, gfloat value_r)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (n_frames * sizeof (gfloat) * 2);
  gfloat *data;
  GstMapInfo map;
  gint i;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gfloat *) map.data;
  for (i = n_frames; i--;) {
    *accumulator += 1;
    *accumulator %= 96;

    if (*accumulator < 48) {
      *data++ = value_l;
      *data++ = value_r;
    } else {
      *data++ = -value_l;
      *data++ = -value_r;
    }
  }
  gst_buffer_unmap (buf, &map);

  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

static GstBuffer *
test_buffer_square_int16_mono (gint * accumulator, gint sample_rate,
    gint depth, gsize n_frames, gint16 value)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (n_frames * sizeof (gint16));
  gint16 *data;
  GstMapInfo map;
  gint i;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gint16 *) map.data;
  for (i = n_frames; i--;) {
    *accumulator += 1;
    *accumulator %= 96;

    if (*accumulator < 48)
      *data++ = value;
    else
      *data++ = -MAX (value, -32767);
  }
  gst_buffer_unmap (buf, &map);

  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

static GstBuffer *
test_buffer_square_int16_stereo (gint * accumulator, gint sample_rate,
    gint depth, gsize n_frames, gint16 value_l, gint16 value_r)
{
  GstBuffer *buf = gst_buffer_new_and_alloc (n_frames * sizeof (gint16) * 2);
  gint16 *data;
  GstMapInfo map;
  gint i;

  gst_buffer_map (buf, &map, GST_MAP_WRITE);
  data = (gint16 *) map.data;
  for (i = n_frames; i--;) {
    *accumulator += 1;
    *accumulator %= 96;

    if (*accumulator < 48) {
      *data++ = value_l;
      *data++ = value_r;
    } else {
      *data++ = -MAX (value_l, -32767);
      *data++ = -MAX (value_r, -32767);
    }
  }
  gst_buffer_unmap (buf, &map);

  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);

  return buf;
}

static void
push_buffer (GstBuffer * buf)
{
  /* gst_pad_push steals a reference. */
  fail_unless (gst_pad_push (mysrcpad, buf) == GST_FLOW_OK);
  ASSERT_BUFFER_REFCOUNT (buf, "buf", 1);
}

/*** Start of the tests. ***/

/* This test looks redundant, but early versions of the element
 * crashed when doing, well, nothing: */

GST_START_TEST (test_no_buffer)
{
  GstElement *element = setup_rganalysis ();

  set_playing_state (element);
  send_eos_event (element);
  poll_eos (element);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_no_buffer_album_1)
{
  GstElement *element = setup_rganalysis ();

  set_playing_state (element);

  /* Single track: */
  send_stream_start_event (element);
  send_segment_event (element);
  send_eos_event (element);
  poll_eos (element);

  /* First album: */
  g_object_set (element, "num-tracks", 3, NULL);

  send_flush_events (element);
  send_segment_event (element);
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 2);

  send_flush_events (element);
  send_segment_event (element);
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 1);

  send_flush_events (element);
  send_segment_event (element);
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 0);

  /* Second album: */
  g_object_set (element, "num-tracks", 2, NULL);

  send_flush_events (element);
  send_segment_event (element);
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 1);

  send_flush_events (element);
  send_segment_event (element);
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 0);

  /* Single track: */
  send_flush_events (element);
  send_segment_event (element);
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 0);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_no_buffer_album_2)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator = 0;
  gint i;

  g_object_set (element, "num-tracks", 3, NULL);
  set_playing_state (element);

  /* No buffer for the first track. */
  send_stream_start_event (element);
  send_segment_event (element);
  send_eos_event (element);
  /* No tags should be posted, there was nothing to analyze: */
  poll_eos (element);
  fail_unless_num_tracks (element, 2);

  /* A test waveform with known gain result as second track: */

  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (F32), 44100, 1);
  send_segment_event (element);
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_mono (&accumulator, 44100, 512,
            0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, -6.20);
  /* Album is not finished yet: */
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 1);

  /* No buffer for the last track. */

  send_flush_events (element);
  send_segment_event (element);
  send_eos_event (element);

  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_album_peak (tag_list, 0.25);
  fail_unless_album_gain (tag_list, -6.20);
  /* No track tags should be posted, as there was no data for it: */
  fail_if_track_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 0);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_empty_buffers)
{
  GstElement *element = setup_rganalysis ();

  set_playing_state (element);

  /* Single track: */
  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 44100, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (44100, 0, 0.0, 0.0));
  send_eos_event (element);
  poll_eos (element);

  /* First album: */
  g_object_set (element, "num-tracks", 2, NULL);

  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (44100, 0, 0.0, 0.0));
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 1);

  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (44100, 0, 0.0, 0.0));
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 0);

  /* Second album, with a single track: */
  g_object_set (element, "num-tracks", 1, NULL);
  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (44100, 0, 0.0, 0.0));
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 0);

  /* Single track: */
  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (44100, 0, 0.0, 0.0));
  send_eos_event (element);
  poll_eos (element);

  cleanup_rganalysis (element);
}

GST_END_TEST;

/* Tests for correctness of the peak values. */

/* Float peak test.  For stereo, one channel has the constant value of -1.369,
 * the other one 0.0.  This tests many things: The result peak value should
 * occur on any channel.  The peak is of course the absolute amplitude, so 1.369
 * should be the result.  This will also detect if the code uses the absolute
 * value during the comparison.  If it is buggy it will return 0.0 since 0.0 >
 * -1.369.  Furthermore, this makes sure that there is no problem with headroom
 * (exceeding 0dBFS).  In the wild you get float samples > 1.0 from stuff like
 * vorbis. */

GST_START_TEST (test_peak_float)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;

  set_playing_state (element);
  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 512, -1.369, 0.0));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.369);
  gst_tag_list_unref (tag_list);

  /* Swapped channels. */
  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 512, 0.0, -1.369));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.369);
  gst_tag_list_unref (tag_list);

  /* Mono. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 1);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_mono (8000, 512, -1.369));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.369);
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_peak_int16_16)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;

  set_playing_state (element);

  /* Half amplitude. */
  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_stereo (8000, 16, 512, 1 << 14, 0));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  gst_tag_list_unref (tag_list);

  /* Swapped channels. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_stereo (8000, 16, 512, 0, 1 << 14));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  gst_tag_list_unref (tag_list);

  /* Mono. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 1);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_mono (8000, 16, 512, 1 << 14));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  gst_tag_list_unref (tag_list);

  /* Half amplitude, negative variant. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_stereo (8000, 16, 512, -(1 << 14), 0));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  gst_tag_list_unref (tag_list);

  /* Swapped channels. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_stereo (8000, 16, 512, 0, -(1 << 14)));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  gst_tag_list_unref (tag_list);

  /* Mono. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 1);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_mono (8000, 16, 512, -(1 << 14)));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  gst_tag_list_unref (tag_list);


  /* Now check for correct normalization of the peak value: Sample
   * values of this format range from -32768 to 32767.  So for the
   * highest positive amplitude we do not reach 1.0, only for
   * -32768! */

  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_stereo (8000, 16, 512, 32767, 0));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 32767. / 32768.);
  gst_tag_list_unref (tag_list);

  /* Swapped channels. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_stereo (8000, 16, 512, 0, 32767));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 32767. / 32768.);
  gst_tag_list_unref (tag_list);

  /* Mono. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 1);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_mono (8000, 16, 512, 32767));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 32767. / 32768.);
  gst_tag_list_unref (tag_list);


  /* Negative variant, reaching 1.0. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_stereo (8000, 16, 512, -32768, 0));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.0);
  gst_tag_list_unref (tag_list);

  /* Swapped channels. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_stereo (8000, 16, 512, 0, -32768));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.0);
  gst_tag_list_unref (tag_list);

  /* Mono. */
  send_flush_events (element);
  send_caps_event (GST_AUDIO_NE (S16), 8000, 1);
  send_segment_event (element);
  push_buffer (test_buffer_const_int16_mono (8000, 16, 512, -32768));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.0);
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_peak_album)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;

  g_object_set (element, "num-tracks", 2, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 1024, 1.0, 0.0));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.0);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 1);

  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 1024, 0.0, 0.5));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  fail_unless_album_peak (tag_list, 1.0);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 0);

  /* Try a second album: */
  g_object_set (element, "num-tracks", 3, NULL);

  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 1024, 0.4, 0.4));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.4);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 2);

  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 1024, 0.45, 0.45));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.45);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 1);

  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 1024, 0.2, 0.2));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.2);
  fail_unless_album_peak (tag_list, 0.45);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 0);

  /* And now a single track, not in album mode (num-tracks is 0
   * now): */
  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 1024, 0.1, 0.1));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.1);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

/* Switching from track to album mode. */

GST_START_TEST (test_peak_track_album)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;

  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 1);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_mono (8000, 1024, 1.0));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.0);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);

  g_object_set (element, "num-tracks", 1, NULL);
  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_mono (8000, 1024, 0.5));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  fail_unless_album_peak (tag_list, 0.5);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 0);

  cleanup_rganalysis (element);
}

GST_END_TEST;

/* Disabling album processing before the end of the album.  Probably a rare edge
 * case and applications should not rely on this to work.  They need to send the
 * element to the READY state to clear up after an aborted album anyway since
 * they might need to process another album afterwards. */

GST_START_TEST (test_peak_album_abort_to_track)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;

  g_object_set (element, "num-tracks", 2, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 2);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 1024, 1.0, 0.0));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.0);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 1);

  g_object_set (element, "num-tracks", 0, NULL);

  send_flush_events (element);
  send_segment_event (element);
  push_buffer (test_buffer_const_float_stereo (8000, 1024, 0.0, 0.5));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_gain_album)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator;
  gint i;

  g_object_set (element, "num-tracks", 3, NULL);
  set_playing_state (element);

  /* The three tracks are constructed such that if any of these is in fact
   * ignored for the album gain, the album gain will differ. */
  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 44100, 2);
  send_segment_event (element);
  accumulator = 0;
  for (i = 8; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.75, 0.75));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.75);
  fail_unless_track_gain (tag_list, -15.70);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);

  send_flush_events (element);
  send_segment_event (element);
  accumulator = 0;
  for (i = 12; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.5, 0.5));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.5);
  fail_unless_track_gain (tag_list, -12.22);
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);

  send_flush_events (element);
  send_segment_event (element);
  accumulator = 0;
  for (i = 180; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.25, 0.25));
  send_eos_event (element);

  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, -6.20);
  fail_unless_album_peak (tag_list, 0.75);
  /* Strangely, wavegain reports -12.17 for the album, but the fixed
   * metaflac agrees to us.  Could be a 32767 vs. 32768 issue. */
  fail_unless_album_gain (tag_list, -12.18);
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

/* Checks ensuring that the "forced" property works as advertised. */

GST_START_TEST (test_forced)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator = 0;
  gint i;

  g_object_set (element, "forced", FALSE, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 44100, 2);
  send_segment_event (element);
  tag_list = gst_tag_list_new_empty ();
  /* Provided values are totally arbitrary. */
  gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND,
      GST_TAG_TRACK_PEAK, 1.0, GST_TAG_TRACK_GAIN, 2.21, NULL);
  send_tag_event (element, tag_list);

  for (i = 20; i--;)
    push_buffer (test_buffer_const_float_stereo (44100, 512, 0.5, 0.5));
  send_eos_event (element);

  /* This fails if a tag message is generated: */
  /* Same values as above */
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_gain (tag_list, 2.21);
  fail_unless_track_peak (tag_list, 1.0);
  gst_tag_list_unref (tag_list);

  /* Now back to a track without tags. */
  send_flush_events (element);
  send_segment_event (element);
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.25, 0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (44100));
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

/* Sending track gain and peak in separate tag lists. */

GST_START_TEST (test_forced_separate)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator = 0;
  gint i;

  g_object_set (element, "forced", FALSE, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 44100, 2);
  send_segment_event (element);
  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND, GST_TAG_TRACK_GAIN, 2.21,
      NULL);
  send_tag_event (element, tag_list);

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND, GST_TAG_TRACK_PEAK, 1.0,
      NULL);
  send_tag_event (element, tag_list);

  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.5, 0.5));
  send_eos_event (element);

  /* Same values as above */
  tag_list = poll_tags_only (element);
  fail_unless_track_gain (tag_list, 2.21);
  gst_tag_list_unref (tag_list);

  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 1.0);
  gst_tag_list_unref (tag_list);

  /* Now a track without tags. */
  send_flush_events (element);
  send_segment_event (element);
  accumulator = 0;
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.25, 0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (44100));
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

/* A TAG event is sent _after_ data has already been processed.  In real
 * pipelines, this could happen if there is more than one rganalysis element (by
 * accident).  While it would have analyzed all the data prior to receiving the
 * event, I expect it to not post its results if not forced.  This test is
 * almost equivalent to test_forced. */

GST_START_TEST (test_forced_after_data)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator = 0;
  gint i;

  g_object_set (element, "forced", FALSE, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 2);
  send_segment_event (element);
  for (i = 20; i--;)
    push_buffer (test_buffer_const_float_stereo (8000, 512, 0.5, 0.5));

  tag_list = gst_tag_list_new_empty ();
  gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND,
      GST_TAG_TRACK_PEAK, 1.0, GST_TAG_TRACK_GAIN, 2.21, NULL);
  send_tag_event (element, tag_list);

  send_eos_event (element);

  /* Same values as above */
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_gain (tag_list, 2.21);
  fail_unless_track_peak (tag_list, 1.0);
  gst_tag_list_unref (tag_list);

  send_flush_events (element);
  send_segment_event (element);
  /* Now back to a normal track, this one has no tags: */
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 8000, 512, 0.25,
            0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (8000));
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

/* Like test_forced, but *analyze* an album afterwards.  The two tests following
 * this one check the *skipping* of albums. */

GST_START_TEST (test_forced_album)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator;
  gint i;

  g_object_set (element, "forced", FALSE, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 44100, 2);
  send_segment_event (element);
  tag_list = gst_tag_list_new_empty ();
  /* Provided values are totally arbitrary. */
  gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND,
      GST_TAG_TRACK_PEAK, 1.0, GST_TAG_TRACK_GAIN, 2.21, NULL);
  send_tag_event (element, tag_list);

  accumulator = 0;
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.5, 0.5));
  send_eos_event (element);

  /* Same values as above */
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_gain (tag_list, 2.21);
  fail_unless_track_peak (tag_list, 1.0);
  gst_tag_list_unref (tag_list);

  /* Now an album without tags. */
  g_object_set (element, "num-tracks", 2, NULL);

  send_flush_events (element);
  send_segment_event (element);
  accumulator = 0;
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.25, 0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (44100));
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 1);

  send_flush_events (element);
  send_segment_event (element);
  accumulator = 0;
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.25, 0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (44100));
  fail_unless_album_peak (tag_list, 0.25);
  fail_unless_album_gain (tag_list, get_expected_gain (44100));
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 0);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_forced_album_skip)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator = 0;
  gint i;

  g_object_set (element, "forced", FALSE, "num-tracks", 2, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 2);
  send_segment_event (element);
  tag_list = gst_tag_list_new_empty ();
  /* Provided values are totally arbitrary. */
  gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND,
      GST_TAG_TRACK_PEAK, 0.75, GST_TAG_TRACK_GAIN, 2.21,
      GST_TAG_ALBUM_PEAK, 0.80, GST_TAG_ALBUM_GAIN, -0.11, NULL);
  send_tag_event (element, tag_list);

  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 8000, 512, 0.25,
            0.25));
  send_eos_event (element);

  /* Same values as above */
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_gain (tag_list, 2.21);
  fail_unless_track_peak (tag_list, 0.75);
  gst_tag_list_unref (tag_list);

  fail_unless_num_tracks (element, 1);

  /* This track has no tags, but needs to be skipped anyways since we
   * are in album processing mode. */
  send_flush_events (element);
  send_segment_event (element);
  for (i = 20; i--;)
    push_buffer (test_buffer_const_float_stereo (8000, 512, 0.0, 0.0));
  send_eos_event (element);
  poll_eos (element);
  fail_unless_num_tracks (element, 0);

  /* Normal track after the album.  Of course not to be skipped. */
  send_flush_events (element);
  send_segment_event (element);
  accumulator = 0;
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 8000, 512, 0.25,
            0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (8000));
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_forced_album_no_skip)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator = 0;
  gint i;

  g_object_set (element, "forced", FALSE, "num-tracks", 2, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 2);
  send_segment_event (element);
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 8000, 512, 0.25,
            0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (8000));
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 1);

  /* The second track has indeed full tags, but although being not forced, this
   * one has to be processed because album processing is on. */
  send_flush_events (element);
  send_segment_event (element);
  tag_list = gst_tag_list_new_empty ();
  /* Provided values are totally arbitrary. */
  gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND,
      GST_TAG_TRACK_PEAK, 0.75, GST_TAG_TRACK_GAIN, 2.21,
      GST_TAG_ALBUM_PEAK, 0.80, GST_TAG_ALBUM_GAIN, -0.11, NULL);
  send_tag_event (element, tag_list);
  for (i = 20; i--;)
    push_buffer (test_buffer_const_float_stereo (8000, 512, 0.0, 0.0));
  send_eos_event (element);

  /* the first batch from the tags */
  tag_list = poll_tags_only (element);
  fail_unless_track_peak (tag_list, 0.75);
  fail_unless_track_gain (tag_list, 2.21);
  gst_tag_list_unref (tag_list);

  /* the second from the processing */
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.0);
  fail_unless_track_gain (tag_list, SILENCE_GAIN);
  /* Second track was just silence so the album peak equals the first
   * track's peak. */
  fail_unless_album_peak (tag_list, 0.25);
  /* Statistical processing leads to the second track being
   * ignored for the gain (because it is so short): */
  fail_unless_album_gain (tag_list, get_expected_gain (8000));
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 0);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_forced_abort_album_no_skip)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator = 0;
  gint i;

  g_object_set (element, "forced", FALSE, "num-tracks", 2, NULL);
  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 8000, 2);
  send_segment_event (element);
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 8000, 512, 0.25,
            0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (8000));
  fail_if_album_tags (tag_list);
  gst_tag_list_unref (tag_list);
  fail_unless_num_tracks (element, 1);

  /* Disabling album processing before end of album: */
  g_object_set (element, "num-tracks", 0, NULL);

  send_flush_events (element);
  send_segment_event (element);

  /* Processing a track that has to be skipped. */
  tag_list = gst_tag_list_new_empty ();
  /* Provided values are totally arbitrary. */
  gst_tag_list_add (tag_list, GST_TAG_MERGE_APPEND,
      GST_TAG_TRACK_PEAK, 0.75, GST_TAG_TRACK_GAIN, 2.21,
      GST_TAG_ALBUM_PEAK, 0.80, GST_TAG_ALBUM_GAIN, -0.11, NULL);
  send_tag_event (element, tag_list);
  for (i = 20; i--;)
    push_buffer (test_buffer_const_float_stereo (8000, 512, 0.0, 0.0));
  send_eos_event (element);

  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.75);
  fail_unless_track_gain (tag_list, 2.21);
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_reference_level)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gdouble ref_level;
  gint accumulator = 0;
  gint i;

  set_playing_state (element);

  send_stream_start_event (element);
  send_caps_event (GST_AUDIO_NE (F32), 44100, 2);
  send_segment_event (element);
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.25, 0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (44100));
  fail_if_album_tags (tag_list);
  fail_unless (gst_tag_list_get_double (tag_list, GST_TAG_REFERENCE_LEVEL,
          &ref_level) && MATCH_GAIN (ref_level, 89.),
      "Incorrect reference level tag");
  gst_tag_list_unref (tag_list);

  g_object_set (element, "reference-level", 83., "num-tracks", 2, NULL);

  send_flush_events (element);
  send_segment_event (element);
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.25, 0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (44100) - 6.);
  fail_if_album_tags (tag_list);
  fail_unless (gst_tag_list_get_double (tag_list, GST_TAG_REFERENCE_LEVEL,
          &ref_level) && MATCH_GAIN (ref_level, 83.),
      "Incorrect reference level tag");
  gst_tag_list_unref (tag_list);

  send_flush_events (element);
  send_segment_event (element);
  accumulator = 0;
  for (i = 20; i--;)
    push_buffer (test_buffer_square_float_stereo (&accumulator, 44100, 512,
            0.25, 0.25));
  send_eos_event (element);
  tag_list = poll_tags_followed_by_eos (element);
  fail_unless_track_peak (tag_list, 0.25);
  fail_unless_track_gain (tag_list, get_expected_gain (44100) - 6.);
  fail_unless_album_peak (tag_list, 0.25);
  /* We provided the same waveform twice, with a reset separating
   * them.  Therefore, the album gain matches the track gain. */
  fail_unless_album_gain (tag_list, get_expected_gain (44100) - 6.);
  fail_unless (gst_tag_list_get_double (tag_list, GST_TAG_REFERENCE_LEVEL,
          &ref_level) && MATCH_GAIN (ref_level, 83.),
      "Incorrect reference level tag");
  gst_tag_list_unref (tag_list);

  cleanup_rganalysis (element);
}

GST_END_TEST;

GST_START_TEST (test_all_formats)
{
  GstElement *element = setup_rganalysis ();
  GstTagList *tag_list;
  gint accumulator = 0;
  gint i, j;

  set_playing_state (element);
  send_stream_start_event (element);
  for (i = G_N_ELEMENTS (supported_rates); i--;) {
    send_flush_events (element);
    send_caps_event (GST_AUDIO_NE (F32), supported_rates[i].sample_rate, 2);
    send_segment_event (element);
    accumulator = 0;
    for (j = 0; j < 4; j++)
      push_buffer (test_buffer_square_float_stereo (&accumulator,
              supported_rates[i].sample_rate, 512, 0.25, 0.25));
    send_caps_event (GST_AUDIO_NE (F32), supported_rates[i].sample_rate, 1);
    for (j = 0; j < 3; j++)
      push_buffer (test_buffer_square_float_mono (&accumulator,
              supported_rates[i].sample_rate, 512, 0.25));
    send_caps_event (GST_AUDIO_NE (S16), supported_rates[i].sample_rate, 2);
    for (j = 0; j < 4; j++)
      push_buffer (test_buffer_square_int16_stereo (&accumulator,
              supported_rates[i].sample_rate, 16, 512, 1 << 13, 1 << 13));
    send_caps_event (GST_AUDIO_NE (S16), supported_rates[i].sample_rate, 1);
    for (j = 0; j < 3; j++)
      push_buffer (test_buffer_square_int16_mono (&accumulator,
              supported_rates[i].sample_rate, 16, 512, 1 << 13));
    send_eos_event (element);
    tag_list = poll_tags_followed_by_eos (element);
    fail_unless_track_peak (tag_list, 0.25);
    fail_unless_track_gain (tag_list, supported_rates[i].gain);
    gst_tag_list_unref (tag_list);
  }

  cleanup_rganalysis (element);
}

GST_END_TEST;

/* Checks ensuring all advertised supported sample rates are really
 * accepted, for integer and float, mono and stereo.  This also
 * verifies that the correct gain is computed for all formats (except
 * odd bit depths). */

#define MAKE_GAIN_TEST_FLOAT_MONO(sample_rate)                        \
  GST_START_TEST (test_gain_float_mono_##sample_rate)                 \
{                                                                     \
  GstElement *element = setup_rganalysis ();                          \
  GstTagList *tag_list;                                               \
  gint accumulator = 0;                                               \
  gint i;                                                             \
                                                                      \
  set_playing_state (element);                                        \
  send_stream_start_event (element);                                  \
  send_caps_event (GST_AUDIO_NE (F32), sample_rate, 1);               \
  send_segment_event (element);                                       \
                                                                      \
  for (i = 0; i < 20; i++)                                            \
    push_buffer (test_buffer_square_float_mono (&accumulator,         \
            sample_rate, 512, 0.25));                                 \
  send_eos_event (element);                                           \
  tag_list = poll_tags_followed_by_eos (element);                                     \
  fail_unless_track_peak (tag_list, 0.25);                            \
  fail_unless_track_gain (tag_list,                                   \
      get_expected_gain (sample_rate));                               \
  gst_tag_list_unref (tag_list);                                       \
                                                                      \
  cleanup_rganalysis (element);                                       \
}                                                                     \
                                                                      \
GST_END_TEST;

#define MAKE_GAIN_TEST_FLOAT_STEREO(sample_rate)                      \
  GST_START_TEST (test_gain_float_stereo_##sample_rate)               \
{                                                                     \
  GstElement *element = setup_rganalysis ();                          \
  GstTagList *tag_list;                                               \
  gint accumulator = 0;                                               \
  gint i;                                                             \
                                                                      \
  set_playing_state (element);                                        \
  send_stream_start_event (element);                                  \
  send_caps_event (GST_AUDIO_NE (F32), sample_rate, 2);               \
  send_segment_event (element);                                       \
                                                                      \
  for (i = 0; i < 20; i++)                                            \
    push_buffer (test_buffer_square_float_stereo (&accumulator,       \
            sample_rate, 512, 0.25, 0.25));                           \
  send_eos_event (element);                                           \
  tag_list = poll_tags_followed_by_eos (element);                                     \
  fail_unless_track_peak (tag_list, 0.25);                            \
  fail_unless_track_gain (tag_list,                                   \
      get_expected_gain (sample_rate));                               \
  gst_tag_list_unref (tag_list);                                       \
                                                                      \
  cleanup_rganalysis (element);                                       \
}                                                                     \
                                                                      \
GST_END_TEST;

#define MAKE_GAIN_TEST_INT16_MONO(sample_rate, depth)                 \
  GST_START_TEST (test_gain_int16_##depth##_mono_##sample_rate)       \
{                                                                     \
  GstElement *element = setup_rganalysis ();                          \
  GstTagList *tag_list;                                               \
  gint accumulator = 0;                                               \
  gint i;                                                             \
                                                                      \
  set_playing_state (element);                                        \
  send_stream_start_event (element);                                  \
  send_caps_event (GST_AUDIO_NE (S16), sample_rate, 1);               \
  send_segment_event (element);                                       \
                                                                      \
  for (i = 0; i < 20; i++)                                            \
    push_buffer (test_buffer_square_int16_mono (&accumulator,         \
            sample_rate, depth, 512, 1 << (13 + depth - 16)));        \
                                                                      \
  send_eos_event (element);                                           \
  tag_list = poll_tags_followed_by_eos (element);                                     \
  fail_unless_track_peak (tag_list, 0.25);                            \
  fail_unless_track_gain (tag_list,                                   \
      get_expected_gain (sample_rate));                               \
  gst_tag_list_unref (tag_list);                                       \
                                                                      \
  cleanup_rganalysis (element);                                       \
}                                                                     \
                                                                      \
GST_END_TEST;

#define MAKE_GAIN_TEST_INT16_STEREO(sample_rate, depth)               \
  GST_START_TEST (test_gain_int16_##depth##_stereo_##sample_rate)     \
{                                                                     \
  GstElement *element = setup_rganalysis ();                          \
  GstTagList *tag_list;                                               \
  gint accumulator = 0;                                               \
  gint i;                                                             \
                                                                      \
  set_playing_state (element);                                        \
  send_stream_start_event (element);                                  \
  send_caps_event (GST_AUDIO_NE (S16), sample_rate, 2);               \
  send_segment_event (element);                                       \
                                                                      \
  for (i = 0; i < 20; i++)                                            \
    push_buffer (test_buffer_square_int16_stereo (&accumulator,       \
            sample_rate, depth, 512, 1 << (13 + depth - 16),          \
            1 << (13 + depth - 16)));                                 \
  send_eos_event (element);                                           \
  tag_list = poll_tags_followed_by_eos (element);                                     \
  fail_unless_track_peak (tag_list, 0.25);                            \
  fail_unless_track_gain (tag_list,                                   \
      get_expected_gain (sample_rate));                               \
  gst_tag_list_unref (tag_list);                                       \
                                                                      \
  cleanup_rganalysis (element);                                       \
}                                                                     \
                                                                      \
GST_END_TEST;

MAKE_GAIN_TEST_FLOAT_MONO (8000);
MAKE_GAIN_TEST_FLOAT_MONO (11025);
MAKE_GAIN_TEST_FLOAT_MONO (12000);
MAKE_GAIN_TEST_FLOAT_MONO (16000);
MAKE_GAIN_TEST_FLOAT_MONO (22050);
MAKE_GAIN_TEST_FLOAT_MONO (24000);
MAKE_GAIN_TEST_FLOAT_MONO (32000);
MAKE_GAIN_TEST_FLOAT_MONO (44100);
MAKE_GAIN_TEST_FLOAT_MONO (48000);

MAKE_GAIN_TEST_FLOAT_STEREO (8000);
MAKE_GAIN_TEST_FLOAT_STEREO (11025);
MAKE_GAIN_TEST_FLOAT_STEREO (12000);
MAKE_GAIN_TEST_FLOAT_STEREO (16000);
MAKE_GAIN_TEST_FLOAT_STEREO (22050);
MAKE_GAIN_TEST_FLOAT_STEREO (24000);
MAKE_GAIN_TEST_FLOAT_STEREO (32000);
MAKE_GAIN_TEST_FLOAT_STEREO (44100);
MAKE_GAIN_TEST_FLOAT_STEREO (48000);

MAKE_GAIN_TEST_INT16_MONO (8000, 16);
MAKE_GAIN_TEST_INT16_MONO (11025, 16);
MAKE_GAIN_TEST_INT16_MONO (12000, 16);
MAKE_GAIN_TEST_INT16_MONO (16000, 16);
MAKE_GAIN_TEST_INT16_MONO (22050, 16);
MAKE_GAIN_TEST_INT16_MONO (24000, 16);
MAKE_GAIN_TEST_INT16_MONO (32000, 16);
MAKE_GAIN_TEST_INT16_MONO (44100, 16);
MAKE_GAIN_TEST_INT16_MONO (48000, 16);

MAKE_GAIN_TEST_INT16_STEREO (8000, 16);
MAKE_GAIN_TEST_INT16_STEREO (11025, 16);
MAKE_GAIN_TEST_INT16_STEREO (12000, 16);
MAKE_GAIN_TEST_INT16_STEREO (16000, 16);
MAKE_GAIN_TEST_INT16_STEREO (22050, 16);
MAKE_GAIN_TEST_INT16_STEREO (24000, 16);
MAKE_GAIN_TEST_INT16_STEREO (32000, 16);
MAKE_GAIN_TEST_INT16_STEREO (44100, 16);
MAKE_GAIN_TEST_INT16_STEREO (48000, 16);

static Suite *
rganalysis_suite (void)
{
  Suite *s = suite_create ("rganalysis");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_no_buffer);
  tcase_add_test (tc_chain, test_no_buffer_album_1);
  tcase_add_test (tc_chain, test_no_buffer_album_2);
  tcase_add_test (tc_chain, test_empty_buffers);

  tcase_add_test (tc_chain, test_peak_float);
  tcase_add_test (tc_chain, test_peak_int16_16);

  tcase_add_test (tc_chain, test_peak_album);
  tcase_add_test (tc_chain, test_peak_track_album);
  tcase_add_test (tc_chain, test_peak_album_abort_to_track);

  tcase_add_test (tc_chain, test_gain_album);

  tcase_add_test (tc_chain, test_forced);
  tcase_add_test (tc_chain, test_forced_separate);
  tcase_add_test (tc_chain, test_forced_after_data);
  tcase_add_test (tc_chain, test_forced_album);
  tcase_add_test (tc_chain, test_forced_album_skip);
  tcase_add_test (tc_chain, test_forced_album_no_skip);
  tcase_add_test (tc_chain, test_forced_abort_album_no_skip);

  tcase_add_test (tc_chain, test_reference_level);

  tcase_add_test (tc_chain, test_all_formats);

  tcase_add_test (tc_chain, test_gain_float_mono_8000);
  tcase_add_test (tc_chain, test_gain_float_mono_11025);
  tcase_add_test (tc_chain, test_gain_float_mono_12000);
  tcase_add_test (tc_chain, test_gain_float_mono_16000);
  tcase_add_test (tc_chain, test_gain_float_mono_22050);
  tcase_add_test (tc_chain, test_gain_float_mono_24000);
  tcase_add_test (tc_chain, test_gain_float_mono_32000);
  tcase_add_test (tc_chain, test_gain_float_mono_44100);
  tcase_add_test (tc_chain, test_gain_float_mono_48000);

  tcase_add_test (tc_chain, test_gain_float_stereo_8000);
  tcase_add_test (tc_chain, test_gain_float_stereo_11025);
  tcase_add_test (tc_chain, test_gain_float_stereo_12000);
  tcase_add_test (tc_chain, test_gain_float_stereo_16000);
  tcase_add_test (tc_chain, test_gain_float_stereo_22050);
  tcase_add_test (tc_chain, test_gain_float_stereo_24000);
  tcase_add_test (tc_chain, test_gain_float_stereo_32000);
  tcase_add_test (tc_chain, test_gain_float_stereo_44100);
  tcase_add_test (tc_chain, test_gain_float_stereo_48000);

  tcase_add_test (tc_chain, test_gain_int16_16_mono_8000);
  tcase_add_test (tc_chain, test_gain_int16_16_mono_11025);
  tcase_add_test (tc_chain, test_gain_int16_16_mono_12000);
  tcase_add_test (tc_chain, test_gain_int16_16_mono_16000);
  tcase_add_test (tc_chain, test_gain_int16_16_mono_22050);
  tcase_add_test (tc_chain, test_gain_int16_16_mono_24000);
  tcase_add_test (tc_chain, test_gain_int16_16_mono_32000);
  tcase_add_test (tc_chain, test_gain_int16_16_mono_44100);
  tcase_add_test (tc_chain, test_gain_int16_16_mono_48000);

  tcase_add_test (tc_chain, test_gain_int16_16_stereo_8000);
  tcase_add_test (tc_chain, test_gain_int16_16_stereo_11025);
  tcase_add_test (tc_chain, test_gain_int16_16_stereo_12000);
  tcase_add_test (tc_chain, test_gain_int16_16_stereo_16000);
  tcase_add_test (tc_chain, test_gain_int16_16_stereo_22050);
  tcase_add_test (tc_chain, test_gain_int16_16_stereo_24000);
  tcase_add_test (tc_chain, test_gain_int16_16_stereo_32000);
  tcase_add_test (tc_chain, test_gain_int16_16_stereo_44100);
  tcase_add_test (tc_chain, test_gain_int16_16_stereo_48000);

  return s;
}

GST_CHECK_MAIN (rganalysis);
