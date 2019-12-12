/* GStreamer DTMF source
 *
 * gstdtmfsrc.c:
 *
 * Copyright (C) <2007> Collabora.
 *   Contact: Youness Alaoui <youness.alaoui@collabora.co.uk>
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
 * SECTION:element-dtmfsrc
 * @title: dtmfsrc
 * @see_also: rtpdtmsrc, rtpdtmfmuxx
 *
 * The DTMFSrc element generates DTMF (ITU-T Q.23 Specification) tone packets on request
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
 * * `method` (G_TYPE_INT, 2): The method used for sending event, this element will react if this
 *   field is absent or 2.
 *
 * For example, the following code informs the pipeline (and in turn, the
 * DTMFSrc element inside the pipeline) about the start of a DTMF named
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
#include <math.h>

#include <glib.h>

#include "gstdtmfcommon.h"

#include "gstdtmfsrc.h"

#include <gst/audio/audio.h>

#define GST_TONE_DTMF_TYPE_EVENT 1
#define DEFAULT_PACKET_INTERVAL  50     /* ms */
#define MIN_PACKET_INTERVAL      10     /* ms */
#define MAX_PACKET_INTERVAL      50     /* ms */
#define DEFAULT_SAMPLE_RATE      8000
#define SAMPLE_SIZE              16
#define CHANNELS                 1
#define MIN_DUTY_CYCLE           (MIN_INTER_DIGIT_INTERVAL + MIN_PULSE_DURATION)


typedef struct st_dtmf_key
{
  const char *event_name;
  int event_encoding;
  float low_frequency;
  float high_frequency;
} DTMF_KEY;

static const DTMF_KEY DTMF_KEYS[] = {
  {"DTMF_KEY_EVENT_0", 0, 941, 1336},
  {"DTMF_KEY_EVENT_1", 1, 697, 1209},
  {"DTMF_KEY_EVENT_2", 2, 697, 1336},
  {"DTMF_KEY_EVENT_3", 3, 697, 1477},
  {"DTMF_KEY_EVENT_4", 4, 770, 1209},
  {"DTMF_KEY_EVENT_5", 5, 770, 1336},
  {"DTMF_KEY_EVENT_6", 6, 770, 1477},
  {"DTMF_KEY_EVENT_7", 7, 852, 1209},
  {"DTMF_KEY_EVENT_8", 8, 852, 1336},
  {"DTMF_KEY_EVENT_9", 9, 852, 1477},
  {"DTMF_KEY_EVENT_S", 10, 941, 1209},
  {"DTMF_KEY_EVENT_P", 11, 941, 1477},
  {"DTMF_KEY_EVENT_A", 12, 697, 1633},
  {"DTMF_KEY_EVENT_B", 13, 770, 1633},
  {"DTMF_KEY_EVENT_C", 14, 852, 1633},
  {"DTMF_KEY_EVENT_D", 15, 941, 1633},
};

#define MAX_DTMF_EVENTS 16

enum
{
  DTMF_KEY_EVENT_1 = 1,
  DTMF_KEY_EVENT_2 = 2,
  DTMF_KEY_EVENT_3 = 3,
  DTMF_KEY_EVENT_4 = 4,
  DTMF_KEY_EVENT_5 = 5,
  DTMF_KEY_EVENT_6 = 6,
  DTMF_KEY_EVENT_7 = 7,
  DTMF_KEY_EVENT_8 = 8,
  DTMF_KEY_EVENT_9 = 9,
  DTMF_KEY_EVENT_0 = 0,
  DTMF_KEY_EVENT_STAR = 10,
  DTMF_KEY_EVENT_POUND = 11,
  DTMF_KEY_EVENT_A = 12,
  DTMF_KEY_EVENT_B = 13,
  DTMF_KEY_EVENT_C = 14,
  DTMF_KEY_EVENT_D = 15,
};

GST_DEBUG_CATEGORY_STATIC (gst_dtmf_src_debug);
#define GST_CAT_DEFAULT gst_dtmf_src_debug

enum
{
  PROP_0,
  PROP_INTERVAL,
};

static GstStaticPadTemplate gst_dtmf_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) \"" GST_AUDIO_NE (S16) "\", "
        "rate = " GST_AUDIO_RATE_RANGE ", " "channels = (int) 1, "
        "layout = (string)interleaved")
    );

#define parent_class gst_dtmf_src_parent_class
G_DEFINE_TYPE (GstDTMFSrc, gst_dtmf_src, GST_TYPE_BASE_SRC);

