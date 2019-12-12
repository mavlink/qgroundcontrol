/* ex: set tabstop=2 shiftwidth=2 expandtab: */
/* GStreamer
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
#  include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/pbutils/pbutils.h>
#include <gst/video/video.h>

/* Included to not duplicate gst_rtp_h264_add_sps_pps () */
#include "gstrtph264depay.h"

#include "gstrtph264pay.h"
#include "gstrtputils.h"


#define IDR_TYPE_ID    5
#define SPS_TYPE_ID    7
#define PPS_TYPE_ID    8
#define AUD_TYPE_ID    9
#define STAP_A_TYPE_ID 24
#define FU_A_TYPE_ID   28

GST_DEBUG_CATEGORY_STATIC (rtph264pay_debug);
#define GST_CAT_DEFAULT (rtph264pay_debug)

#define GST_TYPE_RTP_H264_AGGREGATE_MODE \
  (gst_rtp_h264_aggregate_mode_get_type ())


static GType
gst_rtp_h264_aggregate_mode_get_type (void)
{
  static GType type = 0;
  static const GEnumValue values[] = {
    {GST_RTP_H264_AGGREGATE_NONE, "Do not aggregate NAL units", "none"},
    {GST_RTP_H264_AGGREGATE_ZERO_LATENCY,
        "Aggregate NAL units until a VCL unit is included", "zero-latency"},
    {GST_RTP_H264_AGGREGATE_MAX_STAP,
        "Aggregate all NAL units with the same timestamp (adds one frame of"
          " latency)", "max-stap"},
    {0, NULL, NULL},
  };

  if (!type) {
    type = g_enum_register_static ("GstRtpH264AggregateMode", values);
  }
  return type;
}



/* references:
*
 * RFC 3984
 */

static GstStaticPadTemplate gst_rtp_h264_pay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h264, "
        "stream-format = (string) avc, alignment = (string) au;"
        "video/x-h264, "
        "stream-format = (string) byte-stream, alignment = (string) { nal, au }")
    );

static GstStaticPadTemplate gst_rtp_h264_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"H264\"")
    );

#define DEFAULT_SPROP_PARAMETER_SETS    NULL
#define DEFAULT_CONFIG_INTERVAL         0
#define DEFAULT_AGGREGATE_MODE          GST_RTP_H264_AGGREGATE_ZERO_LATENCY

enum
{
  PROP_0,
  PROP_SPROP_PARAMETER_SETS,
  PROP_CONFIG_INTERVAL,
  PROP_AGGREGATE_MODE,
};

static void gst_rtp_h264_pay_finalize (GObject * object);

static void gst_rtp_h264_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_h264_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstCaps *gst_rtp_h264_pay_getcaps (GstRTPBasePayload * payload,
    GstPad * pad, GstCaps * filter);
static gboolean gst_rtp_h264_pay_setcaps (GstRTPBasePayload * basepayload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_h264_pay_handle_buffer (GstRTPBasePayload * pad,
    GstBuffer * buffer);
static gboolean gst_rtp_h264_pay_sink_event (GstRTPBasePayload * payload,
    GstEvent * event);
static GstStateChangeReturn gst_rtp_h264_pay_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_rtp_h264_pay_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query);

static void gst_rtp_h264_pay_reset_bundle (GstRtpH264Pay * rtph264pay);

#define gst_rtp_h264_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpH264Pay, gst_rtp_h264_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_h264_pay_class_init (GstRtpH264PayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->set_property = gst_rtp_h264_pay_set_property;
  gobject_class->get_property = gst_rtp_h264_pay_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_SPROP_PARAMETER_SETS, g_param_spec_string ("sprop-parameter-sets",
          "sprop-parameter-sets",
          "The base64 sprop-parameter-sets to set in out caps (set to NULL to "
          "extract from stream)",
          DEFAULT_SPROP_PARAMETER_SETS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));

  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_CONFIG_INTERVAL,
      g_param_spec_int ("config-interval",
          "SPS PPS Send Interval",
          "Send SPS and PPS Insertion Interval in seconds (sprop parameter sets "
          "will be multiplexed in the data stream when detected.) "
          "(0 = disabled, -1 = send with every IDR frame)",
          -1, 3600, DEFAULT_CONFIG_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
      );

  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_AGGREGATE_MODE,
      g_param_spec_enum ("aggregate-mode",
          "Attempt to use aggregate packets",
          "Bundle suitable SPS/PPS NAL units into STAP-A "
          "aggregate packets. ",
          GST_TYPE_RTP_H264_AGGREGATE_MODE,
          DEFAULT_AGGREGATE_MODE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
      );

  gobject_class->finalize = gst_rtp_h264_pay_finalize;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h264_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_h264_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class, "RTP H264 payloader",
      "Codec/Payloader/Network/RTP",
      "Payload-encode H264 video into RTP packets (RFC 3984)",
      "Laurent Glayal <spglegle@yahoo.fr>");

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_h264_pay_change_state);

  gstrtpbasepayload_class->get_caps = gst_rtp_h264_pay_getcaps;
  gstrtpbasepayload_class->set_caps = gst_rtp_h264_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_h264_pay_handle_buffer;
  gstrtpbasepayload_class->sink_event = gst_rtp_h264_pay_sink_event;

  GST_DEBUG_CATEGORY_INIT (rtph264pay_debug, "rtph264pay", 0,
      "H264 RTP Payloader");
}

static void
gst_rtp_h264_pay_init (GstRtpH264Pay * rtph264pay)
{
  rtph264pay->queue = g_array_new (FALSE, FALSE, sizeof (guint));
  rtph264pay->profile = 0;
  rtph264pay->sps = g_ptr_array_new_with_free_func (
      (GDestroyNotify) gst_buffer_unref);
  rtph264pay->pps = g_ptr_array_new_with_free_func (
      (GDestroyNotify) gst_buffer_unref);
  rtph264pay->last_spspps = -1;
  rtph264pay->spspps_interval = DEFAULT_CONFIG_INTERVAL;
  rtph264pay->aggregate_mode = DEFAULT_AGGREGATE_MODE;
  rtph264pay->delta_unit = FALSE;
  rtph264pay->discont = FALSE;

  rtph264pay->adapter = gst_adapter_new ();

  gst_pad_set_query_function (GST_RTP_BASE_PAYLOAD_SRCPAD (rtph264pay),
      gst_rtp_h264_pay_src_query);
}

static void
gst_rtp_h264_pay_clear_sps_pps (GstRtpH264Pay * rtph264pay)
{
  g_ptr_array_set_size (rtph264pay->sps, 0);
  g_ptr_array_set_size (rtph264pay->pps, 0);
}

