/* GStreamer RTP DTMF source
 *
 * gstrtpdtmfsrc.c:
 *
 * Copyright (C) <2007> Nokia Corporation.
 *   Contact: Zeeshan Ali <zeeshan.ali@nokia.com>
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *               2000,2005 Wim Taymans <wim@fluendo.com>
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
 * SECTION:element-rtpdtmfsrc
 * @title: rtpdtmfsrc
 * @see_also: dtmfsrc, rtpdtmfdepay, rtpdtmfmux
 *
 * The RTPDTMFSrc element generates RTP DTMF (RFC 2833) event packets on request
 * from application. The application communicates the beginning and end of a
 * DTMF event using custom upstream gstreamer events. To report a DTMF event, an
 * application must send an event of type GST_EVENT_CUSTOM_UPSTREAM, having a
 * structure of name "dtmf-event" with fields set according to the following
 * table:
 *
 * * `type` (G_TYPE_INT, 0-1): The application uses this field to specify which of the two methods
 *   specified in RFC 2833 to use. The value should be 0 for tones and 1 for
 *   named events. Tones are specified by their frequencies and events are specified
 *   by their number. This element can only take events as input. Do not confuse
 *   with "method" which specified the output.
 *
 * * `number` (G_TYPE_INT, 0-15): The event number.
 *
 * * `volume` (G_TYPE_INT, 0-36): This field describes the power level of the tone, expressed in dBm0
 *   after dropping the sign. Power levels range from 0 to -63 dBm0. The range of
 *   valid DTMF is from 0 to -36 dBm0. Can be omitted if start is set to FALSE.
 *
 * * `start` (G_TYPE_BOOLEAN, True or False): Whether the event is starting or ending.
 *
 * * `method` (G_TYPE_INT, 1): The method used for sending event, this element will react if this
 *   field is absent or 1.
 *
 * For example, the following code informs the pipeline (and in turn, the
 * RTPDTMFSrc element inside the pipeline) about the start of an RTP DTMF named
 * event '1' of volume -25 dBm0:
 *
 * |[
 * structure = gst_structure_new ("dtmf-event",
 *                    "type", G_TYPE_INT, 1,
 *                    "number", G_TYPE_INT, 1,
 *                    "volume", G_TYPE_INT, 25,
 *                    "start", G_TYPE_BOOLEAN, TRUE, NULL);
 *
 * event = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, structure);
 * gst_element_send_event (pipeline, event);
 * ]|
 *
 * When a DTMF tone actually starts or stop, a "dtmf-event-processed"
 * element #GstMessage with the same fields as the "dtmf-event"
 * #GstEvent that was used to request the event. Also, if any event
 * has not been processed when the element goes from the PAUSED to the
 * READY state, then a "dtmf-event-dropped" message is posted on the
 * #GstBus in the order that they were received.
  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "gstrtpdtmfsrc.h"

#define GST_RTP_DTMF_TYPE_EVENT  1
#define DEFAULT_PTIME            40     /* ms */
#define DEFAULT_SSRC             -1
#define DEFAULT_PT               96
#define DEFAULT_TIMESTAMP_OFFSET -1
#define DEFAULT_SEQNUM_OFFSET    -1
#define DEFAULT_CLOCK_RATE       8000

#define DEFAULT_PACKET_REDUNDANCY 1
#define MIN_PACKET_REDUNDANCY 1
#define MAX_PACKET_REDUNDANCY 5

GST_DEBUG_CATEGORY_STATIC (gst_rtp_dtmf_src_debug);
#define GST_CAT_DEFAULT gst_rtp_dtmf_src_debug

/* signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SSRC,
  PROP_TIMESTAMP_OFFSET,
  PROP_SEQNUM_OFFSET,
  PROP_PT,
  PROP_CLOCK_RATE,
  PROP_TIMESTAMP,
  PROP_SEQNUM,
  PROP_REDUNDANCY
};

static GstStaticPadTemplate gst_rtp_dtmf_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) [ 96, 127 ], "
        "clock-rate = (int) [ 0, MAX ], "
        "encoding-name = (string) \"TELEPHONE-EVENT\"")
    /*  "events = (string) \"0-15\" */
    );


G_DEFINE_TYPE (GstRTPDTMFSrc, gst_rtp_dtmf_src, GST_TYPE_BASE_SRC);

static void gst_rtp_dtmf_src_finalize (GObject * object);

static void gst_rtp_dtmf_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_dtmf_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_rtp_dtmf_src_handle_event (GstBaseSrc * basesrc,
    GstEvent * event);
static GstStateChangeReturn gst_rtp_dtmf_src_change_state (GstElement * element,
    GstStateChange transition);
static void gst_rtp_dtmf_src_add_start_event (GstRTPDTMFSrc * dtmfsrc,
    gint event_number, gint event_volume);
static void gst_rtp_dtmf_src_add_stop_event (GstRTPDTMFSrc * dtmfsrc);

static gboolean gst_rtp_dtmf_src_unlock (GstBaseSrc * src);
static gboolean gst_rtp_dtmf_src_unlock_stop (GstBaseSrc * src);
static GstFlowReturn gst_rtp_dtmf_src_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);
static gboolean gst_rtp_dtmf_src_negotiate (GstBaseSrc * basesrc);
static gboolean gst_rtp_dtmf_src_query (GstBaseSrc * basesrc, GstQuery * query);


