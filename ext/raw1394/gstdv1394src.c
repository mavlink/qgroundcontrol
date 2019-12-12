/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *               <2000> Daniel Fischer <dan@f3c.com>
 *               <2004> Wim Taymans <wim@fluendo.com>
 *               <2006> Zaheer Abbas Merali <zaheerabbas at merali dot org>
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
 * SECTION:element-dv1394src
 * @title: dv1394src
 *
 * Read DV (digital video) data from firewire port.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 dv1394src ! queue ! dvdemux name=d ! queue ! dvdec ! xvimagesink d. ! queue ! alsasink
 * ]| This pipeline captures from the firewire port and displays it (might need
 * format converters for audio/video).
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <libavc1394/avc1394.h>
#include <libavc1394/avc1394_vcr.h>
#include <libavc1394/rom1394.h>
#include <libraw1394/raw1394.h>
#ifdef HAVE_LIBIEC61883
#include <libiec61883/iec61883.h>
#endif

#include <gst/gst.h>

#include "gstdv1394src.h"
#include "gst1394probe.h"
#include "gst1394clock.h"


#define CONTROL_STOP            'S'     /* stop the select call */
#define CONTROL_SOCKETS(src)   src->control_sock
#define WRITE_SOCKET(src)      src->control_sock[1]
#define READ_SOCKET(src)       src->control_sock[0]

#define SEND_COMMAND(src, command)          \
G_STMT_START {                              \
  int G_GNUC_UNUSED _res; unsigned char c; c = command;   \
  _res = write (WRITE_SOCKET(src), &c, 1);  \
} G_STMT_END

#define READ_COMMAND(src, command, res)        \
G_STMT_START {                                 \
  res = read(READ_SOCKET(src), &command, 1);   \
} G_STMT_END


GST_DEBUG_CATEGORY_STATIC (dv1394src_debug);
#define GST_CAT_DEFAULT (dv1394src_debug)

#define PAL_FRAMESIZE 144000
#define PAL_FRAMERATE 25

#define NTSC_FRAMESIZE 120000
#define NTSC_FRAMERATE 30

enum
{
  SIGNAL_FRAME_DROPPED,
  /* FILL ME */
  LAST_SIGNAL
};

#define DEFAULT_PORT    -1
#define DEFAULT_CHANNEL   63
#define DEFAULT_CONSECUTIVE 1
#define DEFAULT_SKIP    0
#define DEFAULT_DROP_INCOMPLETE TRUE
#define DEFAULT_USE_AVC   TRUE
#define DEFAULT_GUID    0

enum
{
  PROP_0,
  PROP_PORT,
  PROP_CHANNEL,
  PROP_CONSECUTIVE,
  PROP_SKIP,
  PROP_DROP_INCOMPLETE,
  PROP_USE_AVC,
  PROP_GUID,
  PROP_DEVICE_NAME
};

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-dv, "
        "format = (string) { NTSC, PAL }, " "systemstream = (boolean) true")
    );

static void gst_dv1394src_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

static void gst_dv1394src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dv1394src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_dv1394src_dispose (GObject * object);

static GstClock *gst_dv1394src_provide_clock (GstElement * element);
static GstStateChangeReturn gst_dv1394_src_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_dv1394src_start (GstBaseSrc * bsrc);
static gboolean gst_dv1394src_stop (GstBaseSrc * bsrc);
static gboolean gst_dv1394src_unlock (GstBaseSrc * bsrc);

static GstFlowReturn gst_dv1394src_create (GstPushSrc * psrc, GstBuffer ** buf);

static gboolean gst_dv1394src_query (GstBaseSrc * src, GstQuery * query);
static void gst_dv1394src_update_device_name (GstDV1394Src * src);

#define gst_dv1394src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstDV1394Src, gst_dv1394src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_dv1394src_uri_handler_init));

static guint gst_dv1394src_signals[LAST_SIGNAL] = { 0 };

