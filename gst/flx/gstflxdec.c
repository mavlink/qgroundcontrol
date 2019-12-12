/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@temple-baptist.com>
 * Copyright (C) <2016> Matthew Waters <matthew@centricular.com>
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
 * SECTION:element-flxdec
 * @title: flxdec
 *
 * This element decodes fli/flc/flx-video into raw video
 */
/*
 * http://www.coolutils.com/Formats/FLI
 * http://woodshole.er.usgs.gov/operations/modeling/flc.html
 * http://www.compuphase.com/flic.htm
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "flx_fmt.h"
#include "gstflxdec.h"
#include <gst/video/video.h>

#define JIFFIE  (GST_SECOND/70)

GST_DEBUG_CATEGORY_STATIC (flxdec_debug);
#define GST_CAT_DEFAULT flxdec_debug

/* input */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-fli")
    );

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define RGB_ORDER "xRGB"
#else
#define RGB_ORDER "BGRx"
#endif

/* output */
static GstStaticPadTemplate src_video_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (RGB_ORDER))
    );

static void gst_flxdec_dispose (GstFlxDec * flxdec);

static GstFlowReturn gst_flxdec_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buf);
static gboolean gst_flxdec_sink_event_handler (GstPad * pad,
    GstObject * parent, GstEvent * event);

static GstStateChangeReturn gst_flxdec_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_flxdec_src_query_handler (GstPad * pad, GstObject * parent,
    GstQuery * query);

static gboolean flx_decode_color (GstFlxDec * flxdec, GstByteReader * reader,
    GstByteWriter * writer, gint scale);
static gboolean flx_decode_brun (GstFlxDec * flxdec,
    GstByteReader * reader, GstByteWriter * writer);
static gboolean flx_decode_delta_fli (GstFlxDec * flxdec,
    GstByteReader * reader, GstByteWriter * writer);
static gboolean flx_decode_delta_flc (GstFlxDec * flxdec,
    GstByteReader * reader, GstByteWriter * writer);

#define rndalign(off) ((off) + ((off) & 1))

#define gst_flxdec_parent_class parent_class
G_DEFINE_TYPE (GstFlxDec, gst_flxdec, GST_TYPE_ELEMENT);

static void
gst_flxdec_class_init (GstFlxDecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = (GObjectFinalizeFunc) gst_flxdec_dispose;

  GST_DEBUG_CATEGORY_INIT (flxdec_debug, "flxdec", 0, "FLX video decoder");

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_flxdec_change_state);

  gst_element_class_set_static_metadata (gstelement_class, "FLX video decoder",
      "Codec/Decoder/Video",
      "FLC/FLI/FLX video decoder",
      "Sepp Wijnands <mrrazz@garbage-coderz.net>, Zeeshan Ali <zeenix@gmail.com>");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_video_factory));
}

static void
gst_flxdec_init (GstFlxDec * flxdec)
{
  flxdec->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_element_add_pad (GST_ELEMENT (flxdec), flxdec->sinkpad);
  gst_pad_set_chain_function (flxdec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flxdec_chain));
  gst_pad_set_event_function (flxdec->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flxdec_sink_event_handler));

  flxdec->srcpad = gst_pad_new_from_static_template (&src_video_factory, "src");
  gst_element_add_pad (GST_ELEMENT (flxdec), flxdec->srcpad);
  gst_pad_set_query_function (flxdec->srcpad,
      GST_DEBUG_FUNCPTR (gst_flxdec_src_query_handler));

  gst_pad_use_fixed_caps (flxdec->srcpad);

  flxdec->adapter = gst_adapter_new ();
}

static void
gst_flxdec_dispose (GstFlxDec * flxdec)
{
  if (flxdec->adapter) {
    g_object_unref (flxdec->adapter);
    flxdec->adapter = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose ((GObject *) flxdec);
}

static gboolean
gst_flxdec_src_query_handler (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  GstFlxDec *flxdec = (GstFlxDec *) parent;
  gboolean ret = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      if (format != GST_FORMAT_TIME)
        goto done;

      gst_query_set_duration (query, format, flxdec->duration);

      ret = TRUE;
    }
    default:
      break;
  }