static void
gst_rtp_h264_pay_finalize (GObject * object)
{
  GstRtpH264Pay *rtph264pay;

  rtph264pay = GST_RTP_H264_PAY (object);

  g_array_free (rtph264pay->queue, TRUE);

  g_ptr_array_free (rtph264pay->sps, TRUE);
  g_ptr_array_free (rtph264pay->pps, TRUE);

  g_free (rtph264pay->sprop_parameter_sets);

  g_object_unref (rtph264pay->adapter);
  gst_rtp_h264_pay_reset_bundle (rtph264pay);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static const gchar all_levels[][4] = {
  "1",
  "1b",
  "1.1",
  "1.2",
  "1.3",
  "2",
  "2.1",
  "2.2",
  "3",
  "3.1",
  "3.2",
  "4",
  "4.1",
  "4.2",
  "5",
  "5.1"
};

static GstCaps *
gst_rtp_h264_pay_getcaps (GstRTPBasePayload * payload, GstPad * pad,
    GstCaps * filter)
{
  GstCaps *template_caps;
  GstCaps *allowed_caps;
  GstCaps *caps, *icaps;
  gboolean append_unrestricted;
  guint i;

  allowed_caps =
      gst_pad_peer_query_caps (GST_RTP_BASE_PAYLOAD_SRCPAD (payload), NULL);

  if (allowed_caps == NULL)
    return NULL;

  template_caps =
      gst_static_pad_template_get_caps (&gst_rtp_h264_pay_sink_template);

  if (gst_caps_is_any (allowed_caps)) {
    caps = gst_caps_ref (template_caps);
    goto done;
  }

  if (gst_caps_is_empty (allowed_caps)) {
    caps = gst_caps_ref (allowed_caps);
    goto done;
  }

  caps = gst_caps_new_empty ();

  append_unrestricted = FALSE;
  for (i = 0; i < gst_caps_get_size (allowed_caps); i++) {
    GstStructure *s = gst_caps_get_structure (allowed_caps, i);
    GstStructure *new_s = gst_structure_new_empty ("video/x-h264");
    const gchar *profile_level_id;

    profile_level_id = gst_structure_get_string (s, "profile-level-id");

    if (profile_level_id && strlen (profile_level_id) == 6) {
      const gchar *profile;
      const gchar *level;
      long int spsint;
      guint8 sps[3];

      spsint = strtol (profile_level_id, NULL, 16);
      sps[0] = spsint >> 16;
      sps[1] = spsint >> 8;
      sps[2] = spsint;

      profile = gst_codec_utils_h264_get_profile (sps, 3);
      level = gst_codec_utils_h264_get_level (sps, 3);

      if (profile && level) {
        GST_LOG_OBJECT (payload, "In caps, have profile %s and level %s",
            profile, level);

        if (!strcmp (profile, "constrained-baseline"))
          gst_structure_set (new_s, "profile", G_TYPE_STRING, profile, NULL);
        else {
          GValue val = { 0, };
          GValue profiles = { 0, };

          g_value_init (&profiles, GST_TYPE_LIST);
          g_value_init (&val, G_TYPE_STRING);

          g_value_set_static_string (&val, profile);
          gst_value_list_append_value (&profiles, &val);

          g_value_set_static_string (&val, "constrained-baseline");
          gst_value_list_append_value (&profiles, &val);

          gst_structure_take_value (new_s, "profile", &profiles);
        }

        if (!strcmp (level, "1"))
          gst_structure_set (new_s, "level", G_TYPE_STRING, level, NULL);
        else {
          GValue levels = { 0, };
          GValue val = { 0, };
          int j;

          g_value_init (&levels, GST_TYPE_LIST);
          g_value_init (&val, G_TYPE_STRING);

          for (j = 0; j < G_N_ELEMENTS (all_levels); j++) {
            g_value_set_static_string (&val, all_levels[j]);
            gst_value_list_prepend_value (&levels, &val);
            if (!strcmp (level, all_levels[j]))
              break;
          }
          gst_structure_take_value (new_s, "level", &levels);
        }
      } else {
        /* Invalid profile-level-id means baseline */

        gst_structure_set (new_s,
            "profile", G_TYPE_STRING, "constrained-baseline", NULL);
      }
    } else {
      /* No profile-level-id means baseline or unrestricted */

      gst_structure_set (new_s,
          "profile", G_TYPE_STRING, "constrained-baseline", NULL);
      append_unrestricted = TRUE;
    }

    caps = gst_caps_merge_structure (caps, new_s);
  }

  if (append_unrestricted) {
    caps =
        gst_caps_merge_structure (caps, gst_structure_new ("video/x-h264", NULL,
            NULL));
  }

  icaps = gst_caps_intersect (caps, template_caps);
  gst_caps_unref (caps);
  caps = icaps;

done:
  if (filter) {
    GST_DEBUG_OBJECT (payload, "Intersect %" GST_PTR_FORMAT " and filter %"
        GST_PTR_FORMAT, caps, filter);
    icaps = gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = icaps;
  }

  gst_caps_unref (template_caps);
  gst_caps_unref (allowed_caps);

  GST_LOG_OBJECT (payload, "returning caps %" GST_PTR_FORMAT, caps);
  return caps;
}

static gboolean
gst_rtp_h264_pay_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  GstRtpH264Pay *rtph264pay = GST_RTP_H264_PAY (parent);

  if (GST_QUERY_TYPE (query) == GST_QUERY_LATENCY) {
    gboolean retval;
    gboolean live;
    GstClockTime min_latency, max_latency;

    retval = gst_pad_query_default (pad, parent, query);
    if (!retval)
      return retval;

    if (rtph264pay->stream_format == GST_H264_STREAM_FORMAT_UNKNOWN ||
        rtph264pay->alignment == GST_H264_ALIGNMENT_UNKNOWN)
      return FALSE;

    gst_query_parse_latency (query, &live, &min_latency, &max_latency);

    if (rtph264pay->aggregate_mode == GST_RTP_H264_AGGREGATE_MAX_STAP &&
        rtph264pay->alignment != GST_H264_ALIGNMENT_AU && rtph264pay->fps_num) {
      GstClockTime one_frame = gst_util_uint64_scale_int (GST_SECOND,
          rtph264pay->fps_denum, rtph264pay->fps_num);

      min_latency += one_frame;
      max_latency += one_frame;
      gst_query_set_latency (query, live, min_latency, max_latency);
    }
    return TRUE;
  }

  return gst_pad_query_default (pad, parent, query);
}


/* take the currently configured SPS and PPS lists and set them on the caps as
 * sprop-parameter-sets */
static gboolean
gst_rtp_h264_pay_set_sps_pps (GstRTPBasePayload * basepayload)
{
  GstRtpH264Pay *payloader = GST_RTP_H264_PAY (basepayload);
  gchar *profile;
  gchar *set;
  GString *sprops;
  guint count;
  gboolean res;
  GstMapInfo map;
  guint i;

  sprops = g_string_new ("");
  count = 0;

  /* build the sprop-parameter-sets */
  for (i = 0; i < payloader->sps->len; i++) {
    GstBuffer *sps_buf =
        GST_BUFFER_CAST (g_ptr_array_index (payloader->sps, i));

    gst_buffer_map (sps_buf, &map, GST_MAP_READ);
    set = g_base64_encode (map.data, map.size);
    gst_buffer_unmap (sps_buf, &map);

    g_string_append_printf (sprops, "%s%s", count ? "," : "", set);
    g_free (set);
    count++;
  }
  for (i = 0; i < payloader->pps->len; i++) {
    GstBuffer *pps_buf =
        GST_BUFFER_CAST (g_ptr_array_index (payloader->pps, i));

    gst_buffer_map (pps_buf, &map, GST_MAP_READ);
    set = g_base64_encode (map.data, map.size);
    gst_buffer_unmap (pps_buf, &map);

    g_string_append_printf (sprops, "%s%s", count ? "," : "", set);
    g_free (set);
    count++;
  }

  if (G_LIKELY (count)) {
    if (payloader->profile != 0) {
      /* profile is 24 bit. Force it to respect the limit */
      profile = g_strdup_printf ("%06x", payloader->profile & 0xffffff);
      /* combine into output caps */
      res = gst_rtp_base_payload_set_outcaps (basepayload,
          "packetization-mode", G_TYPE_STRING, "1",
          "profile-level-id", G_TYPE_STRING, profile,
          "sprop-parameter-sets", G_TYPE_STRING, sprops->str, NULL);
      g_free (profile);
    } else {
      res = gst_rtp_base_payload_set_outcaps (basepayload,
          "packetization-mode", G_TYPE_STRING, "1",
          "sprop-parameter-sets", G_TYPE_STRING, sprops->str, NULL);
    }

  } else {
    res = gst_rtp_base_payload_set_outcaps (basepayload, NULL);
  }
  g_string_free (sprops, TRUE);

  return res;
}


