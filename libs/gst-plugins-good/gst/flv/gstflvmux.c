/* GStreamer
 *
 * Copyright (c) 2008,2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 * Copyright (c) 2008-2017 Collabora Ltd
 *  @author: Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *  @author: Vincent Penquerc'h <vincent.penquerch@collabora.com>
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

/**
 * SECTION:element-flvmux
 * @title: flvmux
 *
 * flvmux muxes different streams into an FLV file.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v flvmux name=mux ! filesink location=test.flv  audiotestsrc samplesperbuffer=44100 num-buffers=10 ! faac ! mux.  videotestsrc num-buffers=250 ! video/x-raw,framerate=25/1 ! x264enc ! mux.
 * ]| This pipeline encodes a test audio and video stream and muxes both into an FLV file.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include <gst/audio/audio.h>

#include "gstflvmux.h"
#include "amfdefs.h"

GST_DEBUG_CATEGORY_STATIC (flvmux_debug);
#define GST_CAT_DEFAULT flvmux_debug

enum
{
  PROP_0,
  PROP_STREAMABLE,
  PROP_METADATACREATOR,
  PROP_ENCODER
};

#define DEFAULT_STREAMABLE FALSE
#define MAX_INDEX_ENTRIES 128
#define DEFAULT_METADATACREATOR "GStreamer " PACKAGE_VERSION " FLV muxer"

static GstStaticPadTemplate src_templ = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-flv")
    );

static GstStaticPadTemplate videosink_templ = GST_STATIC_PAD_TEMPLATE ("video",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("video/x-flash-video; "
        "video/x-flash-screen; "
        "video/x-vp6-flash; " "video/x-vp6-alpha; "
        "video/x-h264, stream-format=avc;")
    );

static GstStaticPadTemplate audiosink_templ = GST_STATIC_PAD_TEMPLATE ("audio",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS
    ("audio/x-adpcm, layout = (string) swf, channels = (int) { 1, 2 }, rate = (int) { 5512, 11025, 22050, 44100 }; "
        "audio/mpeg, mpegversion = (int) 1, layer = (int) 3, channels = (int) { 1, 2 }, rate = (int) { 5512, 8000, 11025, 22050, 44100 }, parsed = (boolean) TRUE; "
        "audio/mpeg, mpegversion = (int) { 4, 2 }, stream-format = (string) raw; "
        "audio/x-nellymoser, channels = (int) { 1, 2 }, rate = (int) { 5512, 8000, 11025, 16000, 22050, 44100 }; "
        "audio/x-raw, format = (string) { U8, S16LE}, layout = (string) interleaved, channels = (int) { 1, 2 }, rate = (int) { 5512, 11025, 22050, 44100 }; "
        "audio/x-alaw, channels = (int) { 1, 2 }, rate = (int) 8000; "
        "audio/x-mulaw, channels = (int) { 1, 2 }, rate = (int) 8000; "
        "audio/x-speex, channels = (int) 1, rate = (int) 16000;")
    );

G_DEFINE_TYPE (GstFlvMuxPad, gst_flv_mux_pad, GST_TYPE_AGGREGATOR_PAD);

#define gst_flv_mux_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstFlvMux, gst_flv_mux, GST_TYPE_AGGREGATOR,
    G_IMPLEMENT_INTERFACE (GST_TYPE_TAG_SETTER, NULL));

static GstFlowReturn
gst_flv_mux_aggregate (GstAggregator * aggregator, gboolean timeout);
static gboolean
gst_flv_mux_sink_event (GstAggregator * aggregator, GstAggregatorPad * pad,
    GstEvent * event);

static GstAggregatorPad *gst_flv_mux_create_new_pad (GstAggregator * agg,
    GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps);
static void gst_flv_mux_release_pad (GstElement * element, GstPad * pad);

static gboolean gst_flv_mux_video_pad_setcaps (GstFlvMuxPad * pad,
    GstCaps * caps);
static gboolean gst_flv_mux_audio_pad_setcaps (GstFlvMuxPad * pad,
    GstCaps * caps);

static void gst_flv_mux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_flv_mux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_flv_mux_finalize (GObject * object);

static void gst_flv_mux_reset (GstElement * element);
static void gst_flv_mux_reset_pad (GstFlvMuxPad * pad);

static void gst_flv_mux_pad_finalize (GObject * object);

static gboolean gst_flv_mux_start (GstAggregator * aggregator);
static GstFlowReturn gst_flv_mux_flush (GstAggregator * aggregator);
static GstClockTime gst_flv_mux_get_next_time (GstAggregator * aggregator);
static GstFlowReturn gst_flv_mux_write_eos (GstFlvMux * mux);
static GstFlowReturn gst_flv_mux_write_header (GstFlvMux * mux);
static GstFlowReturn gst_flv_mux_rewrite_header (GstFlvMux * mux);
static gboolean gst_flv_mux_are_all_pads_eos (GstFlvMux * mux);
static GstFlowReturn gst_flv_mux_update_src_caps (GstAggregator * aggregator,
    GstCaps * caps, GstCaps ** ret);

static GstFlowReturn
gst_flv_mux_pad_flush (GstAggregatorPad * pad, GstAggregator * aggregator)
{
  GstFlvMuxPad *flvpad = GST_FLV_MUX_PAD (pad);

  flvpad->last_timestamp = 0;
  flvpad->pts = GST_CLOCK_STIME_NONE;
  flvpad->dts = GST_CLOCK_STIME_NONE;

  return GST_FLOW_OK;
}

static void
gst_flv_mux_pad_class_init (GstFlvMuxPadClass * klass)
{
  GstAggregatorPadClass *aggregatorpad_class = (GstAggregatorPadClass *) klass;
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->finalize = gst_flv_mux_pad_finalize;

  aggregatorpad_class->flush = GST_DEBUG_FUNCPTR (gst_flv_mux_pad_flush);
}

static void
gst_flv_mux_pad_init (GstFlvMuxPad * pad)
{
  gst_flv_mux_reset_pad (pad);
}

typedef struct
{
  gdouble position;
  gdouble time;
} GstFlvMuxIndexEntry;

static void
gst_flv_mux_index_entry_free (GstFlvMuxIndexEntry * entry)
{
  g_slice_free (GstFlvMuxIndexEntry, entry);
}

static GstBuffer *
_gst_buffer_new_wrapped (gpointer mem, gsize size, GFreeFunc free_func)
{
  GstBuffer *buf;

  buf = gst_buffer_new ();
  gst_buffer_append_memory (buf,
      gst_memory_new_wrapped (free_func ? 0 : GST_MEMORY_FLAG_READONLY,
          mem, size, 0, size, mem, free_func));

  return buf;
}

static void
_gst_buffer_new_and_alloc (gsize size, GstBuffer ** buffer, guint8 ** data)
{
  g_return_if_fail (data != NULL);
  g_return_if_fail (buffer != NULL);

  *data = g_malloc (size);
  *buffer = _gst_buffer_new_wrapped (*data, size, g_free);
}

static void
gst_flv_mux_class_init (GstFlvMuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstAggregatorClass *gstaggregator_class;

  GST_DEBUG_CATEGORY_INIT (flvmux_debug, "flvmux", 0, "FLV muxer");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstaggregator_class = (GstAggregatorClass *) klass;

  gobject_class->get_property = gst_flv_mux_get_property;
  gobject_class->set_property = gst_flv_mux_set_property;
  gobject_class->finalize = gst_flv_mux_finalize;

  /* FIXME: ideally the right mode of operation should be detected
   * automatically using queries when parameter not specified. */
  /**
   * GstFlvMux:streamable
   *
   * If True, the output will be streaming friendly. (ie without indexes and
   * duration)
   */
  g_object_class_install_property (gobject_class, PROP_STREAMABLE,
      g_param_spec_boolean ("streamable", "streamable",
          "If set to true, the output should be as if it is to be streamed "
          "and hence no indexes written or duration written.",
          DEFAULT_STREAMABLE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_METADATACREATOR,
      g_param_spec_string ("metadatacreator", "metadatacreator",
          "The value of metadatacreator in the meta packet.",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ENCODER,
      g_param_spec_string ("encoder", "encoder",
          "The value of encoder in the meta packet.",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstaggregator_class->create_new_pad =
      GST_DEBUG_FUNCPTR (gst_flv_mux_create_new_pad);
  gstelement_class->release_pad = GST_DEBUG_FUNCPTR (gst_flv_mux_release_pad);

  gstaggregator_class->start = GST_DEBUG_FUNCPTR (gst_flv_mux_start);
  gstaggregator_class->aggregate = GST_DEBUG_FUNCPTR (gst_flv_mux_aggregate);
  gstaggregator_class->sink_event = GST_DEBUG_FUNCPTR (gst_flv_mux_sink_event);
  gstaggregator_class->flush = GST_DEBUG_FUNCPTR (gst_flv_mux_flush);
  gstaggregator_class->get_next_time =
      GST_DEBUG_FUNCPTR (gst_flv_mux_get_next_time);
  gstaggregator_class->update_src_caps =
      GST_DEBUG_FUNCPTR (gst_flv_mux_update_src_caps);

  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &videosink_templ, GST_TYPE_FLV_MUX_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &audiosink_templ, GST_TYPE_FLV_MUX_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &src_templ, GST_TYPE_AGGREGATOR_PAD);
  gst_element_class_set_static_metadata (gstelement_class, "FLV muxer",
      "Codec/Muxer",
      "Muxes video/audio streams into a FLV stream",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  GST_DEBUG_CATEGORY_INIT (flvmux_debug, "flvmux", 0, "FLV muxer");
}

static void
gst_flv_mux_init (GstFlvMux * mux)
{
  mux->srcpad = GST_AGGREGATOR_CAST (mux)->srcpad;

  /* property */
  mux->streamable = DEFAULT_STREAMABLE;
  mux->metadatacreator = g_strdup (DEFAULT_METADATACREATOR);
  mux->encoder = g_strdup (DEFAULT_METADATACREATOR);

  mux->new_tags = FALSE;

  gst_flv_mux_reset (GST_ELEMENT (mux));
}

static void
gst_flv_mux_finalize (GObject * object)
{
  GstFlvMux *mux = GST_FLV_MUX (object);

  gst_flv_mux_reset (GST_ELEMENT (object));
  g_free (mux->metadatacreator);
  g_free (mux->encoder);

  G_OBJECT_CLASS (gst_flv_mux_parent_class)->finalize (object);
}

static void
gst_flv_mux_pad_finalize (GObject * object)
{
  GstFlvMuxPad *pad = GST_FLV_MUX_PAD (object);

  gst_flv_mux_reset_pad (pad);

  G_OBJECT_CLASS (gst_flv_mux_pad_parent_class)->finalize (object);
}

static GstFlowReturn
gst_flv_mux_flush (GstAggregator * aggregator)
{
  /* TODO: What is the right behaviour on flush? Should we just ignore it ?
   * This still needs to be defined. */

  gst_flv_mux_reset (GST_ELEMENT (aggregator));
  return GST_FLOW_OK;
}

static gboolean
gst_flv_mux_start (GstAggregator * aggregator)
{
  gst_flv_mux_reset (GST_ELEMENT (aggregator));
  return TRUE;
}

static void
gst_flv_mux_reset (GstElement * element)
{
  GstFlvMux *mux = GST_FLV_MUX (element);

  g_list_foreach (mux->index, (GFunc) gst_flv_mux_index_entry_free, NULL);
  g_list_free (mux->index);
  mux->index = NULL;
  mux->byte_count = 0;

  mux->duration = GST_CLOCK_TIME_NONE;
  mux->new_tags = FALSE;
  mux->first_timestamp = GST_CLOCK_STIME_NONE;
  mux->last_dts = 0;

  mux->state = GST_FLV_MUX_STATE_HEADER;
  mux->sent_header = FALSE;

  /* tags */
  gst_tag_setter_reset_tags (GST_TAG_SETTER (mux));
}

/* Extract per-codec relevant tags for
 * insertion into the metadata later - ie bitrate,
 * but maybe others in the future */
static void
gst_flv_mux_store_codec_tags (GstFlvMux * mux,
    GstFlvMuxPad * flvpad, GstTagList * list)
{
  /* Look for a bitrate as either nominal or actual bitrate tag */
  if (gst_tag_list_get_uint (list, GST_TAG_NOMINAL_BITRATE, &flvpad->bitrate)
      || gst_tag_list_get_uint (list, GST_TAG_BITRATE, &flvpad->bitrate)) {
    GST_DEBUG_OBJECT (mux, "Stored bitrate for pad %" GST_PTR_FORMAT " = %u",
        flvpad, flvpad->bitrate);
  }
}

static gboolean
gst_flv_mux_sink_event (GstAggregator * aggregator, GstAggregatorPad * pad,
    GstEvent * event)
{
  GstFlvMux *mux = GST_FLV_MUX (aggregator);
  GstFlvMuxPad *flvpad = (GstFlvMuxPad *) pad;
  gboolean ret = TRUE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);

      if (mux->video_pad == flvpad) {
        ret = gst_flv_mux_video_pad_setcaps (flvpad, caps);
      } else if (mux->audio_pad == flvpad) {
        ret = gst_flv_mux_audio_pad_setcaps (flvpad, caps);
      } else {
        g_assert_not_reached ();
      }
      break;
    }
    case GST_EVENT_TAG:{
      GstTagList *list;
      GstTagSetter *setter = GST_TAG_SETTER (mux);
      const GstTagMergeMode mode = gst_tag_setter_get_tag_merge_mode (setter);

      gst_event_parse_tag (event, &list);
      gst_tag_setter_merge_tags (setter, list, mode);
      gst_flv_mux_store_codec_tags (mux, flvpad, list);
      mux->new_tags = TRUE;
      ret = TRUE;
      break;
    }
    default:
      break;
  }

  if (!ret)
    return FALSE;

  return GST_AGGREGATOR_CLASS (parent_class)->sink_event (aggregator, pad,
      event);;
}

