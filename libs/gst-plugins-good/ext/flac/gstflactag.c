/* GStreamer
 * Copyright (C) 2003 Christophe Fergeau <teuf@gnome.org>
 * Copyright (C) 2008 Jonathan Matthew <jonathan@d14n.org>
 * Copyright (C) 2008 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * gstflactag.c: plug-in for reading/modifying vorbis comments in flac files
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
 * SECTION:element-flactag
 * @title: flactag
 * @see_also: #flacenc, #flacdec, #GstTagSetter
 *
 * The flactag element can change the tag contained within a raw
 * FLAC stream. Specifically, it modifies the comments header packet
 * of the FLAC stream.
 *
 * Applications can set the tags to write using the #GstTagSetter interface.
 * Tags contained within the FLAC bitstream will be picked up
 * automatically (and merged according to the merge mode set via the tag
 * setter interface).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v filesrc location=foo.flac ! flactag ! filesink location=bar.flac
 * ]| This element is not useful with gst-launch, because it does not support
 * setting the tags on a #GstTagSetter interface. Conceptually, the element
 * will usually be used in this order though.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/gsttagsetter.h>
#include <gst/base/gstadapter.h>
#include <gst/tag/tag.h>
#include <string.h>

#include "gstflactag.h"

GST_DEBUG_CATEGORY_STATIC (flactag_debug);
#define GST_CAT_DEFAULT flactag_debug

/* elementfactory information */
static GstStaticPadTemplate flac_tag_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-flac")
    );

static GstStaticPadTemplate flac_tag_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-flac")
    );


static void gst_flac_tag_dispose (GObject * object);

static GstFlowReturn gst_flac_tag_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);

static GstStateChangeReturn gst_flac_tag_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_flac_tag_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

#define gst_flac_tag_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstFlacTag, gst_flac_tag, GST_TYPE_ELEMENT,
    G_IMPLEMENT_INTERFACE (GST_TYPE_TAG_SETTER, NULL));