static gboolean
gst_rtp_h264_pay_setcaps (GstRTPBasePayload * basepayload, GstCaps * caps)
{
  GstRtpH264Pay *rtph264pay;
  GstStructure *str;
  const GValue *value;
  GstMapInfo map;
  guint8 *data;
  gsize size;
  GstBuffer *buffer;
  const gchar *alignment, *stream_format;

  rtph264pay = GST_RTP_H264_PAY (basepayload);

  str = gst_caps_get_structure (caps, 0);

  /* we can only set the output caps when we found the sprops and profile
   * NALs */
  gst_rtp_base_payload_set_options (basepayload, "video", TRUE, "H264", 90000);

  rtph264pay->alignment = GST_H264_ALIGNMENT_UNKNOWN;
  alignment = gst_structure_get_string (str, "alignment");
  if (alignment) {
    if (g_str_equal (alignment, "au"))
      rtph264pay->alignment = GST_H264_ALIGNMENT_AU;
    if (g_str_equal (alignment, "nal"))
      rtph264pay->alignment = GST_H264_ALIGNMENT_NAL;
  }

  rtph264pay->stream_format = GST_H264_STREAM_FORMAT_UNKNOWN;
  stream_format = gst_structure_get_string (str, "stream-format");
  if (stream_format) {
    if (g_str_equal (stream_format, "avc"))
      rtph264pay->stream_format = GST_H264_STREAM_FORMAT_AVC;
    if (g_str_equal (stream_format, "byte-stream"))
      rtph264pay->stream_format = GST_H264_STREAM_FORMAT_BYTESTREAM;
  }

  if (!gst_structure_get_fraction (str, "framerate", &rtph264pay->fps_num,
          &rtph264pay->fps_denum))
    rtph264pay->fps_num = rtph264pay->fps_denum = 0;

  /* packetized AVC video has a codec_data */
  if ((value = gst_structure_get_value (str, "codec_data"))) {
    guint num_sps, num_pps;
    gint i, nal_size;

    GST_DEBUG_OBJECT (rtph264pay, "have packetized h264");

    buffer = gst_value_get_buffer (value);

    gst_buffer_map (buffer, &map, GST_MAP_READ);
    data = map.data;
    size = map.size;

    /* parse the avcC data */
    if (size < 7)
      goto avcc_too_small;
    /* parse the version, this must be 1 */
    if (data[0] != 1)
      goto wrong_version;

    /* AVCProfileIndication */
    /* profile_compat */
    /* AVCLevelIndication */
    rtph264pay->profile = (data[1] << 16) | (data[2] << 8) | data[3];
    GST_DEBUG_OBJECT (rtph264pay, "profile %06x", rtph264pay->profile);

    /* 6 bits reserved | 2 bits lengthSizeMinusOne */
    /* this is the number of bytes in front of the NAL units to mark their
     * length */
    rtph264pay->nal_length_size = (data[4] & 0x03) + 1;
    GST_DEBUG_OBJECT (rtph264pay, "nal length %u", rtph264pay->nal_length_size);
    /* 3 bits reserved | 5 bits numOfSequenceParameterSets */
    num_sps = data[5] & 0x1f;
    GST_DEBUG_OBJECT (rtph264pay, "num SPS %u", num_sps);

    data += 6;
    size -= 6;

    /* create the sprop-parameter-sets */
    for (i = 0; i < num_sps; i++) {
      GstBuffer *sps_buf;

      if (size < 2)
        goto avcc_error;

      nal_size = (data[0] << 8) | data[1];
      data += 2;
      size -= 2;

      GST_LOG_OBJECT (rtph264pay, "SPS %d size %d", i, nal_size);

      if (size < nal_size)
        goto avcc_error;

      /* make a buffer out of it and add to SPS list */
      sps_buf = gst_buffer_new_and_alloc (nal_size);
      gst_buffer_fill (sps_buf, 0, data, nal_size);
      gst_rtp_h264_add_sps_pps (GST_ELEMENT (rtph264pay), rtph264pay->sps,
          rtph264pay->pps, sps_buf);
      data += nal_size;
      size -= nal_size;
    }
    if (size < 1)
      goto avcc_error;

    /* 8 bits numOfPictureParameterSets */
    num_pps = data[0];
    data += 1;
    size -= 1;

    GST_DEBUG_OBJECT (rtph264pay, "num PPS %u", num_pps);
    for (i = 0; i < num_pps; i++) {
      GstBuffer *pps_buf;

      if (size < 2)
        goto avcc_error;

      nal_size = (data[0] << 8) | data[1];
      data += 2;
      size -= 2;

      GST_LOG_OBJECT (rtph264pay, "PPS %d size %d", i, nal_size);

      if (size < nal_size)
        goto avcc_error;

      /* make a buffer out of it and add to PPS list */
      pps_buf = gst_buffer_new_and_alloc (nal_size);
      gst_buffer_fill (pps_buf, 0, data, nal_size);
      gst_rtp_h264_add_sps_pps (GST_ELEMENT (rtph264pay), rtph264pay->sps,
          rtph264pay->pps, pps_buf);

      data += nal_size;
      size -= nal_size;
    }

    /* and update the caps with the collected data */
    if (!gst_rtp_h264_pay_set_sps_pps (basepayload))
      goto set_sps_pps_failed;

    gst_buffer_unmap (buffer, &map);
  } else {
    GST_DEBUG_OBJECT (rtph264pay, "have bytestream h264");
  }

  return TRUE;

avcc_too_small:
  {
    GST_ERROR_OBJECT (rtph264pay, "avcC size %" G_GSIZE_FORMAT " < 7", size);
    goto error;
  }
wrong_version:
  {
    GST_ERROR_OBJECT (rtph264pay, "wrong avcC version");
    goto error;
  }
avcc_error:
  {
    GST_ERROR_OBJECT (rtph264pay, "avcC too small ");
    goto error;
  }
set_sps_pps_failed:
  {
    GST_ERROR_OBJECT (rtph264pay, "failed to set sps/pps");
    goto error;
  }
error:
  {
    gst_buffer_unmap (buffer, &map);
    return FALSE;
  }
}

static void
gst_rtp_h264_pay_parse_sprop_parameter_sets (GstRtpH264Pay * rtph264pay)
{
  const gchar *ps;
  gchar **params;
  guint len;
  gint i;
  GstBuffer *buf;

  ps = rtph264pay->sprop_parameter_sets;
  if (ps == NULL)
    return;

  gst_rtp_h264_pay_clear_sps_pps (rtph264pay);

  params = g_strsplit (ps, ",", 0);
  len = g_strv_length (params);

  GST_DEBUG_OBJECT (rtph264pay, "we have %d params", len);

  for (i = 0; params[i]; i++) {
    gsize nal_len;
    GstMapInfo map;
    guint8 *nalp;
    guint save = 0;
    gint state = 0;

    nal_len = strlen (params[i]);
    buf = gst_buffer_new_and_alloc (nal_len);

    gst_buffer_map (buf, &map, GST_MAP_WRITE);
    nalp = map.data;
    nal_len = g_base64_decode_step (params[i], nal_len, nalp, &state, &save);
    gst_buffer_unmap (buf, &map);
    gst_buffer_resize (buf, 0, nal_len);

    if (!nal_len) {
      gst_buffer_unref (buf);
      continue;
    }

    gst_rtp_h264_add_sps_pps (GST_ELEMENT (rtph264pay), rtph264pay->sps,
        rtph264pay->pps, buf);
  }
  g_strfreev (params);
}