static gboolean
gst_flv_mux_video_pad_setcaps (GstFlvMuxPad * pad, GstCaps * caps)
{
  GstFlvMux *mux = GST_FLV_MUX (gst_pad_get_parent (pad));
  gboolean ret = TRUE;
  GstStructure *s;
  guint old_codec;
  GstBuffer *old_codec_data = NULL;

  old_codec = pad->codec;
  if (pad->codec_data)
    old_codec_data = gst_buffer_ref (pad->codec_data);

  s = gst_caps_get_structure (caps, 0);

  if (strcmp (gst_structure_get_name (s), "video/x-flash-video") == 0) {
    pad->codec = 2;
  } else if (strcmp (gst_structure_get_name (s), "video/x-flash-screen") == 0) {
    pad->codec = 3;
  } else if (strcmp (gst_structure_get_name (s), "video/x-vp6-flash") == 0) {
    pad->codec = 4;
  } else if (strcmp (gst_structure_get_name (s), "video/x-vp6-alpha") == 0) {
    pad->codec = 5;
  } else if (strcmp (gst_structure_get_name (s), "video/x-h264") == 0) {
    pad->codec = 7;
  } else {
    ret = FALSE;
  }

  if (ret && gst_structure_has_field (s, "codec_data")) {
    const GValue *val = gst_structure_get_value (s, "codec_data");

    if (val)
      gst_buffer_replace (&pad->codec_data, gst_value_get_buffer (val));
    else if (!val && pad->codec_data)
      gst_buffer_unref (pad->codec_data);
  }

  if (ret && mux->streamable && mux->state != GST_FLV_MUX_STATE_HEADER) {
    if (old_codec != pad->codec) {
      pad->info_changed = TRUE;
    }

    if (old_codec_data && pad->codec_data) {
      GstMapInfo map;

      gst_buffer_map (old_codec_data, &map, GST_MAP_READ);
      if (map.size != gst_buffer_get_size (pad->codec_data) ||
          gst_buffer_memcmp (pad->codec_data, 0, map.data, map.size))
        pad->info_changed = TRUE;

      gst_buffer_unmap (old_codec_data, &map);
    } else if (!old_codec_data && pad->codec_data) {
      pad->info_changed = TRUE;
    }

    if (pad->info_changed)
      mux->state = GST_FLV_MUX_STATE_HEADER;
  }

  if (old_codec_data)
    gst_buffer_unref (old_codec_data);

  gst_object_unref (mux);

  return ret;
}

