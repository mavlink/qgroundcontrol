/* GStreamer
 * Copyright (C) 2006 Sjoerd Simons <sjoerd@luon.net>
 * Copyright (C) 2004 Wim Taymans <wim@fluendo.com>
 *
 * gstmultipartdemux.c: multipart stream demuxer
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
 * SECTION:element-multipartdemux
 * @title: multipartdemux
 * @see_also: #GstMultipartMux
 *
 * MultipartDemux uses the Content-type field of incoming buffers to demux and
 * push data to dynamic source pads. Most of the time multipart streams are
 * sequential JPEG frames generated from a live source such as a network source
 * or a camera.
 *
 * The output buffers of the multipartdemux typically have no timestamps and are
 * usually played as fast as possible (at the rate that the source provides the
 * data).
 *
 * the content in multipart files is separated with a boundary string that can
 * be configured specifically with the #GstMultipartDemux:boundary property
 * otherwise it will be autodetected.
 *
 * ## Sample pipelines
 * |[
 * gst-launch-1.0 filesrc location=/tmp/test.multipart ! multipartdemux ! image/jpeg,framerate=\(fraction\)5/1 ! jpegparse ! jpegdec ! videoconvert ! autovideosink
 * ]| a simple pipeline to demux a multipart file muxed with #GstMultipartMux
 * containing JPEG frames.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "multipartdemux.h"

GST_DEBUG_CATEGORY_STATIC (gst_multipart_demux_debug);
#define GST_CAT_DEFAULT gst_multipart_demux_debug

#define DEFAULT_BOUNDARY		NULL
#define DEFAULT_SINGLE_STREAM	FALSE

enum
{
  PROP_0,
  PROP_BOUNDARY,
  PROP_SINGLE_STREAM
};

static GstStaticPadTemplate multipart_demux_src_template_factory =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate multipart_demux_sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("multipart/x-mixed-replace")
    );

typedef struct
{
  const gchar *key;
  const gchar *val;
} GstNamesMap;

/* convert from mime types to gst structure names. Add more when needed. The
 * mime-type is stored as lowercase */
static const GstNamesMap gstnames[] = {
  /* RFC 2046 says audio/basic is mulaw, mono, 8000Hz */
  {"audio/basic", "audio/x-mulaw, channels=1, rate=8000"},
  {"audio/g726-16",
      "audio/x-adpcm, bitrate=16000, layout=g726, channels=1, rate=8000"},
  {"audio/g726-24",
      "audio/x-adpcm, bitrate=24000, layout=g726, channels=1, rate=8000"},
  {"audio/g726-32",
      "audio/x-adpcm, bitrate=32000, layout=g726, channels=1, rate=8000"},
  {"audio/g726-40",
      "audio/x-adpcm, bitrate=40000, layout=g726, channels=1, rate=8000"},
  /* Panasonic Network Cameras non-standard types */
  {"audio/g726",
      "audio/x-adpcm, bitrate=32000, layout=g726, channels=1, rate=8000"},
  {NULL, NULL}
};


static GstFlowReturn gst_multipart_demux_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);
static gboolean gst_multipart_demux_event (GstPad * pad,
    GstObject * parent, GstEvent * event);

static GstStateChangeReturn gst_multipart_demux_change_state (GstElement *
    element, GstStateChange transition);

static void gst_multipart_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_multipart_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void gst_multipart_demux_dispose (GObject * object);

#define gst_multipart_demux_parent_class parent_class
G_DEFINE_TYPE (GstMultipartDemux, gst_multipart_demux, GST_TYPE_ELEMENT);