done:
  if (!ret)
    ret = gst_pad_query_default (pad, parent, query);

  return ret;
}

static gboolean
gst_flxdec_sink_event_handler (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstFlxDec *flxdec;
  gboolean ret;

  flxdec = GST_FLXDEC (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    {
      gst_event_copy_segment (event, &flxdec->segment);
      if (flxdec->segment.format != GST_FORMAT_TIME) {
        GST_DEBUG_OBJECT (flxdec, "generating TIME segment");
        gst_segment_init (&flxdec->segment, GST_FORMAT_TIME);
        gst_event_unref (event);
        event = gst_event_new_segment (&flxdec->segment);
      }

      if (gst_pad_has_current_caps (flxdec->srcpad)) {
        ret = gst_pad_event_default (pad, parent, event);
      } else {
        flxdec->need_segment = TRUE;
        gst_event_unref (event);
        ret = TRUE;
      }
      break;
    }
    case GST_EVENT_FLUSH_STOP:
      gst_segment_init (&flxdec->segment, GST_FORMAT_UNDEFINED);
      ret = gst_pad_event_default (pad, parent, event);
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

static gboolean
flx_decode_chunks (GstFlxDec * flxdec, gulong n_chunks, GstByteReader * reader,
    GstByteWriter * writer)
{
  gboolean ret = TRUE;

  while (n_chunks--) {
    GstByteReader chunk;
    guint32 size;
    guint16 type;

    if (!gst_byte_reader_get_uint32_le (reader, &size))
      goto parse_error;
    if (!gst_byte_reader_get_uint16_le (reader, &type))
      goto parse_error;
    GST_LOG_OBJECT (flxdec, "chunk has type 0x%02x size %d", type, size);

    if (!gst_byte_reader_get_sub_reader (reader, &chunk,
            size - FlxFrameChunkSize)) {
      GST_ERROR_OBJECT (flxdec, "Incorrect size in the chunk header");
      goto error;
    }

    switch (type) {
      case FLX_COLOR64:
        ret = flx_decode_color (flxdec, &chunk, writer, 2);
        break;

      case FLX_COLOR256:
        ret = flx_decode_color (flxdec, &chunk, writer, 0);
        break;

      case FLX_BRUN:
        ret = flx_decode_brun (flxdec, &chunk, writer);
        break;

      case FLX_LC:
        ret = flx_decode_delta_fli (flxdec, &chunk, writer);
        break;

      case FLX_SS2:
        ret = flx_decode_delta_flc (flxdec, &chunk, writer);
        break;

      case FLX_BLACK:
        ret = gst_byte_writer_fill (writer, 0, flxdec->size);
        break;

      case FLX_MINI:
        break;

      default:
        GST_WARNING ("Unimplemented chunk type: 0x%02x size: %d - skipping",
            type, size);
        break;
    }

    if (!ret)
      break;
  }

  return ret;

parse_error:
  GST_ERROR_OBJECT (flxdec, "Failed to decode chunk");
error:
  return FALSE;
}


static gboolean
flx_decode_color (GstFlxDec * flxdec, GstByteReader * reader,
    GstByteWriter * writer, gint scale)
{
  guint8 count, indx;
  guint16 packs;

  if (!gst_byte_reader_get_uint16_le (reader, &packs))
    goto error;
  indx = 0;

  GST_LOG ("GstFlxDec: cmap packs: %d", (guint) packs);
  while (packs--) {
    const guint8 *data;
    guint16 actual_count;

    /* color map index + skip count */
    if (!gst_byte_reader_get_uint8 (reader, &indx))
      goto error;

    /* number of rgb triplets */
    if (!gst_byte_reader_get_uint8 (reader, &count))
      goto error;

    actual_count = count == 0 ? 256 : count;

    if (!gst_byte_reader_get_data (reader, count * 3, &data))
      goto error;

    GST_LOG_OBJECT (flxdec, "cmap count: %d (indx: %d)", actual_count, indx);
    flx_set_palette_vector (flxdec->converter, indx, actual_count,
        (guchar *) data, scale);
  }

  return TRUE;

error:
  GST_ERROR_OBJECT (flxdec, "Error decoding color palette");
  return FALSE;
}

static gboolean
flx_decode_brun (GstFlxDec * flxdec, GstByteReader * reader,
    GstByteWriter * writer)
{
  gulong lines, row;

  g_return_val_if_fail (flxdec != NULL, FALSE);

  lines = flxdec->hdr.height;
  while (lines--) {
    /* packet count.  
     * should not be used anymore, since the flc format can
     * contain more then 255 RLE packets. we use the frame 
     * width instead. 
     */
    if (!gst_byte_reader_skip (reader, 1))
      goto error;

    row = flxdec->hdr.width;
    while (row) {
      gint8 count;

      if (!gst_byte_reader_get_int8 (reader, &count))
        goto error;

      if (count <= 0) {
        const guint8 *data;

        /* literal run */
        count = ABS (count);

        GST_LOG_OBJECT (flxdec, "have literal run of size %d", count);

        if (count > row) {
          GST_ERROR_OBJECT (flxdec, "Invalid BRUN line detected. "
              "bytes to write exceeds the end of the row");
          return FALSE;
        }
        row -= count;

        if (!gst_byte_reader_get_data (reader, count, &data))
          goto error;
        if (!gst_byte_writer_put_data (writer, data, count))
          goto error;
      } else {
        guint8 x;

        GST_LOG_OBJECT (flxdec, "have replicate run of size %d", count);

        if (count > row) {
          GST_ERROR_OBJECT (flxdec, "Invalid BRUN packet detected."
              "bytes to write exceeds the end of the row");
          return FALSE;
        }

        /* replicate run */
        row -= count;

        if (!gst_byte_reader_get_uint8 (reader, &x))
          goto error;
        if (!gst_byte_writer_fill (writer, x, count))
          goto error;
      }
    }
  }

  return TRUE;

error:
  GST_ERROR_OBJECT (flxdec, "Failed to decode BRUN packet");
  return FALSE;
}

static gboolean
flx_decode_delta_fli (GstFlxDec * flxdec, GstByteReader * reader,
    GstByteWriter * writer)
{
  guint16 start_line, lines;
  guint line_start_i;

  g_return_val_if_fail (flxdec != NULL, FALSE);
  g_return_val_if_fail (flxdec->delta_data != NULL, FALSE);

  /* use last frame for delta */
  if (!gst_byte_writer_put_data (writer, flxdec->delta_data, flxdec->size))
    goto error;

  if (!gst_byte_reader_get_uint16_le (reader, &start_line))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &lines))
    goto error;
  GST_LOG_OBJECT (flxdec, "height %d start line %d line count %d",
      flxdec->hdr.height, start_line, lines);

  if (start_line + lines > flxdec->hdr.height) {
    GST_ERROR_OBJECT (flxdec, "Invalid FLI packet detected. too many lines.");
    return FALSE;
  }

  line_start_i = flxdec->hdr.width * start_line;
  if (!gst_byte_writer_set_pos (writer, line_start_i))
    goto error;

  while (lines--) {
    guint8 packets;

    /* packet count */
    if (!gst_byte_reader_get_uint8 (reader, &packets))
      goto error;
    GST_LOG_OBJECT (flxdec, "have %d packets", packets);

    while (packets--) {
      /* skip count */
      guint8 skip;
      gint8 count;
      if (!gst_byte_reader_get_uint8 (reader, &skip))
        goto error;

      /* skip bytes */
      if (!gst_byte_writer_set_pos (writer,
              gst_byte_writer_get_pos (writer) + skip))
        goto error;

      /* RLE count */
      if (!gst_byte_reader_get_int8 (reader, &count))
        goto error;

      if (count < 0) {
        guint8 x;

        /* literal run */
        count = ABS (count);
        GST_LOG_OBJECT (flxdec, "have literal run of size %d at offset %d",
            count, skip);

        if (skip + count > flxdec->hdr.width) {
          GST_ERROR_OBJECT (flxdec, "Invalid FLI packet detected. "
              "line too long.");
          return FALSE;
        }

        if (!gst_byte_reader_get_uint8 (reader, &x))
          goto error;
        if (!gst_byte_writer_fill (writer, x, count))
          goto error;
      } else {
        const guint8 *data;

        GST_LOG_OBJECT (flxdec, "have replicate run of size %d at offset %d",
            count, skip);

        if (skip + count > flxdec->hdr.width) {
          GST_ERROR_OBJECT (flxdec, "Invalid FLI packet detected. "
              "line too long.");
          return FALSE;
        }

        /* replicate run */
        if (!gst_byte_reader_get_data (reader, count, &data))
          goto error;
        if (!gst_byte_writer_put_data (writer, data, count))
          goto error;
      }
    }
    line_start_i += flxdec->hdr.width;
    if (!gst_byte_writer_set_pos (writer, line_start_i))
      goto error;
  }

  return TRUE;

error:
  GST_ERROR_OBJECT (flxdec, "Failed to decode FLI packet");
  return FALSE;
}