static void
gst_dv1394src_class_init (GstDV1394SrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_dv1394src_set_property;
  gobject_class->get_property = gst_dv1394src_get_property;
  gobject_class->dispose = gst_dv1394src_dispose;

  gstelement_class->provide_clock = gst_dv1394src_provide_clock;
  gstelement_class->change_state = gst_dv1394_src_change_state;

  gst_dv1394src_signals[SIGNAL_FRAME_DROPPED] =
      g_signal_new ("frame-dropped", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (GstDV1394SrcClass, frame_dropped),
      NULL, NULL, NULL, G_TYPE_NONE, 0);

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PORT,
      g_param_spec_int ("port", "Port", "Port number (-1 automatic)",
          -1, 16, DEFAULT_PORT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CHANNEL,
      g_param_spec_int ("channel", "Channel", "Channel number for listening",
          0, 64, DEFAULT_CHANNEL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CONSECUTIVE,
      g_param_spec_int ("consecutive", "consecutive frames",
          "send n consecutive frames after skipping", 1, G_MAXINT,
          DEFAULT_CONSECUTIVE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SKIP,
      g_param_spec_int ("skip", "skip frames", "skip n frames",
          0, G_MAXINT, DEFAULT_SKIP,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DROP_INCOMPLETE,
      g_param_spec_boolean ("drop-incomplete", "drop incomplete",
          "drop incomplete frames", DEFAULT_DROP_INCOMPLETE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_USE_AVC,
      g_param_spec_boolean ("use-avc", "Use AV/C", "Use AV/C VTR control",
          DEFAULT_USE_AVC, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_GUID,
      g_param_spec_uint64 ("guid", "GUID",
          "select one of multiple DV devices by its GUID. use a hexadecimal "
          "like 0xhhhhhhhhhhhhhhhh. (0 = no guid)", 0, G_MAXUINT64,
          DEFAULT_GUID, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstDV1394Src:device-name:
   *
   * Descriptive name of the currently opened device
   */
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "device name",
          "user-friendly name of the device", "Default",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gstbasesrc_class->negotiate = NULL;
  gstbasesrc_class->start = gst_dv1394src_start;
  gstbasesrc_class->stop = gst_dv1394src_stop;
  gstbasesrc_class->unlock = gst_dv1394src_unlock;
  gstbasesrc_class->query = gst_dv1394src_query;

  gstpushsrc_class->create = gst_dv1394src_create;

  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);

  gst_element_class_set_static_metadata (gstelement_class,
      "Firewire (1394) DV video source", "Source/Video",
      "Source for DV video data from firewire port",
      "Erik Walthinsen <omega@temple-baptist.com>, "
      "Daniel Fischer <dan@f3c.com>, " "Wim Taymans <wim@fluendo.com>, "
      "Zaheer Abbas Merali <zaheerabbas at merali dot org>");

  GST_DEBUG_CATEGORY_INIT (dv1394src_debug, "dv1394src", 0,
      "DV firewire source");
}

static void
gst_dv1394src_init (GstDV1394Src * dv1394src)
{
  GstPad *srcpad = GST_BASE_SRC_PAD (dv1394src);

  gst_base_src_set_live (GST_BASE_SRC (dv1394src), TRUE);
  gst_base_src_set_format (GST_BASE_SRC (dv1394src), GST_FORMAT_TIME);
  gst_base_src_set_do_timestamp (GST_BASE_SRC (dv1394src), TRUE);
  gst_pad_use_fixed_caps (srcpad);

  dv1394src->port = DEFAULT_PORT;
  dv1394src->channel = DEFAULT_CHANNEL;

  dv1394src->consecutive = DEFAULT_CONSECUTIVE;
  dv1394src->skip = DEFAULT_SKIP;
  dv1394src->drop_incomplete = DEFAULT_DROP_INCOMPLETE;
  dv1394src->use_avc = DEFAULT_USE_AVC;
  dv1394src->guid = DEFAULT_GUID;
  dv1394src->uri = g_strdup_printf ("dv://%d", dv1394src->port);
  dv1394src->device_name = g_strdup_printf ("Default");

  READ_SOCKET (dv1394src) = -1;
  WRITE_SOCKET (dv1394src) = -1;

  /* initialized when first header received */
  dv1394src->frame_size = 0;

  dv1394src->buf = NULL;
  dv1394src->frame = NULL;
  dv1394src->frame_sequence = 0;

  dv1394src->provided_clock = gst_1394_clock_new ("dv1394clock");
}

static void
gst_dv1394src_dispose (GObject * object)
{
  GstDV1394Src *src = GST_DV1394SRC (object);

  if (src->provided_clock) {
    g_object_unref (src->provided_clock);
  }

  g_free (src->uri);
  src->uri = NULL;

  g_free (src->device_name);
  src->device_name = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_dv1394src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDV1394Src *filter = GST_DV1394SRC (object);

  switch (prop_id) {
    case PROP_PORT:
      filter->port = g_value_get_int (value);
      g_free (filter->uri);
      filter->uri = g_strdup_printf ("dv://%d", filter->port);
      break;
    case PROP_CHANNEL:
      filter->channel = g_value_get_int (value);
      break;
    case PROP_SKIP:
      filter->skip = g_value_get_int (value);
      break;
    case PROP_CONSECUTIVE:
      filter->consecutive = g_value_get_int (value);
      break;
    case PROP_DROP_INCOMPLETE:
      filter->drop_incomplete = g_value_get_boolean (value);
      break;
    case PROP_USE_AVC:
      filter->use_avc = g_value_get_boolean (value);
      break;
    case PROP_GUID:
      filter->guid = g_value_get_uint64 (value);
      gst_dv1394src_update_device_name (filter);
      break;
    default:
      break;
  }
}

static void
gst_dv1394src_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstDV1394Src *filter = GST_DV1394SRC (object);

  switch (prop_id) {
    case PROP_PORT:
      g_value_set_int (value, filter->port);
      break;
    case PROP_CHANNEL:
      g_value_set_int (value, filter->channel);
      break;
    case PROP_SKIP:
      g_value_set_int (value, filter->skip);
      break;
    case PROP_CONSECUTIVE:
      g_value_set_int (value, filter->consecutive);
      break;
    case PROP_DROP_INCOMPLETE:
      g_value_set_boolean (value, filter->drop_incomplete);
      break;
    case PROP_USE_AVC:
      g_value_set_boolean (value, filter->use_avc);
      break;
    case PROP_GUID:
      g_value_set_uint64 (value, filter->guid);
      break;
    case PROP_DEVICE_NAME:
      g_value_set_string (value, filter->device_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstClock *
gst_dv1394src_provide_clock (GstElement * element)
{
  GstDV1394Src *dv1394src = GST_DV1394SRC (element);

  return GST_CLOCK_CAST (gst_object_ref (dv1394src->provided_clock));
}

static GstStateChangeReturn
gst_dv1394_src_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstDV1394Src *src = GST_DV1394SRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      gst_element_post_message (element,
          gst_message_new_clock_lost (GST_OBJECT_CAST (element),
              GST_CLOCK_CAST (src->provided_clock)));
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      gst_element_post_message (element,
          gst_message_new_clock_provide (GST_OBJECT_CAST (element),
              GST_CLOCK_CAST (src->provided_clock), TRUE));
      break;
    default:
      break;
  }

  return ret;
}

#ifdef HAVE_LIBIEC61883
static GstDV1394Src *
gst_dv1394src_from_raw1394handle (raw1394handle_t handle)
{
  iec61883_dv_t dv = (iec61883_dv_t) raw1394_get_userdata (handle);
  iec61883_dv_fb_t dv_fb =
      (iec61883_dv_fb_t) iec61883_dv_get_callback_data (dv);
  return GST_DV1394SRC (iec61883_dv_fb_get_callback_data (dv_fb));
}
#else /* HAVE_LIBIEC61883 */
static GstDV1394Src *
gst_dv1394src_from_raw1394handle (raw1394handle_t handle)
{
  return GST_DV1394SRC (raw1394_get_userdata (handle));
}
#endif /* HAVE_LIBIEC61883 */

#ifdef HAVE_LIBIEC61883
static int
gst_dv1394src_iec61883_receive (unsigned char *data, int len,
    int complete, void *cbdata)
{
  GstDV1394Src *dv1394src = GST_DV1394SRC (cbdata);

  if (G_UNLIKELY (!gst_pad_has_current_caps (GST_BASE_SRC_PAD (dv1394src)))) {
    GstCaps *caps;
    unsigned char *p = data;

    // figure format (NTSC/PAL)
    if (p[3] & 0x80) {
      // PAL
      dv1394src->frame_size = PAL_FRAMESIZE;
      dv1394src->frame_rate = PAL_FRAMERATE;
      GST_DEBUG ("PAL data");
      caps = gst_caps_new_simple ("video/x-dv",
          "format", G_TYPE_STRING, "PAL",
          "systemstream", G_TYPE_BOOLEAN, TRUE, NULL);
    } else {
      // NTSC (untested)
      dv1394src->frame_size = NTSC_FRAMESIZE;
      dv1394src->frame_rate = NTSC_FRAMERATE;
      GST_DEBUG
          ("NTSC data [untested] - please report success/failure to <dan@f3c.com>");
      caps = gst_caps_new_simple ("video/x-dv",
          "format", G_TYPE_STRING, "NTSC",
          "systemstream", G_TYPE_BOOLEAN, TRUE, NULL);
    }
    gst_pad_set_caps (GST_BASE_SRC_PAD (dv1394src), caps);
    gst_caps_unref (caps);
  }

  dv1394src->frame = NULL;
  if (G_LIKELY ((dv1394src->frame_sequence + 1) % (dv1394src->skip +
              dv1394src->consecutive) < dv1394src->consecutive)) {
    if (complete && len == dv1394src->frame_size) {
      GstBuffer *buf;

      buf = gst_buffer_new_and_alloc (dv1394src->frame_size);

      GST_BUFFER_OFFSET (buf) = dv1394src->frame_sequence;
      gst_buffer_fill (buf, 0, data, len);
      dv1394src->buf = buf;
    }
  }
  dv1394src->frame_sequence++;
  return 0;
}

#else
static int
gst_dv1394src_iso_receive (raw1394handle_t handle, int channel, size_t len,
    quadlet_t * data)
{
  GstDV1394Src *dv1394src = gst_dv1394src_from_raw1394handle (handle);

  if (len > 16) {
    /*
       the following code taken from kino-0.51 (Dan Dennedy/Charles Yates)
       Kindly relicensed under the LGPL. See the commit log for version 1.6 of
       this file in CVS.
     */
    unsigned char *p = (unsigned char *) &data[3];

    int section_type = p[0] >> 5;       /* section type is in bits 5 - 7 */
    int dif_sequence = p[1] >> 4;       /* dif sequence number is in bits 4 - 7 */
    int dif_block = p[2];

    /* if we are at the beginning of a frame, 
       we set buf=frame, and alloc a new buffer for frame
     */
    if (section_type == 0 && dif_sequence == 0) {       // dif header
      if (!GST_PAD_CAPS (GST_BASE_SRC_PAD (dv1394src))) {
        GstCaps *caps;

        // figure format (NTSC/PAL)
        if (p[3] & 0x80) {
          // PAL
          dv1394src->frame_size = PAL_FRAMESIZE;
          dv1394src->frame_rate = PAL_FRAMERATE;
          GST_DEBUG ("PAL data");
          caps = gst_caps_new_simple ("video/x-dv",
              "format", G_TYPE_STRING, "PAL",
              "systemstream", G_TYPE_BOOLEAN, TRUE, NULL);
        } else {
          // NTSC (untested)
          dv1394src->frame_size = NTSC_FRAMESIZE;
          dv1394src->frame_rate = NTSC_FRAMERATE;
          GST_DEBUG
              ("NTSC data [untested] - please report success/failure to <dan@f3c.com>");
          caps = gst_caps_new_simple ("video/x-dv",
              "format", G_TYPE_STRING, "NTSC",
              "systemstream", G_TYPE_BOOLEAN, TRUE, NULL);
        }
        gst_pad_set_caps (GST_BASE_SRC_PAD (dv1394src), caps);
        gst_caps_unref (caps);
      }
      // drop last frame when not complete
      if (!dv1394src->drop_incomplete
          || dv1394src->bytes_in_frame == dv1394src->frame_size) {
        dv1394src->buf = dv1394src->frame;
      } else {
        GST_INFO_OBJECT (GST_ELEMENT (dv1394src), "incomplete frame dropped");
        g_signal_emit (G_OBJECT (dv1394src),
            gst_dv1394src_signals[SIGNAL_FRAME_DROPPED], 0);
        if (dv1394src->frame) {
          gst_buffer_unref (dv1394src->frame);
        }
      }
      if ((dv1394src->frame_sequence + 1) % (dv1394src->skip +
              dv1394src->consecutive) < dv1394src->consecutive) {
        GstBuffer *buf;
        gint64 i64;

        buf = gst_buffer_new_and_alloc (dv1394src->frame_size);

        /* fill in offset, duration, timestamp */
        GST_BUFFER_OFFSET (buf) = dv1394src->frame_sequence;
        dv1394src->frame = buf;
      }
      dv1394src->frame_sequence++;
      dv1394src->bytes_in_frame = 0;
    }

    if (dv1394src->frame != NULL) {
      guint8 *data = GST_BUFFER_DATA (dv1394src->frame);

      switch (section_type) {
        case 0:                /* 1 Header block */
          /* p[3] |= 0x80; // hack to force PAL data */
          memcpy (data + dif_sequence * 150 * 80, p, 480);
          break;

        case 1:                /* 2 Subcode blocks */
          memcpy (data + dif_sequence * 150 * 80 + (1 + dif_block) * 80, p,
              480);
          break;

        case 2:                /* 3 VAUX blocks */
          memcpy (data + dif_sequence * 150 * 80 + (3 + dif_block) * 80, p,
              480);
          break;

        case 3:                /* 9 Audio blocks interleaved with video */
          memcpy (data + dif_sequence * 150 * 80 + (6 + dif_block * 16) * 80, p,
              480);
          break;

        case 4:                /* 135 Video blocks interleaved with audio */
          memcpy (data + dif_sequence * 150 * 80 + (7 + (dif_block / 15) +
                  dif_block) * 80, p, 480);
          break;

        default:               /* we can't handle any other data */
          break;
      }
      dv1394src->bytes_in_frame += 480;
    }
  }

  return 0;
}
#endif
/*
 * When an ieee1394 bus reset happens, usually a device has been removed
 * or added.  We send a message on the message bus with the node count 
 * and whether the capture device used in this element connected, disconnected 
 * or was unchanged
 * Message structure:
 * nodecount - integer with number of nodes on bus
 * current-device-change - integer (1 if device connected, 0 if no change to
 *                         current device status, -1 if device disconnected)
 */
static int
gst_dv1394src_bus_reset (raw1394handle_t handle, unsigned int generation)
{
  GstDV1394Src *src;
  gint nodecount;
  GstMessage *message;
  GstStructure *structure;
  gint current_device_change;
  gint i;

  src = gst_dv1394src_from_raw1394handle (handle);

  GST_INFO_OBJECT (src, "have bus reset");

  /* update generation - told to do so by docs */
  raw1394_update_generation (handle, generation);
  nodecount = raw1394_get_nodecount (handle);
  /* allocate memory for portinfo */

  /* current_device_change is -1 if camera disconnected, 0 if other device
   * connected or 1 if camera has now connected */
  current_device_change = -1;
  for (i = 0; i < nodecount; i++) {
    if (src->guid == rom1394_get_guid (handle, i)) {
      /* Camera is with us */
      GST_DEBUG ("Camera is with us");
      if (!src->connected) {
        current_device_change = 1;
        src->connected = TRUE;
      } else
        current_device_change = 0;
    }
  }
  if (src->connected && current_device_change == -1) {
    GST_DEBUG ("Camera has disconnected");
    src->connected = FALSE;
  } else if (!src->connected && current_device_change == -1) {
    GST_DEBUG ("Camera is still not with us");
    current_device_change = 0;
  }

  structure = gst_structure_new ("ieee1394-bus-reset", "nodecount", G_TYPE_INT,
      nodecount, "current-device-change", G_TYPE_INT, current_device_change,
      NULL);
  message = gst_message_new_element (GST_OBJECT (src), structure);
  gst_element_post_message (GST_ELEMENT (src), message);

  return 0;
}

static GstFlowReturn
gst_dv1394src_create (GstPushSrc * psrc, GstBuffer ** buf)
{
  GstDV1394Src *dv1394src = GST_DV1394SRC (psrc);
  struct pollfd pollfds[2];

  pollfds[0].fd = raw1394_get_fd (dv1394src->handle);
  pollfds[0].events = POLLIN | POLLERR | POLLHUP | POLLPRI;
  pollfds[1].fd = READ_SOCKET (dv1394src);
  pollfds[1].events = POLLIN | POLLERR | POLLHUP | POLLPRI;

  if (G_UNLIKELY (dv1394src->buf)) {
    /* maybe we had an error before, and there's a stale buffer? */
    gst_buffer_unref (dv1394src->buf);
    dv1394src->buf = NULL;
  }

  while (TRUE) {
    int res = poll (pollfds, 2, -1);

    if (G_UNLIKELY (res < 0)) {
      if (errno == EAGAIN || errno == EINTR)
        continue;
      else
        goto error_while_polling;
    }

    if (G_UNLIKELY (pollfds[1].revents)) {
      char command;

      if (pollfds[1].revents & POLLIN)
        READ_COMMAND (dv1394src, command, res);

      goto told_to_stop;
    } else if (G_LIKELY (pollfds[0].revents & POLLIN)) {
      /* shouldn't block in theory */
      raw1394_loop_iterate (dv1394src->handle);

      if (dv1394src->buf)
        break;
    }
  }

  g_assert (dv1394src->buf);

  *buf = dv1394src->buf;
  dv1394src->buf = NULL;
  return GST_FLOW_OK;

error_while_polling:
  {
    GST_ELEMENT_ERROR (dv1394src, RESOURCE, READ, (NULL), GST_ERROR_SYSTEM);
    return GST_FLOW_EOS;
  }
told_to_stop:
  {
    GST_DEBUG_OBJECT (dv1394src, "told to stop, shutting down");
    return GST_FLOW_FLUSHING;
  }
}

static int
gst_dv1394src_discover_avc_node (GstDV1394Src * src)
{
  int node = -1;
  int i, j = 0;
  int m = src->num_ports;

  if (src->port >= 0) {
    /* search on explicit port */
    j = src->port;
    m = j + 1;
  }

  /* loop over all our ports */
  for (; j < m && node == -1; j++) {
    raw1394handle_t handle;
    struct raw1394_portinfo pinf[16];

    /* open the port */
    handle = raw1394_new_handle ();
    if (!handle) {
      GST_WARNING ("raw1394 - failed to get handle: %s.\n", strerror (errno));
      continue;
    }
    if (raw1394_get_port_info (handle, pinf, 16) < 0) {
      GST_WARNING ("raw1394 - failed to get port info: %s.\n",
          strerror (errno));
      goto next;
    }

    /* tell raw1394 which host adapter to use */
    if (raw1394_set_port (handle, j) < 0) {
      GST_WARNING ("raw1394 - failed to set set port: %s.\n", strerror (errno));
      goto next;
    }

    /* now loop over all the nodes */
    for (i = 0; i < raw1394_get_nodecount (handle); i++) {
      /* are we looking for an explicit GUID ? */
      if (src->guid != 0) {
        if (src->guid == rom1394_get_guid (handle, i)) {
          node = i;
          src->port = j;
          g_free (src->uri);
          src->uri = g_strdup_printf ("dv://%d", src->port);
          break;
        }
      } else {
        rom1394_directory rom_dir;

        /* select first AV/C Tape Recorder Player node */
        if (rom1394_get_directory (handle, i, &rom_dir) < 0) {
          GST_WARNING ("error reading config rom directory for node %d\n", i);
          continue;
        }
        if ((rom1394_get_node_type (&rom_dir) == ROM1394_NODE_TYPE_AVC) &&
            avc1394_check_subunit_type (handle, i, AVC1394_SUBUNIT_TYPE_VCR)) {
          node = i;
          src->port = j;
          src->guid = rom1394_get_guid (handle, i);
          g_free (src->uri);
          src->uri = g_strdup_printf ("dv://%d", src->port);
          g_free (src->device_name);
          src->device_name = g_strdup (rom_dir.label);
          break;
        }
        rom1394_free_directory (&rom_dir);
      }
    }
  next:
    raw1394_destroy_handle (handle);
  }
  return node;
}

static gboolean
gst_dv1394src_start (GstBaseSrc * bsrc)
{
  GstDV1394Src *src = GST_DV1394SRC (bsrc);
  int control_sock[2];

  src->connected = FALSE;

  if (socketpair (PF_UNIX, SOCK_STREAM, 0, control_sock) < 0)
    goto socket_pair;

  READ_SOCKET (src) = control_sock[0];
  WRITE_SOCKET (src) = control_sock[1];

  if (fcntl (READ_SOCKET (src), F_SETFL, O_NONBLOCK) < 0)
    GST_ERROR_OBJECT (src, "failed to make read socket non-blocking: %s",
        g_strerror (errno));
  if (fcntl (WRITE_SOCKET (src), F_SETFL, O_NONBLOCK) < 0)
    GST_ERROR_OBJECT (src, "failed to make write socket non-blocking: %s",
        g_strerror (errno));

  src->handle = raw1394_new_handle ();

  if (!src->handle) {
    if (errno == EACCES)
      goto permission_denied;
    else if (errno == ENOENT)
      goto not_found;
    else
      goto no_handle;
  }

  src->num_ports = raw1394_get_port_info (src->handle, src->pinfo, 16);

  if (src->num_ports == 0)
    goto no_ports;

  if (src->use_avc || src->port == -1)
    src->avc_node = gst_dv1394src_discover_avc_node (src);

  /* lets destroy handle and create one on port
     this is more reliable than setting port on
     the existing handle */
  raw1394_destroy_handle (src->handle);
  src->handle = raw1394_new_handle_on_port (src->port);
  if (!src->handle)
    goto cannot_set_port;

  raw1394_set_userdata (src->handle, src);
  raw1394_set_bus_reset_handler (src->handle, gst_dv1394src_bus_reset);

#ifdef HAVE_LIBIEC61883
  if ((src->iec61883dv =
          iec61883_dv_fb_init (src->handle,
              gst_dv1394src_iec61883_receive, src)) == NULL)
    goto cannot_initialise_dv;

#else
  raw1394_set_iso_handler (src->handle, src->channel,
      gst_dv1394src_iso_receive);
#endif

  GST_DEBUG_OBJECT (src, "successfully opened up 1394 connection");
  src->connected = TRUE;

#ifdef HAVE_LIBIEC61883
  if (iec61883_dv_fb_start (src->iec61883dv, src->channel) != 0)
    goto cannot_start;
#else
  if (raw1394_start_iso_rcv (src->handle, src->channel) < 0)
    goto cannot_start;
#endif

  if (src->use_avc) {
    raw1394handle_t avc_handle = raw1394_new_handle_on_port (src->port);

    /* start the VCR */
    if (avc_handle) {
      if (!avc1394_vcr_is_recording (avc_handle, src->avc_node)
          && avc1394_vcr_is_playing (avc_handle, src->avc_node)
          != AVC1394_VCR_OPERAND_PLAY_FORWARD)
        avc1394_vcr_play (avc_handle, src->avc_node);
      raw1394_destroy_handle (avc_handle);
    } else {
      GST_WARNING_OBJECT (src, "Starting VCR via avc1394 failed: %s",
          g_strerror (errno));
    }
  }

  gst_1394_clock_set_handle (src->provided_clock, src->handle);

  return TRUE;

socket_pair:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ_WRITE, (NULL),
        GST_ERROR_SYSTEM);
    return FALSE;
  }
permission_denied:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL), GST_ERROR_SYSTEM);
    return FALSE;
  }
not_found:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (NULL), GST_ERROR_SYSTEM);
    return FALSE;
  }