static void
gst_multipart_demux_class_init (GstMultipartDemuxClass * klass)
{
  int i;

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->dispose = gst_multipart_demux_dispose;
  gobject_class->set_property = gst_multipart_set_property;
  gobject_class->get_property = gst_multipart_get_property;

  g_object_class_install_property (gobject_class, PROP_BOUNDARY,
      g_param_spec_string ("boundary", "Boundary",
          "The boundary string separating data, automatic if NULL",
          DEFAULT_BOUNDARY,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  /**
   * GstMultipartDemux:single-stream:
   *
   * Assume that there is only one stream whose content-type will
   * not change and emit no-more-pads as soon as the first boundary
   * content is parsed, decoded, and pads are linked.
   */
  g_object_class_install_property (gobject_class, PROP_SINGLE_STREAM,
      g_param_spec_boolean ("single-stream", "Single Stream",
          "Assume that there is only one stream whose content-type will not change and emit no-more-pads as soon as the first boundary content is parsed, decoded, and pads are linked",
          DEFAULT_SINGLE_STREAM, G_PARAM_READWRITE));

  /* populate gst names and mime types pairs */
  klass->gstnames = g_hash_table_new (g_str_hash, g_str_equal);
  for (i = 0; gstnames[i].key; i++) {
    g_hash_table_insert (klass->gstnames, (gpointer) gstnames[i].key,
        (gpointer) gstnames[i].val);
  }

  gstelement_class->change_state = gst_multipart_demux_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &multipart_demux_sink_template_factory);
  gst_element_class_add_static_pad_template (gstelement_class,
      &multipart_demux_src_template_factory);
  gst_element_class_set_static_metadata (gstelement_class, "Multipart demuxer",
      "Codec/Demuxer", "demux multipart streams",
      "Wim Taymans <wim.taymans@gmail.com>, Sjoerd Simons <sjoerd@luon.net>");
}

static void
gst_multipart_demux_init (GstMultipartDemux * multipart)
{
  /* create the sink pad */
  multipart->sinkpad =
      gst_pad_new_from_static_template (&multipart_demux_sink_template_factory,
      "sink");
  gst_element_add_pad (GST_ELEMENT_CAST (multipart), multipart->sinkpad);
  gst_pad_set_chain_function (multipart->sinkpad,
      GST_DEBUG_FUNCPTR (gst_multipart_demux_chain));
  gst_pad_set_event_function (multipart->sinkpad,
      GST_DEBUG_FUNCPTR (gst_multipart_demux_event));

  multipart->adapter = gst_adapter_new ();
  multipart->boundary = DEFAULT_BOUNDARY;
  multipart->mime_type = NULL;
  multipart->content_length = -1;
  multipart->header_completed = FALSE;
  multipart->scanpos = 0;
  multipart->singleStream = DEFAULT_SINGLE_STREAM;
  multipart->have_group_id = FALSE;
  multipart->group_id = G_MAXUINT;
}

static void
gst_multipart_demux_remove_src_pads (GstMultipartDemux * demux)
{
  while (demux->srcpads != NULL) {
    GstMultipartPad *mppad = demux->srcpads->data;

    gst_element_remove_pad (GST_ELEMENT (demux), mppad->pad);
    g_free (mppad->mime);
    g_free (mppad);
    demux->srcpads = g_slist_delete_link (demux->srcpads, demux->srcpads);
  }
  demux->srcpads = NULL;
  demux->numpads = 0;
}

static void
gst_multipart_demux_dispose (GObject * object)
{
  GstMultipartDemux *demux = GST_MULTIPART_DEMUX (object);

  if (demux->adapter != NULL)
    g_object_unref (demux->adapter);
  demux->adapter = NULL;
  g_free (demux->boundary);
  demux->boundary = NULL;
  g_free (demux->mime_type);
  demux->mime_type = NULL;
  gst_multipart_demux_remove_src_pads (demux);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static const gchar *
gst_multipart_demux_get_gstname (GstMultipartDemux * demux, gchar * mimetype)
{
  GstMultipartDemuxClass *klass;
  const gchar *gstname;

  klass = GST_MULTIPART_DEMUX_GET_CLASS (demux);

  /* use hashtable to convert to gst name */
  gstname = g_hash_table_lookup (klass->gstnames, mimetype);
  if (gstname == NULL) {
    /* no gst name mapping, use mime type */
    gstname = mimetype;
  }
  GST_DEBUG_OBJECT (demux, "gst name for %s is %s", mimetype, gstname);
  return gstname;
}

static GstFlowReturn
gst_multipart_combine_flows (GstMultipartDemux * demux, GstMultipartPad * pad,
    GstFlowReturn ret)
{
  GSList *walk;

  /* store the value */
  pad->last_ret = ret;

  /* any other error that is not-linked can be returned right
   * away */
  if (ret != GST_FLOW_NOT_LINKED)
    goto done;

  /* only return NOT_LINKED if all other pads returned NOT_LINKED */
  for (walk = demux->srcpads; walk; walk = g_slist_next (walk)) {
    GstMultipartPad *opad = (GstMultipartPad *) walk->data;

    ret = opad->last_ret;
    /* some other return value (must be SUCCESS but we can return
     * other values as well) */
    if (ret != GST_FLOW_NOT_LINKED)
      goto done;
  }
  /* if we get here, all other pads were unlinked and we return
   * NOT_LINKED then */
done:
  return ret;
}

static GstMultipartPad *
gst_multipart_find_pad_by_mime (GstMultipartDemux * demux, gchar * mime,
    gboolean * created)
{
  GSList *walk;

  walk = demux->srcpads;
  while (walk) {
    GstMultipartPad *pad = (GstMultipartPad *) walk->data;

    if (!strcmp (pad->mime, mime)) {
      if (created) {
        *created = FALSE;
      }
      return pad;
    }

    walk = walk->next;
  }
  /* pad not found, create it */
  {
    GstPad *pad;
    GstMultipartPad *mppad;
    gchar *name;
    const gchar *capsname;
    GstCaps *caps;
    gchar *stream_id;
    GstEvent *event;

    mppad = g_new0 (GstMultipartPad, 1);

    GST_DEBUG_OBJECT (demux, "creating pad with mime: %s", mime);

    name = g_strdup_printf ("src_%u", demux->numpads);
    pad =
        gst_pad_new_from_static_template (&multipart_demux_src_template_factory,
        name);
    g_free (name);

    mppad->pad = pad;
    mppad->mime = g_strdup (mime);
    mppad->last_ret = GST_FLOW_OK;
    mppad->last_ts = GST_CLOCK_TIME_NONE;
    mppad->discont = TRUE;

    demux->srcpads = g_slist_prepend (demux->srcpads, mppad);
    demux->numpads++;

    gst_pad_use_fixed_caps (pad);
    gst_pad_set_active (pad, TRUE);

    /* prepare and send stream-start */
    if (!demux->have_group_id) {
      event = gst_pad_get_sticky_event (demux->sinkpad,
          GST_EVENT_STREAM_START, 0);

      if (event) {
        demux->have_group_id =
            gst_event_parse_group_id (event, &demux->group_id);
        gst_event_unref (event);
      } else if (!demux->have_group_id) {
        demux->have_group_id = TRUE;
        demux->group_id = gst_util_group_id_next ();
      }
    }

    stream_id = gst_pad_create_stream_id (pad,
        GST_ELEMENT_CAST (demux), demux->mime_type);

    event = gst_event_new_stream_start (stream_id);
    if (demux->have_group_id)
      gst_event_set_group_id (event, demux->group_id);

    gst_pad_store_sticky_event (pad, event);
    g_free (stream_id);
    gst_event_unref (event);

    /* take the mime type, convert it to the caps name */
    capsname = gst_multipart_demux_get_gstname (demux, mime);
    caps = gst_caps_from_string (capsname);
    GST_DEBUG_OBJECT (demux, "caps for pad: %s", capsname);
    gst_pad_set_caps (pad, caps);
    gst_element_add_pad (GST_ELEMENT_CAST (demux), pad);
    gst_caps_unref (caps);

    if (created) {
      *created = TRUE;
    }

    if (demux->singleStream) {
      gst_element_no_more_pads (GST_ELEMENT_CAST (demux));
    }

    return mppad;
  }
}

static gboolean
get_line_end (const guint8 * data, const guint8 * dataend, guint8 ** end,
    guint8 ** next)
{
  guint8 *x;
  gboolean foundr = FALSE;

  for (x = (guint8 *) data; x < dataend; x++) {
    if (*x == '\r') {
      foundr = TRUE;
    } else if (*x == '\n') {
      *end = x - (foundr ? 1 : 0);
      *next = x + 1;
      return TRUE;
    }
  }
  return FALSE;
}

static guint
get_mime_len (const guint8 * data, guint maxlen)
{
  guint8 *x;

  x = (guint8 *) data;
  while (*x != '\0' && *x != '\r' && *x != '\n' && *x != ';') {
    x++;
  }
  return x - data;
}

static gint
multipart_parse_header (GstMultipartDemux * multipart)
{
  const guint8 *data;
  const guint8 *dataend;
  gchar *boundary;
  int boundary_len;
  int datalen;
  guint8 *pos;
  guint8 *end, *next;

  datalen = gst_adapter_available (multipart->adapter);
  data = gst_adapter_map (multipart->adapter, datalen);
  dataend = data + datalen;

  /* Skip leading whitespace, pos endposition should at least leave space for
   * the boundary and a \n */
  for (pos = (guint8 *) data; pos < dataend - 4 && g_ascii_isspace (*pos);
      pos++);

  if (pos >= dataend - 4)
    goto need_more_data;

  if (G_UNLIKELY (pos[0] != '-' || pos[1] != '-')) {
    GST_DEBUG_OBJECT (multipart, "No boundary available");
    goto wrong_header;
  }

  /* First the boundary */
  if (!get_line_end (pos, dataend, &end, &next))
    goto need_more_data;

  /* Ignore the leading -- */
  boundary_len = end - pos - 2;
  boundary = (gchar *) pos + 2;
  if (boundary_len < 1) {
    GST_DEBUG_OBJECT (multipart, "No boundary available");
    goto wrong_header;
  }

  if (G_UNLIKELY (multipart->boundary == NULL)) {
    /* First time we see the boundary, copy it */
    multipart->boundary = g_strndup (boundary, boundary_len);
    multipart->boundary_len = boundary_len;
  } else if (G_UNLIKELY (boundary_len != multipart->boundary_len)) {
    /* Something odd is going on, either the boundary indicated EOS or it's
     * invalid */
    if (G_UNLIKELY (boundary_len == multipart->boundary_len + 2 &&
            !strncmp (boundary, multipart->boundary, multipart->boundary_len) &&
            !strncmp (boundary + multipart->boundary_len, "--", 2)))
      goto eos;

    GST_DEBUG_OBJECT (multipart,
        "Boundary length doesn't match detected boundary (%d <> %d",
        boundary_len, multipart->boundary_len);
    goto wrong_header;
  } else if (G_UNLIKELY (strncmp (boundary, multipart->boundary, boundary_len))) {
    GST_DEBUG_OBJECT (multipart, "Boundary doesn't match previous boundary");
    goto wrong_header;
  }

  pos = next;
  while (get_line_end (pos, dataend, &end, &next)) {
    guint len = end - pos;

    if (len == 0) {
      /* empty line, data starts behind us */
      GST_DEBUG_OBJECT (multipart,
          "Parsed the header - boundary: %s, mime-type: %s, content-length: %d",
          multipart->boundary, multipart->mime_type, multipart->content_length);
      gst_adapter_unmap (multipart->adapter);
      return next - data;
    }

    if (len >= 14 && !g_ascii_strncasecmp ("content-type:", (gchar *) pos, 13)) {
      guint mime_len;

      /* only take the mime type up to the first ; if any. After ; there can be
       * properties that we don't handle yet. */
      mime_len = get_mime_len (pos + 14, len - 14);

      g_free (multipart->mime_type);
      multipart->mime_type = g_ascii_strdown ((gchar *) pos + 14, mime_len);
    } else if (len >= 15 &&
        !g_ascii_strncasecmp ("content-length:", (gchar *) pos, 15)) {
      multipart->content_length =
          g_ascii_strtoull ((gchar *) pos + 15, NULL, 10);
    }
    pos = next;
  }

need_more_data:
  GST_DEBUG_OBJECT (multipart, "Need more data for the header");
  gst_adapter_unmap (multipart->adapter);

  return MULTIPART_NEED_MORE_DATA;

wrong_header:
  {
    GST_ELEMENT_ERROR (multipart, STREAM, DEMUX, (NULL),
        ("Boundary not found in the multipart header"));
    gst_adapter_unmap (multipart->adapter);
    return MULTIPART_DATA_ERROR;
  }
eos:
  {
    GST_DEBUG_OBJECT (multipart, "we are EOS");
    gst_adapter_unmap (multipart->adapter);
    return MULTIPART_DATA_EOS;
  }
}

static gint
multipart_find_boundary (GstMultipartDemux * multipart, gint * datalen)
{
  /* Adaptor is positioned at the start of the data */
  const guint8 *data, *pos;
  const guint8 *dataend;
  gint len;

  if (multipart->content_length >= 0) {
    /* fast path, known content length :) */
    len = multipart->content_length;
    if (gst_adapter_available (multipart->adapter) >= len + 2) {
      *datalen = len;
      data = gst_adapter_map (multipart->adapter, len + 1);

      /* If data[len] contains \r then assume a newline is \r\n */
      if (data[len] == '\r')
        len += 2;
      else if (data[len] == '\n')
        len += 1;

      gst_adapter_unmap (multipart->adapter);
      /* Don't check if boundary is actually there, but let the header parsing
       * bail out if it isn't */
      return len;
    } else {
      /* need more data */
      return MULTIPART_NEED_MORE_DATA;
    }
  }

  len = gst_adapter_available (multipart->adapter);
  if (len == 0)
    return MULTIPART_NEED_MORE_DATA;
  data = gst_adapter_map (multipart->adapter, len);
  dataend = data + len;

  for (pos = data + multipart->scanpos;
      pos <= dataend - multipart->boundary_len - 2; pos++) {
    if (*pos == '-' && pos[1] == '-' &&
        !strncmp ((gchar *) pos + 2,
            multipart->boundary, multipart->boundary_len)) {
      /* Found the boundary! Check if there was a newline before the boundary */
      len = pos - data;
      if (pos - 2 > data && pos[-2] == '\r')
        len -= 2;
      else if (pos - 1 > data && pos[-1] == '\n')
        len -= 1;
      *datalen = len;

      gst_adapter_unmap (multipart->adapter);
      multipart->scanpos = 0;
      return pos - data;
    }
  }
  gst_adapter_unmap (multipart->adapter);
  multipart->scanpos = pos - data;
  return MULTIPART_NEED_MORE_DATA;
}

static gboolean
gst_multipart_demux_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstMultipartDemux *multipart;

  multipart = GST_MULTIPART_DEMUX (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      if (!multipart->srcpads) {
        GST_ELEMENT_ERROR (multipart, STREAM, WRONG_TYPE,
            ("This stream contains no valid streams."),
            ("Got EOS before adding any pads"));
        gst_event_unref (event);
        return FALSE;
      } else {
        return gst_pad_event_default (pad, parent, event);
      }
      break;
    default:
      return gst_pad_event_default (pad, parent, event);
  }
}

static GstFlowReturn
gst_multipart_demux_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstMultipartDemux *multipart;
  GstAdapter *adapter;
  gint size = 1;
  GstFlowReturn res;

  multipart = GST_MULTIPART_DEMUX (parent);
  adapter = multipart->adapter;

  res = GST_FLOW_OK;

  if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DISCONT)) {
    GSList *l;

    for (l = multipart->srcpads; l != NULL; l = l->next) {
      GstMultipartPad *srcpad = l->data;

      srcpad->discont = TRUE;
    }
    gst_adapter_clear (adapter);
  }
  gst_adapter_push (adapter, buf);

  while (gst_adapter_available (adapter) > 0) {
    GstMultipartPad *srcpad;
    GstBuffer *outbuf;
    gboolean created;
    gint datalen;

    if (G_UNLIKELY (!multipart->header_completed)) {
      if ((size = multipart_parse_header (multipart)) < 0) {
        goto nodata;
      } else {
        gst_adapter_flush (adapter, size);
        multipart->header_completed = TRUE;
      }
    }
    if ((size = multipart_find_boundary (multipart, &datalen)) < 0) {
      goto nodata;
    }

    /* Invalidate header info */
    multipart->header_completed = FALSE;
    multipart->content_length = -1;

    if (G_UNLIKELY (datalen <= 0)) {
      GST_DEBUG_OBJECT (multipart, "skipping empty content.");
      gst_adapter_flush (adapter, size - datalen);
    } else if (G_UNLIKELY (!multipart->mime_type)) {
      GST_DEBUG_OBJECT (multipart, "content has no MIME type.");
      gst_adapter_flush (adapter, size - datalen);
    } else {
      GstClockTime ts;

      srcpad =
          gst_multipart_find_pad_by_mime (multipart,
          multipart->mime_type, &created);

      ts = gst_adapter_prev_pts (adapter, NULL);
      outbuf = gst_adapter_take_buffer (adapter, datalen);
      gst_adapter_flush (adapter, size - datalen);

      if (created) {
        GstTagList *tags;
        GstSegment segment;

        gst_segment_init (&segment, GST_FORMAT_TIME);

        /* Push new segment, first buffer has 0 timestamp */
        gst_pad_push_event (srcpad->pad, gst_event_new_segment (&segment));

        tags = gst_tag_list_new (GST_TAG_CONTAINER_FORMAT, "Multipart", NULL);
        gst_tag_list_set_scope (tags, GST_TAG_SCOPE_GLOBAL);
        gst_pad_push_event (srcpad->pad, gst_event_new_tag (tags));
      }

      outbuf = gst_buffer_make_writable (outbuf);
      if (srcpad->last_ts == GST_CLOCK_TIME_NONE || srcpad->last_ts != ts) {
        GST_BUFFER_TIMESTAMP (outbuf) = ts;
        srcpad->last_ts = ts;
      } else {
        GST_BUFFER_TIMESTAMP (outbuf) = GST_CLOCK_TIME_NONE;
      }

      if (srcpad->discont) {
        GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
        srcpad->discont = FALSE;
      } else {
        GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_DISCONT);
      }

      GST_DEBUG_OBJECT (multipart,
          "pushing buffer with timestamp %" GST_TIME_FORMAT,
          GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)));
      res = gst_pad_push (srcpad->pad, outbuf);
      res = gst_multipart_combine_flows (multipart, srcpad, res);
      if (res != GST_FLOW_OK)
        break;
    }
  }