static gboolean
flx_decode_delta_flc (GstFlxDec * flxdec, GstByteReader * reader,
    GstByteWriter * writer)
{
  guint16 lines, start_l;

  g_return_val_if_fail (flxdec != NULL, FALSE);
  g_return_val_if_fail (flxdec->delta_data != NULL, FALSE);

  /* use last frame for delta */
  if (!gst_byte_writer_put_data (writer, flxdec->delta_data, flxdec->size))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &lines))
    goto error;

  if (lines > flxdec->hdr.height) {
    GST_ERROR_OBJECT (flxdec, "Invalid FLC packet detected. too many lines.");
    return FALSE;
  }

  start_l = lines;

  while (lines) {
    guint16 opcode;

    if (!gst_byte_writer_set_pos (writer,
            flxdec->hdr.width * (start_l - lines)))
      goto error;

    /* process opcode(s) */
    while (TRUE) {
      if (!gst_byte_reader_get_uint16_le (reader, &opcode))
        goto error;
      if ((opcode & 0xc000) == 0)
        break;

      if ((opcode & 0xc000) == 0xc000) {
        /* line skip count */
        gulong skip = (0x10000 - opcode);
        if (skip > flxdec->hdr.height) {
          GST_ERROR_OBJECT (flxdec, "Invalid FLC packet detected. "
              "skip line count too big.");
          return FALSE;
        }
        start_l += skip;
        if (!gst_byte_writer_set_pos (writer,
                gst_byte_writer_get_pos (writer) + flxdec->hdr.width * skip))
          goto error;
      } else {
        /* last pixel */
        if (!gst_byte_writer_set_pos (writer,
                gst_byte_writer_get_pos (writer) + flxdec->hdr.width))
          goto error;
        if (!gst_byte_writer_put_uint8 (writer, opcode & 0xff))
          goto error;
      }
    }

    /* last opcode is the packet count */
    GST_LOG_OBJECT (flxdec, "have %d packets", opcode);
    while (opcode--) {
      /* skip count */
      guint8 skip;
      gint8 count;

      if (!gst_byte_reader_get_uint8 (reader, &skip))
        goto error;
      if (!gst_byte_writer_set_pos (writer,
              gst_byte_writer_get_pos (writer) + skip))
        goto error;

      /* RLE count */
      if (!gst_byte_reader_get_int8 (reader, &count))
        goto error;

      if (count < 0) {
        guint16 x;

        /* replicate word run */
        count = ABS (count);

        GST_LOG_OBJECT (flxdec, "have replicate run of size %d at offset %d",
            count, skip);

        if (skip + count > flxdec->hdr.width) {
          GST_ERROR_OBJECT (flxdec, "Invalid FLC packet detected. "
              "line too long.");
          return FALSE;
        }

        if (!gst_byte_reader_get_uint16_le (reader, &x))
          goto error;

        while (count--) {
          if (!gst_byte_writer_put_uint16_le (writer, x)) {
            goto error;
          }
        }
      } else {
        GST_LOG_OBJECT (flxdec, "have literal run of size %d at offset %d",
            count, skip);

        if (skip + count > flxdec->hdr.width) {
          GST_ERROR_OBJECT (flxdec, "Invalid FLC packet detected. "
              "line too long.");
          return FALSE;
        }

        while (count--) {
          guint16 x;

          if (!gst_byte_reader_get_uint16_le (reader, &x))
            goto error;
          if (!gst_byte_writer_put_uint16_le (writer, x))
            goto error;
        }
      }
    }
    lines--;
  }

  return TRUE;

