/* GStreamer v4l2 radio tuner element
 * Copyright (C) 2010, 2011 Alexey Chernov <4ernov@gmail.com>
 *
 * gstv4l2radio.c - V4L2 radio tuner element
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
 * SECTION:element-v4l2radio
 * @title: v4l2radio
 *
 * v4l2radio can be used to control radio device
 * and to tune it to different radiostations.
 *
 * ## Example launch lines
 * |[
 * gst-launch-1.0 v4l2radio device=/dev/radio0 frequency=101200000
 * gst-launch-1.0 alsasrc device=hw:1 ! audioconvert ! audioresample ! alsasink
 * ]|
 * First pipeline tunes the radio device /dev/radio0 to station 101.2 MHz,
 * second pipeline connects digital audio out (hw:1) to default sound card.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "gst/gst-i18n-plugin.h"

#include "gstv4l2object.h"
#include "gstv4l2tuner.h"
#include "gstv4l2radio.h"

GST_DEBUG_CATEGORY_STATIC (v4l2radio_debug);
#define GST_CAT_DEFAULT v4l2radio_debug

#define DEFAULT_PROP_DEVICE "/dev/radio0"
#define MIN_FREQUENCY		 87500000
#define DEFAULT_FREQUENCY	100000000
#define MAX_FREQUENCY		108000000

enum
{
  ARG_0,
  ARG_DEVICE,
  ARG_FREQUENCY
};

static gboolean
gst_v4l2radio_fill_channel_list (GstV4l2Radio * radio)
{
  int res;
  struct v4l2_tuner vtun;
  struct v4l2_capability vc;
  GstV4l2TunerChannel *v4l2channel;
  GstTunerChannel *channel;

  GstElement *e;

  GstV4l2Object *v4l2object;

  e = GST_ELEMENT (radio);
  v4l2object = radio->v4l2object;

  GST_DEBUG_OBJECT (e, "getting audio enumeration");
  GST_V4L2_CHECK_OPEN (v4l2object);

  GST_DEBUG_OBJECT (e, "  audio input");

  memset (&vc, 0, sizeof (vc));

  res = v4l2object->ioctl (v4l2object->video_fd, VIDIOC_QUERYCAP, &vc);
  if (res < 0)
    goto caps_failed;

  if (vc.capabilities & V4L2_CAP_DEVICE_CAPS)
    v4l2object->device_caps = vc.device_caps;
  else
    v4l2object->device_caps = vc.capabilities;

  if (!(v4l2object->device_caps & V4L2_CAP_TUNER))
    goto not_a_tuner;

  /* getting audio input */
  memset (&vtun, 0, sizeof (vtun));
  vtun.index = 0;

  res = v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_TUNER, &vtun);
  if (res < 0)
    goto tuner_failed;

  GST_LOG_OBJECT (e, "   index:     %d", vtun.index);
  GST_LOG_OBJECT (e, "   name:      '%s'", vtun.name);
  GST_LOG_OBJECT (e, "   type:      %016x", (guint) vtun.type);
  GST_LOG_OBJECT (e, "   caps:      %016x", (guint) vtun.capability);
  GST_LOG_OBJECT (e, "   rlow:      %016x", (guint) vtun.rangelow);
  GST_LOG_OBJECT (e, "   rhigh:     %016x", (guint) vtun.rangehigh);
  GST_LOG_OBJECT (e, "   audmode:   %016x", (guint) vtun.audmode);

  v4l2channel = g_object_new (GST_TYPE_V4L2_TUNER_CHANNEL, NULL);
  channel = GST_TUNER_CHANNEL (v4l2channel);
  channel->label = g_strdup ((const gchar *) vtun.name);
  channel->flags = GST_TUNER_CHANNEL_FREQUENCY | GST_TUNER_CHANNEL_AUDIO;
  v4l2channel->index = 0;
  v4l2channel->tuner = 0;

  channel->freq_multiplicator =
      62.5 * ((vtun.capability & V4L2_TUNER_CAP_LOW) ? 1 : 1000);
  channel->min_frequency = vtun.rangelow * channel->freq_multiplicator;
  channel->max_frequency = vtun.rangehigh * channel->freq_multiplicator;
  channel->min_signal = 0;
  channel->max_signal = 0xffff;

  v4l2object->channels =
      g_list_prepend (v4l2object->channels, (gpointer) channel);

  v4l2object->channels = g_list_reverse (v4l2object->channels);

  GST_DEBUG_OBJECT (e, "done");
  return TRUE;

  /* ERRORS */