static void
gst_rtp_dtmf_src_class_init (GstRTPDTMFSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_rtp_dtmf_src_debug,
      "rtpdtmfsrc", 0, "rtpdtmfsrc element");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_dtmf_src_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP DTMF packet generator", "Source/Network",
      "Generates RTP DTMF packets", "Zeeshan Ali <zeeshan.ali@nokia.com>");

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_finalize);
  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_get_property);

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_TIMESTAMP,
      g_param_spec_uint ("timestamp", "Timestamp",
          "The RTP timestamp of the last processed packet",
          0, G_MAXUINT, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SEQNUM,
      g_param_spec_uint ("seqnum", "Sequence number",
          "The RTP sequence number of the last processed packet",
          0, G_MAXUINT, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_TIMESTAMP_OFFSET, g_param_spec_int ("timestamp-offset",
          "Timestamp Offset",
          "Offset to add to all outgoing timestamps (-1 = random)", -1,
          G_MAXINT, DEFAULT_TIMESTAMP_OFFSET,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SEQNUM_OFFSET,
      g_param_spec_int ("seqnum-offset", "Sequence number Offset",
          "Offset to add to all outgoing seqnum (-1 = random)", -1, G_MAXINT,
          DEFAULT_SEQNUM_OFFSET, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CLOCK_RATE,
      g_param_spec_uint ("clock-rate", "clockrate",
          "The clock-rate at which to generate the dtmf packets",
          0, G_MAXUINT, DEFAULT_CLOCK_RATE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SSRC,
      g_param_spec_uint ("ssrc", "SSRC",
          "The SSRC of the packets (-1 == random)",
          0, G_MAXUINT, DEFAULT_SSRC,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PT,
      g_param_spec_uint ("pt", "payload type",
          "The payload type of the packets",
          0, 0x80, DEFAULT_PT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_REDUNDANCY,
      g_param_spec_uint ("packet-redundancy", "Packet Redundancy",
          "Number of packets to send to indicate start and stop dtmf events",
          MIN_PACKET_REDUNDANCY, MAX_PACKET_REDUNDANCY,
          DEFAULT_PACKET_REDUNDANCY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_change_state);

  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_unlock);
  gstbasesrc_class->unlock_stop =
      GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_unlock_stop);

  gstbasesrc_class->event = GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_handle_event);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_create);
  gstbasesrc_class->negotiate = GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_negotiate);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_rtp_dtmf_src_query);
}

static void
gst_rtp_dtmf_src_event_free (GstRTPDTMFSrcEvent * event)
{
  if (event) {
    if (event->payload)
      g_slice_free (GstRTPDTMFPayload, event->payload);
    g_slice_free (GstRTPDTMFSrcEvent, event);
  }
}

static void
gst_rtp_dtmf_src_init (GstRTPDTMFSrc * object)
{
  gst_base_src_set_format (GST_BASE_SRC (object), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (object), TRUE);

  object->ssrc = DEFAULT_SSRC;
  object->seqnum_offset = DEFAULT_SEQNUM_OFFSET;
  object->ts_offset = DEFAULT_TIMESTAMP_OFFSET;
  object->pt = DEFAULT_PT;
  object->clock_rate = DEFAULT_CLOCK_RATE;
  object->ptime = DEFAULT_PTIME;
  object->packet_redundancy = DEFAULT_PACKET_REDUNDANCY;

  object->event_queue =
      g_async_queue_new_full ((GDestroyNotify) gst_rtp_dtmf_src_event_free);
  object->payload = NULL;

  GST_DEBUG_OBJECT (object, "init done");
}

static void
gst_rtp_dtmf_src_finalize (GObject * object)
{
  GstRTPDTMFSrc *dtmfsrc;

  dtmfsrc = GST_RTP_DTMF_SRC (object);

  if (dtmfsrc->event_queue) {
    g_async_queue_unref (dtmfsrc->event_queue);
    dtmfsrc->event_queue = NULL;
  }


  G_OBJECT_CLASS (gst_rtp_dtmf_src_parent_class)->finalize (object);
}

static gboolean
gst_rtp_dtmf_src_handle_dtmf_event (GstRTPDTMFSrc * dtmfsrc,
    const GstStructure * event_structure)
{
  gint event_type;
  gboolean start;
  gint method;
  GstClockTime last_stop;
  gint event_number;
  gint event_volume;
  gboolean correct_order;

  if (!gst_structure_get_int (event_structure, "type", &event_type) ||
      !gst_structure_get_boolean (event_structure, "start", &start) ||
      event_type != GST_RTP_DTMF_TYPE_EVENT)
    goto failure;

  if (gst_structure_get_int (event_structure, "method", &method)) {
    if (method != 1) {
      goto failure;
    }
  }

  if (start)
    if (!gst_structure_get_int (event_structure, "number", &event_number) ||
        !gst_structure_get_int (event_structure, "volume", &event_volume))
      goto failure;

  GST_OBJECT_LOCK (dtmfsrc);
  if (gst_structure_get_clock_time (event_structure, "last-stop", &last_stop))
    dtmfsrc->last_stop = last_stop;
  else
    dtmfsrc->last_stop = GST_CLOCK_TIME_NONE;
  correct_order = (start != dtmfsrc->last_event_was_start);
  dtmfsrc->last_event_was_start = start;
  GST_OBJECT_UNLOCK (dtmfsrc);

  if (!correct_order)
    goto failure;

  if (start) {
    if (!gst_structure_get_int (event_structure, "number", &event_number) ||
        !gst_structure_get_int (event_structure, "volume", &event_volume))
      goto failure;

    GST_DEBUG_OBJECT (dtmfsrc, "Received start event %d with volume %d",
        event_number, event_volume);
    gst_rtp_dtmf_src_add_start_event (dtmfsrc, event_number, event_volume);
  }

  else {
    GST_DEBUG_OBJECT (dtmfsrc, "Received stop event");
    gst_rtp_dtmf_src_add_stop_event (dtmfsrc);
  }

  return TRUE;
failure:
  return FALSE;
}