no_handle:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("can't get raw1394 handle (%s)", g_strerror (errno)));
    return FALSE;
  }
no_ports:
  {
    raw1394_destroy_handle (src->handle);
    src->handle = NULL;
    GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (NULL),
        ("no ports available for raw1394"));
    return FALSE;
  }
cannot_set_port:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("can't set 1394 port %d", src->port));
    return FALSE;
  }
cannot_start:
  {
    raw1394_destroy_handle (src->handle);
    src->handle = NULL;
#ifdef HAVE_LIBIEC61883
    iec61883_dv_fb_close (src->iec61883dv);
    src->iec61883dv = NULL;
#endif
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("can't start 1394 iso receive"));
    return FALSE;
  }
#ifdef HAVE_LIBIEC61883
cannot_initialise_dv:
  {
    raw1394_destroy_handle (src->handle);
    src->handle = NULL;
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("can't initialise iec61883 dv"));
    return FALSE;
  }
#endif
}

static gboolean
gst_dv1394src_stop (GstBaseSrc * bsrc)
{
  GstDV1394Src *src = GST_DV1394SRC (bsrc);

  close (READ_SOCKET (src));
  close (WRITE_SOCKET (src));
  READ_SOCKET (src) = -1;
  WRITE_SOCKET (src) = -1;
#ifdef HAVE_LIBIEC61883
  iec61883_dv_fb_close (src->iec61883dv);
#else
  raw1394_stop_iso_rcv (src->handle, src->channel);
#endif

  if (src->use_avc) {
    raw1394handle_t avc_handle = raw1394_new_handle_on_port (src->port);

    /* pause and stop the VCR */
    if (avc_handle) {
      if (!avc1394_vcr_is_recording (avc_handle, src->avc_node)
          && (avc1394_vcr_is_playing (avc_handle, src->avc_node)
              != AVC1394_VCR_OPERAND_PLAY_FORWARD_PAUSE))
        avc1394_vcr_pause (avc_handle, src->avc_node);
      avc1394_vcr_stop (avc_handle, src->avc_node);
      raw1394_destroy_handle (avc_handle);
    } else {
      GST_WARNING_OBJECT (src, "Starting VCR via avc1394 failed: %s",
          g_strerror (errno));
    }
  }

  gst_1394_clock_unset_handle (src->provided_clock);

  raw1394_destroy_handle (src->handle);

  return TRUE;
}