tuner_failed:
  {
    GST_ELEMENT_ERROR (e, RESOURCE, SETTINGS,
        (_("Failed to get settings of tuner %d on device '%s'."),
            vtun.index, v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
caps_failed:
  {
    GST_ELEMENT_ERROR (e, RESOURCE, SETTINGS,
        (_("Error getting capabilities for device '%s'."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
not_a_tuner:
  {
    GST_ELEMENT_ERROR (e, RESOURCE, SETTINGS,
        (_("Device '%s' is not a tuner."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

static gboolean
gst_v4l2radio_get_input (GstV4l2Object * v4l2object, gint * input)
{
  GST_DEBUG_OBJECT (v4l2object->element, "trying to get radio input");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (!v4l2object->channels)
    goto input_failed;

  *input = 0;

  GST_DEBUG_OBJECT (v4l2object->element, "input: %d", 0);

  return TRUE;

  /* ERRORS */
input_failed:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to get radio input on device '%s'. "),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

static gboolean
gst_v4l2radio_set_input (GstV4l2Object * v4l2object, gint input)
{
  GST_DEBUG_OBJECT (v4l2object->element, "trying to set input to %d", input);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (!v4l2object->channels)
    goto input_failed;

  return TRUE;

  /* ERRORS */
input_failed:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to set input %d on device %s."),
            input, v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

static gboolean
gst_v4l2radio_set_mute_on (GstV4l2Radio * radio, gboolean on)
{
  gint res;
  struct v4l2_control vctrl;

  GST_DEBUG_OBJECT (radio, "setting current tuner mute state: %d", on);

  if (!GST_V4L2_IS_OPEN (radio->v4l2object))
    return FALSE;

  memset (&vctrl, 0, sizeof (vctrl));
  vctrl.id = V4L2_CID_AUDIO_MUTE;
  vctrl.value = on;

  GST_DEBUG_OBJECT (radio, "radio fd: %d", radio->v4l2object->video_fd);

  res = ioctl (radio->v4l2object->video_fd, VIDIOC_S_CTRL, &vctrl);
  GST_DEBUG_OBJECT (radio, "mute state change result: %d", res);
  if (res < 0)
    goto freq_failed;

  return TRUE;

  /* ERRORS */
freq_failed:
  {
    GST_ELEMENT_WARNING (radio, RESOURCE, SETTINGS,
        (_("Failed to change mute state for device '%s'."),
            radio->v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

static gboolean
gst_v4l2radio_set_mute (GstV4l2Radio * radio)
{
  return gst_v4l2radio_set_mute_on (radio, TRUE);
}

static gboolean
gst_v4l2radio_set_unmute (GstV4l2Radio * radio)
{
  return gst_v4l2radio_set_mute_on (radio, FALSE);
}

GST_IMPLEMENT_V4L2_TUNER_METHODS (GstV4l2Radio, gst_v4l2radio);

static void gst_v4l2radio_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

static void
gst_v4l2radio_tuner_interface_reinit (GstTunerInterface * iface)
{
  gst_v4l2radio_tuner_interface_init (iface);
}

#define gst_v4l2radio_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstV4l2Radio, gst_v4l2radio, GST_TYPE_ELEMENT,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_v4l2radio_uri_handler_init);
    G_IMPLEMENT_INTERFACE (GST_TYPE_TUNER,
        gst_v4l2radio_tuner_interface_reinit));

static void gst_v4l2radio_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_v4l2radio_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_v4l2radio_finalize (GstV4l2Radio * radio);
static void gst_v4l2radio_dispose (GObject * object);
static GstStateChangeReturn gst_v4l2radio_change_state (GstElement * element,
    GstStateChange transition);

static void
gst_v4l2radio_class_init (GstV4l2RadioClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->dispose = gst_v4l2radio_dispose;
  gobject_class->finalize = (GObjectFinalizeFunc) gst_v4l2radio_finalize;
  gobject_class->set_property = gst_v4l2radio_set_property;
  gobject_class->get_property = gst_v4l2radio_get_property;

  g_object_class_install_property (gobject_class, ARG_DEVICE,
      g_param_spec_string ("device", "Radio device location",
          "Video4Linux2 radio device location",
          DEFAULT_PROP_DEVICE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, ARG_FREQUENCY,
      g_param_spec_int ("frequency", "Station frequency",
          "Station frequency in Hz",
          MIN_FREQUENCY, MAX_FREQUENCY, DEFAULT_FREQUENCY, G_PARAM_READWRITE));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_v4l2radio_change_state);

  gst_element_class_set_static_metadata (gstelement_class,
      "Radio (video4linux2) Tuner",
      "Tuner",
      "Controls a Video4Linux2 radio device",
      "Alexey Chernov <4ernov@gmail.com>");

  klass->v4l2_class_devices = NULL;

  GST_DEBUG_CATEGORY_INIT (v4l2radio_debug, "v4l2radio", 0,
      "V4l2 radio element");
}

static void
gst_v4l2radio_init (GstV4l2Radio * filter)
{
  filter->v4l2object = gst_v4l2_object_new (GST_ELEMENT (filter),
      GST_OBJECT (filter), V4L2_BUF_TYPE_VIDEO_CAPTURE, DEFAULT_PROP_DEVICE,
      gst_v4l2radio_get_input, gst_v4l2radio_set_input, NULL);

  filter->v4l2object->frequency = DEFAULT_FREQUENCY;
  g_free (filter->v4l2object->videodev);
  filter->v4l2object->videodev = g_strdup (DEFAULT_PROP_DEVICE);
}

static void
gst_v4l2radio_dispose (GObject * object)
{
  GstV4l2Radio *radio = GST_V4L2RADIO (object);
  gst_v4l2_close (radio->v4l2object);
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_v4l2radio_finalize (GstV4l2Radio * radio)
{
  gst_v4l2_object_destroy (radio->v4l2object);
  G_OBJECT_CLASS (parent_class)->finalize ((GObject *) (radio));
}

static gboolean
gst_v4l2radio_open (GstV4l2Radio * radio)
{
  GstV4l2Object *v4l2object;

  v4l2object = radio->v4l2object;
  if (gst_v4l2_open (v4l2object))
    return gst_v4l2radio_fill_channel_list (radio);
  else
    return FALSE;
}

static void
gst_v4l2radio_set_defaults (GstV4l2Radio * radio)
{
  GstV4l2Object *v4l2object;
  GstTunerChannel *channel = NULL;
  GstTuner *tuner;

  v4l2object = radio->v4l2object;

  if (!GST_IS_TUNER (v4l2object->element))
    return;

  tuner = GST_TUNER (v4l2object->element);

  if (v4l2object->channel)
    channel = gst_tuner_find_channel_by_name (tuner, v4l2object->channel);
  if (channel) {
    gst_tuner_set_channel (tuner, channel);
  } else {
    channel =
        GST_TUNER_CHANNEL (gst_tuner_get_channel (GST_TUNER
            (v4l2object->element)));
    if (channel) {
      g_free (v4l2object->channel);
      v4l2object->channel = g_strdup (channel->label);
      gst_tuner_channel_changed (tuner, channel);
    }
  }

  if (channel
      && GST_TUNER_CHANNEL_HAS_FLAG (channel, GST_TUNER_CHANNEL_FREQUENCY)) {
    if (v4l2object->frequency != 0) {
      gst_tuner_set_frequency (tuner, channel, v4l2object->frequency);
    } else {
      v4l2object->frequency = gst_tuner_get_frequency (tuner, channel);
      if (v4l2object->frequency == 0) {
        /* guess */
        gst_tuner_set_frequency (tuner, channel, MIN_FREQUENCY);
      } else {
      }
    }
  }
}

static gboolean
gst_v4l2radio_start (GstV4l2Radio * radio)
{
  if (!gst_v4l2radio_open (radio))
    return FALSE;

  gst_v4l2radio_set_defaults (radio);

  return TRUE;
}

static gboolean
gst_v4l2radio_stop (GstV4l2Radio * radio)
{
  if (!gst_v4l2_object_close (radio->v4l2object))
    return FALSE;

  return TRUE;
}

static GstStateChangeReturn
gst_v4l2radio_change_state (GstElement * element, GstStateChange transition)
{
  GstV4l2Radio *radio;
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  radio = GST_V4L2RADIO (element);
  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      /*start radio */
      if (!gst_v4l2radio_start (radio))
        ret = GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      /*unmute radio */
      if (!gst_v4l2radio_set_unmute (radio))
        ret = GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      /*mute radio */
      if (!gst_v4l2radio_set_mute (radio))
        ret = GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      /*stop radio */
      if (!gst_v4l2radio_stop (radio))
        ret = GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  return ret;
}

static void
gst_v4l2radio_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstV4l2Radio *radio = GST_V4L2RADIO (object);
  gint frequency;
  switch (prop_id) {
    case ARG_DEVICE:
      g_free (radio->v4l2object->videodev);
      radio->v4l2object->videodev =
          g_strdup ((gchar *) g_value_get_string (value));
      break;
    case ARG_FREQUENCY:
      frequency = g_value_get_int (value);
      if (frequency >= MIN_FREQUENCY && frequency <= MAX_FREQUENCY) {
        radio->v4l2object->frequency = frequency;
        gst_v4l2_set_frequency (radio->v4l2object, 0,
            radio->v4l2object->frequency);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_v4l2radio_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstV4l2Radio *radio = GST_V4L2RADIO (object);

  switch (prop_id) {
    case ARG_DEVICE:
      g_value_set_string (value, radio->v4l2object->videodev);
      break;
    case ARG_FREQUENCY:
      if (gst_v4l2_get_frequency (radio->v4l2object,
              0, &(radio->v4l2object->frequency)))
        g_value_set_int (value, radio->v4l2object->frequency);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstURIHandler interface */
static GstURIType
gst_v4l2radio_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_v4l2radio_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { "radio", NULL };

  return protocols;
}

static gchar *
gst_v4l2radio_uri_get_uri (GstURIHandler * handler)
{
  GstV4l2Radio *radio = GST_V4L2RADIO (handler);

  if (radio->v4l2object->videodev != NULL) {
    if (gst_v4l2_get_frequency (radio->v4l2object,
            0, &(radio->v4l2object->frequency))) {
      return g_strdup_printf ("radio://%4.1f",
          radio->v4l2object->frequency / 1e6);
    }
  }

  return g_strdup ("radio://");
}

static gboolean
gst_v4l2radio_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  GstV4l2Radio *radio = GST_V4L2RADIO (handler);
  gdouble dfreq;
  gint ifreq;
  const gchar *freq;
  gchar *end;

  if (strcmp (uri, "radio://") != 0) {
    freq = uri + 8;

    dfreq = g_ascii_strtod (freq, &end);

    if (errno || strlen (end))
      goto uri_failed;

    ifreq = dfreq * 1e6;
    g_object_set (radio, "frequency", ifreq, NULL);

  } else
    goto uri_failed;

  return TRUE;

uri_failed:
  g_set_error_literal (error, GST_URI_ERROR, GST_URI_ERROR_BAD_REFERENCE,
      "Bad radio URI, could not parse frequency");
  return FALSE;
}

static void
gst_v4l2radio_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_v4l2radio_uri_get_type;
  iface->get_protocols = gst_v4l2radio_uri_get_protocols;
  iface->get_uri = gst_v4l2radio_uri_get_uri;
  iface->set_uri = gst_v4l2radio_uri_set_uri;
}