static void gst_dtmf_src_finalize (GObject * object);

static void gst_dtmf_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dtmf_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_dtmf_src_handle_event (GstBaseSrc * src, GstEvent * event);
static gboolean gst_dtmf_src_send_event (GstElement * src, GstEvent * event);
static GstStateChangeReturn gst_dtmf_src_change_state (GstElement * element,
    GstStateChange transition);
static GstFlowReturn gst_dtmf_src_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);
static void gst_dtmf_src_add_start_event (GstDTMFSrc * dtmfsrc,
    gint event_number, gint event_volume);
static void gst_dtmf_src_add_stop_event (GstDTMFSrc * dtmfsrc);

static gboolean gst_dtmf_src_unlock (GstBaseSrc * src);

static gboolean gst_dtmf_src_unlock_stop (GstBaseSrc * src);
static gboolean gst_dtmf_src_negotiate (GstBaseSrc * basesrc);
static gboolean gst_dtmf_src_query (GstBaseSrc * basesrc, GstQuery * query);


static void
gst_dtmf_src_class_init (GstDTMFSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);


  GST_DEBUG_CATEGORY_INIT (gst_dtmf_src_debug, "dtmfsrc", 0, "dtmfsrc element");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_dtmf_src_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "DTMF tone generator", "Source/Audio", "Generates DTMF tones",
      "Youness Alaoui <youness.alaoui@collabora.co.uk>");


  gobject_class->finalize = gst_dtmf_src_finalize;
  gobject_class->set_property = gst_dtmf_src_set_property;
  gobject_class->get_property = gst_dtmf_src_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_INTERVAL,
      g_param_spec_uint ("interval", "Interval between tone packets",
          "Interval in ms between two tone packets", MIN_PACKET_INTERVAL,
          MAX_PACKET_INTERVAL, DEFAULT_PACKET_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_dtmf_src_change_state);
  gstelement_class->send_event = GST_DEBUG_FUNCPTR (gst_dtmf_src_send_event);
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_dtmf_src_unlock);
  gstbasesrc_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_dtmf_src_unlock_stop);

  gstbasesrc_class->event = GST_DEBUG_FUNCPTR (gst_dtmf_src_handle_event);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_dtmf_src_create);
  gstbasesrc_class->negotiate = GST_DEBUG_FUNCPTR (gst_dtmf_src_negotiate);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_dtmf_src_query);
}

static void
event_free (GstDTMFSrcEvent * event)
{
  if (event)
    g_slice_free (GstDTMFSrcEvent, event);
}

static void
gst_dtmf_src_init (GstDTMFSrc * dtmfsrc)
{
  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (dtmfsrc), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (dtmfsrc), TRUE);

  dtmfsrc->interval = DEFAULT_PACKET_INTERVAL;

  dtmfsrc->event_queue = g_async_queue_new_full ((GDestroyNotify) event_free);
  dtmfsrc->last_event = NULL;

  dtmfsrc->sample_rate = DEFAULT_SAMPLE_RATE;

  GST_DEBUG_OBJECT (dtmfsrc, "init done");
}

static void
gst_dtmf_src_finalize (GObject * object)
{
  GstDTMFSrc *dtmfsrc;

  dtmfsrc = GST_DTMF_SRC (object);

  if (dtmfsrc->event_queue) {
    g_async_queue_unref (dtmfsrc->event_queue);
    dtmfsrc->event_queue = NULL;
  }

  G_OBJECT_CLASS (gst_dtmf_src_parent_class)->finalize (object);
}

