/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2005 Wim Taymans <wim@fluendo.com>
 *                    2007 Andy Wingo <wingo at pobox.com>
 *                    2008 Sebastian Dröge <slomo@circular-chaos.rg>
 *
 * interleave.c: interleave samples, mostly based on adder.
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

/* TODO:
 *       - handle caps changes
 *       - handle more queries/events
 */

/**
 * SECTION:element-interleave
 * @title: interleave
 * @see_also: deinterleave
 *
 * Merges separate mono inputs into one interleaved stream.
 *
 * This element handles all raw floating point sample formats and all signed integer sample formats. The first
 * caps on one of the sinkpads will set the caps of the output so usually an audioconvert element should be
 * placed before every sinkpad of interleave.
 *
 * It's possible to change the number of channels while the pipeline is running by adding or removing
 * some of the request pads but this will change the caps of the output buffers. Changing the input
 * caps is _not_ supported yet.
 *
 * The channel number of every sinkpad in the out can be retrieved from the "channel" property of the pad.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=file.mp3 ! decodebin ! audioconvert ! "audio/x-raw,channels=2" ! deinterleave name=d  interleave name=i ! audioconvert ! wavenc ! filesink location=test.wav    d.src_0 ! queue ! audioconvert ! i.sink_1    d.src_1 ! queue ! audioconvert ! i.sink_0
 * ]| Decodes and deinterleaves a Stereo MP3 file into separate channels and
 * then interleaves the channels again to a WAV file with the channels
 * exchanged.
 * |[
 * gst-launch-1.0 interleave name=i ! audioconvert ! wavenc ! filesink location=file.wav  filesrc location=file1.wav ! decodebin ! audioconvert ! "audio/x-raw,channels=1,channel-mask=(bitmask)0x1" ! queue ! i.sink_0   filesrc location=file2.wav ! decodebin ! audioconvert ! "audio/x-raw,channels=1,channel-mask=(bitmask)0x2" ! queue ! i.sink_1
 * ]| Interleaves two Mono WAV files to a single Stereo WAV file. Having
 * channel-masks defined in the sink pads ensures a sane mapping of the mono
 * streams into the stereo stream. NOTE: the proper way to map channels in
 * code is by using the channel-positions property of the interleave element.
 *
 */

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <string.h>
#include "interleave.h"

#include <gst/audio/audio.h>
#include <gst/audio/audio-enumtypes.h>

GST_DEBUG_CATEGORY_STATIC (gst_interleave_debug);
#define GST_CAT_DEFAULT gst_interleave_debug

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("audio/x-raw, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) 1, "
        "format = (string) " GST_AUDIO_FORMATS_ALL ", "
        "layout = (string) {non-interleaved, interleaved}")
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "rate = (int) [ 1, MAX ], "
        "channels = (int) [ 1, MAX ], "
        "format = (string) " GST_AUDIO_FORMATS_ALL ", "
        "layout = (string) interleaved")
    );

#define MAKE_FUNC(type) \
static void interleave_##type (guint##type *out, guint##type *in, \
    guint stride, guint nframes) \
{ \
  gint i; \
  \
  for (i = 0; i < nframes; i++) { \
    *out = in[i]; \
    out += stride; \
  } \
}

MAKE_FUNC (8);
MAKE_FUNC (16);
MAKE_FUNC (32);
MAKE_FUNC (64);

static void
interleave_24 (guint8 * out, guint8 * in, guint stride, guint nframes)
{
  gint i;

  for (i = 0; i < nframes; i++) {
    memcpy (out, in, 3);
    out += stride * 3;
    in += 3;
  }
}

typedef struct
{
  GstPad parent;
  guint channel;
} GstInterleavePad;

enum
{
  PROP_PAD_0,
  PROP_PAD_CHANNEL
};

static void gst_interleave_pad_class_init (GstPadClass * klass);

#define GST_TYPE_INTERLEAVE_PAD (gst_interleave_pad_get_type())
#define GST_INTERLEAVE_PAD(pad) (G_TYPE_CHECK_INSTANCE_CAST((pad),GST_TYPE_INTERLEAVE_PAD,GstInterleavePad))
#define GST_INTERLEAVE_PAD_CAST(pad) ((GstInterleavePad *) pad)
#define GST_IS_INTERLEAVE_PAD(pad) (G_TYPE_CHECK_INSTANCE_TYPE((pad),GST_TYPE_INTERLEAVE_PAD))
static GType
gst_interleave_pad_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0)) {
    type = g_type_register_static_simple (GST_TYPE_PAD,
        g_intern_static_string ("GstInterleavePad"), sizeof (GstPadClass),
        (GClassInitFunc) gst_interleave_pad_class_init,
        sizeof (GstInterleavePad), NULL, 0);
  }
  return type;
}