static gboolean
gst_rtp_dtmf_src_handle_custom_upstream (GstRTPDTMFSrc * dtmfsrc,
    GstEvent * event)
{
  gboolean result = FALSE;
  gchar *struct_str;
  const GstStructure *structure;

  GstState state;
  GstStateChangeReturn ret;

  ret = gst_element_get_state (GST_ELEMENT (dtmfsrc), &state, NULL, 0);
  if (ret != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PLAYING) {
    GST_DEBUG_OBJECT (dtmfsrc, "Received event while not in PLAYING state");
    goto ret;
  }

  GST_DEBUG_OBJECT (dtmfsrc, "Received event is of our interest");
  structure = gst_event_get_structure (event);
  struct_str = gst_structure_to_string (structure);
  GST_DEBUG_OBJECT (dtmfsrc, "Event has structure %s", struct_str);
  g_free (struct_str);
  if (structure && gst_structure_has_name (structure, "dtmf-event"))
    result = gst_rtp_dtmf_src_handle_dtmf_event (dtmfsrc, structure);

ret:
  return result;
}

static gboolean
gst_rtp_dtmf_src_handle_event (GstBaseSrc * basesrc, GstEvent * event)
{
  GstRTPDTMFSrc *dtmfsrc;
  gboolean result = FALSE;

  dtmfsrc = GST_RTP_DTMF_SRC (basesrc);

  GST_DEBUG_OBJECT (dtmfsrc, "Received an event on the src pad");
  if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_UPSTREAM) {
    result = gst_rtp_dtmf_src_handle_custom_upstream (dtmfsrc, event);
  }

  return result;
}