error:
  GST_ERROR_OBJECT (flxdec, "Failed to decode FLI packet");
  return FALSE;
}

static gboolean
_read_flx_header (GstFlxDec * flxdec, GstByteReader * reader, FlxHeader * flxh)
{
  memset (flxh, 0, sizeof (*flxh));

  if (!gst_byte_reader_get_uint32_le (reader, &flxh->size))
    goto error;
  if (flxh->size < FlxHeaderSize) {
    GST_ERROR_OBJECT (flxdec, "Invalid file size in the header");
    return FALSE;
  }

  if (!gst_byte_reader_get_uint16_le (reader, &flxh->type))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->frames))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->width))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->height))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->depth))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->flags))
    goto error;
  if (!gst_byte_reader_get_uint32_le (reader, &flxh->speed))
    goto error;
  if (!gst_byte_reader_skip (reader, 2))        /* reserved */
    goto error;
  /* FLC */
  if (!gst_byte_reader_get_uint32_le (reader, &flxh->created))
    goto error;
  if (!gst_byte_reader_get_uint32_le (reader, &flxh->creator))
    goto error;
  if (!gst_byte_reader_get_uint32_le (reader, &flxh->updated))
    goto error;
  if (!gst_byte_reader_get_uint32_le (reader, &flxh->updater))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->aspect_dx))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->aspect_dy))
    goto error;
  /* EGI */
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->ext_flags))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->keyframes))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->totalframes))
    goto error;
  if (!gst_byte_reader_get_uint32_le (reader, &flxh->req_memory))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->max_regions))
    goto error;
  if (!gst_byte_reader_get_uint16_le (reader, &flxh->transp_num))
    goto error;
  if (!gst_byte_reader_skip (reader, 24))       /* reserved */
    goto error;
  /* FLC */
  if (!gst_byte_reader_get_uint32_le (reader, &flxh->oframe1))
    goto error;
  if (!gst_byte_reader_get_uint32_le (reader, &flxh->oframe2))
    goto error;
  if (!gst_byte_reader_skip (reader, 40))       /* reserved */
    goto error;

  return TRUE;