static gboolean
gst_dtmf_src_handle_dtmf_event (GstDTMFSrc * dtmfsrc, GstEvent * event)
{
  const GstStructure *event_structure;
  GstStateChangeReturn sret;
  GstState state;
  gint event_type;
  gboolean start;
  gint method;
  GstClockTime last_stop;
  gint event_number;
  gint event_volume;
  gboolean correct_order;

  sret = gst_element_get_state (GST_ELEMENT (dtmfsrc), &state, NULL, 0);
  if (sret != GST_STATE_CHANGE_SUCCESS || state != GST_STATE_PLAYING) {
    GST_DEBUG_OBJECT (dtmfsrc, "dtmf-event, but not in PLAYING state");
    goto failure;
  }

  event_structure = gst_event_get_structure (event);

  if (!gst_structure_get_int (event_structure, "type", &event_type) ||
      !gst_structure_get_boolean (event_structure, "start", &start) ||
      (start == TRUE && event_type != GST_TONE_DTMF_TYPE_EVENT))
    goto failure;

  if (gst_structure_get_int (event_structure, "method", &method)) {
    if (method != 2) {
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
    GST_DEBUG_OBJECT (dtmfsrc, "Received start event %d with volume %d",
        event_number, event_volume);
    gst_dtmf_src_add_start_event (dtmfsrc, event_number, event_volume);
  }

  else {
    GST_DEBUG_OBJECT (dtmfsrc, "Received stop event");
    gst_dtmf_src_add_stop_event (dtmfsrc);
  }

  return TRUE;
failure:
  return FALSE;
}

static gboolean
gst_dtmf_src_handle_event (GstBaseSrc * src, GstEvent * event)
{
  GstDTMFSrc *dtmfsrc;
  gboolean result = FALSE;

  dtmfsrc = GST_DTMF_SRC (src);

  GST_LOG_OBJECT (dtmfsrc, "Received an %s event on the src pad",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_UPSTREAM:
      if (gst_event_has_name (event, "dtmf-event")) {
        result = gst_dtmf_src_handle_dtmf_event (dtmfsrc, event);
        break;
      }
      /* fall through */
    default:
      result = GST_BASE_SRC_CLASS (parent_class)->event (src, event);
      break;
  }

  return result;
}


static gboolean
gst_dtmf_src_send_event (GstElement * element, GstEvent * event)
{
  GstDTMFSrc *dtmfsrc = GST_DTMF_SRC (element);
  gboolean ret;

  GST_LOG_OBJECT (dtmfsrc, "Received an %s event via send_event",
      GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CUSTOM_BOTH:
    case GST_EVENT_CUSTOM_BOTH_OOB:
    case GST_EVENT_CUSTOM_UPSTREAM:
    case GST_EVENT_CUSTOM_DOWNSTREAM:
    case GST_EVENT_CUSTOM_DOWNSTREAM_OOB:
      if (gst_event_has_name (event, "dtmf-event")) {
        ret = gst_dtmf_src_handle_dtmf_event (dtmfsrc, event);
        break;
      }
      /* fall through */
    default:
      ret = GST_ELEMENT_CLASS (parent_class)->send_event (element, event);
      break;
  }

  return ret;
}

static void
gst_dtmf_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDTMFSrc *dtmfsrc;

  dtmfsrc = GST_DTMF_SRC (object);

  switch (prop_id) {
    case PROP_INTERVAL:
      dtmfsrc->interval = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dtmf_src_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstDTMFSrc *dtmfsrc;

  dtmfsrc = GST_DTMF_SRC (object);

  switch (prop_id) {
    case PROP_INTERVAL:
      g_value_set_uint (value, dtmfsrc->interval);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dtmf_prepare_timestamps (GstDTMFSrc * dtmfsrc)
{
  GstClockTime last_stop;
  GstClockTime timestamp;

  GST_OBJECT_LOCK (dtmfsrc);
  last_stop = dtmfsrc->last_stop;
  GST_OBJECT_UNLOCK (dtmfsrc);

  if (GST_CLOCK_TIME_IS_VALID (last_stop)) {
    timestamp = last_stop;
  } else {
    GstClock *clock;

    /* If there is no valid start time, lets use now as the start time */

    clock = gst_element_get_clock (GST_ELEMENT (dtmfsrc));
    if (clock != NULL) {
      timestamp = gst_clock_get_time (clock)
          - gst_element_get_base_time (GST_ELEMENT (dtmfsrc));
      gst_object_unref (clock);
    } else {
      gchar *dtmf_name = gst_element_get_name (dtmfsrc);
      GST_ERROR_OBJECT (dtmfsrc, "No clock set for element %s", dtmf_name);
      dtmfsrc->timestamp = GST_CLOCK_TIME_NONE;
      g_free (dtmf_name);
      return;
    }
  }

  /* Make sure the timestamp always goes forward */
  if (timestamp > dtmfsrc->timestamp)
    dtmfsrc->timestamp = timestamp;
}

static void
gst_dtmf_src_add_start_event (GstDTMFSrc * dtmfsrc, gint event_number,
    gint event_volume)
{

  GstDTMFSrcEvent *event = g_slice_new0 (GstDTMFSrcEvent);
  event->event_type = DTMF_EVENT_TYPE_START;
  event->sample = 0;
  event->event_number = CLAMP (event_number, MIN_EVENT, MAX_EVENT);
  event->volume = CLAMP (event_volume, MIN_VOLUME, MAX_VOLUME);

  g_async_queue_push (dtmfsrc->event_queue, event);
}

static void
gst_dtmf_src_add_stop_event (GstDTMFSrc * dtmfsrc)
{

  GstDTMFSrcEvent *event = g_slice_new0 (GstDTMFSrcEvent);
  event->event_type = DTMF_EVENT_TYPE_STOP;
  event->sample = 0;
  event->event_number = 0;
  event->volume = 0;

  g_async_queue_push (dtmfsrc->event_queue, event);
}

static GstBuffer *
gst_dtmf_src_generate_silence (float duration, gint sample_rate)
{
  gint buf_size;

  /* Create a buffer with data set to 0 */
  buf_size = ((duration / 1000) * sample_rate * SAMPLE_SIZE * CHANNELS) / 8;

  return gst_buffer_new_wrapped (g_malloc0 (buf_size), buf_size);
}

static GstBuffer *
gst_dtmf_src_generate_tone (GstDTMFSrcEvent * event, DTMF_KEY key,
    float duration, gint sample_rate)
{
  GstBuffer *buffer;
  GstMapInfo map;
  gint16 *p;
  gint tone_size;
  double i = 0;
  double amplitude, f1, f2;
  double volume_factor;
  static GstAllocationParams params = { 0, 1, 0, 0, };

  /* Create a buffer for the tone */
  tone_size = ((duration / 1000) * sample_rate * SAMPLE_SIZE * CHANNELS) / 8;

  buffer = gst_buffer_new_allocate (NULL, tone_size, &params);

  gst_buffer_map (buffer, &map, GST_MAP_READWRITE);
  p = (gint16 *) map.data;

  volume_factor = pow (10, (-event->volume) / 20);

  /*
   * For each sample point we calculate 'x' as the
   * the amplitude value.
   */
  for (i = 0; i < (tone_size / (SAMPLE_SIZE / 8)); i++) {
    /*
     * We add the fundamental frequencies together.
     */
    f1 = sin (2 * M_PI * key.low_frequency * (event->sample / sample_rate));
    f2 = sin (2 * M_PI * key.high_frequency * (event->sample / sample_rate));

    amplitude = (f1 + f2) / 2;

    /* Adjust the volume */
    amplitude *= volume_factor;

    /* Make the [-1:1] interval into a [-32767:32767] interval */
    amplitude *= 32767;

    /* Store it in the data buffer */
    *(p++) = (gint16) amplitude;

    (event->sample)++;
  }

  gst_buffer_unmap (buffer, &map);

  return buffer;
}



static GstBuffer *
gst_dtmf_src_create_next_tone_packet (GstDTMFSrc * dtmfsrc,
    GstDTMFSrcEvent * event)
{
  GstBuffer *buf = NULL;
  gboolean send_silence = FALSE;

  GST_LOG_OBJECT (dtmfsrc, "Creating buffer for tone %s",
      DTMF_KEYS[event->event_number].event_name);

  if (event->packet_count * dtmfsrc->interval < MIN_INTER_DIGIT_INTERVAL) {
    send_silence = TRUE;
  }

  if (send_silence) {
    GST_LOG_OBJECT (dtmfsrc, "Generating silence");
    buf = gst_dtmf_src_generate_silence (dtmfsrc->interval,
        dtmfsrc->sample_rate);
  } else {
    GST_LOG_OBJECT (dtmfsrc, "Generating tone");
    buf = gst_dtmf_src_generate_tone (event, DTMF_KEYS[event->event_number],
        dtmfsrc->interval, dtmfsrc->sample_rate);
  }
  event->packet_count++;


  /* timestamp and duration of GstBuffer */
  GST_BUFFER_DURATION (buf) = dtmfsrc->interval * GST_MSECOND;
  GST_BUFFER_TIMESTAMP (buf) = dtmfsrc->timestamp;

  GST_LOG_OBJECT (dtmfsrc, "Creating new buffer with event %u duration "
      " gst: %" GST_TIME_FORMAT " at %" GST_TIME_FORMAT,
      event->event_number, GST_TIME_ARGS (GST_BUFFER_DURATION (buf)),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)));

  dtmfsrc->timestamp += GST_BUFFER_DURATION (buf);

  return buf;
}

static void
gst_dtmf_src_post_message (GstDTMFSrc * dtmfsrc, const gchar * message_name,
    GstDTMFSrcEvent * event)
{
  GstStructure *s = NULL;

  switch (event->event_type) {
    case DTMF_EVENT_TYPE_START:
      s = gst_structure_new (message_name,
          "type", G_TYPE_INT, 1,
          "method", G_TYPE_INT, 2,
          "start", G_TYPE_BOOLEAN, TRUE,
          "number", G_TYPE_INT, event->event_number,
          "volume", G_TYPE_INT, event->volume, NULL);
      break;
    case DTMF_EVENT_TYPE_STOP:
      s = gst_structure_new (message_name,
          "type", G_TYPE_INT, 1, "method", G_TYPE_INT, 2,
          "start", G_TYPE_BOOLEAN, FALSE, NULL);
      break;
    case DTMF_EVENT_TYPE_PAUSE_TASK:
      return;
  }

  if (s)
    gst_element_post_message (GST_ELEMENT (dtmfsrc),
        gst_message_new_element (GST_OBJECT (dtmfsrc), s));
}

static GstFlowReturn
gst_dtmf_src_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  GstBuffer *buf = NULL;
  GstDTMFSrcEvent *event;
  GstDTMFSrc *dtmfsrc;
  GstClock *clock;
  GstClockID *clockid;
  GstClockReturn clockret;

  dtmfsrc = GST_DTMF_SRC (basesrc);

  do {

    if (dtmfsrc->last_event == NULL) {
      GST_DEBUG_OBJECT (dtmfsrc, "popping");
      event = g_async_queue_pop (dtmfsrc->event_queue);

      GST_DEBUG_OBJECT (dtmfsrc, "popped %d", event->event_type);

      switch (event->event_type) {
        case DTMF_EVENT_TYPE_STOP:
          GST_WARNING_OBJECT (dtmfsrc,
              "Received a DTMF stop event when already stopped");
          gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-dropped", event);
          break;
        case DTMF_EVENT_TYPE_START:
          gst_dtmf_prepare_timestamps (dtmfsrc);

          event->packet_count = 0;
          dtmfsrc->last_event = event;
          event = NULL;
          gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-processed",
              dtmfsrc->last_event);
          break;
        case DTMF_EVENT_TYPE_PAUSE_TASK:
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
      if (event)
        g_slice_free (GstDTMFSrcEvent, event);
    } else if (dtmfsrc->last_event->packet_count * dtmfsrc->interval >=
        MIN_DUTY_CYCLE) {
      event = g_async_queue_try_pop (dtmfsrc->event_queue);

      if (event != NULL) {

        switch (event->event_type) {
          case DTMF_EVENT_TYPE_START:
            GST_WARNING_OBJECT (dtmfsrc,
                "Received two consecutive DTMF start events");
            gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-dropped", event);
            break;
          case DTMF_EVENT_TYPE_STOP:
            g_slice_free (GstDTMFSrcEvent, dtmfsrc->last_event);
            dtmfsrc->last_event = NULL;
            gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-processed", event);
            break;
          case DTMF_EVENT_TYPE_PAUSE_TASK:
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
        g_slice_free (GstDTMFSrcEvent, event);
      }
    }
  } while (dtmfsrc->last_event == NULL);

  GST_LOG_OBJECT (dtmfsrc, "end event check, now wait for the proper time");

  clock = gst_element_get_clock (GST_ELEMENT (basesrc));

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

  if (clockret == GST_CLOCK_UNSCHEDULED) {
    goto paused;
  }

  buf = gst_dtmf_src_create_next_tone_packet (dtmfsrc, dtmfsrc->last_event);

  GST_LOG_OBJECT (dtmfsrc, "Created buffer of size %" G_GSIZE_FORMAT,
      gst_buffer_get_size (buf));
  *buffer = buf;

  return GST_FLOW_OK;

paused_locked:
  GST_OBJECT_UNLOCK (dtmfsrc);

paused:

  if (dtmfsrc->last_event) {
    GST_DEBUG_OBJECT (dtmfsrc, "Stopping current event");
    /* Don't forget to release the stream lock */
    g_slice_free (GstDTMFSrcEvent, dtmfsrc->last_event);
    dtmfsrc->last_event = NULL;
  }

  return GST_FLOW_FLUSHING;

}

static gboolean
gst_dtmf_src_unlock (GstBaseSrc * src)
{
  GstDTMFSrc *dtmfsrc = GST_DTMF_SRC (src);
  GstDTMFSrcEvent *event = NULL;

  GST_DEBUG_OBJECT (dtmfsrc, "Called unlock");

  GST_OBJECT_LOCK (dtmfsrc);
  dtmfsrc->paused = TRUE;
  if (dtmfsrc->clockid) {
    gst_clock_id_unschedule (dtmfsrc->clockid);
  }
  GST_OBJECT_UNLOCK (dtmfsrc);

  GST_DEBUG_OBJECT (dtmfsrc, "Pushing the PAUSE_TASK event on unlock request");
  event = g_slice_new0 (GstDTMFSrcEvent);
  event->event_type = DTMF_EVENT_TYPE_PAUSE_TASK;
  g_async_queue_push (dtmfsrc->event_queue, event);

  return TRUE;
}


static gboolean
gst_dtmf_src_unlock_stop (GstBaseSrc * src)
{
  GstDTMFSrc *dtmfsrc = GST_DTMF_SRC (src);

  GST_DEBUG_OBJECT (dtmfsrc, "Unlock stopped");

  GST_OBJECT_LOCK (dtmfsrc);
  dtmfsrc->paused = FALSE;
  GST_OBJECT_UNLOCK (dtmfsrc);

  return TRUE;
}


static gboolean
gst_dtmf_src_negotiate (GstBaseSrc * basesrc)
{
  GstDTMFSrc *dtmfsrc = GST_DTMF_SRC (basesrc);
  GstCaps *caps;
  GstStructure *s;
  gboolean ret;

  caps = gst_pad_get_allowed_caps (GST_BASE_SRC_PAD (basesrc));

  if (!caps)
    caps = gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD (basesrc));

  if (gst_caps_is_empty (caps)) {
    gst_caps_unref (caps);
    return FALSE;
  }

  caps = gst_caps_truncate (caps);

  caps = gst_caps_make_writable (caps);
  s = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (s, "rate", DEFAULT_SAMPLE_RATE);

  if (!gst_structure_get_int (s, "rate", &dtmfsrc->sample_rate)) {
    GST_ERROR_OBJECT (dtmfsrc, "Could not get rate");
    gst_caps_unref (caps);
    return FALSE;
  }

  ret = gst_pad_set_caps (GST_BASE_SRC_PAD (basesrc), caps);

  gst_caps_unref (caps);

  return ret;
}