static gboolean
gst_flv_mux_audio_pad_setcaps (GstFlvMuxPad * pad, GstCaps * caps)
{
  GstFlvMux *mux = GST_FLV_MUX (gst_pad_get_parent (pad));
  gboolean ret = TRUE;
  GstStructure *s;
  guint old_codec, old_rate, old_width, old_channels;
  GstBuffer *old_codec_data = NULL;

  old_codec = pad->codec;
  old_rate = pad->rate;
  old_width = pad->width;
  old_channels = pad->channels;
  if (pad->codec_data)
    old_codec_data = gst_buffer_ref (pad->codec_data);

  s = gst_caps_get_structure (caps, 0);

  if (strcmp (gst_structure_get_name (s), "audio/x-adpcm") == 0) {
    const gchar *layout = gst_structure_get_string (s, "layout");
    if (layout && strcmp (layout, "swf") == 0) {
      pad->codec = 1;
    } else {
      ret = FALSE;
    }
  } else if (strcmp (gst_structure_get_name (s), "audio/mpeg") == 0) {
    gint mpegversion;

    if (gst_structure_get_int (s, "mpegversion", &mpegversion)) {
      if (mpegversion == 1) {
        gint layer;

        if (gst_structure_get_int (s, "layer", &layer) && layer == 3) {
          gint rate;

          if (gst_structure_get_int (s, "rate", &rate) && rate == 8000)
            pad->codec = 14;
          else
            pad->codec = 2;
        } else {
          ret = FALSE;
        }
      } else if (mpegversion == 4 || mpegversion == 2) {
        pad->codec = 10;
      } else {
        ret = FALSE;
      }
    } else {
      ret = FALSE;
    }
  } else if (strcmp (gst_structure_get_name (s), "audio/x-nellymoser") == 0) {
    gint rate, channels;

    if (gst_structure_get_int (s, "rate", &rate)
        && gst_structure_get_int (s, "channels", &channels)) {
      if (channels == 1 && rate == 16000)
        pad->codec = 4;
      else if (channels == 1 && rate == 8000)
        pad->codec = 5;
      else
        pad->codec = 6;
    } else {
      pad->codec = 6;
    }
  } else if (strcmp (gst_structure_get_name (s), "audio/x-raw") == 0) {
    GstAudioInfo info;

    if (gst_audio_info_from_caps (&info, caps)) {
      pad->codec = 3;

      if (GST_AUDIO_INFO_WIDTH (&info) == 8)
        pad->width = 0;
      else if (GST_AUDIO_INFO_WIDTH (&info) == 16)
        pad->width = 1;
      else
        ret = FALSE;
    } else
      ret = FALSE;
  } else if (strcmp (gst_structure_get_name (s), "audio/x-alaw") == 0) {
    pad->codec = 7;
  } else if (strcmp (gst_structure_get_name (s), "audio/x-mulaw") == 0) {
    pad->codec = 8;
  } else if (strcmp (gst_structure_get_name (s), "audio/x-speex") == 0) {
    pad->codec = 11;
  } else {
    ret = FALSE;
  }

  if (ret) {
    gint rate, channels;

    if (gst_structure_get_int (s, "rate", &rate)) {
      if (pad->codec == 10)
        pad->rate = 3;
      else if (rate == 5512)
        pad->rate = 0;
      else if (rate == 11025)
        pad->rate = 1;
      else if (rate == 22050)
        pad->rate = 2;
      else if (rate == 44100)
        pad->rate = 3;
      else if (rate == 8000 && (pad->codec == 5 || pad->codec == 14
              || pad->codec == 7 || pad->codec == 8))
        pad->rate = 0;
      else if (rate == 16000 && (pad->codec == 4 || pad->codec == 11))
        pad->rate = 0;
      else
        ret = FALSE;
    } else if (pad->codec == 10) {
      pad->rate = 3;
    } else {
      ret = FALSE;
    }

    if (gst_structure_get_int (s, "channels", &channels)) {
      if (pad->codec == 4 || pad->codec == 5
          || pad->codec == 6 || pad->codec == 11)
        pad->channels = 0;
      else if (pad->codec == 10)
        pad->channels = 1;
      else if (channels == 1)
        pad->channels = 0;
      else if (channels == 2)
        pad->channels = 1;
      else
        ret = FALSE;
    } else if (pad->codec == 4 || pad->codec == 5 || pad->codec == 6) {
      pad->channels = 0;
    } else if (pad->codec == 10) {
      pad->channels = 1;
    } else {
      ret = FALSE;
    }

    if (pad->codec != 3)
      pad->width = 1;
  }

  if (ret && gst_structure_has_field (s, "codec_data")) {
    const GValue *val = gst_structure_get_value (s, "codec_data");

    if (val)
      gst_buffer_replace (&pad->codec_data, gst_value_get_buffer (val));
    else if (!val && pad->codec_data)
      gst_buffer_unref (pad->codec_data);
  }

  if (ret && mux->streamable && mux->state != GST_FLV_MUX_STATE_HEADER) {
    if (old_codec != pad->codec || old_rate != pad->rate ||
        old_width != pad->width || old_channels != pad->channels) {
      pad->info_changed = TRUE;
    }

    if (old_codec_data && pad->codec_data) {
      GstMapInfo map;

      gst_buffer_map (old_codec_data, &map, GST_MAP_READ);
      if (map.size != gst_buffer_get_size (pad->codec_data) ||
          gst_buffer_memcmp (pad->codec_data, 0, map.data, map.size))
        pad->info_changed = TRUE;

      gst_buffer_unmap (old_codec_data, &map);
    } else if (!old_codec_data && pad->codec_data) {
      pad->info_changed = TRUE;
    }

    if (pad->info_changed)
      mux->state = GST_FLV_MUX_STATE_HEADER;
  }

  if (old_codec_data)
    gst_buffer_unref (old_codec_data);

  gst_object_unref (mux);

  return ret;
}

static void
gst_flv_mux_reset_pad (GstFlvMuxPad * pad)
{
  GST_DEBUG_OBJECT (pad, "resetting pad");

  if (pad->codec_data)
    gst_buffer_unref (pad->codec_data);
  pad->codec_data = NULL;
  pad->codec = G_MAXUINT;
  pad->rate = G_MAXUINT;
  pad->width = G_MAXUINT;
  pad->channels = G_MAXUINT;
  pad->info_changed = FALSE;

  gst_flv_mux_pad_flush (GST_AGGREGATOR_PAD_CAST (pad), NULL);
}

static GstAggregatorPad *
gst_flv_mux_create_new_pad (GstAggregator * agg,
    GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (agg);
  GstAggregatorPad *aggpad;
  GstFlvMux *mux = GST_FLV_MUX (agg);
  GstFlvMuxPad *pad = NULL;
  const gchar *name = NULL;
  gboolean video;

  if (mux->state != GST_FLV_MUX_STATE_HEADER) {
    GST_WARNING_OBJECT (mux, "Can't request pads after writing header");
    return NULL;
  }

  if (templ == gst_element_class_get_pad_template (klass, "audio")) {
    if (mux->audio_pad) {
      GST_WARNING_OBJECT (mux, "Already have an audio pad");
      return NULL;
    }
    name = "audio";
    video = FALSE;
  } else if (templ == gst_element_class_get_pad_template (klass, "video")) {
    if (mux->video_pad) {
      GST_WARNING_OBJECT (mux, "Already have a video pad");
      return NULL;
    }
    name = "video";
    video = TRUE;
  } else {
    GST_WARNING_OBJECT (mux, "Invalid template");
    return NULL;
  }

  aggpad =
      GST_AGGREGATOR_CLASS (gst_flv_mux_parent_class)->create_new_pad (agg,
      templ, name, caps);
  if (aggpad == NULL)
    return NULL;

  pad = GST_FLV_MUX_PAD (aggpad);

  gst_flv_mux_reset_pad (pad);

  if (video)
    mux->video_pad = pad;
  else
    mux->audio_pad = pad;

  return aggpad;
}

static void
gst_flv_mux_release_pad (GstElement * element, GstPad * pad)
{
  GstFlvMux *mux = GST_FLV_MUX (element);
  GstFlvMuxPad *flvpad = GST_FLV_MUX_PAD (pad);

  gst_pad_set_active (pad, FALSE);
  gst_flv_mux_reset_pad (flvpad);

  if (flvpad == mux->video_pad) {
    mux->video_pad = NULL;
  } else if (flvpad == mux->audio_pad) {
    mux->audio_pad = NULL;
  } else {
    GST_WARNING_OBJECT (pad, "Pad is not known audio or video pad");
  }

  gst_element_remove_pad (element, pad);
}

static GstFlowReturn
gst_flv_mux_push (GstFlvMux * mux, GstBuffer * buffer)
{
  GstAggregator *agg = GST_AGGREGATOR (mux);
  GstAggregatorPad *srcpad = GST_AGGREGATOR_PAD (agg->srcpad);

  if (GST_BUFFER_PTS_IS_VALID (buffer))
    srcpad->segment.position = GST_BUFFER_PTS (buffer);

  /* pushing the buffer that rewrites the header will make it no longer be the
   * total output size in bytes, but it doesn't matter at that point */
  mux->byte_count += gst_buffer_get_size (buffer);

  return gst_aggregator_finish_buffer (GST_AGGREGATOR_CAST (mux), buffer);
}

static GstBuffer *
gst_flv_mux_create_header (GstFlvMux * mux)
{
  GstBuffer *header;
  guint8 *data;
  gboolean have_audio;
  gboolean have_video;

  _gst_buffer_new_and_alloc (9 + 4, &header, &data);

  data[0] = 'F';
  data[1] = 'L';
  data[2] = 'V';
  data[3] = 0x01;               /* Version */

  have_audio = (mux->audio_pad && mux->audio_pad->codec != G_MAXUINT);
  have_video = (mux->video_pad && mux->video_pad->codec != G_MAXUINT);

  data[4] = (have_audio << 2) | have_video;     /* flags */
  GST_WRITE_UINT32_BE (data + 5, 9);    /* data offset */
  GST_WRITE_UINT32_BE (data + 9, 0);    /* previous tag size */

  return header;
}

static GstBuffer *
gst_flv_mux_preallocate_index (GstFlvMux * mux)
{
  GstBuffer *tmp;
  guint8 *data;
  gint preallocate_size;

  /* preallocate index of size:
   *  - 'keyframes' ECMA array key: 2 + 9 = 11 bytes
   *  - nested ECMA array header, length and end marker: 8 bytes
   *  - 'times' and 'filepositions' keys: 22 bytes
   *  - two strict arrays headers and lengths: 10 bytes
   *  - each index entry: 18 bytes
   */
  preallocate_size = 11 + 8 + 22 + 10 + MAX_INDEX_ENTRIES * 18;
  GST_DEBUG_OBJECT (mux, "preallocating %d bytes for the index",
      preallocate_size);

  _gst_buffer_new_and_alloc (preallocate_size, &tmp, &data);

  /* prefill the space with a gstfiller: <spaces> script tag variable */
  GST_WRITE_UINT16_BE (data, 9);        /* 9 characters */
  memcpy (data + 2, "gstfiller", 9);
  GST_WRITE_UINT8 (data + 11, AMF0_STRING_MARKER);      /* a string value */
  GST_WRITE_UINT16_BE (data + 12, preallocate_size - 14);
  memset (data + 14, ' ', preallocate_size - 14);       /* the rest is spaces */
  return tmp;
}