static void
gst_rtp_dtmf_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRTPDTMFSrc *dtmfsrc;

  dtmfsrc = GST_RTP_DTMF_SRC (object);

  switch (prop_id) {
    case PROP_TIMESTAMP_OFFSET:
      dtmfsrc->ts_offset = g_value_get_int (value);
      break;
    case PROP_SEQNUM_OFFSET:
      dtmfsrc->seqnum_offset = g_value_get_int (value);
      break;
    case PROP_CLOCK_RATE:
      dtmfsrc->clock_rate = g_value_get_uint (value);
      dtmfsrc->dirty = TRUE;
      break;
    case PROP_SSRC:
      dtmfsrc->ssrc = g_value_get_uint (value);
      break;
    case PROP_PT:
      dtmfsrc->pt = g_value_get_uint (value);
      dtmfsrc->dirty = TRUE;
      break;
    case PROP_REDUNDANCY:
      dtmfsrc->packet_redundancy = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_dtmf_src_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstRTPDTMFSrc *dtmfsrc;

  dtmfsrc = GST_RTP_DTMF_SRC (object);

  switch (prop_id) {
    case PROP_TIMESTAMP_OFFSET:
      g_value_set_int (value, dtmfsrc->ts_offset);
      break;
    case PROP_SEQNUM_OFFSET:
      g_value_set_int (value, dtmfsrc->seqnum_offset);
      break;
    case PROP_CLOCK_RATE:
      g_value_set_uint (value, dtmfsrc->clock_rate);
      break;
    case PROP_SSRC:
      g_value_set_uint (value, dtmfsrc->ssrc);
      break;
    case PROP_PT:
      g_value_set_uint (value, dtmfsrc->pt);
      break;
    case PROP_TIMESTAMP:
      g_value_set_uint (value, dtmfsrc->rtp_timestamp);
      break;
    case PROP_SEQNUM:
      g_value_set_uint (value, dtmfsrc->seqnum);
      break;
    case PROP_REDUNDANCY:
      g_value_set_uint (value, dtmfsrc->packet_redundancy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_rtp_dtmf_prepare_timestamps (GstRTPDTMFSrc * dtmfsrc)
{
  GstClockTime last_stop;

  GST_OBJECT_LOCK (dtmfsrc);
  last_stop = dtmfsrc->last_stop;
  GST_OBJECT_UNLOCK (dtmfsrc);

  if (GST_CLOCK_TIME_IS_VALID (last_stop)) {
    dtmfsrc->start_timestamp = last_stop;
  } else {
    GstClock *clock = gst_element_get_clock (GST_ELEMENT (dtmfsrc));

    if (clock == NULL)
      return FALSE;

    dtmfsrc->start_timestamp = gst_clock_get_time (clock)
        - gst_element_get_base_time (GST_ELEMENT (dtmfsrc));
    gst_object_unref (clock);
  }

  /* If the last stop was in the past, then lets add the buffers together */
  if (dtmfsrc->start_timestamp < dtmfsrc->timestamp)
    dtmfsrc->start_timestamp = dtmfsrc->timestamp;

  dtmfsrc->timestamp = dtmfsrc->start_timestamp;

  dtmfsrc->rtp_timestamp = dtmfsrc->ts_base +
      gst_util_uint64_scale_int (gst_segment_to_running_time (&GST_BASE_SRC
          (dtmfsrc)->segment, GST_FORMAT_TIME, dtmfsrc->timestamp),
      dtmfsrc->clock_rate, GST_SECOND);

  return TRUE;
}


static void
gst_rtp_dtmf_src_add_start_event (GstRTPDTMFSrc * dtmfsrc, gint event_number,
    gint event_volume)
{

  GstRTPDTMFSrcEvent *event = g_slice_new0 (GstRTPDTMFSrcEvent);
  event->event_type = RTP_DTMF_EVENT_TYPE_START;

  event->payload = g_slice_new0 (GstRTPDTMFPayload);
  event->payload->event = CLAMP (event_number, MIN_EVENT, MAX_EVENT);
  event->payload->volume = CLAMP (event_volume, MIN_VOLUME, MAX_VOLUME);

  g_async_queue_push (dtmfsrc->event_queue, event);
}

static void
gst_rtp_dtmf_src_add_stop_event (GstRTPDTMFSrc * dtmfsrc)
{

  GstRTPDTMFSrcEvent *event = g_slice_new0 (GstRTPDTMFSrcEvent);
  event->event_type = RTP_DTMF_EVENT_TYPE_STOP;

  g_async_queue_push (dtmfsrc->event_queue, event);
}


static void
gst_rtp_dtmf_prepare_rtp_headers (GstRTPDTMFSrc * dtmfsrc,
    GstRTPBuffer * rtpbuf)
{
  gst_rtp_buffer_set_ssrc (rtpbuf, dtmfsrc->current_ssrc);
  gst_rtp_buffer_set_payload_type (rtpbuf, dtmfsrc->pt);
  /* Only the very first packet gets a marker */
  if (dtmfsrc->first_packet) {
    gst_rtp_buffer_set_marker (rtpbuf, TRUE);
  } else if (dtmfsrc->last_packet) {
    dtmfsrc->payload->e = 1;
  }

  dtmfsrc->seqnum++;
  gst_rtp_buffer_set_seq (rtpbuf, dtmfsrc->seqnum);

  /* timestamp of RTP header */
  gst_rtp_buffer_set_timestamp (rtpbuf, dtmfsrc->rtp_timestamp);
}

static GstBuffer *
gst_rtp_dtmf_src_create_next_rtp_packet (GstRTPDTMFSrc * dtmfsrc)
{
  GstBuffer *buf;
  GstRTPDTMFPayload *payload;
  GstRTPBuffer rtpbuffer = GST_RTP_BUFFER_INIT;

  buf = gst_rtp_buffer_new_allocate (sizeof (GstRTPDTMFPayload), 0, 0);

  gst_rtp_buffer_map (buf, GST_MAP_READWRITE, &rtpbuffer);

  gst_rtp_dtmf_prepare_rtp_headers (dtmfsrc, &rtpbuffer);

  /* timestamp and duration of GstBuffer */
  /* Redundant buffer have no duration ... */
  if (dtmfsrc->redundancy_count > 1)
    GST_BUFFER_DURATION (buf) = 0;
  else
    GST_BUFFER_DURATION (buf) = dtmfsrc->ptime * GST_MSECOND;
  GST_BUFFER_PTS (buf) = dtmfsrc->timestamp;

  payload = (GstRTPDTMFPayload *) gst_rtp_buffer_get_payload (&rtpbuffer);

  /* copy payload and convert to network-byte order */
  memmove (payload, dtmfsrc->payload, sizeof (GstRTPDTMFPayload));

  payload->duration = g_htons (payload->duration);

  if (dtmfsrc->redundancy_count <= 1 && dtmfsrc->last_packet) {
    GstClockTime inter_digit_interval = MIN_INTER_DIGIT_INTERVAL;

    if (inter_digit_interval % dtmfsrc->ptime != 0)
      inter_digit_interval += dtmfsrc->ptime -
          (MIN_INTER_DIGIT_INTERVAL % dtmfsrc->ptime);

    GST_BUFFER_DURATION (buf) += inter_digit_interval * GST_MSECOND;
  }

  GST_LOG_OBJECT (dtmfsrc, "Creating new buffer with event %u duration "
      " gst: %" GST_TIME_FORMAT " at %" GST_TIME_FORMAT "(rtp ts:%u dur:%u)",
      dtmfsrc->payload->event, GST_TIME_ARGS (GST_BUFFER_DURATION (buf)),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)), dtmfsrc->rtp_timestamp,
      dtmfsrc->payload->duration);

  /* duration of DTMF payloadfor the NEXT packet */
  /* not updated for redundant packets */
  if (dtmfsrc->redundancy_count <= 1)
    dtmfsrc->payload->duration += dtmfsrc->ptime * dtmfsrc->clock_rate / 1000;

  if (GST_CLOCK_TIME_IS_VALID (dtmfsrc->timestamp))
    dtmfsrc->timestamp += GST_BUFFER_DURATION (buf);

  gst_rtp_buffer_unmap (&rtpbuffer);

  return buf;
}

static GstMessage *
gst_dtmf_src_prepare_message (GstRTPDTMFSrc * dtmfsrc,
    const gchar * message_name, GstRTPDTMFSrcEvent * event)
{
  GstStructure *s;

  switch (event->event_type) {
    case RTP_DTMF_EVENT_TYPE_START:
      s = gst_structure_new (message_name,
          "type", G_TYPE_INT, 1,
          "method", G_TYPE_INT, 1,
          "start", G_TYPE_BOOLEAN, TRUE,
          "number", G_TYPE_INT, event->payload->event,
          "volume", G_TYPE_INT, event->payload->volume, NULL);
      break;
    case RTP_DTMF_EVENT_TYPE_STOP:
      s = gst_structure_new (message_name,
          "type", G_TYPE_INT, 1, "method", G_TYPE_INT, 1,
          "start", G_TYPE_BOOLEAN, FALSE, NULL);
      break;
    case RTP_DTMF_EVENT_TYPE_PAUSE_TASK:
      return NULL;
    default:
      return NULL;
  }

  return gst_message_new_element (GST_OBJECT (dtmfsrc), s);
}

static void
gst_dtmf_src_post_message (GstRTPDTMFSrc * dtmfsrc, const gchar * message_name,
    GstRTPDTMFSrcEvent * event)
{
  GstMessage *m = gst_dtmf_src_prepare_message (dtmfsrc, message_name, event);


  if (m)
    gst_element_post_message (GST_ELEMENT (dtmfsrc), m);
}


static GstFlowReturn
gst_rtp_dtmf_src_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  GstRTPDTMFSrcEvent *event;
  GstRTPDTMFSrc *dtmfsrc;
  GstClock *clock;
  GstClockID *clockid;
  GstClockReturn clockret;
  GstMessage *message;
  GQueue messages = G_QUEUE_INIT;

  dtmfsrc = GST_RTP_DTMF_SRC (basesrc);

  do {

    if (dtmfsrc->payload == NULL) {
      GST_DEBUG_OBJECT (dtmfsrc, "popping");
      event = g_async_queue_pop (dtmfsrc->event_queue);

      GST_DEBUG_OBJECT (dtmfsrc, "popped %d", event->event_type);

      switch (event->event_type) {
        case RTP_DTMF_EVENT_TYPE_STOP:
          GST_WARNING_OBJECT (dtmfsrc,
              "Received a DTMF stop event when already stopped");
          gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-dropped", event);
          break;

        case RTP_DTMF_EVENT_TYPE_START:
          dtmfsrc->first_packet = TRUE;
          dtmfsrc->last_packet = FALSE;
          /* Set the redundancy on the first packet */
          dtmfsrc->redundancy_count = dtmfsrc->packet_redundancy;
          if (!gst_rtp_dtmf_prepare_timestamps (dtmfsrc))
            goto no_clock;

          g_queue_push_tail (&messages,
              gst_dtmf_src_prepare_message (dtmfsrc, "dtmf-event-processed",
                  event));
          dtmfsrc->payload = event->payload;
          dtmfsrc->payload->duration =
              dtmfsrc->ptime * dtmfsrc->clock_rate / 1000;
          event->payload = NULL;
          break;

        case RTP_DTMF_EVENT_TYPE_PAUSE_TASK:
          /*
           * We're pushing it back because it has to stay in there until
           * the task is really paused (and the queue will then be flushed
           */
          GST_OBJECT_LOCK (dtmfsrc);
          if (dtmfsrc->paused) {
            g_async_queue_push (dtmfsrc->event_queue, event);
            goto paused_locked;
          }
          GST_OBJECT_UNLOCK (dtmfsrc);
          break;
      }

      gst_rtp_dtmf_src_event_free (event);
    } else if (!dtmfsrc->first_packet && !dtmfsrc->last_packet &&
        (dtmfsrc->timestamp - dtmfsrc->start_timestamp) / GST_MSECOND >=
        MIN_PULSE_DURATION) {
      GST_DEBUG_OBJECT (dtmfsrc, "try popping");
      event = g_async_queue_try_pop (dtmfsrc->event_queue);


      if (event != NULL) {
        GST_DEBUG_OBJECT (dtmfsrc, "try popped %d", event->event_type);

        switch (event->event_type) {
          case RTP_DTMF_EVENT_TYPE_START:
            GST_WARNING_OBJECT (dtmfsrc,
                "Received two consecutive DTMF start events");
            gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-dropped", event);
            break;

          case RTP_DTMF_EVENT_TYPE_STOP:
            dtmfsrc->first_packet = FALSE;
            dtmfsrc->last_packet = TRUE;
            /* Set the redundancy on the last packet */
            dtmfsrc->redundancy_count = dtmfsrc->packet_redundancy;
            g_queue_push_tail (&messages,
                gst_dtmf_src_prepare_message (dtmfsrc, "dtmf-event-processed",
                    event));
            break;

          case RTP_DTMF_EVENT_TYPE_PAUSE_TASK:
            /*
             * We're pushing it back because it has to stay in there until
             * the task is really paused (and the queue will then be flushed)
             */
            GST_DEBUG_OBJECT (dtmfsrc, "pushing pause_task...");
            GST_OBJECT_LOCK (dtmfsrc);
            if (dtmfsrc->paused) {
              g_async_queue_push (dtmfsrc->event_queue, event);
              goto paused_locked;
            }
            GST_OBJECT_UNLOCK (dtmfsrc);
            break;
        }
        gst_rtp_dtmf_src_event_free (event);
      }
    }
  } while (dtmfsrc->payload == NULL);


  GST_DEBUG_OBJECT (dtmfsrc, "Processed events, now lets wait on the clock");

  clock = gst_element_get_clock (GST_ELEMENT (basesrc));
  if (!clock)
    goto no_clock;
  clockid = gst_clock_new_single_shot_id (clock, dtmfsrc->timestamp +
      gst_element_get_base_time (GST_ELEMENT (dtmfsrc)));
  gst_object_unref (clock);

  GST_OBJECT_LOCK (dtmfsrc);
  if (!dtmfsrc->paused) {
    dtmfsrc->clockid = clockid;
    GST_OBJECT_UNLOCK (dtmfsrc);

    clockret = gst_clock_id_wait (clockid, NULL);

    GST_OBJECT_LOCK (dtmfsrc);
    if (dtmfsrc->paused)
      clockret = GST_CLOCK_UNSCHEDULED;
  } else {
    clockret = GST_CLOCK_UNSCHEDULED;
  }
  gst_clock_id_unref (clockid);
  dtmfsrc->clockid = NULL;
  GST_OBJECT_UNLOCK (dtmfsrc);

  while ((message = g_queue_pop_head (&messages)) != NULL)
    gst_element_post_message (GST_ELEMENT (dtmfsrc), message);

  if (clockret == GST_CLOCK_UNSCHEDULED) {
    goto paused;
  }

send_last:

  if (dtmfsrc->dirty)
    if (!gst_rtp_dtmf_src_negotiate (basesrc))
      return GST_FLOW_NOT_NEGOTIATED;

  /* create buffer to hold the payload */
  *buffer = gst_rtp_dtmf_src_create_next_rtp_packet (dtmfsrc);

  if (dtmfsrc->redundancy_count)
    dtmfsrc->redundancy_count--;

  /* Only the very first one has a marker */
  dtmfsrc->first_packet = FALSE;

  /* This is the end of the event */
  if (dtmfsrc->last_packet == TRUE && dtmfsrc->redundancy_count == 0) {

    g_slice_free (GstRTPDTMFPayload, dtmfsrc->payload);
    dtmfsrc->payload = NULL;

    dtmfsrc->last_packet = FALSE;
  }

  return GST_FLOW_OK;

paused_locked:

  GST_OBJECT_UNLOCK (dtmfsrc);

paused:

  if (dtmfsrc->payload) {
    dtmfsrc->first_packet = FALSE;
    dtmfsrc->last_packet = TRUE;
    /* Set the redundanc on the last packet */
    dtmfsrc->redundancy_count = dtmfsrc->packet_redundancy;
    goto send_last;
  } else {
    return GST_FLOW_FLUSHING;
  }

no_clock:
  GST_ELEMENT_ERROR (dtmfsrc, STREAM, MUX, ("No available clock"),
      ("No available clock"));
  gst_pad_pause_task (GST_BASE_SRC_PAD (dtmfsrc));
  return GST_FLOW_ERROR;
}


