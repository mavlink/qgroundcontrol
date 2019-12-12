/* GStreamer
 * Copyright (C) <2008> Edward Hervey <bilboed@bilboed.com>
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
 * SECTION:element-hdv1394src
 * @title: hdv1394src
 *
 * Read MPEG-TS data from firewire port.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 hdv1394src ! queue ! decodebin name=d ! queue ! xvimagesink d. ! queue ! alsasink
 * ]| captures from the firewire port and plays the streams.
 * |[
 * gst-launch-1.0 hdv1394src ! queue ! filesink location=mydump.ts
 * ]| capture to a disk file
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
#include <libiec61883/iec61883.h>

#include <gst/gst.h>

#include "gsthdv1394src.h"
#include "gst1394probe.h"


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


GST_DEBUG_CATEGORY_STATIC (hdv1394src_debug);
#define GST_CAT_DEFAULT (hdv1394src_debug)

#define DEFAULT_PORT    -1
#define DEFAULT_CHANNEL   63
#define DEFAULT_USE_AVC   TRUE
#define DEFAULT_GUID    0

enum
{
  PROP_0,
  PROP_PORT,
  PROP_CHANNEL,
  PROP_USE_AVC,
  PROP_GUID,
  PROP_DEVICE_NAME
};

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS
    ("video/mpegts,systemstream=(boolean)true,packetsize=(int)188")
    );

static void gst_hdv1394src_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

static void gst_hdv1394src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_hdv1394src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_hdv1394src_dispose (GObject * object);

static gboolean gst_hdv1394src_start (GstBaseSrc * bsrc);
static gboolean gst_hdv1394src_stop (GstBaseSrc * bsrc);
static gboolean gst_hdv1394src_unlock (GstBaseSrc * bsrc);

static GstFlowReturn gst_hdv1394src_create (GstPushSrc * psrc,
    GstBuffer ** buf);

static void gst_hdv1394src_update_device_name (GstHDV1394Src * src);

#define gst_hdv1394src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstHDV1394Src, gst_hdv1394src, GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER,
        gst_hdv1394src_uri_handler_init));

static void
gst_hdv1394src_class_init (GstHDV1394SrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_hdv1394src_set_property;
  gobject_class->get_property = gst_hdv1394src_get_property;
  gobject_class->dispose = gst_hdv1394src_dispose;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_PORT,
      g_param_spec_int ("port", "Port", "Port number (-1 automatic)",
          -1, 16, DEFAULT_PORT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CHANNEL,
      g_param_spec_int ("channel", "Channel", "Channel number for listening",
          0, 64, DEFAULT_CHANNEL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_USE_AVC,
      g_param_spec_boolean ("use-avc", "Use AV/C", "Use AV/C VTR control",
          DEFAULT_USE_AVC, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_GUID,
      g_param_spec_uint64 ("guid", "GUID",
          "select one of multiple DV devices by its GUID. use a hexadecimal "
          "like 0xhhhhhhhhhhhhhhhh. (0 = no guid)", 0, G_MAXUINT64,
          DEFAULT_GUID, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstHDV1394Src:device-name:
   *
   * Descriptive name of the currently opened device
   */
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "device name",
          "user-friendly name of the device", "Default",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gstbasesrc_class->negotiate = NULL;
  gstbasesrc_class->start = gst_hdv1394src_start;
  gstbasesrc_class->stop = gst_hdv1394src_stop;
  gstbasesrc_class->unlock = gst_hdv1394src_unlock;

  gstpushsrc_class->create = gst_hdv1394src_create;

  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);

  gst_element_class_set_static_metadata (gstelement_class,
      "Firewire (1394) HDV video source", "Source/Video",
      "Source for MPEG-TS video data from firewire port",
      "Edward Hervey <bilboed@bilboed.com>");

  GST_DEBUG_CATEGORY_INIT (hdv1394src_debug, "hdv1394src", 0,
      "MPEG-TS firewire source");
}

static void
gst_hdv1394src_init (GstHDV1394Src * dv1394src)
{
  GstPad *srcpad = GST_BASE_SRC_PAD (dv1394src);

  gst_base_src_set_live (GST_BASE_SRC (dv1394src), TRUE);
  gst_pad_use_fixed_caps (srcpad);

  dv1394src->port = DEFAULT_PORT;
  dv1394src->channel = DEFAULT_CHANNEL;

  dv1394src->use_avc = DEFAULT_USE_AVC;
  dv1394src->guid = DEFAULT_GUID;
  dv1394src->uri = g_strdup_printf ("hdv://%d", dv1394src->port);
  dv1394src->device_name = g_strdup_printf ("Default");

  READ_SOCKET (dv1394src) = -1;
  WRITE_SOCKET (dv1394src) = -1;

  dv1394src->frame_sequence = 0;
}