static GstBuffer *
gst_flv_mux_create_number_script_value (const gchar * name, gdouble value)
{
  GstBuffer *tmp;
  guint8 *data;
  gsize len = strlen (name);

  _gst_buffer_new_and_alloc (2 + len + 1 + 8, &tmp, &data);

  GST_WRITE_UINT16_BE (data, len);
  data += 2;                    /* name length */
  memcpy (data, name, len);
  data += len;
  *data++ = AMF0_NUMBER_MARKER; /* double type */
  GST_WRITE_DOUBLE_BE (data, value);

  return tmp;
}

static GstBuffer *
gst_flv_mux_create_metadata (GstFlvMux * mux)
{
  const GstTagList *tags;
  GstBuffer *script_tag, *tmp;
  GstMapInfo map;
  guint32 dts;
  guint8 *data;
  gint i, n_tags, tags_written = 0;

  tags = gst_tag_setter_get_tag_list (GST_TAG_SETTER (mux));

  dts = mux->last_dts;

  /* Timestamp must start at zero */
  if (GST_CLOCK_STIME_IS_VALID (mux->first_timestamp)) {
    dts -= mux->first_timestamp / GST_MSECOND;
  }

  GST_DEBUG_OBJECT (mux, "Creating metadata, dts %i, tags = %" GST_PTR_FORMAT,
      dts, tags);

  /* FIXME perhaps some bytewriter'ing here ... */

  _gst_buffer_new_and_alloc (11, &script_tag, &data);

  data[0] = 18;

  /* Data size, unknown for now */
  data[1] = 0;
  data[2] = 0;
  data[3] = 0;

  /* Timestamp */
  GST_WRITE_UINT24_BE (data + 4, dts);
  data[7] = (((guint) dts) >> 24) & 0xff;

  /* Stream ID */
  data[8] = data[9] = data[10] = 0;

  _gst_buffer_new_and_alloc (13, &tmp, &data);
  data[0] = AMF0_STRING_MARKER; /* string */
  data[1] = 0;
  data[2] = 10;                 /* length 10 */
  memcpy (&data[3], "onMetaData", 10);

  script_tag = gst_buffer_append (script_tag, tmp);

  n_tags = (tags) ? gst_tag_list_n_tags (tags) : 0;
  _gst_buffer_new_and_alloc (5, &tmp, &data);
  data[0] = 8;                  /* ECMA array */
  GST_WRITE_UINT32_BE (data + 1, n_tags);
  script_tag = gst_buffer_append (script_tag, tmp);

  /* Some players expect the 'duration' to be always set. Fill it out later,
     after querying the pads or after getting EOS */
  if (!mux->streamable) {
    tmp = gst_flv_mux_create_number_script_value ("duration", 86400);
    script_tag = gst_buffer_append (script_tag, tmp);
    tags_written++;

    /* Sometimes the information about the total file size is useful for the
       player. It will be filled later, after getting EOS */
    tmp = gst_flv_mux_create_number_script_value ("filesize", 0);
    script_tag = gst_buffer_append (script_tag, tmp);
    tags_written++;

    /* Preallocate space for the index to be written at EOS */
    tmp = gst_flv_mux_preallocate_index (mux);
    script_tag = gst_buffer_append (script_tag, tmp);
  } else {
    GST_DEBUG_OBJECT (mux, "not preallocating index, streamable mode");
  }

  for (i = 0; tags && i < n_tags; i++) {
    const gchar *tag_name = gst_tag_list_nth_tag_name (tags, i);
    if (!strcmp (tag_name, GST_TAG_DURATION)) {
      guint64 dur;

      if (!gst_tag_list_get_uint64 (tags, GST_TAG_DURATION, &dur))
        continue;
      mux->duration = dur;
    } else if (!strcmp (tag_name, GST_TAG_ARTIST) ||
        !strcmp (tag_name, GST_TAG_TITLE)) {
      gchar *s;
      const gchar *t = NULL;

      if (!strcmp (tag_name, GST_TAG_ARTIST))
        t = "creator";
      else if (!strcmp (tag_name, GST_TAG_TITLE))
        t = "title";

      if (!gst_tag_list_get_string (tags, tag_name, &s))
        continue;

      _gst_buffer_new_and_alloc (2 + strlen (t) + 1 + 2 + strlen (s),
          &tmp, &data);
      data[0] = 0;              /* tag name length */
      data[1] = strlen (t);
      memcpy (&data[2], t, strlen (t));
      data[2 + strlen (t)] = 2; /* string */
      data[3 + strlen (t)] = (strlen (s) >> 8) & 0xff;
      data[4 + strlen (t)] = (strlen (s)) & 0xff;
      memcpy (&data[5 + strlen (t)], s, strlen (s));
      script_tag = gst_buffer_append (script_tag, tmp);

      g_free (s);
      tags_written++;
    }
  }

  if (!mux->streamable && mux->duration == GST_CLOCK_TIME_NONE) {
    GList *l;
    guint64 dur;

    for (l = GST_ELEMENT_CAST (mux)->sinkpads; l; l = l->next) {
      GstFlvMuxPad *pad = GST_FLV_MUX_PAD (l->data);

      if (gst_pad_peer_query_duration (GST_PAD (pad), GST_FORMAT_TIME,
              (gint64 *) & dur) && dur != GST_CLOCK_TIME_NONE) {
        if (mux->duration == GST_CLOCK_TIME_NONE)
          mux->duration = dur;
        else
          mux->duration = MAX (dur, mux->duration);
      }
    }
  }

  if (!mux->streamable && mux->duration != GST_CLOCK_TIME_NONE) {
    gdouble d;
    GstMapInfo map;

    d = gst_guint64_to_gdouble (mux->duration);
    d /= (gdouble) GST_SECOND;

    GST_DEBUG_OBJECT (mux, "determined the duration to be %f", d);
    gst_buffer_map (script_tag, &map, GST_MAP_WRITE);
    GST_WRITE_DOUBLE_BE (map.data + 29 + 2 + 8 + 1, d);
    gst_buffer_unmap (script_tag, &map);
  }

  if (mux->video_pad && mux->video_pad->codec != G_MAXUINT) {
    GstCaps *caps = NULL;

    if (mux->video_pad)
      caps = gst_pad_get_current_caps (GST_PAD (mux->video_pad));

    if (caps != NULL) {
      GstStructure *s;
      gint size;
      gint num, den;

      GST_DEBUG_OBJECT (mux, "putting videocodecid %d in the metadata",
          mux->video_pad->codec);

      tmp = gst_flv_mux_create_number_script_value ("videocodecid",
          mux->video_pad->codec);
      script_tag = gst_buffer_append (script_tag, tmp);
      tags_written++;

      s = gst_caps_get_structure (caps, 0);
      gst_caps_unref (caps);

      if (gst_structure_get_int (s, "width", &size)) {
        GST_DEBUG_OBJECT (mux, "putting width %d in the metadata", size);

        tmp = gst_flv_mux_create_number_script_value ("width", size);
        script_tag = gst_buffer_append (script_tag, tmp);
        tags_written++;
      }

      if (gst_structure_get_int (s, "height", &size)) {
        GST_DEBUG_OBJECT (mux, "putting height %d in the metadata", size);

        tmp = gst_flv_mux_create_number_script_value ("height", size);
        script_tag = gst_buffer_append (script_tag, tmp);
        tags_written++;
      }

      if (gst_structure_get_fraction (s, "pixel-aspect-ratio", &num, &den)) {
        gdouble d;

        d = num;
        GST_DEBUG_OBJECT (mux, "putting AspectRatioX %f in the metadata", d);

        tmp = gst_flv_mux_create_number_script_value ("AspectRatioX", d);
        script_tag = gst_buffer_append (script_tag, tmp);
        tags_written++;

        d = den;
        GST_DEBUG_OBJECT (mux, "putting AspectRatioY %f in the metadata", d);

        tmp = gst_flv_mux_create_number_script_value ("AspectRatioY", d);
        script_tag = gst_buffer_append (script_tag, tmp);
        tags_written++;
      }

      if (gst_structure_get_fraction (s, "framerate", &num, &den)) {
        gdouble d;

        gst_util_fraction_to_double (num, den, &d);
        GST_DEBUG_OBJECT (mux, "putting framerate %f in the metadata", d);

        tmp = gst_flv_mux_create_number_script_value ("framerate", d);
        script_tag = gst_buffer_append (script_tag, tmp);
        tags_written++;
      }

      GST_DEBUG_OBJECT (mux, "putting videodatarate %u KB/s in the metadata",
          mux->video_pad->bitrate / 1024);
      tmp = gst_flv_mux_create_number_script_value ("videodatarate",
          mux->video_pad->bitrate / 1024);
      script_tag = gst_buffer_append (script_tag, tmp);
      tags_written++;
    }
  }

  if (mux->audio_pad && mux->audio_pad->codec != G_MAXUINT) {
    GST_DEBUG_OBJECT (mux, "putting audiocodecid %d in the metadata",
        mux->audio_pad->codec);

    tmp = gst_flv_mux_create_number_script_value ("audiocodecid",
        mux->audio_pad->codec);
    script_tag = gst_buffer_append (script_tag, tmp);
    tags_written++;

    GST_DEBUG_OBJECT (mux, "putting audiodatarate %u KB/s in the metadata",
        mux->audio_pad->bitrate / 1024);
    tmp = gst_flv_mux_create_number_script_value ("audiodatarate",
        mux->audio_pad->bitrate / 1024);
    script_tag = gst_buffer_append (script_tag, tmp);
    tags_written++;
  }

  _gst_buffer_new_and_alloc (2 + 15 + 1 + 2 + strlen (mux->metadatacreator),
      &tmp, &data);
  data[0] = 0;                  /* 15 bytes name */
  data[1] = 15;
  memcpy (&data[2], "metadatacreator", 15);
  data[17] = 2;                 /* string */
  data[18] = (strlen (mux->metadatacreator) >> 8) & 0xff;
  data[19] = (strlen (mux->metadatacreator)) & 0xff;
  memcpy (&data[20], mux->metadatacreator, strlen (mux->metadatacreator));
  script_tag = gst_buffer_append (script_tag, tmp);
  tags_written++;

  _gst_buffer_new_and_alloc (2 + 7 + 1 + 2 + strlen (mux->encoder),
      &tmp, &data);
  data[0] = 0;                  /* 7 bytes name */
  data[1] = 7;
  memcpy (&data[2], "encoder", 7);
  data[9] = 2;                  /* string */
  data[10] = (strlen (mux->encoder) >> 8) & 0xff;
  data[11] = (strlen (mux->encoder)) & 0xff;
  memcpy (&data[12], mux->encoder, strlen (mux->encoder));
  script_tag = gst_buffer_append (script_tag, tmp);
  tags_written++;

  {
    time_t secs;
    struct tm tm;
    gchar *s;
    static const gchar *weekdays[] = {
      "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const gchar *months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
      "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    secs = g_get_real_time () / G_USEC_PER_SEC;
#ifdef HAVE_GMTIME_R
    gmtime_r (&secs, &tm);
#else
    tm = *gmtime (&secs);
#endif

    s = g_strdup_printf ("%s %s %d %02d:%02d:%02d %d", weekdays[tm.tm_wday],
        months[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
        tm.tm_year + 1900);

    _gst_buffer_new_and_alloc (2 + 12 + 1 + 2 + strlen (s), &tmp, &data);
    data[0] = 0;                /* 12 bytes name */
    data[1] = 12;
    memcpy (&data[2], "creationdate", 12);
    data[14] = 2;               /* string */
    data[15] = (strlen (s) >> 8) & 0xff;
    data[16] = (strlen (s)) & 0xff;
    memcpy (&data[17], s, strlen (s));
    script_tag = gst_buffer_append (script_tag, tmp);

    g_free (s);
    tags_written++;
  }

  if (!tags_written) {
    gst_buffer_unref (script_tag);
    script_tag = NULL;
    goto exit;
  }

  _gst_buffer_new_and_alloc (2 + 0 + 1, &tmp, &data);
  data[0] = 0;                  /* 0 byte size */
  data[1] = 0;
  data[2] = 9;                  /* end marker */
  script_tag = gst_buffer_append (script_tag, tmp);


  _gst_buffer_new_and_alloc (4, &tmp, &data);
  GST_WRITE_UINT32_BE (data, gst_buffer_get_size (script_tag));
  script_tag = gst_buffer_append (script_tag, tmp);

  gst_buffer_map (script_tag, &map, GST_MAP_WRITE);
  map.data[1] = ((gst_buffer_get_size (script_tag) - 11 - 4) >> 16) & 0xff;
  map.data[2] = ((gst_buffer_get_size (script_tag) - 11 - 4) >> 8) & 0xff;
  map.data[3] = ((gst_buffer_get_size (script_tag) - 11 - 4) >> 0) & 0xff;

  GST_WRITE_UINT32_BE (map.data + 11 + 13 + 1, tags_written);
  gst_buffer_unmap (script_tag, &map);

exit:
  return script_tag;
}

static GstBuffer *
gst_flv_mux_buffer_to_tag_internal (GstFlvMux * mux, GstBuffer * buffer,
    GstFlvMuxPad * pad, gboolean is_codec_data)
{
  GstBuffer *tag;
  GstMapInfo map;
  guint size;
  guint32 pts, dts, cts;
  guint8 *data, *bdata = NULL;
  gsize bsize = 0;

  if (!GST_CLOCK_STIME_IS_VALID (pad->dts)) {
    pts = dts = pad->last_timestamp / GST_MSECOND;
  } else {
    pts = pad->pts / GST_MSECOND;
    dts = pad->dts / GST_MSECOND;
  }

  /* We prevent backwards timestamps because they confuse librtmp,
   * it expects timestamps to go forward not only inside one stream, but
   * also between the audio & video streams.
   */
  if (dts < mux->last_dts) {
    GST_WARNING_OBJECT (pad, "Got backwards dts! (%" GST_TIME_FORMAT
        " < %" GST_TIME_FORMAT ")", GST_TIME_ARGS (dts * GST_MSECOND),
        GST_TIME_ARGS (mux->last_dts * GST_MSECOND));
    dts = mux->last_dts;
  }
  mux->last_dts = dts;


  /* Be safe in case TS are buggy */
  if (pts > dts)
    cts = pts - dts;
  else
    cts = 0;

  /* Timestamp must start at zero */
  if (GST_CLOCK_STIME_IS_VALID (mux->first_timestamp)) {
    dts -= mux->first_timestamp / GST_MSECOND;
    pts = dts + cts;
  }

  GST_LOG_OBJECT (mux, "got pts %i dts %i cts %i", pts, dts, cts);

  if (buffer != NULL) {
    gst_buffer_map (buffer, &map, GST_MAP_READ);
    bdata = map.data;
    bsize = map.size;
  }

  size = 11;
  if (mux->video_pad == pad) {
    size += 1;
    if (pad->codec == 7)
      size += 4 + bsize;
    else
      size += bsize;
  } else {
    size += 1;
    if (pad->codec == 10)
      size += 1 + bsize;
    else
      size += bsize;
  }
  size += 4;

  _gst_buffer_new_and_alloc (size, &tag, &data);
  memset (data, 0, size);

  data[0] = (mux->video_pad == pad) ? 9 : 8;

  data[1] = ((size - 11 - 4) >> 16) & 0xff;
  data[2] = ((size - 11 - 4) >> 8) & 0xff;
  data[3] = ((size - 11 - 4) >> 0) & 0xff;


  GST_WRITE_UINT24_BE (data + 4, dts);
  data[7] = (((guint) dts) >> 24) & 0xff;

  data[8] = data[9] = data[10] = 0;

  if (mux->video_pad == pad) {
    if (buffer && GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT))
      data[11] |= 2 << 4;
    else
      data[11] |= 1 << 4;

    data[11] |= pad->codec & 0x0f;

    if (pad->codec == 7) {
      if (is_codec_data) {
        data[12] = 0;
        GST_WRITE_UINT24_BE (data + 13, 0);
      } else if (bsize == 0) {
        /* AVC end of sequence */
        data[12] = 2;
        GST_WRITE_UINT24_BE (data + 13, 0);
      } else {
        /* ACV NALU */
        data[12] = 1;
        GST_WRITE_UINT24_BE (data + 13, cts);
      }
      memcpy (data + 11 + 1 + 4, bdata, bsize);
    } else {
      memcpy (data + 11 + 1, bdata, bsize);
    }
  } else {
    data[11] |= (pad->codec << 4) & 0xf0;
    data[11] |= (pad->rate << 2) & 0x0c;
    data[11] |= (pad->width << 1) & 0x02;
    data[11] |= (pad->channels << 0) & 0x01;

    GST_DEBUG_OBJECT (mux, "Creating byte %02x with "
        "codec:%d, rate:%d, width:%d, channels:%d",
        data[11], pad->codec, pad->rate, pad->width, pad->channels);

    if (pad->codec == 10) {
      data[12] = is_codec_data ? 0 : 1;

      memcpy (data + 11 + 1 + 1, bdata, bsize);
    } else {
      memcpy (data + 11 + 1, bdata, bsize);
    }
  }

  if (buffer)
    gst_buffer_unmap (buffer, &map);

  GST_WRITE_UINT32_BE (data + size - 4, size - 4);

  GST_BUFFER_PTS (tag) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_DTS (tag) = GST_CLOCK_TIME_NONE;
  GST_BUFFER_DURATION (tag) = GST_CLOCK_TIME_NONE;

  if (buffer) {
    /* if we are streamable we copy over timestamps and offsets,
       if not just copy the offsets */
    if (mux->streamable) {
      GstClockTime timestamp = GST_CLOCK_TIME_NONE;

      if (gst_segment_to_running_time_full (&GST_AGGREGATOR_PAD (pad)->segment,
              GST_FORMAT_TIME, GST_BUFFER_DTS_OR_PTS (buffer),
              &timestamp) == 1) {
        GST_BUFFER_PTS (tag) = timestamp;
        GST_BUFFER_DURATION (tag) = GST_BUFFER_DURATION (buffer);
      }
      GST_BUFFER_OFFSET (tag) = GST_BUFFER_OFFSET_NONE;
      GST_BUFFER_OFFSET_END (tag) = GST_BUFFER_OFFSET_NONE;
    } else {
      GST_BUFFER_OFFSET (tag) = GST_BUFFER_OFFSET (buffer);
      GST_BUFFER_OFFSET_END (tag) = GST_BUFFER_OFFSET_END (buffer);
    }

    /* mark the buffer if it's an audio buffer and there's also video being muxed
     * or it's a video interframe */
    if (mux->video_pad == pad &&
        GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT))
      GST_BUFFER_FLAG_SET (tag, GST_BUFFER_FLAG_DELTA_UNIT);
  } else {
    GST_BUFFER_FLAG_SET (tag, GST_BUFFER_FLAG_DELTA_UNIT);
    GST_BUFFER_OFFSET (tag) = GST_BUFFER_OFFSET_END (tag) =
        GST_BUFFER_OFFSET_NONE;
  }

  return tag;
}