nodata:
  if (G_UNLIKELY (size == MULTIPART_DATA_ERROR))
    return GST_FLOW_ERROR;
  if (G_UNLIKELY (size == MULTIPART_DATA_EOS))
    return GST_FLOW_EOS;

  return res;
}

static GstStateChangeReturn
gst_multipart_demux_change_state (GstElement * element,
    GstStateChange transition)
{
  GstMultipartDemux *multipart;
  GstStateChangeReturn ret;

  multipart = GST_MULTIPART_DEMUX (element);

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      multipart->header_completed = FALSE;
      g_free (multipart->boundary);
      multipart->boundary = NULL;
      g_free (multipart->mime_type);
      multipart->mime_type = NULL;
      gst_adapter_clear (multipart->adapter);
      multipart->content_length = -1;
      multipart->scanpos = 0;
      gst_multipart_demux_remove_src_pads (multipart);
      multipart->have_group_id = FALSE;
      multipart->group_id = G_MAXUINT;
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}


static void
gst_multipart_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMultipartDemux *filter;

  filter = GST_MULTIPART_DEMUX (object);

  switch (prop_id) {
    case PROP_BOUNDARY:
      /* Not really that useful anymore as we can reliably autoscan */
      g_free (filter->boundary);
      filter->boundary = g_value_dup_string (value);
      if (filter->boundary != NULL) {
        filter->boundary_len = strlen (filter->boundary);
      }
      break;
    case PROP_SINGLE_STREAM:
      filter->singleStream = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_multipart_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstMultipartDemux *filter;

  filter = GST_MULTIPART_DEMUX (object);

  switch (prop_id) {
    case PROP_BOUNDARY:
      g_value_set_string (value, filter->boundary);
      break;
    case PROP_SINGLE_STREAM:
      g_value_set_boolean (value, filter->singleStream);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}



gboolean
gst_multipart_demux_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_multipart_demux_debug,
      "multipartdemux", 0, "multipart demuxer");

  return gst_element_register (plugin, "multipartdemux", GST_RANK_PRIMARY,
      GST_TYPE_MULTIPART_DEMUX);
}