static guint
next_start_code (const guint8 * data, guint size)
{
  /* Boyer-Moore string matching algorithm, in a degenerative
   * sense because our search 'alphabet' is binary - 0 & 1 only.
   * This allow us to simplify the general BM algorithm to a very
   * simple form. */
  /* assume 1 is in the 3th byte */
  guint offset = 2;

  while (offset < size) {
    if (1 == data[offset]) {
      unsigned int shift = offset;

      if (0 == data[--shift]) {
        if (0 == data[--shift]) {
          return shift;
        }
      }
      /* The jump is always 3 because of the 1 previously matched.
       * All the 0's must be after this '1' matched at offset */
      offset += 3;
    } else if (0 == data[offset]) {
      /* maybe next byte is 1? */
      offset++;
    } else {
      /* can jump 3 bytes forward */
      offset += 3;
    }
    /* at each iteration, we rescan in a backward manner until
     * we match 0.0.1 in reverse order. Since our search string
     * has only 2 'alpabets' (i.e. 0 & 1), we know that any
     * mismatch will force us to shift a fixed number of steps */
  }
  GST_DEBUG ("Cannot find next NAL start code. returning %u", size);

  return size;
}

static gboolean
gst_rtp_h264_pay_decode_nal (GstRtpH264Pay * payloader,
    const guint8 * data, guint size, GstClockTime dts, GstClockTime pts)
{
  guint8 header, type;
  gboolean updated;

  /* default is no update */
  updated = FALSE;

  GST_DEBUG ("NAL payload len=%u", size);

  header = data[0];
  type = header & 0x1f;

  /* We record the timestamp of the last SPS/PPS so
   * that we can insert them at regular intervals and when needed. */
  if (SPS_TYPE_ID == type || PPS_TYPE_ID == type) {
    GstBuffer *nal;

    /* trailing 0x0 are not part of the SPS/PPS */
    while (size > 0 && data[size - 1] == 0x0)
      size--;

    /* encode the entire SPS NAL in base64 */
    GST_DEBUG ("Found %s %x %x %x Len=%u", type == SPS_TYPE_ID ? "SPS" : "PPS",
        (header >> 7), (header >> 5) & 3, type, size);

    nal = gst_buffer_new_allocate (NULL, size, NULL);
    gst_buffer_fill (nal, 0, data, size);

    updated = gst_rtp_h264_add_sps_pps (GST_ELEMENT (payloader),
        payloader->sps, payloader->pps, nal);

    /* remember when we last saw SPS */
    if (pts != -1)
      payloader->last_spspps =
          gst_segment_to_running_time (&GST_RTP_BASE_PAYLOAD_CAST
          (payloader)->segment, GST_FORMAT_TIME, pts);
  } else {
    GST_DEBUG ("NAL: %x %x %x Len = %u", (header >> 7),
        (header >> 5) & 3, type, size);
  }

  return updated;
}

static GstFlowReturn
gst_rtp_h264_pay_payload_nal (GstRTPBasePayload * basepayload,
    GstBuffer * paybuf, GstClockTime dts, GstClockTime pts, gboolean end_of_au,
    gboolean delta_unit, gboolean discont);

static GstFlowReturn
gst_rtp_h264_pay_payload_nal_single (GstRTPBasePayload * basepayload,
    GstBuffer * paybuf, GstClockTime dts, GstClockTime pts, gboolean end_of_au,
    gboolean delta_unit, gboolean discont);

static GstFlowReturn
gst_rtp_h264_pay_payload_nal_fragment (GstRTPBasePayload * basepayload,
    GstBuffer * paybuf, GstClockTime dts, GstClockTime pts, gboolean end_of_au,
    gboolean delta_unit, gboolean discont, guint8 nal_header);

static GstFlowReturn
gst_rtp_h264_pay_payload_nal_bundle (GstRTPBasePayload * basepayload,
    GstBuffer * paybuf, GstClockTime dts, GstClockTime pts, gboolean end_of_au,
    gboolean delta_unit, gboolean discont, guint8 nal_header);

static GstFlowReturn
gst_rtp_h264_pay_send_sps_pps (GstRTPBasePayload * basepayload,
    GstClockTime dts, GstClockTime pts, gboolean delta_unit, gboolean discont)
{
  GstRtpH264Pay *rtph264pay = GST_RTP_H264_PAY (basepayload);
  GstFlowReturn ret = GST_FLOW_OK;
  gboolean sent_all_sps_pps = TRUE;
  guint i;

  for (i = 0; i < rtph264pay->sps->len; i++) {
    GstBuffer *sps_buf =
        GST_BUFFER_CAST (g_ptr_array_index (rtph264pay->sps, i));

    GST_DEBUG_OBJECT (rtph264pay, "inserting SPS in the stream");
    /* resend SPS */
    ret = gst_rtp_h264_pay_payload_nal (basepayload, gst_buffer_ref (sps_buf),
        dts, pts, FALSE, delta_unit, discont);
    /* Not critical here; but throw a warning */
    if (ret != GST_FLOW_OK) {
      sent_all_sps_pps = FALSE;
      GST_WARNING_OBJECT (basepayload, "Problem pushing SPS");
    }
  }
  for (i = 0; i < rtph264pay->pps->len; i++) {
    GstBuffer *pps_buf =
        GST_BUFFER_CAST (g_ptr_array_index (rtph264pay->pps, i));

    GST_DEBUG_OBJECT (rtph264pay, "inserting PPS in the stream");
    /* resend PPS */
    ret = gst_rtp_h264_pay_payload_nal (basepayload, gst_buffer_ref (pps_buf),
        dts, pts, FALSE, TRUE, FALSE);
    /* Not critical here; but throw a warning */
    if (ret != GST_FLOW_OK) {
      sent_all_sps_pps = FALSE;
      GST_WARNING_OBJECT (basepayload, "Problem pushing PPS");
    }
  }

  if (pts != -1 && sent_all_sps_pps)
    rtph264pay->last_spspps =
        gst_segment_to_running_time (&basepayload->segment, GST_FORMAT_TIME,
        pts);

  return ret;
}

/* @delta_unit: if %FALSE the first packet sent won't have the
 * GST_BUFFER_FLAG_DELTA_UNIT flag.
 * @discont: if %TRUE the first packet sent will have the
 * GST_BUFFER_FLAG_DISCONT flag.
 */