static inline GstBuffer *
gst_flv_mux_buffer_to_tag (GstFlvMux * mux, GstBuffer * buffer,
    GstFlvMuxPad * pad)
{
  return gst_flv_mux_buffer_to_tag_internal (mux, buffer, pad, FALSE);
}

static inline GstBuffer *
gst_flv_mux_codec_data_buffer_to_tag (GstFlvMux * mux, GstBuffer * buffer,
    GstFlvMuxPad * pad)
{
  return gst_flv_mux_buffer_to_tag_internal (mux, buffer, pad, TRUE);
}

static inline GstBuffer *
gst_flv_mux_eos_to_tag (GstFlvMux * mux, GstFlvMuxPad * pad)
{
  return gst_flv_mux_buffer_to_tag_internal (mux, NULL, pad, FALSE);
}

static void
gst_flv_mux_put_buffer_in_streamheader (GValue * streamheader,
    GstBuffer * buffer)
{
  GValue value = { 0 };
  GstBuffer *buf;

  g_value_init (&value, GST_TYPE_BUFFER);
  buf = gst_buffer_copy (buffer);
  gst_value_set_buffer (&value, buf);
  gst_buffer_unref (buf);
  gst_value_array_append_value (streamheader, &value);
  g_value_unset (&value);
}

static GstCaps *
gst_flv_mux_prepare_src_caps (GstFlvMux * mux, GstBuffer ** header_buf,
    GstBuffer ** metadata_buf, GstBuffer ** video_codec_data_buf,
    GstBuffer ** audio_codec_data_buf)
{
  GstBuffer *header, *metadata;
  GstBuffer *video_codec_data, *audio_codec_data;
  GstCaps *caps;
  GstStructure *structure;
  GValue streamheader = { 0 };
  GList *l;

  header = gst_flv_mux_create_header (mux);
  metadata = gst_flv_mux_create_metadata (mux);
  video_codec_data = NULL;
  audio_codec_data = NULL;

  for (l = GST_ELEMENT_CAST (mux)->sinkpads; l != NULL; l = l->next) {
    GstFlvMuxPad *pad = l->data;

    /* Get H.264 and AAC codec data, if present */
    if (pad && mux->video_pad == pad && pad->codec == 7) {
      if (pad->codec_data == NULL)
        GST_WARNING_OBJECT (mux, "Codec data for video stream not found, "
            "output might not be playable");
      else
        video_codec_data =
            gst_flv_mux_codec_data_buffer_to_tag (mux, pad->codec_data, pad);
    } else if (pad && mux->audio_pad == pad && pad->codec == 10) {
      if (pad->codec_data == NULL)
        GST_WARNING_OBJECT (mux, "Codec data for audio stream not found, "
            "output might not be playable");
      else
        audio_codec_data =
            gst_flv_mux_codec_data_buffer_to_tag (mux, pad->codec_data, pad);
    }
  }

  /* mark buffers that will go in the streamheader */
  GST_BUFFER_FLAG_SET (header, GST_BUFFER_FLAG_HEADER);
  GST_BUFFER_FLAG_SET (metadata, GST_BUFFER_FLAG_HEADER);
  if (video_codec_data != NULL) {
    GST_BUFFER_FLAG_SET (video_codec_data, GST_BUFFER_FLAG_HEADER);
    /* mark as a delta unit, so downstream will not try to synchronize on that
     * buffer - to actually start playback you need a real video keyframe */
    GST_BUFFER_FLAG_SET (video_codec_data, GST_BUFFER_FLAG_DELTA_UNIT);
  }
  if (audio_codec_data != NULL) {
    GST_BUFFER_FLAG_SET (audio_codec_data, GST_BUFFER_FLAG_HEADER);
  }

  /* put buffers in streamheader */
  g_value_init (&streamheader, GST_TYPE_ARRAY);
  gst_flv_mux_put_buffer_in_streamheader (&streamheader, header);
  gst_flv_mux_put_buffer_in_streamheader (&streamheader, metadata);
  if (video_codec_data != NULL)
    gst_flv_mux_put_buffer_in_streamheader (&streamheader, video_codec_data);
  if (audio_codec_data != NULL)
    gst_flv_mux_put_buffer_in_streamheader (&streamheader, audio_codec_data);

  /* create the caps and put the streamheader in them */
  caps = gst_caps_new_empty_simple ("video/x-flv");
  structure = gst_caps_get_structure (caps, 0);
  gst_structure_set_value (structure, "streamheader", &streamheader);
  g_value_unset (&streamheader);

  if (header_buf) {
    *header_buf = header;
  } else {
    gst_buffer_unref (header);
  }

  if (metadata_buf) {
    *metadata_buf = metadata;
  } else {
    gst_buffer_unref (metadata);
  }

  if (video_codec_data_buf) {
    *video_codec_data_buf = video_codec_data;
  } else if (video_codec_data) {
    gst_buffer_unref (video_codec_data);
  }

  if (audio_codec_data_buf) {
    *audio_codec_data_buf = audio_codec_data;
  } else if (audio_codec_data) {
    gst_buffer_unref (audio_codec_data);
  }

  return caps;
}