static void
gst_hdv1394src_dispose (GObject * object)
{
  GstHDV1394Src *src = GST_HDV1394SRC (object);

  g_free (src->uri);
  src->uri = NULL;

  g_free (src->device_name);
  src->device_name = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_hdv1394src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstHDV1394Src *filter = GST_HDV1394SRC (object);

  switch (prop_id) {
    case PROP_PORT:
      filter->port = g_value_get_int (value);
      g_free (filter->uri);
      filter->uri = g_strdup_printf ("hdv://%d", filter->port);
      break;
    case PROP_CHANNEL:
      filter->channel = g_value_get_int (value);
      break;
    case PROP_USE_AVC:
      filter->use_avc = g_value_get_boolean (value);
      break;
    case PROP_GUID:
      filter->guid = g_value_get_uint64 (value);
      gst_hdv1394src_update_device_name (filter);
      break;
    default:
      break;
  }
}

static void
gst_hdv1394src_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstHDV1394Src *filter = GST_HDV1394SRC (object);

  switch (prop_id) {
    case PROP_PORT:
      g_value_set_int (value, filter->port);
      break;
    case PROP_CHANNEL:
      g_value_set_int (value, filter->channel);
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

static GstHDV1394Src *
gst_hdv1394src_from_raw1394handle (raw1394handle_t handle)
{
  iec61883_mpeg2_t mpeg2 = (iec61883_mpeg2_t) raw1394_get_userdata (handle);
  return GST_HDV1394SRC (iec61883_mpeg2_get_callback_data (mpeg2));
}

/* Within one loop iteration (which may call _receive() many times), it seems
 * as though '*data' will always be different.
 *
 * We can therefore assume that any '*data' given to us will stay allocated until
 * the next loop iteration.
 */

static int
gst_hdv1394src_iec61883_receive (unsigned char *data, int len,
    unsigned int dropped, void *cbdata)
{
  GstHDV1394Src *dv1394src = GST_HDV1394SRC (cbdata);

  GST_LOG ("data:%p, len:%d, dropped:%d", data, len, dropped);

  /* error out if we don't have enough room ! */
  if (G_UNLIKELY (dv1394src->outoffset > (2048 * 188 - len)))
    return -1;

  if (G_LIKELY (len == IEC61883_MPEG2_TSP_SIZE)) {
    memcpy ((guint8 *) dv1394src->outdata + dv1394src->outoffset, data, len);
    dv1394src->outoffset += len;
  }
  dv1394src->frame_sequence++;
  return 0;
}

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
gst_hdv1394src_bus_reset (raw1394handle_t handle, unsigned int generation)
{
  GstHDV1394Src *src;
  gint nodecount;
  GstMessage *message;
  GstStructure *structure;
  gint current_device_change;
  gint i;

  src = gst_hdv1394src_from_raw1394handle (handle);

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
gst_hdv1394src_create (GstPushSrc * psrc, GstBuffer ** buf)
{
  GstHDV1394Src *dv1394src = GST_HDV1394SRC (psrc);
  struct pollfd pollfds[2];

  pollfds[0].fd = raw1394_get_fd (dv1394src->handle);
  pollfds[0].events = POLLIN | POLLERR | POLLHUP | POLLPRI;
  pollfds[1].fd = READ_SOCKET (dv1394src);
  pollfds[1].events = POLLIN | POLLERR | POLLHUP | POLLPRI;

  /* allocate a 2048 samples buffer */
  dv1394src->outdata = g_malloc (2048 * 188);
  dv1394src->outoffset = 0;

  GST_DEBUG ("Create...");

  while (TRUE) {
    int res = poll (pollfds, 2, -1);

    GST_LOG ("res:%d", res);

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
      int pt;

      pt = dv1394src->frame_sequence;
      /* shouldn't block in theory */
      GST_LOG ("Iterating ! (%d)", dv1394src->frame_sequence);
      raw1394_loop_iterate (dv1394src->handle);
      GST_LOG ("After iteration : %d (diff:%d)",
          dv1394src->frame_sequence, dv1394src->frame_sequence - pt);
      if (dv1394src->outoffset)
        break;
    }
  }

  g_assert (dv1394src->outoffset);

  GST_LOG ("We have some frames (%u bytes)", (guint) dv1394src->outoffset);

  /* Create the buffer */
  *buf = gst_buffer_new_wrapped (dv1394src->outdata, dv1394src->outoffset);
  dv1394src->outdata = NULL;
  dv1394src->outoffset = 0;

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
gst_hdv1394src_discover_avc_node (GstHDV1394Src * src)
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
gst_hdv1394src_start (GstBaseSrc * bsrc)
{
  GstHDV1394Src *src = GST_HDV1394SRC (bsrc);
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
    src->avc_node = gst_hdv1394src_discover_avc_node (src);

  /* lets destroy handle and create one on port
     this is more reliable than setting port on
     the existing handle */
  raw1394_destroy_handle (src->handle);
  src->handle = raw1394_new_handle_on_port (src->port);
  if (!src->handle)
    goto cannot_set_port;

  raw1394_set_userdata (src->handle, src);
  raw1394_set_bus_reset_handler (src->handle, gst_hdv1394src_bus_reset);

  {
    nodeid_t m_node = (src->avc_node | 0xffc0);
    int m_channel = -1;
    int m_bandwidth = 0;
    int m_outputPort = -1;
    int m_inputPort = -1;

    m_channel = iec61883_cmp_connect (src->handle, m_node, &m_outputPort,
        raw1394_get_local_id (src->handle), &m_inputPort, &m_bandwidth);

    if (m_channel >= 0) {
      src->channel = m_channel;
    }
  }


  if ((src->iec61883mpeg2 =
          iec61883_mpeg2_recv_init (src->handle,
              gst_hdv1394src_iec61883_receive, src)) == NULL)
    goto cannot_initialise_dv;

#if 0
  raw1394_set_iso_handler (src->handle, src->channel,
      gst_hdv1394src_iso_receive);
#endif

  GST_DEBUG_OBJECT (src, "successfully opened up 1394 connection");
  src->connected = TRUE;

  if (iec61883_mpeg2_recv_start (src->iec61883mpeg2, src->channel) != 0)
    goto cannot_start;
#if 0
  if (raw1394_start_iso_rcv (src->handle, src->channel) < 0)
    goto cannot_start;
#endif

  if (src->use_avc) {
    raw1394handle_t avc_handle = raw1394_new_handle_on_port (src->port);

    GST_LOG ("We have an avc_handle");

    /* start the VCR */
    if (avc_handle) {
      if (!avc1394_vcr_is_recording (avc_handle, src->avc_node)
          && avc1394_vcr_is_playing (avc_handle, src->avc_node)
          != AVC1394_VCR_OPERAND_PLAY_FORWARD) {
        GST_LOG ("Calling avc1394_vcr_play()");
        avc1394_vcr_play (avc_handle, src->avc_node);
      }
      raw1394_destroy_handle (avc_handle);
    } else {
      GST_WARNING_OBJECT (src, "Starting VCR via avc1394 failed: %s",
          g_strerror (errno));
    }
  }

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
    iec61883_mpeg2_close (src->iec61883mpeg2);
    src->iec61883mpeg2 = NULL;
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("can't start 1394 iso receive"));
    return FALSE;
  }
cannot_initialise_dv:
  {
    raw1394_destroy_handle (src->handle);
    src->handle = NULL;
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("can't initialise iec61883 hdv"));
    return FALSE;
  }
}