static GstFlowReturn
gst_rtp_h264_pay_payload_nal (GstRTPBasePayload * basepayload,
    GstBuffer * paybuf, GstClockTime dts, GstClockTime pts, gboolean end_of_au,
    gboolean delta_unit, gboolean discont)
{
  GstRtpH264Pay *rtph264pay;
  guint8 nal_header, nal_type;
  gboolean send_spspps;
  guint size;

  rtph264pay = GST_RTP_H264_PAY (basepayload);
  size = gst_buffer_get_size (paybuf);

  gst_buffer_extract (paybuf, 0, &nal_header, 1);
  nal_type = nal_header & 0x1f;

  /* These payload type are reserved for STAP-A, STAP-B, MTAP16, and MTAP24
   * as internally used NAL types */
  switch (nal_type) {
    case 24:
    case 25:
    case 26:
    case 27:
      GST_WARNING_OBJECT (rtph264pay, "Ignoring reserved NAL TYPE=%d",
          nal_type);
      gst_buffer_unref (paybuf);
      return GST_FLOW_OK;
    default:
      break;
  }

  GST_DEBUG_OBJECT (rtph264pay,
      "payloading NAL Unit: datasize=%u type=%d pts=%" GST_TIME_FORMAT,
      size, nal_type, GST_TIME_ARGS (pts));

  /* should set src caps before pushing stuff,
   * and if we did not see enough SPS/PPS, that may not be the case */
  if (G_UNLIKELY (!gst_pad_has_current_caps (GST_RTP_BASE_PAYLOAD_SRCPAD
              (basepayload))))
    gst_rtp_h264_pay_set_sps_pps (basepayload);

  send_spspps = FALSE;

  /* check if we need to emit an SPS/PPS now */
  if (nal_type == IDR_TYPE_ID && rtph264pay->spspps_interval > 0) {
    if (rtph264pay->last_spspps != -1) {
      guint64 diff;
      GstClockTime running_time =
          gst_segment_to_running_time (&basepayload->segment, GST_FORMAT_TIME,
          pts);

      GST_LOG_OBJECT (rtph264pay,
          "now %" GST_TIME_FORMAT ", last SPS/PPS %" GST_TIME_FORMAT,
          GST_TIME_ARGS (running_time),
          GST_TIME_ARGS (rtph264pay->last_spspps));

      /* calculate diff between last SPS/PPS in milliseconds */
      if (running_time > rtph264pay->last_spspps)
        diff = running_time - rtph264pay->last_spspps;
      else
        diff = 0;

      GST_DEBUG_OBJECT (rtph264pay,
          "interval since last SPS/PPS %" GST_TIME_FORMAT,
          GST_TIME_ARGS (diff));

      /* bigger than interval, queue SPS/PPS */
      if (GST_TIME_AS_SECONDS (diff) >= rtph264pay->spspps_interval) {
        GST_DEBUG_OBJECT (rtph264pay, "time to send SPS/PPS");
        send_spspps = TRUE;
      }
    } else {
      /* no know previous SPS/PPS time, send now */
      GST_DEBUG_OBJECT (rtph264pay, "no previous SPS/PPS time, send now");
      send_spspps = TRUE;
    }
  } else if (nal_type == IDR_TYPE_ID && rtph264pay->spspps_interval == -1) {
    GST_DEBUG_OBJECT (rtph264pay, "sending SPS/PPS before current IDR frame");
    /* send SPS/PPS before every IDR frame */
    send_spspps = TRUE;
  }

  if (send_spspps || rtph264pay->send_spspps) {
    /* we need to send SPS/PPS now first. FIXME, don't use the pts for
     * checking when we need to send SPS/PPS but convert to running_time first. */
    GstFlowReturn ret;

    rtph264pay->send_spspps = FALSE;

    ret = gst_rtp_h264_pay_send_sps_pps (basepayload, dts, pts, delta_unit,
        discont);
    if (ret != GST_FLOW_OK) {
      gst_buffer_unref (paybuf);
      return ret;
    }

    delta_unit = TRUE;
    discont = FALSE;
  }

  if (rtph264pay->aggregate_mode != GST_RTP_H264_AGGREGATE_NONE)
    return gst_rtp_h264_pay_payload_nal_bundle (basepayload, paybuf, dts, pts,
        end_of_au, delta_unit, discont, nal_header);

  return gst_rtp_h264_pay_payload_nal_fragment (basepayload, paybuf, dts, pts,
      end_of_au, delta_unit, discont, nal_header);
}

static GstFlowReturn
gst_rtp_h264_pay_payload_nal_fragment (GstRTPBasePayload * basepayload,
    GstBuffer * paybuf, GstClockTime dts, GstClockTime pts, gboolean end_of_au,
    gboolean delta_unit, gboolean discont, guint8 nal_header)
{
  GstRtpH264Pay *rtph264pay;
  guint mtu, size, max_fragment_size, max_fragments, ii, pos;
  GstBuffer *outbuf;
  guint8 *payload;
  GstBufferList *list = NULL;
  GstRTPBuffer rtp = { NULL };

  rtph264pay = GST_RTP_H264_PAY (basepayload);
  mtu = GST_RTP_BASE_PAYLOAD_MTU (rtph264pay);
  size = gst_buffer_get_size (paybuf);

  if (gst_rtp_buffer_calc_packet_len (size, 0, 0) <= mtu) {
    /* We don't need to fragment this packet */
    GST_DEBUG_OBJECT (rtph264pay,
        "sending NAL Unit: datasize=%u mtu=%u", size, mtu);
    return gst_rtp_h264_pay_payload_nal_single (basepayload, paybuf, dts, pts,
        end_of_au, delta_unit, discont);
  }

  GST_DEBUG_OBJECT (basepayload,
      "using FU-A fragmentation for NAL Unit: datasize=%u mtu=%u", size, mtu);

  /* We keep 2 bytes for FU indicator and FU Header */
  max_fragment_size = gst_rtp_buffer_calc_payload_len (mtu - 2, 0, 0);
  max_fragments = (size + max_fragment_size - 2) / max_fragment_size;
  list = gst_buffer_list_new_sized (max_fragments);

  /* Start at the NALU payload */
  for (pos = 1, ii = 0; pos < size; pos += max_fragment_size, ii++) {
    guint remaining, fragment_size;
    gboolean first_fragment, last_fragment;

    remaining = size - pos;
    fragment_size = MIN (remaining, max_fragment_size);
    first_fragment = (pos == 1);
    last_fragment = (remaining <= max_fragment_size);

    GST_DEBUG_OBJECT (basepayload,
        "creating FU-A packet %u/%u, size %u",
        ii + 1, max_fragments, fragment_size);

    /* use buffer lists
     * create buffer without payload containing only the RTP header
     * (memory block at index 0) */
    outbuf = gst_rtp_buffer_new_allocate (2, 0, 0);

    gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

    GST_BUFFER_DTS (outbuf) = dts;
    GST_BUFFER_PTS (outbuf) = pts;
    payload = gst_rtp_buffer_get_payload (&rtp);

    /* If it's the last fragment and the end of this au, mark the end of
     * slice */
    gst_rtp_buffer_set_marker (&rtp, last_fragment && end_of_au);

    /* FU indicator */
    payload[0] = (nal_header & 0x60) | FU_A_TYPE_ID;

    /* FU Header */
    payload[1] = (first_fragment << 7) | (last_fragment << 6) |
        (nal_header & 0x1f);

    gst_rtp_buffer_unmap (&rtp);

    /* insert payload memory block */
    gst_rtp_copy_video_meta (rtph264pay, outbuf, paybuf);
    gst_buffer_copy_into (outbuf, paybuf, GST_BUFFER_COPY_MEMORY, pos,
        fragment_size);

    if (!delta_unit)
      /* Only the first packet sent should not have the flag */
      delta_unit = TRUE;
    else
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

    if (discont) {
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
      /* Only the first packet sent should have the flag */
      discont = FALSE;
    }

    /* add the buffer to the buffer list */
    gst_buffer_list_add (list, outbuf);
  }

  GST_DEBUG_OBJECT (rtph264pay,
      "sending FU-A fragments: n=%u datasize=%u mtu=%u", ii, size, mtu);

  gst_buffer_unref (paybuf);
  return gst_rtp_base_payload_push_list (basepayload, list);
}

static GstFlowReturn
gst_rtp_h264_pay_payload_nal_single (GstRTPBasePayload * basepayload,
    GstBuffer * paybuf, GstClockTime dts, GstClockTime pts, gboolean end_of_au,
    gboolean delta_unit, gboolean discont)
{
  GstRtpH264Pay *rtph264pay;
  GstBuffer *outbuf;
  GstRTPBuffer rtp = { NULL };

  rtph264pay = GST_RTP_H264_PAY (basepayload);

  /* create buffer without payload containing only the RTP header
   * (memory block at index 0) */
  outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);

  gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

  /* Mark the end of a frame */
  gst_rtp_buffer_set_marker (&rtp, end_of_au);

  /* timestamp the outbuffer */
  GST_BUFFER_PTS (outbuf) = pts;
  GST_BUFFER_DTS (outbuf) = dts;

  if (delta_unit)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

  if (discont)
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);

  gst_rtp_buffer_unmap (&rtp);

  /* insert payload memory block */
  gst_rtp_copy_video_meta (rtph264pay, outbuf, paybuf);
  outbuf = gst_buffer_append (outbuf, paybuf);

  /* push the buffer to the next element */
  return gst_rtp_base_payload_push (basepayload, outbuf);
}