static GstFlowReturn
gst_flv_mux_write_header (GstFlvMux * mux)
{
  GstBuffer *header, *metadata;
  GstBuffer *video_codec_data, *audio_codec_data;
  GstCaps *caps;
  GstFlowReturn ret;

  header = metadata = video_codec_data = audio_codec_data = NULL;

  /* if not streaming, check if downstream is seekable */
  if (!mux->streamable) {
    gboolean seekable;
    GstQuery *query;

    query = gst_query_new_seeking (GST_FORMAT_BYTES);
    if (gst_pad_peer_query (mux->srcpad, query)) {
      gst_query_parse_seeking (query, NULL, &seekable, NULL, NULL);
      GST_INFO_OBJECT (mux, "downstream is %sseekable", seekable ? "" : "not ");
    } else {
      /* have to assume seeking is supported if query not handled downstream */
      GST_WARNING_OBJECT (mux, "downstream did not handle seeking query");
      seekable = FALSE;
    }
    if (!seekable) {
      mux->streamable = TRUE;
      g_object_notify (G_OBJECT (mux), "streamable");
      GST_WARNING_OBJECT (mux, "downstream is not seekable, but "
          "streamable=false. Will ignore that and create streamable output "
          "instead");
    }
    gst_query_unref (query);
  }

  if (mux->streamable && mux->sent_header) {
    GstBuffer **video_codec_data_p = NULL, **audio_codec_data_p = NULL;

    if (mux->video_pad && mux->video_pad->info_changed)
      video_codec_data_p = &video_codec_data;
    if (mux->audio_pad && mux->audio_pad->info_changed)
      audio_codec_data_p = &audio_codec_data;

    caps = gst_flv_mux_prepare_src_caps (mux,
        NULL, NULL, video_codec_data_p, audio_codec_data_p);
  } else {
    caps = gst_flv_mux_prepare_src_caps (mux,
        &header, &metadata, &video_codec_data, &audio_codec_data);
  }

  gst_aggregator_set_src_caps (GST_AGGREGATOR_CAST (mux), caps);

  gst_caps_unref (caps);

  /* push the header buffer, the metadata and the codec info, if any */
  if (header != NULL) {
    ret = gst_flv_mux_push (mux, header);
    if (ret != GST_FLOW_OK)
      goto failure_header;
    mux->sent_header = TRUE;
  }
  if (metadata != NULL) {
    ret = gst_flv_mux_push (mux, metadata);
    if (ret != GST_FLOW_OK)
      goto failure_metadata;
    mux->new_tags = FALSE;
  }
  if (video_codec_data != NULL) {
    ret = gst_flv_mux_push (mux, video_codec_data);
    if (ret != GST_FLOW_OK)
      goto failure_video_codec_data;
    mux->video_pad->info_changed = FALSE;
  }
  if (audio_codec_data != NULL) {
    ret = gst_flv_mux_push (mux, audio_codec_data);
    if (ret != GST_FLOW_OK)
      goto failure_audio_codec_data;
    mux->audio_pad->info_changed = FALSE;
  }
  return GST_FLOW_OK;

failure_header:
  gst_buffer_unref (metadata);

failure_metadata:
  if (video_codec_data != NULL)
    gst_buffer_unref (video_codec_data);

failure_video_codec_data:
  if (audio_codec_data != NULL)
    gst_buffer_unref (audio_codec_data);

failure_audio_codec_data:
  return ret;
}