static gboolean
gst_dtmf_src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  GstDTMFSrc *dtmfsrc = GST_DTMF_SRC (basesrc);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:
    {
      GstClockTime latency;

      latency = dtmfsrc->interval * GST_MSECOND;
      gst_query_set_latency (query, gst_base_src_is_live (basesrc), latency,
          GST_CLOCK_TIME_NONE);
      GST_DEBUG_OBJECT (dtmfsrc, "Reporting latency of %" GST_TIME_FORMAT,
          GST_TIME_ARGS (latency));
      res = TRUE;
    }
      break;
    default:
      res = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);
      break;
  }

  return res;
}

static GstStateChangeReturn
gst_dtmf_src_change_state (GstElement * element, GstStateChange transition)
{
  GstDTMFSrc *dtmfsrc;
  GstStateChangeReturn result;
  gboolean no_preroll = FALSE;
  GstDTMFSrcEvent *event = NULL;

  dtmfsrc = GST_DTMF_SRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      /* Flushing the event queue */
      event = g_async_queue_try_pop (dtmfsrc->event_queue);

      while (event != NULL) {
        gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-dropped", event);
        g_slice_free (GstDTMFSrcEvent, event);
        event = g_async_queue_try_pop (dtmfsrc->event_queue);
      }
      dtmfsrc->last_event_was_start = FALSE;
      dtmfsrc->timestamp = 0;
      no_preroll = TRUE;
      break;
    default:
      break;
  }

  if ((result =
          GST_ELEMENT_CLASS (gst_dtmf_src_parent_class)->change_state (element,
              transition)) == GST_STATE_CHANGE_FAILURE)
    goto failure;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      no_preroll = TRUE;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG_OBJECT (dtmfsrc, "Flushing event queue");
      /* Flushing the event queue */
      event = g_async_queue_try_pop (dtmfsrc->event_queue);

      while (event != NULL) {
        gst_dtmf_src_post_message (dtmfsrc, "dtmf-event-dropped", event);
        g_slice_free (GstDTMFSrcEvent, event);
        event = g_async_queue_try_pop (dtmfsrc->event_queue);
      }
      dtmfsrc->last_event_was_start = FALSE;

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

gboolean
gst_dtmf_src_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "dtmfsrc",
      GST_RANK_NONE, GST_TYPE_DTMF_SRC);
}