static gboolean
gst_rtp_dtmf_src_negotiate (GstBaseSrc * basesrc)
{
  GstCaps *srccaps, *peercaps;
  GstRTPDTMFSrc *dtmfsrc = GST_RTP_DTMF_SRC (basesrc);
  gboolean ret;

  /* fill in the defaults, there properties cannot be negotiated. */
  srccaps = gst_caps_new_simple ("application/x-rtp",
      "media", G_TYPE_STRING, "audio",
      "encoding-name", G_TYPE_STRING, "TELEPHONE-EVENT", NULL);

  /* the peer caps can override some of the defaults */
  peercaps = gst_pad_peer_query_caps (GST_BASE_SRC_PAD (basesrc), NULL);
  if (peercaps == NULL) {
    /* no peer caps, just add the other properties */
    gst_caps_set_simple (srccaps,
        "payload", G_TYPE_INT, dtmfsrc->pt,
        "ssrc", G_TYPE_UINT, dtmfsrc->current_ssrc,
        "timestamp-offset", G_TYPE_UINT, dtmfsrc->ts_base,
        "clock-rate", G_TYPE_INT, dtmfsrc->clock_rate,
        "seqnum-offset", G_TYPE_UINT, dtmfsrc->seqnum_base, NULL);

    GST_DEBUG_OBJECT (dtmfsrc, "no peer caps: %" GST_PTR_FORMAT, srccaps);
  } else {
    GstCaps *temp;
    GstStructure *s;
    const GValue *value;
    gint pt;
    gint clock_rate;

    /* peer provides caps we can use to fixate, intersect. This always returns a
     * writable caps. */
    temp = gst_caps_intersect (srccaps, peercaps);
    gst_caps_unref (srccaps);
    gst_caps_unref (peercaps);

    if (!temp) {
      GST_DEBUG_OBJECT (dtmfsrc, "Could not get intersection with peer caps");
      return FALSE;
    }

    if (gst_caps_is_empty (temp)) {
      GST_DEBUG_OBJECT (dtmfsrc, "Intersection with peer caps is empty");
      gst_caps_unref (temp);
      return FALSE;
    }

    /* now fixate, start by taking the first caps */
    temp = gst_caps_truncate (temp);
    temp = gst_caps_make_writable (temp);
    srccaps = temp;

    /* get first structure */
    s = gst_caps_get_structure (srccaps, 0);

    if (gst_structure_get_int (s, "payload", &pt)) {
      /* use peer pt */
      dtmfsrc->pt = pt;
      GST_LOG_OBJECT (dtmfsrc, "using peer pt %d", pt);
    } else {
      if (gst_structure_has_field (s, "payload")) {
        /* can only fixate if there is a field */
        gst_structure_fixate_field_nearest_int (s, "payload", dtmfsrc->pt);
        gst_structure_get_int (s, "payload", &pt);
        GST_LOG_OBJECT (dtmfsrc, "using peer pt %d", pt);
      } else {
        /* no pt field, use the internal pt */
        pt = dtmfsrc->pt;
        gst_structure_set (s, "payload", G_TYPE_INT, pt, NULL);
        GST_LOG_OBJECT (dtmfsrc, "using internal pt %d", pt);
      }
    }

    if (gst_structure_get_int (s, "clock-rate", &clock_rate)) {
      dtmfsrc->clock_rate = clock_rate;
      GST_LOG_OBJECT (dtmfsrc, "using clock-rate from caps %d",
          dtmfsrc->clock_rate);
    } else {
      GST_LOG_OBJECT (dtmfsrc, "using existing clock-rate %d",
          dtmfsrc->clock_rate);
    }
    gst_structure_set (s, "clock-rate", G_TYPE_INT, dtmfsrc->clock_rate, NULL);


    if (gst_structure_has_field_typed (s, "ssrc", G_TYPE_UINT)) {
      value = gst_structure_get_value (s, "ssrc");
      dtmfsrc->current_ssrc = g_value_get_uint (value);
      GST_LOG_OBJECT (dtmfsrc, "using peer ssrc %08x", dtmfsrc->current_ssrc);
    } else {
      /* FIXME, fixate_nearest_uint would be even better */
      gst_structure_set (s, "ssrc", G_TYPE_UINT, dtmfsrc->current_ssrc, NULL);
      GST_LOG_OBJECT (dtmfsrc, "using internal ssrc %08x",
          dtmfsrc->current_ssrc);
    }

    if (gst_structure_has_field_typed (s, "timestamp-offset", G_TYPE_UINT)) {
      value = gst_structure_get_value (s, "timestamp-offset");
      dtmfsrc->ts_base = g_value_get_uint (value);
      GST_LOG_OBJECT (dtmfsrc, "using peer timestamp-offset %u",
          dtmfsrc->ts_base);
    } else {
      /* FIXME, fixate_nearest_uint would be even better */
      gst_structure_set (s, "timestamp-offset", G_TYPE_UINT, dtmfsrc->ts_base,
          NULL);
      GST_LOG_OBJECT (dtmfsrc, "using internal timestamp-offset %u",
          dtmfsrc->ts_base);
    }
    if (gst_structure_has_field_typed (s, "seqnum-offset", G_TYPE_UINT)) {
      value = gst_structure_get_value (s, "seqnum-offset");
      dtmfsrc->seqnum_base = g_value_get_uint (value);
      GST_LOG_OBJECT (dtmfsrc, "using peer seqnum-offset %u",
          dtmfsrc->seqnum_base);
    } else {
      /* FIXME, fixate_nearest_uint would be even better */
      gst_structure_set (s, "seqnum-offset", G_TYPE_UINT, dtmfsrc->seqnum_base,
          NULL);
      GST_LOG_OBJECT (dtmfsrc, "using internal seqnum-offset %u",
          dtmfsrc->seqnum_base);
    }

    if (gst_structure_has_field_typed (s, "ptime", G_TYPE_UINT)) {
      value = gst_structure_get_value (s, "ptime");
      dtmfsrc->ptime = g_value_get_uint (value);
      GST_LOG_OBJECT (dtmfsrc, "using peer ptime %u", dtmfsrc->ptime);
    } else if (gst_structure_has_field_typed (s, "maxptime", G_TYPE_UINT)) {
      value = gst_structure_get_value (s, "maxptime");
      dtmfsrc->ptime = g_value_get_uint (value);
      GST_LOG_OBJECT (dtmfsrc, "using peer maxptime as ptime %u",
          dtmfsrc->ptime);
    } else {
      /* FIXME, fixate_nearest_uint would be even better */
      gst_structure_set (s, "ptime", G_TYPE_UINT, dtmfsrc->ptime, NULL);
      GST_LOG_OBJECT (dtmfsrc, "using internal ptime %u", dtmfsrc->ptime);
    }


    GST_DEBUG_OBJECT (dtmfsrc, "with peer caps: %" GST_PTR_FORMAT, srccaps);
  }

  ret = gst_pad_set_caps (GST_BASE_SRC_PAD (basesrc), srccaps);
  gst_caps_unref (srccaps);

  dtmfsrc->dirty = FALSE;

  return ret;

}