static gboolean
gst_dv1394src_unlock (GstBaseSrc * bsrc)
{
  GstDV1394Src *src = GST_DV1394SRC (bsrc);

  SEND_COMMAND (src, CONTROL_STOP);

  return TRUE;
}

static gboolean
gst_dv1394src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_LATENCY:
    {
      gst_query_set_latency (query, TRUE, GST_SECOND / 25, GST_SECOND / 25);
    }
      break;
    default:
      goto not_supported;
  }

  return TRUE;

not_supported:
  return GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);
}

static void
gst_dv1394src_update_device_name (GstDV1394Src * src)
{
  raw1394handle_t handle;
  gint portcount, port, nodecount, node;
  rom1394_directory directory;

  g_free (src->device_name);
  src->device_name = NULL;

  GST_LOG_OBJECT (src, "updating device name for current GUID");

  handle = raw1394_new_handle ();

  if (handle == NULL)
    goto gethandle_failed;

  portcount = raw1394_get_port_info (handle, NULL, 0);
  for (port = 0; port < portcount; port++) {
    if (raw1394_set_port (handle, port) >= 0) {
      nodecount = raw1394_get_nodecount (handle);
      for (node = 0; node < nodecount; node++) {
        if (src->guid == rom1394_get_guid (handle, node)) {
          if (rom1394_get_directory (handle, node, &directory) >= 0) {
            g_free (src->device_name);
            src->device_name = g_strdup (directory.label);
            rom1394_free_directory (&directory);
            goto done;
          } else {
            GST_WARNING ("error reading rom directory for node %d", node);
          }
        }
      }
    }
  }

  src->device_name = g_strdup ("Unknown");      /* FIXME: translate? */

done:

  raw1394_destroy_handle (handle);
  return;

/* ERRORS */
gethandle_failed:
  {
    GST_WARNING ("failed to get raw1394 handle: %s", g_strerror (errno));
    src->device_name = g_strdup ("Unknown");    /* FIXME: translate? */
    return;
  }
}