static void
gst_rtp_h264_pay_reset_bundle (GstRtpH264Pay * rtph264pay)
{
  g_clear_pointer (&rtph264pay->bundle, gst_buffer_list_unref);
  rtph264pay->bundle_size = 0;
  rtph264pay->bundle_contains_vcl = FALSE;
}

static GstFlowReturn
gst_rtp_h264_pay_send_bundle (GstRtpH264Pay * rtph264pay, gboolean end_of_au)
{
  GstRTPBasePayload *basepayload;
  GstBufferList *bundle;
  guint length, bundle_size;
  GstBuffer *first, *outbuf;
  GstClockTime dts, pts;
  gboolean delta, discont;

  bundle_size = rtph264pay->bundle_size;

  if (bundle_size == 0) {
    GST_DEBUG_OBJECT (rtph264pay, "no bundle, nothing to send");
    return GST_FLOW_OK;
  }

  basepayload = GST_RTP_BASE_PAYLOAD (rtph264pay);
  bundle = rtph264pay->bundle;
  length = gst_buffer_list_length (bundle);

  first = gst_buffer_list_get (bundle, 0);
  dts = GST_BUFFER_DTS (first);
  pts = GST_BUFFER_PTS (first);
  delta = GST_BUFFER_FLAG_IS_SET (first, GST_BUFFER_FLAG_DELTA_UNIT);
  discont = GST_BUFFER_FLAG_IS_SET (first, GST_BUFFER_FLAG_DISCONT);

  if (length == 1) {
    /* Push unaggregated NALU */
    outbuf = gst_buffer_ref (first);

    GST_DEBUG_OBJECT (rtph264pay,
        "sending NAL Unit unaggregated: datasize=%u", bundle_size - 2);
  } else {
    guint8 stap_header;
    guint i;

    outbuf = gst_buffer_new_allocate (NULL, sizeof stap_header, NULL);
    stap_header = STAP_A_TYPE_ID;

    for (i = 0; i < length; i++) {
      GstBuffer *buf = gst_buffer_list_get (bundle, i);
      guint8 nal_header;
      GstMemory *size_header;
      GstMapInfo map;

      gst_buffer_extract (buf, 0, &nal_header, sizeof nal_header);

      /* Propagate F bit */
      if ((nal_header & 0x80))
        stap_header |= 0x80;

      /* Select highest nal_ref_idc */
      if ((nal_header & 0x60) > (stap_header & 0x60))
        stap_header = (stap_header & 0x9f) | (nal_header & 0x60);

      /* append NALU size */
      size_header = gst_allocator_alloc (NULL, 2, NULL);
      gst_memory_map (size_header, &map, GST_MAP_WRITE);
      GST_WRITE_UINT16_BE (map.data, gst_buffer_get_size (buf));
      gst_memory_unmap (size_header, &map);
      gst_buffer_append_memory (outbuf, size_header);

      /* append NALU data */
      outbuf = gst_buffer_append (outbuf, gst_buffer_ref (buf));
    }

    gst_buffer_fill (outbuf, 0, &stap_header, sizeof stap_header);

    GST_DEBUG_OBJECT (rtph264pay,
        "sending STAP-A bundle: n=%u header=%02x datasize=%u",
        length, stap_header, bundle_size);
  }

  gst_rtp_h264_pay_reset_bundle (rtph264pay);
  return gst_rtp_h264_pay_payload_nal_single (basepayload, outbuf, dts, pts,
      end_of_au, delta, discont);
}

static gboolean
gst_rtp_h264_pay_payload_nal_bundle (GstRTPBasePayload * basepayload,
    GstBuffer * paybuf, GstClockTime dts, GstClockTime pts, gboolean end_of_au,
    gboolean delta_unit, gboolean discont, guint8 nal_header)
{
  GstRtpH264Pay *rtph264pay;
  GstFlowReturn ret;
  guint mtu, pay_size, bundle_size;
  GstBufferList *bundle;
  guint8 nal_type;
  gboolean start_of_au;

  rtph264pay = GST_RTP_H264_PAY (basepayload);
  nal_type = nal_header & 0x1f;
  mtu = GST_RTP_BASE_PAYLOAD_MTU (rtph264pay);
  pay_size = 2 + gst_buffer_get_size (paybuf);
  bundle = rtph264pay->bundle;
  start_of_au = FALSE;

  if (bundle) {
    GstBuffer *first = gst_buffer_list_get (bundle, 0);

    if (nal_type == AUD_TYPE_ID) {
      GST_DEBUG_OBJECT (rtph264pay, "found access delimiter");
      start_of_au = TRUE;
    } else if (discont) {
      GST_DEBUG_OBJECT (rtph264pay, "found discont");
      start_of_au = TRUE;
    } else if (GST_BUFFER_PTS (first) != pts || GST_BUFFER_DTS (first) != dts) {
      GST_DEBUG_OBJECT (rtph264pay, "found timestamp mismatch");
      start_of_au = TRUE;
    }
  }

  if (start_of_au) {
    GST_DEBUG_OBJECT (rtph264pay, "sending bundle before start of AU");

    ret = gst_rtp_h264_pay_send_bundle (rtph264pay, TRUE);
    if (ret != GST_FLOW_OK)
      goto out;

    bundle = NULL;
  }

  bundle_size = 1 + pay_size;

  if (gst_rtp_buffer_calc_packet_len (bundle_size, 0, 0) > mtu) {
    GST_DEBUG_OBJECT (rtph264pay, "NAL Unit cannot fit in a bundle");

    ret = gst_rtp_h264_pay_send_bundle (rtph264pay, FALSE);
    if (ret != GST_FLOW_OK)
      goto out;

    return gst_rtp_h264_pay_payload_nal_fragment (basepayload, paybuf, dts, pts,
        end_of_au, delta_unit, discont, nal_header);
  }

  bundle_size = rtph264pay->bundle_size + pay_size;

  if (gst_rtp_buffer_calc_packet_len (bundle_size, 0, 0) > mtu) {
    GST_DEBUG_OBJECT (rtph264pay,
        "bundle overflows, sending: bundlesize=%u datasize=2+%u mtu=%u",
        rtph264pay->bundle_size, pay_size - 2, mtu);

    ret = gst_rtp_h264_pay_send_bundle (rtph264pay, FALSE);
    if (ret != GST_FLOW_OK)
      goto out;

    bundle = NULL;
  }

  if (!bundle) {
    GST_DEBUG_OBJECT (rtph264pay, "creating new STAP-A aggregate");
    bundle = rtph264pay->bundle = gst_buffer_list_new ();
    bundle_size = rtph264pay->bundle_size = 1;
    rtph264pay->bundle_contains_vcl = FALSE;
  }

  GST_DEBUG_OBJECT (rtph264pay,
      "bundling NAL Unit: bundlesize=%u datasize=2+%u mtu=%u",
      rtph264pay->bundle_size, pay_size - 2, mtu);

  paybuf = gst_buffer_make_writable (paybuf);
  GST_BUFFER_PTS (paybuf) = pts;
  GST_BUFFER_DTS (paybuf) = dts;

  if (delta_unit)
    GST_BUFFER_FLAG_SET (paybuf, GST_BUFFER_FLAG_DELTA_UNIT);
  else
    GST_BUFFER_FLAG_UNSET (paybuf, GST_BUFFER_FLAG_DELTA_UNIT);

  if (discont)
    GST_BUFFER_FLAG_SET (paybuf, GST_BUFFER_FLAG_DISCONT);
  else
    GST_BUFFER_FLAG_UNSET (paybuf, GST_BUFFER_FLAG_DISCONT);

  gst_buffer_list_add (bundle, gst_buffer_ref (paybuf));
  rtph264pay->bundle_size += pay_size;
  ret = GST_FLOW_OK;

  if ((nal_type >= 1 && nal_type <= 5) || nal_type == 14 ||
      (nal_type >= 20 && nal_type <= 23))
    rtph264pay->bundle_contains_vcl = TRUE;

  if (end_of_au) {
    GST_DEBUG_OBJECT (rtph264pay, "sending bundle at end of AU");
    ret = gst_rtp_h264_pay_send_bundle (rtph264pay, TRUE);
  }

out:
  gst_buffer_unref (paybuf);
  return ret;
}