static void
gst_flac_tag_class_init (GstFlacTagClass * klass)
{
  GstElementClass *gstelement_class;
  GObjectClass *gobject_class;

  GST_DEBUG_CATEGORY_INIT (flactag_debug, "flactag", 0, "flac tag rewriter");

  gstelement_class = (GstElementClass *) klass;
  gobject_class = (GObjectClass *) klass;

  gobject_class->dispose = gst_flac_tag_dispose;
  gstelement_class->change_state = gst_flac_tag_change_state;

  gst_element_class_set_static_metadata (gstelement_class, "FLAC tagger",
      "Formatter/Metadata",
      "Rewrite tags in a FLAC file", "Christophe Fergeau <teuf@gnome.org>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &flac_tag_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &flac_tag_src_template);
}

static void
gst_flac_tag_dispose (GObject * object)
{
  GstFlacTag *tag = GST_FLAC_TAG (object);

  if (tag->adapter) {
    g_object_unref (tag->adapter);
    tag->adapter = NULL;
  }
  if (tag->vorbiscomment) {
    gst_buffer_unref (tag->vorbiscomment);
    tag->vorbiscomment = NULL;
  }
  if (tag->tags) {
    gst_tag_list_unref (tag->tags);
    tag->tags = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
gst_flac_tag_init (GstFlacTag * tag)
{
  /* create the sink and src pads */
  tag->sinkpad =
      gst_pad_new_from_static_template (&flac_tag_sink_template, "sink");
  gst_pad_set_chain_function (tag->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flac_tag_chain));
  gst_pad_set_event_function (tag->sinkpad,
      GST_DEBUG_FUNCPTR (gst_flac_tag_sink_event));
  gst_element_add_pad (GST_ELEMENT (tag), tag->sinkpad);

  tag->srcpad =
      gst_pad_new_from_static_template (&flac_tag_src_template, "src");
  gst_element_add_pad (GST_ELEMENT (tag), tag->srcpad);

  tag->adapter = gst_adapter_new ();
}

static gboolean
gst_flac_tag_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstFlacTag *tag;
  gboolean ret;

  tag = GST_FLAC_TAG (parent);

  GST_DEBUG_OBJECT (pad, "Received %s event on sinkpad, %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
      /* FIXME: parse and store the caps. Once we parsed and built the headers,
       * update the "streamheader" field in the caps and send a new caps event
       */
      ret = gst_pad_push_event (tag->srcpad, event);
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }

  return ret;
}

#define FLAC_MAGIC "fLaC"
#define FLAC_MAGIC_SIZE (sizeof (FLAC_MAGIC) - 1)

static GstFlowReturn
gst_flac_tag_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFlacTag *tag;
  GstFlowReturn ret;
  GstMapInfo map;
  gsize size;

  ret = GST_FLOW_OK;
  tag = GST_FLAC_TAG (parent);

  gst_adapter_push (tag->adapter, buffer);

  GST_LOG_OBJECT (pad, "state: %d", tag->state);

  /* Initial state, we don't even know if we are dealing with a flac file */
  if (tag->state == GST_FLAC_TAG_STATE_INIT) {
    GstBuffer *id_buffer;

    if (gst_adapter_available (tag->adapter) < sizeof (FLAC_MAGIC))
      goto cleanup;

    id_buffer = gst_adapter_take_buffer (tag->adapter, FLAC_MAGIC_SIZE);
    GST_DEBUG_OBJECT (tag, "looking for " FLAC_MAGIC " identifier");
    if (gst_buffer_memcmp (id_buffer, 0, FLAC_MAGIC, FLAC_MAGIC_SIZE) == 0) {

      GST_DEBUG_OBJECT (tag, "pushing " FLAC_MAGIC " identifier buffer");
      ret = gst_pad_push (tag->srcpad, id_buffer);
      if (ret != GST_FLOW_OK)
        goto cleanup;

      tag->state = GST_FLAC_TAG_STATE_METADATA_BLOCKS;
    } else {
      /* FIXME: does that work well with FLAC files containing ID3v2 tags ? */
      gst_buffer_unref (id_buffer);
      GST_ELEMENT_ERROR (tag, STREAM, WRONG_TYPE, (NULL), (NULL));
      ret = GST_FLOW_ERROR;
    }
  }


  /* The fLaC magic string has been skipped, try to detect the beginning
   * of a metadata block
   */
  if (tag->state == GST_FLAC_TAG_STATE_METADATA_BLOCKS) {
    guint type;
    gboolean is_last;
    const guint8 *block_header;

    g_assert (tag->metadata_block_size == 0);
    g_assert (tag->metadata_last_block == FALSE);

    /* The header of a flac metadata block is 4 bytes long:
     * 1st bit: indicates whether this is the last metadata info block
     * 7 next bits: 4 if vorbis comment block
     * 24 next bits: size of the metadata to follow (big endian)
     */
    if (gst_adapter_available (tag->adapter) < 4)
      goto cleanup;

    block_header = gst_adapter_map (tag->adapter, 4);

    is_last = ((block_header[0] & 0x80) == 0x80);
    type = block_header[0] & 0x7F;
    size = (block_header[1] << 16)
        | (block_header[2] << 8)
        | block_header[3];
    gst_adapter_unmap (tag->adapter);

    /* The 4 bytes long header isn't included in the metadata size */
    tag->metadata_block_size = size + 4;
    tag->metadata_last_block = is_last;

    GST_DEBUG_OBJECT (tag,
        "got metadata block: %" G_GSIZE_FORMAT " bytes, type %d, "
        "is vorbiscomment: %d, is last: %d",
        size, type, (type == 0x04), is_last);

    /* Metadata blocks of type 4 are vorbis comment blocks */
    if (type == 0x04) {
      tag->state = GST_FLAC_TAG_STATE_VC_METADATA_BLOCK;
    } else {
      tag->state = GST_FLAC_TAG_STATE_WRITING_METADATA_BLOCK;
    }
  }


  /* Reads a metadata block */
  if ((tag->state == GST_FLAC_TAG_STATE_WRITING_METADATA_BLOCK) ||
      (tag->state == GST_FLAC_TAG_STATE_VC_METADATA_BLOCK)) {
    GstBuffer *metadata_buffer;

    if (gst_adapter_available (tag->adapter) < tag->metadata_block_size)
      goto cleanup;

    metadata_buffer = gst_adapter_take_buffer (tag->adapter,
        tag->metadata_block_size);
    /* clear the is-last flag, as the last metadata block will
     * be the vorbis comment block which we will build ourselves.
     */
    gst_buffer_map (metadata_buffer, &map, GST_MAP_READWRITE);
    map.data[0] &= (~0x80);
    gst_buffer_unmap (metadata_buffer, &map);

    if (tag->state == GST_FLAC_TAG_STATE_WRITING_METADATA_BLOCK) {
      GST_DEBUG_OBJECT (tag, "pushing metadata block buffer");
      ret = gst_pad_push (tag->srcpad, metadata_buffer);
      if (ret != GST_FLOW_OK)
        goto cleanup;
    } else {
      tag->vorbiscomment = metadata_buffer;
    }
    tag->metadata_block_size = 0;
    tag->state = GST_FLAC_TAG_STATE_METADATA_NEXT_BLOCK;
  }

  /* This state is mainly used to be able to stop as soon as we read
   * a vorbiscomment block from the flac file if we are in an only output
   * tags mode
   */
  if (tag->state == GST_FLAC_TAG_STATE_METADATA_NEXT_BLOCK) {
    /* Check if in the previous iteration we read a vorbis comment metadata 
     * block, and stop now if the user only wants to read tags
     */
    if (tag->vorbiscomment != NULL) {
      guint8 id_data[4];
      /* We found some tags, try to parse them and notify the other elements
       * that we encountered some tags
       */
      GST_DEBUG_OBJECT (tag, "emitting vorbiscomment tags");
      gst_buffer_extract (tag->vorbiscomment, 0, id_data, 4);
      tag->tags = gst_tag_list_from_vorbiscomment_buffer (tag->vorbiscomment,
          id_data, 4, NULL);
      if (tag->tags != NULL) {
        gst_pad_push_event (tag->srcpad,
            gst_event_new_tag (gst_tag_list_ref (tag->tags)));
      }

      gst_buffer_unref (tag->vorbiscomment);
      tag->vorbiscomment = NULL;
    }

    /* Skip to next state */
    if (tag->metadata_last_block == FALSE) {
      tag->state = GST_FLAC_TAG_STATE_METADATA_BLOCKS;
    } else {
      tag->state = GST_FLAC_TAG_STATE_ADD_VORBIS_COMMENT;
    }
  }


  /* Creates a vorbis comment block from the metadata which was set
   * on the gstreamer element, and add it to the flac stream
   */
  if (tag->state == GST_FLAC_TAG_STATE_ADD_VORBIS_COMMENT) {
    GstBuffer *buffer;
    const GstTagList *user_tags;
    GstTagList *merged_tags;

    /* merge the tag lists */
    user_tags = gst_tag_setter_get_tag_list (GST_TAG_SETTER (tag));
    if (user_tags != NULL) {
      merged_tags = gst_tag_list_merge (user_tags, tag->tags,
          gst_tag_setter_get_tag_merge_mode (GST_TAG_SETTER (tag)));
    } else {
      merged_tags = gst_tag_list_copy (tag->tags);
    }

    if (merged_tags == NULL) {
      /* If we get a NULL list of tags, we must generate a padding block
       * which is marked as the last metadata block, otherwise we'll
       * end up with a corrupted flac file.
       */
      GST_WARNING_OBJECT (tag, "No tags found");
      buffer = gst_buffer_new_and_alloc (12);
      if (buffer == NULL)
        goto no_buffer;

      gst_buffer_map (buffer, &map, GST_MAP_WRITE);
      memset (map.data, 0, map.size);
      map.data[0] = 0x81;       /* 0x80 = Last metadata block, 
                                 * 0x01 = padding block */
      gst_buffer_unmap (buffer, &map);
    } else {
      guchar header[4];
      guint8 fbit[1];

      memset (header, 0, sizeof (header));
      header[0] = 0x84;         /* 0x80 = Last metadata block, 
                                 * 0x04 = vorbiscomment block */
      buffer = gst_tag_list_to_vorbiscomment_buffer (merged_tags, header,
          sizeof (header), NULL);
      GST_DEBUG_OBJECT (tag, "Writing tags %" GST_PTR_FORMAT, merged_tags);
      gst_tag_list_unref (merged_tags);
      if (buffer == NULL)
        goto no_comment;

      size = gst_buffer_get_size (buffer);
      if ((size < 4) || ((size - 4) > 0xFFFFFF))
        goto comment_too_long;

      fbit[0] = 1;
      /* Get rid of the framing bit at the end of the vorbiscomment buffer 
       * if it exists since libFLAC seems to lose sync because of this
       * bit in gstflacdec
       */
      if (gst_buffer_memcmp (buffer, size - 1, fbit, 1) == 0) {
        buffer = gst_buffer_make_writable (buffer);
        gst_buffer_resize (buffer, 0, size - 1);
      }
    }

    /* The 4 byte metadata block header isn't accounted for in the total
     * size of the metadata block
     */
    gst_buffer_map (buffer, &map, GST_MAP_WRITE);
    map.data[1] = (((map.size - 4) & 0xFF0000) >> 16);
    map.data[2] = (((map.size - 4) & 0x00FF00) >> 8);
    map.data[3] = ((map.size - 4) & 0x0000FF);
    gst_buffer_unmap (buffer, &map);

    GST_DEBUG_OBJECT (tag, "pushing %" G_GSIZE_FORMAT " byte vorbiscomment "
        "buffer", map.size);

    ret = gst_pad_push (tag->srcpad, buffer);
    if (ret != GST_FLOW_OK) {
      goto cleanup;
    }
    tag->state = GST_FLAC_TAG_STATE_AUDIO_DATA;
  }

  /* The metadata blocks have been read, now we are reading audio data */
  if (tag->state == GST_FLAC_TAG_STATE_AUDIO_DATA) {
    GstBuffer *buffer;
    guint avail;

    avail = gst_adapter_available (tag->adapter);
    if (avail > 0) {
      buffer = gst_adapter_take_buffer (tag->adapter, avail);
      ret = gst_pad_push (tag->srcpad, buffer);
    }
  }

cleanup:
  GST_LOG_OBJECT (pad, "state: %d, ret: %d", tag->state, ret);
  return ret;

  /* ERRORS */
no_buffer:
  {
    GST_ELEMENT_ERROR (tag, CORE, TOO_LAZY, (NULL),
        ("Error creating 12-byte buffer for padding block"));
    ret = GST_FLOW_ERROR;
    goto cleanup;
  }
no_comment:
  {
    GST_ELEMENT_ERROR (tag, CORE, TAG, (NULL),
        ("Error converting tag list to vorbiscomment buffer"));
    ret = GST_FLOW_ERROR;
    goto cleanup;
  }
comment_too_long:
  {
    /* FLAC vorbis comment blocks are limited to 2^24 bytes, 
     * while the vorbis specs allow more than that. Shouldn't 
     * be a real world problem though
     */
    GST_ELEMENT_ERROR (tag, CORE, TAG, (NULL),
        ("Vorbis comment of size %" G_GSIZE_FORMAT " too long", size));
    ret = GST_FLOW_ERROR;
    goto cleanup;
  }
}

static GstStateChangeReturn
gst_flac_tag_change_state (GstElement * element, GstStateChange transition)
{
  GstFlacTag *tag;

  tag = GST_FLAC_TAG (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      /* do something to get out of the chain function faster */
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_adapter_clear (tag->adapter);
      if (tag->vorbiscomment) {
        gst_buffer_unref (tag->vorbiscomment);
        tag->vorbiscomment = NULL;
      }
      if (tag->tags) {
        gst_tag_list_unref (tag->tags);
        tag->tags = NULL;
      }
      tag->metadata_block_size = 0;
      tag->metadata_last_block = FALSE;
      tag->state = GST_FLAC_TAG_STATE_INIT;
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
}