static gboolean
gst_rtp_dtmf_src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  GstRTPDTMFSrc *dtmfsrc = GST_RTP_DTMF_SRC (basesrc);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:
    {
      GstClockTime latency;

      latency = dtmfsrc->ptime * GST_MSECOND;
      gst_query_set_latency (query, gst_base_src_is_live (basesrc), latency,
          GST_CLOCK_TIME_NONE);
      GST_DEBUG_OBJECT (dtmfsrc, "Reporting latency of %" GST_TIME_FORMAT,
          GST_TIME_ARGS (latency));
      res = TRUE;
    }
      break;
    default:
      res = GST_BASE_SRC_CLASS (gst_rtp_dtmf_src_parent_class)->query (basesrc,
          query);
      break;
  }

  return res;
}

static void
gst_rtp_dtmf_src_ready_to_paused (GstRTPDTMFSrc * dtmfsrc)
{
  if (dtmfsrc->ssrc == -1)
    dtmfsrc->current_ssrc = g_random_int ();
  else
    dtmfsrc->current_ssrc = dtmfsrc->ssrc;

  if (dtmfsrc->seqnum_offset == -1)
    dtmfsrc->seqnum_base = g_random_int_range (0, G_MAXUINT16);
  else
    dtmfsrc->seqnum_base = dtmfsrc->seqnum_offset;
  dtmfsrc->seqnum = dtmfsrc->seqnum_base;

  if (dtmfsrc->ts_offset == -1)
    dtmfsrc->ts_base = g_random_int ();
  else
    dtmfsrc->ts_base = dtmfsrc->ts_offset;

  dtmfsrc->timestamp = 0;
}