static GstFlowReturn
gst_rtp_h264_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpH264Pay *rtph264pay;
  GstFlowReturn ret;
  gsize size;
  guint nal_len, i;
  GstMapInfo map;
  const guint8 *data;
  GstClockTime dts, pts;
  GArray *nal_queue;
  gboolean avc;
  GstBuffer *paybuf = NULL;
  gsize skip;
  gboolean delayed_not_delta_unit = FALSE;
  gboolean delayed_discont = FALSE;
  gboolean marker = FALSE;
  gboolean draining = (buffer == NULL);

  rtph264pay = GST_RTP_H264_PAY (basepayload);

  /* the input buffer contains one or more NAL units */

  avc = rtph264pay->stream_format == GST_H264_STREAM_FORMAT_AVC;

  if (avc) {
    /* In AVC mode, there is no adapter, so nothing to drain */
    if (draining)
      return GST_FLOW_OK;
    gst_buffer_map (buffer, &map, GST_MAP_READ);
    data = map.data;
    size = map.size;
    pts = GST_BUFFER_PTS (buffer);
    dts = GST_BUFFER_DTS (buffer);
    rtph264pay->delta_unit = GST_BUFFER_FLAG_IS_SET (buffer,
        GST_BUFFER_FLAG_DELTA_UNIT);
    rtph264pay->discont = GST_BUFFER_IS_DISCONT (buffer);
    marker = GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_MARKER);
    GST_DEBUG_OBJECT (basepayload, "got %" G_GSIZE_FORMAT " bytes", size);
  } else {
    if (buffer) {
      if (!GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
        if (gst_adapter_available (rtph264pay->adapter) == 0)
          rtph264pay->delta_unit = FALSE;
        else
          /* This buffer contains a key frame but the adapter isn't empty. So
           * we'll purge it first by sending a first packet and then the second
           * one won't have the DELTA_UNIT flag. */
          delayed_not_delta_unit = TRUE;
      }

      if (GST_BUFFER_IS_DISCONT (buffer)) {
        if (gst_adapter_available (rtph264pay->adapter) == 0)
          rtph264pay->discont = TRUE;
        else
          /* This buffer has the DISCONT flag but the adapter isn't empty. So
           * we'll purge it first by sending a first packet and then the second
           * one will have the DISCONT flag set. */
          delayed_discont = TRUE;
      }

      marker = GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_MARKER);
      gst_adapter_push (rtph264pay->adapter, buffer);
      buffer = NULL;
    }

    /* We want to use the first TS used to construct the following NAL */
    dts = gst_adapter_prev_dts (rtph264pay->adapter, NULL);
    pts = gst_adapter_prev_pts (rtph264pay->adapter, NULL);

    size = gst_adapter_available (rtph264pay->adapter);
    /* Nothing to do here if the adapter is empty, e.g. on EOS */
    if (size == 0)
      return GST_FLOW_OK;
    data = gst_adapter_map (rtph264pay->adapter, size);
    GST_DEBUG_OBJECT (basepayload, "got %" G_GSIZE_FORMAT " bytes", size);
  }

  ret = GST_FLOW_OK;

  /* now loop over all NAL units and put them in a packet */
  if (avc) {
    guint nal_length_size;
    gsize offset = 0;

    nal_length_size = rtph264pay->nal_length_size;

    while (size > nal_length_size) {
      gint i;
      gboolean end_of_au = FALSE;

      nal_len = 0;
      for (i = 0; i < nal_length_size; i++) {
        nal_len = ((nal_len << 8) + data[i]);
      }

      /* skip the length bytes, make sure we don't run past the buffer size */
      data += nal_length_size;
      offset += nal_length_size;
      size -= nal_length_size;

      if (size >= nal_len) {
        GST_DEBUG_OBJECT (basepayload, "got NAL of size %u", nal_len);
      } else {
        nal_len = size;
        GST_DEBUG_OBJECT (basepayload, "got incomplete NAL of size %u",
            nal_len);
      }

      /* If we're at the end of the buffer, then we're at the end of the
       * access unit
       */
      if (size - nal_len <= nal_length_size) {
        if (rtph264pay->alignment == GST_H264_ALIGNMENT_AU || marker)
          end_of_au = TRUE;
      }

      paybuf = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_ALL, offset,
          nal_len);
      ret =
          gst_rtp_h264_pay_payload_nal (basepayload, paybuf, dts, pts,
          end_of_au, rtph264pay->delta_unit, rtph264pay->discont);

      if (!rtph264pay->delta_unit)
        /* Only the first outgoing packet doesn't have the DELTA_UNIT flag */
        rtph264pay->delta_unit = TRUE;

      if (rtph264pay->discont)
        /* Only the first outgoing packet have the DISCONT flag */
        rtph264pay->discont = FALSE;

      if (ret != GST_FLOW_OK)
        break;

      data += nal_len;
      offset += nal_len;
      size -= nal_len;
    }
  } else {
    guint next;
    gboolean update = FALSE;

    /* get offset of first start code */
    next = next_start_code (data, size);

    /* skip to start code, if no start code is found, next will be size and we
     * will not collect data. */
    data += next;
    size -= next;
    nal_queue = rtph264pay->queue;
    skip = next;

    /* array must be empty when we get here */
    g_assert (nal_queue->len == 0);

    GST_DEBUG_OBJECT (basepayload,
        "found first start at %u, bytes left %" G_GSIZE_FORMAT, next, size);

    /* first pass to locate NALs and parse SPS/PPS */
    while (size > 4) {
      /* skip start code */
      data += 3;
      size -= 3;

      /* use next_start_code() to scan buffer.
       * next_start_code() returns the offset in data,
       * starting from zero to the first byte of 0.0.0.1
       * If no start code is found, it returns the value of the
       * 'size' parameter.
       * data is unchanged by the call to next_start_code()
       */
      next = next_start_code (data, size);

      /* nal or au aligned input needs no delaying until next time */
      if (next == size && !draining &&
          rtph264pay->alignment == GST_H264_ALIGNMENT_UNKNOWN) {
        /* Didn't find the start of next NAL and it's not EOS,
         * handle it next time */
        break;
      }

      /* nal length is distance to next start code */
      nal_len = next;

      GST_DEBUG_OBJECT (basepayload, "found next start at %u of size %u", next,
          nal_len);

      if (rtph264pay->sprop_parameter_sets != NULL) {
        /* explicitly set profile and sprop, use those */
        if (rtph264pay->update_caps) {
          if (!gst_rtp_base_payload_set_outcaps (basepayload,
                  "sprop-parameter-sets", G_TYPE_STRING,
                  rtph264pay->sprop_parameter_sets, NULL))
            goto caps_rejected;

          /* parse SPS and PPS from provided parameter set (for insertion) */
          gst_rtp_h264_pay_parse_sprop_parameter_sets (rtph264pay);

          rtph264pay->update_caps = FALSE;

          GST_DEBUG ("outcaps update: sprop-parameter-sets=%s",
              rtph264pay->sprop_parameter_sets);
        }
      } else {
        /* We know our stream is a valid H264 NAL packet,
         * go parse it for SPS/PPS to enrich the caps */
        /* order: make sure to check nal */
        update =
            gst_rtp_h264_pay_decode_nal (rtph264pay, data, nal_len, dts, pts)
            || update;
      }
      /* move to next NAL packet */
      data += nal_len;
      size -= nal_len;

      g_array_append_val (nal_queue, nal_len);
    }

    /* if has new SPS & PPS, update the output caps */
    if (G_UNLIKELY (update))
      if (!gst_rtp_h264_pay_set_sps_pps (basepayload))
        goto caps_rejected;

    /* second pass to payload and push */

    if (nal_queue->len != 0)
      gst_adapter_flush (rtph264pay->adapter, skip);

    for (i = 0; i < nal_queue->len; i++) {
      guint size;
      gboolean end_of_au = FALSE;

      nal_len = g_array_index (nal_queue, guint, i);
      /* skip start code */
      gst_adapter_flush (rtph264pay->adapter, 3);

      /* Trim the end unless we're the last NAL in the stream.
       * In case we're not at the end of the buffer we know the next block
       * starts with 0x000001 so all the 0x00 bytes at the end of this one are
       * trailing 0x0 that can be discarded */
      size = nal_len;
      data = gst_adapter_map (rtph264pay->adapter, size);
      if (i + 1 != nal_queue->len || !draining)
        for (; size > 1 && data[size - 1] == 0x0; size--)
          /* skip */ ;


      /* If it's the last nal unit we have in non-bytestream mode, we can
       * assume it's the end of an access-unit
       *
       * FIXME: We need to wait until the next packet or EOS to
       * actually payload the NAL so we can know if the current NAL is
       * the last one of an access unit or not if we are in bytestream mode
       */
      if (i == nal_queue->len - 1) {
        if (rtph264pay->alignment == GST_H264_ALIGNMENT_AU ||
            marker || draining)
          end_of_au = TRUE;
      }
      paybuf = gst_adapter_take_buffer (rtph264pay->adapter, size);
      g_assert (paybuf);

      /* put the data in one or more RTP packets */
      ret =
          gst_rtp_h264_pay_payload_nal (basepayload, paybuf, dts, pts,
          end_of_au, rtph264pay->delta_unit, rtph264pay->discont);

      if (delayed_not_delta_unit) {
        rtph264pay->delta_unit = FALSE;
        delayed_not_delta_unit = FALSE;
      } else {
        /* Only the first outgoing packet doesn't have the DELTA_UNIT flag */
        rtph264pay->delta_unit = TRUE;
      }

      if (delayed_discont) {
        rtph264pay->discont = TRUE;
        delayed_discont = FALSE;
      } else {
        /* Only the first outgoing packet have the DISCONT flag */
        rtph264pay->discont = FALSE;
      }

      if (ret != GST_FLOW_OK) {
        break;
      }

      /* move to next NAL packet */
      /* Skips the trailing zeros */
      gst_adapter_flush (rtph264pay->adapter, nal_len - size);
    }
    g_array_set_size (nal_queue, 0);
  }

  if (ret == GST_FLOW_OK && rtph264pay->bundle_size > 0 &&
      rtph264pay->aggregate_mode == GST_RTP_H264_AGGREGATE_ZERO_LATENCY &&
      rtph264pay->bundle_contains_vcl) {
    GST_DEBUG_OBJECT (rtph264pay, "sending bundle at end incoming packet");
    ret = gst_rtp_h264_pay_send_bundle (rtph264pay, FALSE);
  }