static GstClockTime
gst_flv_mux_segment_to_running_time (const GstSegment * segment, GstClockTime t)
{
  /* we can get a dts before the segment, if dts < pts and pts is inside
   * the segment, so we consider early times as 0 */
  if (t < segment->start)
    return 0;
  return gst_segment_to_running_time (segment, GST_FORMAT_TIME, t);
}

static void
gst_flv_mux_update_index (GstFlvMux * mux, GstBuffer * buffer,
    GstFlvMuxPad * pad)
{
  /*
   * Add the tag byte offset and to the index if it's a valid seek point, which
   * means it's either a video keyframe or if there is no video pad (in that
   * case every FLV tag is a valid seek point)
   */
  if (mux->video_pad == pad &&
      GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT))
    return;

  if (GST_BUFFER_PTS_IS_VALID (buffer)) {
    GstFlvMuxIndexEntry *entry = g_slice_new (GstFlvMuxIndexEntry);
    GstClockTime pts =
        gst_flv_mux_segment_to_running_time (&GST_AGGREGATOR_PAD
        (pad)->segment, GST_BUFFER_PTS (buffer));
    entry->position = mux->byte_count;
    entry->time = gst_guint64_to_gdouble (pts) / GST_SECOND;
    mux->index = g_list_prepend (mux->index, entry);
  }
}

static GstFlowReturn
gst_flv_mux_write_buffer (GstFlvMux * mux, GstFlvMuxPad * pad,
    GstBuffer * buffer)
{
  GstBuffer *tag;
  GstFlowReturn ret;
  GstClockTime dts =
      gst_flv_mux_segment_to_running_time (&GST_AGGREGATOR_PAD (pad)->segment,
      GST_BUFFER_DTS (buffer));

  /* clipping function arranged for running_time */

  if (!mux->streamable)
    gst_flv_mux_update_index (mux, buffer, pad);

  tag = gst_flv_mux_buffer_to_tag (mux, buffer, pad);

  gst_buffer_unref (buffer);

  ret = gst_flv_mux_push (mux, tag);

  if (ret == GST_FLOW_OK && GST_CLOCK_TIME_IS_VALID (dts))
    pad->last_timestamp = dts;


  return ret;
}

static guint64
gst_flv_mux_determine_duration (GstFlvMux * mux)
{
  GList *l;
  GstClockTime duration = GST_CLOCK_TIME_NONE;

  GST_DEBUG_OBJECT (mux, "trying to determine the duration "
      "from pad timestamps");

  for (l = GST_ELEMENT_CAST (mux)->sinkpads; l != NULL; l = l->next) {
    GstFlvMuxPad *pad = GST_FLV_MUX_PAD (l->data);

    if (pad && (pad->last_timestamp != GST_CLOCK_TIME_NONE)) {
      if (duration == GST_CLOCK_TIME_NONE)
        duration = pad->last_timestamp;
      else
        duration = MAX (duration, pad->last_timestamp);
    }
  }

  return duration;
}

static gboolean
gst_flv_mux_are_all_pads_eos (GstFlvMux * mux)
{
  GList *l;

  for (l = GST_ELEMENT_CAST (mux)->sinkpads; l; l = l->next) {
    GstFlvMuxPad *pad = GST_FLV_MUX_PAD (l->data);

    if (!gst_aggregator_pad_is_eos (GST_AGGREGATOR_PAD (pad)))
      return FALSE;
  }
  return TRUE;
}

static GstFlowReturn
gst_flv_mux_write_eos (GstFlvMux * mux)
{
  GstBuffer *tag;

  if (mux->video_pad == NULL)
    return GST_FLOW_OK;

  tag = gst_flv_mux_eos_to_tag (mux, mux->video_pad);

  return gst_flv_mux_push (mux, tag);
}

static GstFlowReturn
gst_flv_mux_rewrite_header (GstFlvMux * mux)
{
  GstBuffer *rewrite, *index, *tmp;
  GstEvent *event;
  guint8 *data;
  gdouble d;
  GList *l;
  guint32 index_len, allocate_size;
  guint32 i, index_skip;
  GstSegment segment;
  GstClockTime dur;

  if (mux->streamable)
    return GST_FLOW_OK;

  /* seek back to the preallocated index space */
  gst_segment_init (&segment, GST_FORMAT_BYTES);
  segment.start = segment.time = 13 + 29;
  event = gst_event_new_segment (&segment);
  if (!gst_pad_push_event (mux->srcpad, event)) {
    GST_WARNING_OBJECT (mux, "Seek to rewrite header failed");
    return GST_FLOW_OK;
  }

  /* determine duration now based on our own timestamping,
   * so that it is likely many times better and consistent
   * than whatever obtained by some query */
  dur = gst_flv_mux_determine_duration (mux);
  if (dur != GST_CLOCK_TIME_NONE)
    mux->duration = dur;

  /* rewrite the duration tag */
  d = gst_guint64_to_gdouble (mux->duration);
  d /= (gdouble) GST_SECOND;

  GST_DEBUG_OBJECT (mux, "determined the final duration to be %f", d);

  rewrite = gst_flv_mux_create_number_script_value ("duration", d);

  /* rewrite the filesize tag */
  d = gst_guint64_to_gdouble (mux->byte_count);

  GST_DEBUG_OBJECT (mux, "putting total filesize %f in the metadata", d);

  tmp = gst_flv_mux_create_number_script_value ("filesize", d);
  rewrite = gst_buffer_append (rewrite, tmp);

  if (!mux->index) {
    /* no index, so push buffer and return */
    return gst_flv_mux_push (mux, rewrite);
  }

  /* rewrite the index */
  mux->index = g_list_reverse (mux->index);
  index_len = g_list_length (mux->index);

  /* We write at most MAX_INDEX_ENTRIES elements */
  if (index_len > MAX_INDEX_ENTRIES) {
    index_skip = 1 + index_len / MAX_INDEX_ENTRIES;
    index_len = (index_len + index_skip - 1) / index_skip;
  } else {
    index_skip = 1;
  }

  GST_DEBUG_OBJECT (mux, "Index length is %d", index_len);
  /* see size calculation in gst_flv_mux_preallocate_index */
  allocate_size = 11 + 8 + 22 + 10 + index_len * 18;
  GST_DEBUG_OBJECT (mux, "Allocating %d bytes for index", allocate_size);
  _gst_buffer_new_and_alloc (allocate_size, &index, &data);

  GST_WRITE_UINT16_BE (data, 9);        /* the 'keyframes' key */
  memcpy (data + 2, "keyframes", 9);
  GST_WRITE_UINT8 (data + 11, 8);       /* nested ECMA array */
  GST_WRITE_UINT32_BE (data + 12, 2);   /* two elements */
  GST_WRITE_UINT16_BE (data + 16, 5);   /* first string key: 'times' */
  memcpy (data + 18, "times", 5);
  GST_WRITE_UINT8 (data + 23, 10);      /* strict array */
  GST_WRITE_UINT32_BE (data + 24, index_len);
  data += 28;

  /* the keyframes' times */
  for (i = 0, l = mux->index; l; l = l->next, i++) {
    GstFlvMuxIndexEntry *entry = l->data;

    if (i % index_skip != 0)
      continue;
    GST_WRITE_UINT8 (data, 0);  /* numeric (aka double) */
    GST_WRITE_DOUBLE_BE (data + 1, entry->time);
    data += 9;
  }

  GST_WRITE_UINT16_BE (data, 13);       /* second string key: 'filepositions' */
  memcpy (data + 2, "filepositions", 13);
  GST_WRITE_UINT8 (data + 15, 10);      /* strict array */
  GST_WRITE_UINT32_BE (data + 16, index_len);
  data += 20;

  /* the keyframes' file positions */
  for (i = 0, l = mux->index; l; l = l->next, i++) {
    GstFlvMuxIndexEntry *entry = l->data;

    if (i % index_skip != 0)
      continue;
    GST_WRITE_UINT8 (data, 0);
    GST_WRITE_DOUBLE_BE (data + 1, entry->position);
    data += 9;
  }

  GST_WRITE_UINT24_BE (data, 9);        /* finish the ECMA array */

  /* If there is space left in the prefilled area, reinsert the filler.
     There is at least 18  bytes free, so it will always fit. */
  if (index_len < MAX_INDEX_ENTRIES) {
    GstBuffer *tmp;
    guint8 *data;
    guint32 remaining_filler_size;

    _gst_buffer_new_and_alloc (14, &tmp, &data);
    GST_WRITE_UINT16_BE (data, 9);
    memcpy (data + 2, "gstfiller", 9);
    GST_WRITE_UINT8 (data + 11, 2);     /* string */

    /* There is 18 bytes per remaining index entry minus what is used for
     * the'gstfiller' key. The rest is already filled with spaces, so just need
     * to update length. */
    remaining_filler_size = (MAX_INDEX_ENTRIES - index_len) * 18 - 14;
    GST_DEBUG_OBJECT (mux, "Remaining filler size is %d bytes",
        remaining_filler_size);
    GST_WRITE_UINT16_BE (data + 12, remaining_filler_size);
    index = gst_buffer_append (index, tmp);
  }

  rewrite = gst_buffer_append (rewrite, index);

  return gst_flv_mux_push (mux, rewrite);
}