error:
  GST_ERROR_OBJECT (flxdec, "Error reading file header");
  return FALSE;
}

static GstFlowReturn
gst_flxdec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstByteReader reader;
  GstBuffer *input;
  GstMapInfo map_info;
  GstCaps *caps;
  guint available;
  GstFlowReturn res = GST_FLOW_OK;

  GstFlxDec *flxdec;
  FlxHeader *flxh;

  g_return_val_if_fail (buf != NULL, GST_FLOW_ERROR);
  flxdec = (GstFlxDec *) parent;
  g_return_val_if_fail (flxdec != NULL, GST_FLOW_ERROR);

  gst_adapter_push (flxdec->adapter, buf);
  available = gst_adapter_available (flxdec->adapter);
  input = gst_adapter_get_buffer (flxdec->adapter, available);
  if (!gst_buffer_map (input, &map_info, GST_MAP_READ)) {
    GST_ELEMENT_ERROR (flxdec, STREAM, DECODE,
        ("%s", "Failed to map buffer"), (NULL));
    goto error;
  }
  gst_byte_reader_init (&reader, map_info.data, map_info.size);

  if (flxdec->state == GST_FLXDEC_READ_HEADER) {
    if (available >= FlxHeaderSize) {
      GstByteReader header;
      GstCaps *templ;

      if (!gst_byte_reader_get_sub_reader (&reader, &header, FlxHeaderSize)) {
        GST_ELEMENT_ERROR (flxdec, STREAM, DECODE,
            ("%s", "Could not read header"), (NULL));
        goto unmap_input_error;
      }
      gst_adapter_flush (flxdec->adapter, FlxHeaderSize);
      available -= FlxHeaderSize;

      if (!_read_flx_header (flxdec, &header, &flxdec->hdr)) {
        GST_ELEMENT_ERROR (flxdec, STREAM, DECODE,
            ("%s", "Failed to parse header"), (NULL));
        goto unmap_input_error;
      }

      flxh = &flxdec->hdr;

      /* check header */
      if (flxh->type != FLX_MAGICHDR_FLI &&
          flxh->type != FLX_MAGICHDR_FLC && flxh->type != FLX_MAGICHDR_FLX) {
        GST_ELEMENT_ERROR (flxdec, STREAM, WRONG_TYPE, (NULL),
            ("not a flx file (type %x)", flxh->type));
        goto unmap_input_error;
      }

      GST_INFO_OBJECT (flxdec, "size      :  %d", flxh->size);
      GST_INFO_OBJECT (flxdec, "frames    :  %d", flxh->frames);
      GST_INFO_OBJECT (flxdec, "width     :  %d", flxh->width);
      GST_INFO_OBJECT (flxdec, "height    :  %d", flxh->height);
      GST_INFO_OBJECT (flxdec, "depth     :  %d", flxh->depth);
      GST_INFO_OBJECT (flxdec, "speed     :  %d", flxh->speed);

      flxdec->next_time = 0;

      if (flxh->type == FLX_MAGICHDR_FLI) {
        flxdec->frame_time = JIFFIE * flxh->speed;
      } else if (flxh->speed == 0) {
        flxdec->frame_time = GST_SECOND / 70;
      } else {
        flxdec->frame_time = flxh->speed * GST_MSECOND;
      }

      flxdec->duration = flxh->frames * flxdec->frame_time;
      GST_LOG ("duration   :  %" GST_TIME_FORMAT,
          GST_TIME_ARGS (flxdec->duration));

      templ = gst_pad_get_pad_template_caps (flxdec->srcpad);
      caps = gst_caps_copy (templ);
      gst_caps_unref (templ);
      gst_caps_set_simple (caps,
          "width", G_TYPE_INT, flxh->width,
          "height", G_TYPE_INT, flxh->height,
          "framerate", GST_TYPE_FRACTION, (gint) GST_MSECOND,
          (gint) flxdec->frame_time / 1000, NULL);

      gst_pad_set_caps (flxdec->srcpad, caps);
      gst_caps_unref (caps);

      if (flxdec->need_segment) {
        gst_pad_push_event (flxdec->srcpad,
            gst_event_new_segment (&flxdec->segment));
        flxdec->need_segment = FALSE;
      }

      /* zero means 8 */
      if (flxh->depth == 0)
        flxh->depth = 8;

      if (flxh->depth != 8) {
        GST_ELEMENT_ERROR (flxdec, STREAM, WRONG_TYPE,
            ("%s", "Don't know how to decode non 8 bit depth streams"), (NULL));
        goto unmap_input_error;
      }

      flxdec->converter =
          flx_colorspace_converter_new (flxh->width, flxh->height);

      if (flxh->type == FLX_MAGICHDR_FLC || flxh->type == FLX_MAGICHDR_FLX) {
        GST_INFO_OBJECT (flxdec, "(FLC) aspect_dx :  %d", flxh->aspect_dx);
        GST_INFO_OBJECT (flxdec, "(FLC) aspect_dy :  %d", flxh->aspect_dy);
        GST_INFO_OBJECT (flxdec, "(FLC) oframe1   :  0x%08x", flxh->oframe1);
        GST_INFO_OBJECT (flxdec, "(FLC) oframe2   :  0x%08x", flxh->oframe2);
      }

      flxdec->size = ((guint) flxh->width * (guint) flxh->height);
      if (flxdec->size >= G_MAXSIZE / 4) {
        GST_ELEMENT_ERROR (flxdec, STREAM, DECODE,
            ("%s", "Cannot allocate required memory"), (NULL));
        goto unmap_input_error;
      }

      /* create delta and output frame */
      flxdec->frame_data = g_malloc0 (flxdec->size);
      flxdec->delta_data = g_malloc0 (flxdec->size);

      flxdec->state = GST_FLXDEC_PLAYING;
    }
  } else if (flxdec->state == GST_FLXDEC_PLAYING) {
    GstBuffer *out;

    /* while we have enough data in the adapter */
    while (available >= FlxFrameChunkSize && res == GST_FLOW_OK) {
      guint32 size;
      guint16 type;

      if (!gst_byte_reader_get_uint32_le (&reader, &size))
        goto parse_error;
      if (available < size)
        goto need_more_data;

      available -= size;
      gst_adapter_flush (flxdec->adapter, size);

      if (!gst_byte_reader_get_uint16_le (&reader, &type))
        goto parse_error;

      switch (type) {
        case FLX_FRAME_TYPE:{
          GstByteReader chunks;
          GstByteWriter writer;
          guint16 n_chunks;
          GstMapInfo map;

          GST_LOG_OBJECT (flxdec, "Have frame type 0x%02x of size %d", type,
              size);

          if (!gst_byte_reader_get_sub_reader (&reader, &chunks,
                  size - FlxFrameChunkSize))
            goto parse_error;

          if (!gst_byte_reader_get_uint16_le (&chunks, &n_chunks))
            goto parse_error;
          GST_LOG_OBJECT (flxdec, "Have %d chunks", n_chunks);

          if (n_chunks == 0)
            break;
          if (!gst_byte_reader_skip (&chunks, 8))       /* reserved */
            goto parse_error;

          gst_byte_writer_init_with_data (&writer, flxdec->frame_data,
              flxdec->size, TRUE);

          /* decode chunks */
          if (!flx_decode_chunks (flxdec, n_chunks, &chunks, &writer)) {
            GST_ELEMENT_ERROR (flxdec, STREAM, DECODE,
                ("%s", "Could not decode chunk"), NULL);
            goto unmap_input_error;
          }
          gst_byte_writer_reset (&writer);

          /* save copy of the current frame for possible delta. */
          memcpy (flxdec->delta_data, flxdec->frame_data, flxdec->size);

          out = gst_buffer_new_and_alloc (flxdec->size * 4);
          if (!gst_buffer_map (out, &map, GST_MAP_WRITE)) {
            GST_ELEMENT_ERROR (flxdec, STREAM, DECODE,
                ("%s", "Could not map output buffer"), NULL);
            gst_buffer_unref (out);
            goto unmap_input_error;
          }

          /* convert current frame. */
          flx_colorspace_convert (flxdec->converter, flxdec->frame_data,
              map.data);
          gst_buffer_unmap (out, &map);

          GST_BUFFER_TIMESTAMP (out) = flxdec->next_time;
          flxdec->next_time += flxdec->frame_time;

          res = gst_pad_push (flxdec->srcpad, out);
          break;
        }
        default:
          GST_DEBUG_OBJECT (flxdec, "Unknown frame type 0x%02x, skipping %d",
              type, size);
          if (!gst_byte_reader_skip (&reader, size - FlxFrameChunkSize))
            goto parse_error;
          break;
      }
    }
  }