done:
  if (avc) {
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);
  } else {
    gst_adapter_unmap (rtph264pay->adapter);
  }

  return ret;

caps_rejected:
  {
    GST_WARNING_OBJECT (basepayload, "Could not set outcaps");
    g_array_set_size (nal_queue, 0);
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto done;
  }
}

static gboolean
gst_rtp_h264_pay_sink_event (GstRTPBasePayload * payload, GstEvent * event)
{
  gboolean res;
  const GstStructure *s;
  GstRtpH264Pay *rtph264pay = GST_RTP_H264_PAY (payload);
  GstFlowReturn ret = GST_FLOW_OK;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_adapter_clear (rtph264pay->adapter);
      gst_rtp_h264_pay_reset_bundle (rtph264pay);
      break;
    case GST_EVENT_CUSTOM_DOWNSTREAM:
      s = gst_event_get_structure (event);
      if (gst_structure_has_name (s, "GstForceKeyUnit")) {
        gboolean resend_codec_data;

        if (gst_structure_get_boolean (s, "all-headers",
                &resend_codec_data) && resend_codec_data)
          rtph264pay->send_spspps = TRUE;
      }
      break;
    case GST_EVENT_EOS:
    {
      /* call handle_buffer with NULL to flush last NAL from adapter
       * in byte-stream mode
       */
      gst_rtp_h264_pay_handle_buffer (payload, NULL);
      ret = gst_rtp_h264_pay_send_bundle (rtph264pay, TRUE);
      break;
    }
    case GST_EVENT_STREAM_START:
      GST_DEBUG_OBJECT (rtph264pay, "New stream detected => Clear SPS and PPS");
      gst_rtp_h264_pay_clear_sps_pps (rtph264pay);
      ret = gst_rtp_h264_pay_send_bundle (rtph264pay, TRUE);
      break;
    default:
      break;
  }

  if (ret != GST_FLOW_OK)
    return FALSE;

  res = GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->sink_event (payload, event);

  return res;
}

static GstStateChangeReturn
gst_rtp_h264_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstRtpH264Pay *rtph264pay = GST_RTP_H264_PAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      rtph264pay->send_spspps = FALSE;
      gst_adapter_clear (rtph264pay->adapter);
      gst_rtp_h264_pay_reset_bundle (rtph264pay);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      rtph264pay->last_spspps = -1;
      gst_rtp_h264_pay_clear_sps_pps (rtph264pay);
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_rtp_h264_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpH264Pay *rtph264pay;

  rtph264pay = GST_RTP_H264_PAY (object);

  switch (prop_id) {
    case PROP_SPROP_PARAMETER_SETS:
      g_free (rtph264pay->sprop_parameter_sets);
      rtph264pay->sprop_parameter_sets = g_value_dup_string (value);
      rtph264pay->update_caps = TRUE;
      break;
    case PROP_CONFIG_INTERVAL:
      rtph264pay->spspps_interval = g_value_get_int (value);
      break;
    case PROP_AGGREGATE_MODE:
      rtph264pay->aggregate_mode = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_h264_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpH264Pay *rtph264pay;

  rtph264pay = GST_RTP_H264_PAY (object);

  switch (prop_id) {
    case PROP_SPROP_PARAMETER_SETS:
      g_value_set_string (value, rtph264pay->sprop_parameter_sets);
      break;
    case PROP_CONFIG_INTERVAL:
      g_value_set_int (value, rtph264pay->spspps_interval);
      break;
    case PROP_AGGREGATE_MODE:
      g_value_set_enum (value, rtph264pay->aggregate_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_rtp_h264_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtph264pay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_H264_PAY);
}