static GstStateChangeReturn
gst_rtp_dtmf_src_change_state (GstElement * element, GstStateChange transition)
{
  GstRTPDTMFSrc *dtmfsrc;
  GstStateChangeReturn result;
  gboolean no_preroll = FALSE;
  GstRTPDTMFSrcEvent *event = NULL;

  dtmfsrc = GST_RTP_DTMF_SRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_dtmf_src_ready_to_paused (dtmfsrc);

      /* Flushing the event queue */
      while ((event = g_async_queue_try_pop (dtmfsrc->event_queue)) != NULL) {
        gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-dropped", event);
        gst_rtp_dtmf_src_event_free (event);
      }
      dtmfsrc->last_event_was_start = FALSE;

      no_preroll = TRUE;
      break;
    default:
      break;
  }

  if ((result =
          GST_ELEMENT_CLASS (gst_rtp_dtmf_src_parent_class)->change_state
          (element, transition)) == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      no_preroll = TRUE;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:

      /* Flushing the event queue */
      while ((event = g_async_queue_try_pop (dtmfsrc->event_queue)) != NULL) {
        gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-dropped", event);
        gst_rtp_dtmf_src_event_free (event);
      }
      dtmfsrc->last_event_was_start = FALSE;

      /* Indicate that we don't do PRE_ROLL */
      break;

    default:
      break;
  }

  if (no_preroll && result == GST_STATE_CHANGE_SUCCESS)
    result = GST_STATE_CHANGE_NO_PREROLL;

  return result;

  /* ERRORS */