static gboolean
gst_hdv1394src_stop (GstBaseSrc * bsrc)
{
  GstHDV1394Src *src = GST_HDV1394SRC (bsrc);

  close (READ_SOCKET (src));
  close (WRITE_SOCKET (src));
  READ_SOCKET (src) = -1;
  WRITE_SOCKET (src) = -1;

  iec61883_mpeg2_close (src->iec61883mpeg2);
#if 0
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

  raw1394_destroy_handle (src->handle);

  return TRUE;
}

static gboolean
gst_hdv1394src_unlock (GstBaseSrc * bsrc)
{
  GstHDV1394Src *src = GST_HDV1394SRC (bsrc);

  SEND_COMMAND (src, CONTROL_STOP);

  return TRUE;
}

static void
gst_hdv1394src_update_device_name (GstHDV1394Src * src)
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
gst_hdv1394src_uri_get_type (GType type)
{
  return GST_URI_SRC;
}

static const gchar *const *
gst_hdv1394src_uri_get_protocols (GType type)
{
  static const gchar *protocols[] = { (char *) "hdv", NULL };

  return protocols;
}

static gchar *
gst_hdv1394src_uri_get_uri (GstURIHandler * handler)
{
  GstHDV1394Src *gst_hdv1394src = GST_HDV1394SRC (handler);

  return gst_hdv1394src->uri;
}

static gboolean
gst_hdv1394src_uri_set_uri (GstURIHandler * handler, const gchar * uri,
    GError ** error)
{
  gchar *protocol, *location;
  gboolean ret = TRUE;
  GstHDV1394Src *gst_hdv1394src = GST_HDV1394SRC (handler);

  protocol = gst_uri_get_protocol (uri);
  if (strcmp (protocol, "hdv") != 0) {
    g_free (protocol);
    g_set_error (error, GST_URI_ERROR, GST_URI_ERROR_BAD_URI,
        "Invalid HDV URI");
    return FALSE;
  }
  g_free (protocol);

  location = gst_uri_get_location (uri);
  if (location && *location != '\0')
    gst_hdv1394src->port = strtol (location, NULL, 10);
  else
    gst_hdv1394src->port = DEFAULT_PORT;
  g_free (location);
  g_free (gst_hdv1394src->uri);
  gst_hdv1394src->uri = g_strdup_printf ("hdv://%d", gst_hdv1394src->port);

  return ret;
}

static void
gst_hdv1394src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_hdv1394src_uri_get_type;
  iface->get_protocols = gst_hdv1394src_uri_get_protocols;
  iface->get_uri = gst_hdv1394src_uri_get_uri;
  iface->set_uri = gst_hdv1394src_uri_set_uri;
}