static void
gst_interleave_pad_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstInterleavePad *self = GST_INTERLEAVE_PAD (object);

  switch (prop_id) {
    case PROP_PAD_CHANNEL:
      g_value_set_uint (value, self->channel);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_interleave_pad_class_init (GstPadClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->get_property = gst_interleave_pad_get_property;

  g_object_class_install_property (gobject_class,
      PROP_PAD_CHANNEL,
      g_param_spec_uint ("channel",
          "Channel number",
          "Number of the channel of this pad in the output", 0, G_MAXUINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

#define gst_interleave_parent_class parent_class
G_DEFINE_TYPE (GstInterleave, gst_interleave, GST_TYPE_ELEMENT);

enum
{
  PROP_0,
  PROP_CHANNEL_POSITIONS,
  PROP_CHANNEL_POSITIONS_FROM_INPUT
};

static void gst_interleave_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_interleave_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static GstPad *gst_interleave_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_interleave_release_pad (GstElement * element, GstPad * pad);

static GstStateChangeReturn gst_interleave_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_interleave_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query);

static gboolean gst_interleave_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

static gboolean gst_interleave_sink_event (GstCollectPads * pads,
    GstCollectData * data, GstEvent * event, gpointer user_data);
static gboolean gst_interleave_sink_query (GstCollectPads * pads,
    GstCollectData * data, GstQuery * query, gpointer user_data);

static gboolean gst_interleave_sink_setcaps (GstInterleave * self,
    GstPad * pad, const GstCaps * caps, const GstAudioInfo * info);

static GstCaps *gst_interleave_sink_getcaps (GstPad * pad, GstInterleave * self,
    GstCaps * filter);

static GstFlowReturn gst_interleave_collected (GstCollectPads * pads,
    GstInterleave * self);

static void
gst_interleave_finalize (GObject * object)
{
  GstInterleave *self = GST_INTERLEAVE (object);

  if (self->collect) {
    gst_object_unref (self->collect);
    self->collect = NULL;
  }

  if (self->channel_positions
      && self->channel_positions != self->input_channel_positions) {
    g_value_array_free (self->channel_positions);
    self->channel_positions = NULL;
  }

  if (self->input_channel_positions) {
    g_value_array_free (self->input_channel_positions);
    self->input_channel_positions = NULL;
  }

  gst_caps_replace (&self->sinkcaps, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint
compare_positions (gconstpointer a, gconstpointer b, gpointer user_data)
{
  const gint i = *(const gint *) a;
  const gint j = *(const gint *) b;
  const gint *pos = (const gint *) user_data;

  if (pos[i] < pos[j])
    return -1;
  else if (pos[i] > pos[j])
    return 1;
  else
    return 0;
}

static gboolean
gst_interleave_channel_positions_to_mask (GValueArray * positions,
    gint default_ordering_map[64], guint64 * mask)
{
  gint i;
  guint channels;
  GstAudioChannelPosition *pos;
  gboolean ret;

  channels = positions->n_values;
  pos = g_new (GstAudioChannelPosition, channels);

  for (i = 0; i < channels; i++) {
    GValue *val;

    val = g_value_array_get_nth (positions, i);
    pos[i] = g_value_get_enum (val);
  }

  /* sort the default ordering map according to the position order */
  for (i = 0; i < channels; i++) {
    default_ordering_map[i] = i;
  }
  g_qsort_with_data (default_ordering_map, channels,
      sizeof (*default_ordering_map), compare_positions, pos);

  ret = gst_audio_channel_positions_to_mask (pos, channels, FALSE, mask);
  g_free (pos);

  return ret;
}

static void
gst_interleave_set_channel_positions (GstInterleave * self, GstStructure * s)
{
  if (self->channels <= 64 &&
      self->channel_positions != NULL &&
      self->channels == self->channel_positions->n_values) {
    if (!gst_interleave_channel_positions_to_mask (self->channel_positions,
            self->default_channels_ordering_map, &self->channel_mask)) {
      GST_WARNING_OBJECT (self, "Invalid channel positions, using NONE");
      self->channel_mask = 0;
    }
  } else {
    self->channel_mask = 0;
    if (self->channels <= 64) {
      GST_WARNING_OBJECT (self, "Using NONE channel positions");
    }
  }
  gst_structure_set (s, "channel-mask", GST_TYPE_BITMASK, self->channel_mask,
      NULL);
}

static void
gst_interleave_send_stream_start (GstInterleave * self)
{
  GST_OBJECT_LOCK (self);
  if (self->send_stream_start) {
    gchar s_id[32];

    self->send_stream_start = FALSE;
    GST_OBJECT_UNLOCK (self);

    /* stream-start (FIXME: create id based on input ids) */
    g_snprintf (s_id, sizeof (s_id), "interleave-%08x", g_random_int ());
    gst_pad_push_event (self->src, gst_event_new_stream_start (s_id));
  } else {
    GST_OBJECT_UNLOCK (self);
  }
}

static void
gst_interleave_class_init (GstInterleaveClass * klass)
{
  GstElementClass *gstelement_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_interleave_debug, "interleave", 0,
      "interleave element");

  gst_element_class_set_static_metadata (gstelement_class, "Audio interleaver",
      "Filter/Converter/Audio",
      "Folds many mono channels into one interleaved audio stream",
      "Andy Wingo <wingo at pobox.com>, "
      "Sebastian Dröge <slomo@circular-chaos.org>");

  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &src_template);

  /* Reference GstInterleavePad class to have the type registered from
   * a threadsafe context
   */
  g_type_class_ref (GST_TYPE_INTERLEAVE_PAD);

  gobject_class->finalize = gst_interleave_finalize;
  gobject_class->set_property = gst_interleave_set_property;
  gobject_class->get_property = gst_interleave_get_property;

  /**
   * GstInterleave:channel-positions
   *
   * Channel positions: This property controls the channel positions
   * that are used on the src caps. The number of elements should be
   * the same as the number of sink pads and the array should contain
   * a valid list of channel positions. The n-th element of the array
   * is the position of the n-th sink pad.
   *
   * These channel positions will only be used if they're valid and the
   * number of elements is the same as the number of channels. If this
   * is not given a NONE layout will be used.
   *
   */
  g_object_class_install_property (gobject_class, PROP_CHANNEL_POSITIONS,
      g_param_spec_value_array ("channel-positions", "Channel positions",
          "Channel positions used on the output",
          g_param_spec_enum ("channel-position", "Channel position",
              "Channel position of the n-th input",
              GST_TYPE_AUDIO_CHANNEL_POSITION,
              GST_AUDIO_CHANNEL_POSITION_NONE,
              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS),
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstInterleave:channel-positions-from-input
   *
   * Channel positions from input: If this property is set to %TRUE the channel
   * positions will be taken from the input caps if valid channel positions for
   * the output can be constructed from them. If this is set to %TRUE setting the
   * channel-positions property overwrites this property again.
   *
   */
  g_object_class_install_property (gobject_class,
      PROP_CHANNEL_POSITIONS_FROM_INPUT,
      g_param_spec_boolean ("channel-positions-from-input",
          "Channel positions from input",
          "Take channel positions from the input", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_interleave_request_new_pad);
  gstelement_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_interleave_release_pad);
  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_interleave_change_state);
}

static void
gst_interleave_init (GstInterleave * self)
{
  self->src = gst_pad_new_from_static_template (&src_template, "src");

  gst_pad_set_query_function (self->src,
      GST_DEBUG_FUNCPTR (gst_interleave_src_query));
  gst_pad_set_event_function (self->src,
      GST_DEBUG_FUNCPTR (gst_interleave_src_event));

  gst_element_add_pad (GST_ELEMENT (self), self->src);

  self->collect = gst_collect_pads_new ();
  gst_collect_pads_set_function (self->collect,
      (GstCollectPadsFunction) gst_interleave_collected, self);

  self->input_channel_positions = g_value_array_new (0);
  self->channel_positions_from_input = TRUE;
  self->channel_positions = self->input_channel_positions;
}

static void
gst_interleave_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstInterleave *self = GST_INTERLEAVE (object);

  switch (prop_id) {
    case PROP_CHANNEL_POSITIONS:
      if (self->channel_positions &&
          self->channel_positions != self->input_channel_positions)
        g_value_array_free (self->channel_positions);

      self->channel_positions = g_value_dup_boxed (value);
      self->channel_positions_from_input = FALSE;
      break;
    case PROP_CHANNEL_POSITIONS_FROM_INPUT:
      self->channel_positions_from_input = g_value_get_boolean (value);

      if (self->channel_positions_from_input) {
        if (self->channel_positions &&
            self->channel_positions != self->input_channel_positions)
          g_value_array_free (self->channel_positions);
        self->channel_positions = self->input_channel_positions;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_interleave_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstInterleave *self = GST_INTERLEAVE (object);

  switch (prop_id) {
    case PROP_CHANNEL_POSITIONS:
      g_value_set_boxed (value, self->channel_positions);
      break;
    case PROP_CHANNEL_POSITIONS_FROM_INPUT:
      g_value_set_boolean (value, self->channel_positions_from_input);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstPad *
gst_interleave_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * req_name, const GstCaps * caps)
{
  GstInterleave *self = GST_INTERLEAVE (element);
  GstPad *new_pad;
  gchar *pad_name;
  gint channel, padnumber;
  GValue val = { 0, };

  if (templ->direction != GST_PAD_SINK)
    goto not_sink_pad;

  padnumber = g_atomic_int_add (&self->padcounter, 1);

  channel = g_atomic_int_add (&self->channels, 1);
  if (!self->channel_positions_from_input)
    channel = padnumber;

  pad_name = g_strdup_printf ("sink_%u", padnumber);
  new_pad = GST_PAD_CAST (g_object_new (GST_TYPE_INTERLEAVE_PAD,
          "name", pad_name, "direction", templ->direction,
          "template", templ, NULL));
  GST_INTERLEAVE_PAD_CAST (new_pad)->channel = channel;
  GST_DEBUG_OBJECT (self, "requested new pad %s", pad_name);
  g_free (pad_name);

  gst_pad_use_fixed_caps (new_pad);

  gst_collect_pads_add_pad (self->collect, new_pad, sizeof (GstCollectData),
      NULL, TRUE);

  gst_collect_pads_set_event_function (self->collect,
      (GstCollectPadsEventFunction)
      GST_DEBUG_FUNCPTR (gst_interleave_sink_event), self);

  gst_collect_pads_set_query_function (self->collect,
      (GstCollectPadsQueryFunction)
      GST_DEBUG_FUNCPTR (gst_interleave_sink_query), self);

  if (!gst_element_add_pad (element, new_pad))
    goto could_not_add;

  g_value_init (&val, GST_TYPE_AUDIO_CHANNEL_POSITION);
  g_value_set_enum (&val, GST_AUDIO_CHANNEL_POSITION_NONE);
  self->input_channel_positions =
      g_value_array_append (self->input_channel_positions, &val);
  g_value_unset (&val);

  /* Update the src caps if we already have them */
  if (self->sinkcaps) {
    GstCaps *srccaps;
    GstStructure *s;

    /* Take lock to make sure processing finishes first */
    GST_OBJECT_LOCK (self->collect);

    srccaps = gst_caps_copy (self->sinkcaps);
    s = gst_caps_get_structure (srccaps, 0);

    gst_structure_set (s, "channels", G_TYPE_INT, self->channels, NULL);
    gst_interleave_set_channel_positions (self, s);

    gst_interleave_send_stream_start (self);
    gst_pad_set_caps (self->src, srccaps);
    gst_caps_unref (srccaps);

    GST_OBJECT_UNLOCK (self->collect);
  }

  return new_pad;

  /* errors */
not_sink_pad:
  {
    g_warning ("interleave: requested new pad that is not a SINK pad\n");
    return NULL;
  }
could_not_add:
  {
    GST_DEBUG_OBJECT (self, "could not add pad %s", GST_PAD_NAME (new_pad));
    gst_collect_pads_remove_pad (self->collect, new_pad);
    gst_object_unref (new_pad);
    return NULL;
  }
}

static void
gst_interleave_release_pad (GstElement * element, GstPad * pad)
{
  GstInterleave *self = GST_INTERLEAVE (element);
  GList *l;
  GstAudioChannelPosition position;

  g_return_if_fail (GST_IS_INTERLEAVE_PAD (pad));

  /* Take lock to make sure we're not changing this when processing buffers */
  GST_OBJECT_LOCK (self->collect);

  g_atomic_int_add (&self->channels, -1);

  if (gst_pad_has_current_caps (pad))
    g_atomic_int_add (&self->configured_sinkpads_counter, -1);

  position = GST_INTERLEAVE_PAD_CAST (pad)->channel;
  g_value_array_remove (self->input_channel_positions, position);

  /* Update channel numbers */
  GST_OBJECT_LOCK (self);
  for (l = GST_ELEMENT_CAST (self)->sinkpads; l != NULL; l = l->next) {
    GstInterleavePad *ipad = GST_INTERLEAVE_PAD (l->data);

    if (GST_INTERLEAVE_PAD_CAST (pad)->channel < ipad->channel)
      ipad->channel--;
  }
  GST_OBJECT_UNLOCK (self);

  /* Update the src caps if we already have them */
  if (self->sinkcaps) {
    if (self->channels > 0) {
      GstCaps *srccaps;
      GstStructure *s;

      srccaps = gst_caps_copy (self->sinkcaps);
      s = gst_caps_get_structure (srccaps, 0);

      gst_structure_set (s, "channels", G_TYPE_INT, self->channels, NULL);
      gst_interleave_set_channel_positions (self, s);

      gst_interleave_send_stream_start (self);
      gst_pad_set_caps (self->src, srccaps);
      gst_caps_unref (srccaps);
    } else {
      gst_caps_replace (&self->sinkcaps, NULL);
    }
  }

  GST_OBJECT_UNLOCK (self->collect);

  gst_collect_pads_remove_pad (self->collect, pad);
  gst_element_remove_pad (element, pad);
}

static GstStateChangeReturn
gst_interleave_change_state (GstElement * element, GstStateChange transition)
{
  GstInterleave *self;
  GstStateChangeReturn ret;

  self = GST_INTERLEAVE (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      self->timestamp = 0;
      self->offset = 0;
      gst_event_replace (&self->pending_segment, NULL);
      self->send_stream_start = TRUE;
      gst_collect_pads_start (self->collect);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  /* Stop before calling the parent's state change function as
   * GstCollectPads might take locks and we would deadlock in that
   * case
   */
  if (transition == GST_STATE_CHANGE_PAUSED_TO_READY)
    gst_collect_pads_stop (self->collect);

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_caps_replace (&self->sinkcaps, NULL);
      gst_event_replace (&self->pending_segment, NULL);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}

static void
__remove_channels (GstCaps * caps)
{
  GstStructure *s;
  gint i, size;

  size = gst_caps_get_size (caps);
  for (i = 0; i < size; i++) {
    s = gst_caps_get_structure (caps, i);
    gst_structure_remove_field (s, "channel-mask");
    gst_structure_remove_field (s, "channels");
  }
}

static void
__set_channels (GstCaps * caps, gint channels)
{
  GstStructure *s;
  gint i, size;

  size = gst_caps_get_size (caps);
  for (i = 0; i < size; i++) {
    s = gst_caps_get_structure (caps, i);
    if (channels > 0)
      gst_structure_set (s, "channels", G_TYPE_INT, channels, NULL);
    else
      gst_structure_set (s, "channels", GST_TYPE_INT_RANGE, 1, G_MAXINT, NULL);
  }
}

/* we can only accept caps that we and downstream can handle. */
static GstCaps *
gst_interleave_sink_getcaps (GstPad * pad, GstInterleave * self,
    GstCaps * filter)
{
  GstCaps *result, *peercaps, *sinkcaps;

  GST_OBJECT_LOCK (self);

  /* If we already have caps on one of the sink pads return them */
  if (self->sinkcaps) {
    result = gst_caps_copy (self->sinkcaps);
  } else {
    /* get the downstream possible caps */
    peercaps = gst_pad_peer_query_caps (self->src, NULL);

    /* get the allowed caps on this sinkpad */
    sinkcaps = gst_caps_copy (gst_pad_get_pad_template_caps (pad));
    __remove_channels (sinkcaps);
    if (peercaps) {
      peercaps = gst_caps_make_writable (peercaps);
      __remove_channels (peercaps);
      /* if the peer has caps, intersect */
      GST_DEBUG_OBJECT (pad, "intersecting peer and template caps");
      result = gst_caps_intersect (peercaps, sinkcaps);
      gst_caps_unref (peercaps);
      gst_caps_unref (sinkcaps);
    } else {
      /* the peer has no caps (or there is no peer), just use the allowed caps
       * of this sinkpad. */
      GST_DEBUG_OBJECT (pad, "no peer caps, using sinkcaps");
      result = sinkcaps;
    }
    __set_channels (result, 1);
  }

  GST_OBJECT_UNLOCK (self);

  if (filter != NULL) {
    GstCaps *caps = result;

    GST_LOG_OBJECT (pad, "intersecting filter caps %" GST_PTR_FORMAT " with "
        "preliminary result %" GST_PTR_FORMAT, filter, caps);

    result = gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
  }

  GST_DEBUG_OBJECT (pad, "Returning caps %" GST_PTR_FORMAT, result);

  return result;
}

static void
gst_interleave_set_process_function (GstInterleave * self)
{
  switch (self->width) {
    case 8:
      self->func = (GstInterleaveFunc) interleave_8;
      break;
    case 16:
      self->func = (GstInterleaveFunc) interleave_16;
      break;
    case 24:
      self->func = (GstInterleaveFunc) interleave_24;
      break;
    case 32:
      self->func = (GstInterleaveFunc) interleave_32;
      break;
    case 64:
      self->func = (GstInterleaveFunc) interleave_64;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

static gboolean
gst_interleave_sink_setcaps (GstInterleave * self, GstPad * pad,
    const GstCaps * caps, const GstAudioInfo * info)
{
  g_return_val_if_fail (GST_IS_INTERLEAVE_PAD (pad), FALSE);

  /* TODO: handle caps changes */
  if (self->sinkcaps && !gst_caps_is_subset (caps, self->sinkcaps)) {
    goto cannot_change_caps;
  } else {
    GstCaps *srccaps;
    GstStructure *s;
    gboolean res;

    self->width = GST_AUDIO_INFO_WIDTH (info);
    self->rate = GST_AUDIO_INFO_RATE (info);

    gst_interleave_set_process_function (self);

    srccaps = gst_caps_copy (caps);
    s = gst_caps_get_structure (srccaps, 0);

    gst_structure_remove_field (s, "channel-mask");

    gst_structure_set (s, "channels", G_TYPE_INT, self->channels, "layout",
        G_TYPE_STRING, "interleaved", NULL);
    gst_interleave_set_channel_positions (self, s);

    gst_interleave_send_stream_start (self);
    res = gst_pad_set_caps (self->src, srccaps);
    gst_caps_unref (srccaps);

    if (!res)
      goto src_did_not_accept;
  }

  if (!self->sinkcaps) {
    GstCaps *sinkcaps = gst_caps_copy (caps);
    GstStructure *s = gst_caps_get_structure (sinkcaps, 0);

    gst_structure_remove_field (s, "channel-mask");

    GST_DEBUG_OBJECT (self, "setting sinkcaps %" GST_PTR_FORMAT, sinkcaps);

    gst_caps_replace (&self->sinkcaps, sinkcaps);

    gst_caps_unref (sinkcaps);
  }

  return TRUE;

cannot_change_caps:
  {
    GST_WARNING_OBJECT (self, "caps of %" GST_PTR_FORMAT " already set, can't "
        "change", self->sinkcaps);
    return FALSE;
  }
src_did_not_accept:
  {
    GST_WARNING_OBJECT (self, "src did not accept setcaps()");
    return FALSE;
  }
}

static gboolean
gst_interleave_sink_event (GstCollectPads * pads, GstCollectData * data,
    GstEvent * event, gpointer user_data)
{
  GstInterleave *self = GST_INTERLEAVE (user_data);
  gboolean ret = TRUE;

  GST_DEBUG ("Got %s event on pad %s:%s", GST_EVENT_TYPE_NAME (event),
      GST_DEBUG_PAD_NAME (data->pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      GST_OBJECT_LOCK (self);
      gst_event_replace (&self->pending_segment, NULL);
      GST_OBJECT_UNLOCK (self);
      break;
    case GST_EVENT_SEGMENT:
    {
      GST_OBJECT_LOCK (self);
      gst_event_replace (&self->pending_segment, event);
      GST_OBJECT_UNLOCK (self);
      break;
    }
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      GstAudioInfo info;
      GValue *val;
      guint channel;

      gst_event_parse_caps (event, &caps);

      if (!gst_audio_info_from_caps (&info, caps)) {
        GST_WARNING_OBJECT (self, "invalid sink caps");
        gst_event_unref (event);
        event = NULL;
        ret = FALSE;
        break;
      }

      if (self->channel_positions_from_input
          && GST_AUDIO_INFO_CHANNELS (&info) == 1) {
        channel = GST_INTERLEAVE_PAD_CAST (data->pad)->channel;
        val = g_value_array_get_nth (self->input_channel_positions, channel);
        g_value_set_enum (val, GST_AUDIO_INFO_POSITION (&info, 0));
      }

      if (!gst_pad_has_current_caps (data->pad))
        g_atomic_int_add (&self->configured_sinkpads_counter, 1);

      /* Last caps that are set on a sink pad are used as output caps */
      if (g_atomic_int_get (&self->configured_sinkpads_counter) ==
          self->channels) {
        ret = gst_interleave_sink_setcaps (self, data->pad, caps, &info);
        gst_event_unref (event);
        event = NULL;
      }
      break;
    }
    case GST_EVENT_TAG:
      GST_FIXME_OBJECT (self, "FIXME: merge tags and send after stream-start");
      break;
    default:
      break;
  }

  /* now GstCollectPads can take care of the rest, e.g. EOS */
  if (event != NULL)
    return gst_collect_pads_event_default (pads, data, event, FALSE);

  return ret;
}

static gboolean
gst_interleave_sink_query (GstCollectPads * pads,
    GstCollectData * data, GstQuery * query, gpointer user_data)
{
  GstInterleave *self = GST_INTERLEAVE (user_data);
  gboolean ret = TRUE;

  GST_DEBUG ("Got %s query on pad %s:%s", GST_QUERY_TYPE_NAME (query),
      GST_DEBUG_PAD_NAME (data->pad));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:
    {
      GstCaps *filter, *caps;

      gst_query_parse_caps (query, &filter);
      caps = gst_interleave_sink_getcaps (data->pad, self, filter);
      gst_query_set_caps_result (query, caps);
      gst_caps_unref (caps);
      ret = TRUE;
      break;
    }
    default:
      ret = gst_collect_pads_query_default (pads, data, query, FALSE);
      break;
  }

  return ret;
}

static gboolean
gst_interleave_src_query_duration (GstInterleave * self, GstQuery * query)
{
  gint64 max;
  gboolean res;
  GstFormat format;
  GstIterator *it;
  gboolean done;

  /* parse format */
  gst_query_parse_duration (query, &format, NULL);

  max = -1;
  res = TRUE;
  done = FALSE;

  /* Take maximum of all durations */
  it = gst_element_iterate_sink_pads (GST_ELEMENT_CAST (self));
  while (!done) {
    GstIteratorResult ires;

    GValue item = { 0, };

    ires = gst_iterator_next (it, &item);
    switch (ires) {
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
      case GST_ITERATOR_OK:
      {
        GstPad *pad = GST_PAD_CAST (g_value_dup_object (&item));

        gint64 duration;

        /* ask sink peer for duration */
        res &= gst_pad_peer_query_duration (pad, format, &duration);
        /* take max from all valid return values */
        if (res) {
          /* valid unknown length, stop searching */
          if (duration == -1) {
            max = duration;
            done = TRUE;
          }
          /* else see if bigger than current max */
          else if (duration > max)
            max = duration;
        }
        gst_object_unref (pad);
        g_value_unset (&item);
        break;
      }
      case GST_ITERATOR_RESYNC:
        max = -1;
        res = TRUE;
        gst_iterator_resync (it);
        break;
      default:
        res = FALSE;
        done = TRUE;
        break;
    }
  }
  gst_iterator_free (it);

  if (res) {
    /* If in bytes format we have to multiply with the number of channels
     * to get the correct results. All other formats should be fine */
    if (format == GST_FORMAT_BYTES && max != -1)
      max *= self->channels;

    /* and store the max */
    GST_DEBUG_OBJECT (self, "Total duration in format %s: %"
        GST_TIME_FORMAT, gst_format_get_name (format), GST_TIME_ARGS (max));
    gst_query_set_duration (query, format, max);
  }

  return res;
}

static gboolean
gst_interleave_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  GstInterleave *self = GST_INTERLEAVE (parent);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_POSITION:
    {
      GstFormat format;

      gst_query_parse_position (query, &format, NULL);

      switch (format) {
        case GST_FORMAT_TIME:
          /* FIXME, bring to stream time, might be tricky */
          gst_query_set_position (query, format, self->timestamp);
          res = TRUE;
          break;
        case GST_FORMAT_BYTES:
          gst_query_set_position (query, format,
              self->offset * self->channels * self->width);
          res = TRUE;
          break;
        case GST_FORMAT_DEFAULT:
          gst_query_set_position (query, format, self->offset);
          res = TRUE;
          break;
        default:
          break;
      }
      break;
    }
    case GST_QUERY_DURATION:
      res = gst_interleave_src_query_duration (self, query);
      break;
    default:
      /* FIXME, needs a custom query handler because we have multiple
       * sinkpads */
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

static gboolean
forward_event_func (const GValue * item, GValue * ret, GstEvent * event)
{
  GstPad *pad = GST_PAD_CAST (g_value_dup_object (item));
  gst_event_ref (event);
  GST_LOG_OBJECT (pad, "About to send event %s", GST_EVENT_TYPE_NAME (event));
  if (!gst_pad_push_event (pad, event)) {
    g_value_set_boolean (ret, FALSE);
    GST_WARNING_OBJECT (pad, "Sending event  %p (%s) failed.",
        event, GST_EVENT_TYPE_NAME (event));
  } else {
    GST_LOG_OBJECT (pad, "Sent event  %p (%s).",
        event, GST_EVENT_TYPE_NAME (event));
  }
  gst_object_unref (pad);
  return TRUE;
}

static gboolean
forward_event (GstInterleave * self, GstEvent * event)
{
  GstIterator *it;
  GValue vret = { 0 };

  GST_LOG_OBJECT (self, "Forwarding event %p (%s)", event,
      GST_EVENT_TYPE_NAME (event));

  g_value_init (&vret, G_TYPE_BOOLEAN);
  g_value_set_boolean (&vret, TRUE);
  it = gst_element_iterate_sink_pads (GST_ELEMENT_CAST (self));
  gst_iterator_fold (it, (GstIteratorFoldFunction) forward_event_func, &vret,
      event);
  gst_iterator_free (it);
  gst_event_unref (event);

  return g_value_get_boolean (&vret);
}


static gboolean
gst_interleave_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstInterleave *self = GST_INTERLEAVE (parent);
  gboolean result;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_QOS:
      /* QoS might be tricky */
      result = FALSE;
      break;
    case GST_EVENT_SEEK:
    {
      GstSeekFlags flags;

      gst_event_parse_seek (event, NULL, NULL, &flags, NULL, NULL, NULL, NULL);

      /* check if we are flushing */
      if (flags & GST_SEEK_FLAG_FLUSH) {
        /* make sure we accept nothing anymore and return WRONG_STATE */
        gst_collect_pads_set_flushing (self->collect, TRUE);

        /* flushing seek, start flush downstream, the flush will be done
         * when all pads received a FLUSH_STOP. */
        gst_pad_push_event (self->src, gst_event_new_flush_start ());
      }
      result = forward_event (self, event);
      break;
    }
    case GST_EVENT_NAVIGATION:
      /* navigation is rather pointless. */
      result = FALSE;
      break;
    default:
      /* just forward the rest for now */
      result = forward_event (self, event);
      break;
  }

  return result;
}

static GstFlowReturn
gst_interleave_collected (GstCollectPads * pads, GstInterleave * self)
{
  guint size;
  GstBuffer *outbuf = NULL;
  GstFlowReturn ret = GST_FLOW_OK;
  GSList *collected;
  guint nsamples;
  guint ncollected = 0;
  gboolean empty = TRUE;
  gint width = self->width / 8;
  GstMapInfo write_info;
  GstClockTime timestamp = -1;

  size = gst_collect_pads_available (pads);
  if (size == 0)
    goto eos;

  g_return_val_if_fail (self->func != NULL, GST_FLOW_NOT_NEGOTIATED);
  g_return_val_if_fail (self->width > 0, GST_FLOW_NOT_NEGOTIATED);
  g_return_val_if_fail (self->channels > 0, GST_FLOW_NOT_NEGOTIATED);
  g_return_val_if_fail (self->rate > 0, GST_FLOW_NOT_NEGOTIATED);

  g_return_val_if_fail (size % width == 0, GST_FLOW_ERROR);

  GST_DEBUG_OBJECT (self, "Starting to collect %u bytes from %d channels", size,
      self->channels);

  nsamples = size / width;

  outbuf = gst_buffer_new_allocate (NULL, size * self->channels, NULL);

  if (outbuf == NULL || gst_buffer_get_size (outbuf) < size * self->channels) {
    gst_buffer_unref (outbuf);
    return GST_FLOW_NOT_NEGOTIATED;
  }

  gst_buffer_map (outbuf, &write_info, GST_MAP_WRITE);
  memset (write_info.data, 0, size * self->channels);

  for (collected = pads->data; collected != NULL; collected = collected->next) {
    GstCollectData *cdata;
    GstBuffer *inbuf;
    guint8 *outdata;
    GstMapInfo input_info;
    gint channel;

    cdata = (GstCollectData *) collected->data;

    inbuf = gst_collect_pads_take_buffer (pads, cdata, size);
    if (inbuf == NULL) {
      GST_DEBUG_OBJECT (cdata->pad, "No buffer available");
      goto next;
    }
    ncollected++;

    if (timestamp == -1)
      timestamp = GST_BUFFER_TIMESTAMP (inbuf);

    if (GST_BUFFER_FLAG_IS_SET (inbuf, GST_BUFFER_FLAG_GAP))
      goto next;

    empty = FALSE;
    channel = GST_INTERLEAVE_PAD_CAST (cdata->pad)->channel;
    if (self->channels <= 64 && self->channel_mask) {
      channel = self->default_channels_ordering_map[channel];
    }
    outdata = write_info.data + width * channel;

    gst_buffer_map (inbuf, &input_info, GST_MAP_READ);
    self->func (outdata, input_info.data, self->channels, nsamples);
    gst_buffer_unmap (inbuf, &input_info);

  next:
    if (inbuf)
      gst_buffer_unref (inbuf);
  }

  if (ncollected == 0) {
    gst_buffer_unmap (outbuf, &write_info);
    goto eos;
  }

  GST_OBJECT_LOCK (self);
  if (self->pending_segment) {
    GstEvent *event;
    GstSegment segment;

    event = self->pending_segment;
    self->pending_segment = NULL;
    GST_OBJECT_UNLOCK (self);

    /* convert the input segment to time now */
    gst_event_copy_segment (event, &segment);

    if (segment.format != GST_FORMAT_TIME) {
      gst_event_unref (event);

      /* not time, convert */
      switch (segment.format) {
        case GST_FORMAT_BYTES:
          segment.start *= width;
          if (segment.stop != -1)
            segment.stop *= width;
          if (segment.position != -1)
            segment.position *= width;
          /* fallthrough for the samples case */
        case GST_FORMAT_DEFAULT:
          segment.start =
              gst_util_uint64_scale_int (segment.start, GST_SECOND, self->rate);
          if (segment.stop != -1)
            segment.stop =
                gst_util_uint64_scale_int (segment.stop, GST_SECOND,
                self->rate);
          if (segment.position != -1)
            segment.position =
                gst_util_uint64_scale_int (segment.position, GST_SECOND,
                self->rate);
          break;
        default:
          GST_WARNING ("can't convert segment values");
          segment.start = 0;
          segment.stop = -1;
          segment.position = 0;
          break;
      }
      event = gst_event_new_segment (&segment);
    }
    gst_pad_push_event (self->src, event);

    GST_OBJECT_LOCK (self);
  }
  GST_OBJECT_UNLOCK (self);

  if (timestamp != -1) {
    self->offset = gst_util_uint64_scale_int (timestamp, self->rate,
        GST_SECOND);
    self->timestamp = timestamp;
  }

  GST_BUFFER_TIMESTAMP (outbuf) = self->timestamp;
  GST_BUFFER_OFFSET (outbuf) = self->offset;

  self->offset += nsamples;
  self->timestamp = gst_util_uint64_scale_int (self->offset,
      GST_SECOND, self->rate);

  GST_BUFFER_DURATION (outbuf) =
      self->timestamp - GST_BUFFER_TIMESTAMP (outbuf);

  if (empty)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_GAP);

  gst_buffer_unmap (outbuf, &write_info);

  GST_LOG_OBJECT (self, "pushing outbuf, timestamp %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)));
  ret = gst_pad_push (self->src, outbuf);

  return ret;

eos:
  {
    GST_DEBUG_OBJECT (self, "no data available, must be EOS");
    if (outbuf)
      gst_buffer_unref (outbuf);
    gst_pad_push_event (self->src, gst_event_new_eos ());
    return GST_FLOW_EOS;
  }
}