failure:
  {
    GST_ERROR_OBJECT (dtmfsrc, "parent failed state change");
    return result;
  }
}


static gboolean
gst_rtp_dtmf_src_unlock (GstBaseSrc * src)
{
  GstRTPDTMFSrc *dtmfsrc = GST_RTP_DTMF_SRC (src);
  GstRTPDTMFSrcEvent *event = NULL;

  GST_DEBUG_OBJECT (dtmfsrc, "Called unlock");

  GST_OBJECT_LOCK (dtmfsrc);
  dtmfsrc->paused = TRUE;
  if (dtmfsrc->clockid) {
    gst_clock_id_unschedule (dtmfsrc->clockid);
  }
  GST_OBJECT_UNLOCK (dtmfsrc);

  GST_DEBUG_OBJECT (dtmfsrc, "Pushing the PAUSE_TASK event on unlock request");
  event = g_slice_new0 (GstRTPDTMFSrcEvent);
  event->event_type = RTP_DTMF_EVENT_TYPE_PAUSE_TASK;
  g_async_queue_push (dtmfsrc->event_queue, event);

  return TRUE;
}


static gboolean
gst_rtp_dtmf_src_unlock_stop (GstBaseSrc * src)
{
  GstRTPDTMFSrc *dtmfsrc = GST_RTP_DTMF_SRC (src);

  GST_DEBUG_OBJECT (dtmfsrc, "Unlock stopped");

  GST_OBJECT_LOCK (dtmfsrc);
  dtmfsrc->paused = FALSE;
  GST_OBJECT_UNLOCK (dtmfsrc);

  return TRUE;
}

gboolean
gst_rtp_dtmf_src_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpdtmfsrc",
      GST_RANK_NONE, GST_TYPE_RTP_DTMF_SRC);
}