need_more_data:
  gst_buffer_unmap (input, &map_info);
  gst_buffer_unref (input);
  return res;

  /* ERRORS */
parse_error:
  GST_ELEMENT_ERROR (flxdec, STREAM, DECODE,
      ("%s", "Failed to parse stream"), (NULL));
unmap_input_error:
  gst_buffer_unmap (input, &map_info);
error:
  gst_buffer_unref (input);
  return GST_FLOW_ERROR;
}

static GstStateChangeReturn
gst_flxdec_change_state (GstElement * element, GstStateChange transition)
{
  GstFlxDec *flxdec;
  GstStateChangeReturn ret;

  flxdec = GST_FLXDEC (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_adapter_clear (flxdec->adapter);
      flxdec->state = GST_FLXDEC_READ_HEADER;
      gst_segment_init (&flxdec->segment, GST_FORMAT_UNDEFINED);
      flxdec->need_segment = TRUE;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (flxdec->frame_data) {
        g_free (flxdec->frame_data);
        flxdec->frame_data = NULL;
      }
      if (flxdec->delta_data) {
        g_free (flxdec->delta_data);
        flxdec->delta_data = NULL;
      }
      if (flxdec->converter) {
        flx_colorspace_converter_destroy (flxdec->converter);
        flxdec->converter = NULL;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "flxdec",
      GST_RANK_PRIMARY, GST_TYPE_FLXDEC);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    flxdec,
    "FLC/FLI/FLX video decoder",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