static GstFlvMuxPad *
gst_flv_mux_find_best_pad (GstAggregator * aggregator, GstClockTime * ts)
{
  GstAggregatorPad *apad;
  GstFlvMuxPad *pad, *best = NULL;
  GList *l;
  GstBuffer *buffer;
  GstClockTime best_ts = GST_CLOCK_TIME_NONE;

  for (l = GST_ELEMENT_CAST (aggregator)->sinkpads; l; l = l->next) {
    apad = GST_AGGREGATOR_PAD (l->data);
    pad = GST_FLV_MUX_PAD (l->data);
    buffer = gst_aggregator_pad_peek_buffer (GST_AGGREGATOR_PAD (pad));
    if (!buffer)
      continue;
    if (best_ts == GST_CLOCK_TIME_NONE) {
      best = pad;
      best_ts = gst_flv_mux_segment_to_running_time (&apad->segment,
          GST_BUFFER_DTS_OR_PTS (buffer));
    } else if (GST_BUFFER_DTS_OR_PTS (buffer) != GST_CLOCK_TIME_NONE) {
      gint64 t = gst_flv_mux_segment_to_running_time (&apad->segment,
          GST_BUFFER_DTS_OR_PTS (buffer));
      if (t < best_ts) {
        best = pad;
        best_ts = t;
      }
    }
    gst_buffer_unref (buffer);
  }
  GST_DEBUG_OBJECT (aggregator,
      "Best pad found with %" GST_TIME_FORMAT ": %" GST_PTR_FORMAT,
      GST_TIME_ARGS (best_ts), best);
  if (ts)
    *ts = best_ts;
  return best;
}

static GstFlowReturn
gst_flv_mux_aggregate (GstAggregator * aggregator, gboolean timeout)
{
  GstFlvMux *mux = GST_FLV_MUX (aggregator);
  GstFlvMuxPad *best;
  gint64 best_time = GST_CLOCK_STIME_NONE;
  GstFlowReturn ret;
  GstClockTime ts;
  GstBuffer *buffer = NULL;

  if (mux->state == GST_FLV_MUX_STATE_HEADER) {
    if (GST_ELEMENT_CAST (mux)->sinkpads == NULL) {
      GST_ELEMENT_ERROR (mux, STREAM, MUX, (NULL),
          ("No input streams configured"));
      return GST_FLOW_ERROR;
    }

    ret = gst_flv_mux_write_header (mux);
    if (ret != GST_FLOW_OK)
      return ret;
    mux->state = GST_FLV_MUX_STATE_DATA;

    best = gst_flv_mux_find_best_pad (aggregator, &ts);

    if (!mux->streamable || mux->first_timestamp == GST_CLOCK_STIME_NONE) {
      if (best && GST_CLOCK_STIME_IS_VALID (ts))
        mux->first_timestamp = ts;
      else
        mux->first_timestamp = 0;
    }
  } else {
    best = gst_flv_mux_find_best_pad (aggregator, &ts);
  }

  if (mux->new_tags && mux->streamable) {
    GstBuffer *buf = gst_flv_mux_create_metadata (mux);
    if (buf)
      gst_flv_mux_push (mux, buf);
    mux->new_tags = FALSE;
  }

  if (best) {
    buffer = gst_aggregator_pad_pop_buffer (GST_AGGREGATOR_PAD (best));
    g_assert (buffer);
    best->dts =
        gst_flv_mux_segment_to_running_time (&GST_AGGREGATOR_PAD
        (best)->segment, GST_BUFFER_DTS_OR_PTS (buffer));

    if (GST_CLOCK_STIME_IS_VALID (best->dts))
      best_time = best->dts - mux->first_timestamp;

    if (GST_BUFFER_PTS_IS_VALID (buffer))
      best->pts =
          gst_flv_mux_segment_to_running_time (&GST_AGGREGATOR_PAD
          (best)->segment, GST_BUFFER_PTS (buffer));
    else
      best->pts = best->dts;

    GST_LOG_OBJECT (best, "got buffer PTS %" GST_TIME_FORMAT " DTS %"
        GST_STIME_FORMAT, GST_TIME_ARGS (best->pts),
        GST_STIME_ARGS (best->dts));
  } else {
    best_time = GST_CLOCK_STIME_NONE;
  }

  /* The FLV timestamp is an int32 field. For non-live streams error out if a
     bigger timestamp is seen, for live the timestamp will get wrapped in
     gst_flv_mux_buffer_to_tag */
  if (!mux->streamable && (GST_CLOCK_STIME_IS_VALID (best_time))
      && best_time / GST_MSECOND > G_MAXINT32) {
    GST_WARNING_OBJECT (mux, "Timestamp larger than FLV supports - EOS");
    if (buffer) {
      gst_buffer_unref (buffer);
      buffer = NULL;
    }
    best = NULL;
  }

  if (best) {
    return gst_flv_mux_write_buffer (mux, best, buffer);
  } else {
    if (gst_flv_mux_are_all_pads_eos (mux)) {
      gst_flv_mux_write_eos (mux);
      gst_flv_mux_rewrite_header (mux);
      return GST_FLOW_EOS;
    }
    return GST_FLOW_OK;
  }
}

static void
gst_flv_mux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstFlvMux *mux = GST_FLV_MUX (object);

  switch (prop_id) {
    case PROP_STREAMABLE:
      g_value_set_boolean (value, mux->streamable);
      break;
    case PROP_METADATACREATOR:
      g_value_set_string (value, mux->metadatacreator);
      break;
    case PROP_ENCODER:
      g_value_set_string (value, mux->encoder);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_flv_mux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstFlvMux *mux = GST_FLV_MUX (object);

  switch (prop_id) {
    case PROP_STREAMABLE:
      mux->streamable = g_value_get_boolean (value);
      if (mux->streamable)
        gst_tag_setter_set_tag_merge_mode (GST_TAG_SETTER (mux),
            GST_TAG_MERGE_REPLACE);
      else
        gst_tag_setter_set_tag_merge_mode (GST_TAG_SETTER (mux),
            GST_TAG_MERGE_KEEP);
      break;
    case PROP_METADATACREATOR:
      g_free (mux->metadatacreator);
      if (!g_value_get_string (value)) {
        GST_WARNING_OBJECT (mux, "metadatacreator property can not be NULL");
        mux->metadatacreator = g_strdup (DEFAULT_METADATACREATOR);
      } else {
        mux->metadatacreator = g_value_dup_string (value);
      }
      break;
    case PROP_ENCODER:
      g_free (mux->encoder);
      if (!g_value_get_string (value)) {
        GST_WARNING_OBJECT (mux, "encoder property can not be NULL");
        mux->encoder = g_strdup (DEFAULT_METADATACREATOR);
      } else {
        mux->encoder = g_value_dup_string (value);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstClockTime
gst_flv_mux_get_next_time (GstAggregator * aggregator)
{
  GstFlvMux *mux = GST_FLV_MUX (aggregator);
  GstAggregatorPad *agg_audio_pad = GST_AGGREGATOR_PAD_CAST (mux->audio_pad);
  GstAggregatorPad *agg_video_pad = GST_AGGREGATOR_PAD_CAST (mux->video_pad);

  GST_OBJECT_LOCK (aggregator);
  if (mux->state == GST_FLV_MUX_STATE_HEADER &&
      ((mux->audio_pad && mux->audio_pad->codec == G_MAXUINT) ||
          (mux->video_pad && mux->video_pad->codec == G_MAXUINT)))
    goto wait_for_data;

  if (!((agg_audio_pad && gst_aggregator_pad_has_buffer (agg_audio_pad)) ||
          (agg_video_pad && gst_aggregator_pad_has_buffer (agg_video_pad))))
    goto wait_for_data;
  GST_OBJECT_UNLOCK (aggregator);

  return gst_aggregator_simple_get_next_time (aggregator);

wait_for_data:
  GST_OBJECT_UNLOCK (aggregator);
  return GST_CLOCK_TIME_NONE;
}

static GstFlowReturn
gst_flv_mux_update_src_caps (GstAggregator * aggregator,
    GstCaps * caps, GstCaps ** ret)
{
  GstFlvMux *mux = GST_FLV_MUX (aggregator);

  *ret = gst_flv_mux_prepare_src_caps (mux, NULL, NULL, NULL, NULL);

  return GST_FLOW_OK;
}