/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_dv1394src_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_dv1394src_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { (char *) "dv", NULL };

  return protocols;
}

static gchar *
gst_dv1394src_uri_get_uri (GstURIHandler * handler)
{
  GstDV1394Src *gst_dv1394src = GST_DV1394SRC (handler);

  return gst_dv1394src->uri;
}

static gboolean
gst_dv1394src_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  gchar *protocol, *location;
  gboolean ret = TRUE;
  GstDV1394Src *gst_dv1394src = GST_DV1394SRC (handler);

  protocol = gst_uri_get_protocol (uri);
  if (strcmp (protocol, "dv") != 0) {
    g_free (protocol);
    g_set_error (error, GST_URI_ERROR, GST_URI_ERROR_BAD_URI, "Invalid DV URI");
    return FALSE;
  }
  g_free (protocol);

  location = gst_uri_get_location (uri);
  if (location && *location != '\0')
    gst_dv1394src->port = strtol (location, NULL, 10);
  else
    gst_dv1394src->port = DEFAULT_PORT;
  g_free (location);
  g_free (gst_dv1394src->uri);
  gst_dv1394src->uri = g_strdup_printf ("dv://%d", gst_dv1394src->port);

  return ret;
}

static void
gst_dv1394src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_dv1394src_uri_get_type;
  iface->get_protocols = gst_dv1394src_uri_get_protocols;
  iface->get_uri = gst_dv1394src_uri_get_uri;
  iface->set_uri = gst_dv1394src_uri_set_uri;
}
