/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2012 Collabora Ltd.
 *	Author : Edward Hervey <edward@collabora.com>
 * Copyright (C) 2013 Collabora Ltd.
 *	Author : Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *	         Olivier Crete <olivier.crete@collabora.com>
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
 *
 */
/**
 * SECTION:element-pngdec
 * @title: pngdec
 *
 * Decodes png images. If there is no framerate set on sink caps, it sends EOS
 * after the first picture.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstpngdec.h"

#include <stdlib.h>
#include <string.h>
#include <gst/base/gstbytereader.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>
#include <gst/gst-i18n-plugin.h>

GST_DEBUG_CATEGORY_STATIC (pngdec_debug);
#define GST_CAT_DEFAULT pngdec_debug

static gboolean gst_pngdec_libpng_init (GstPngDec * pngdec);

static GstFlowReturn gst_pngdec_caps_create_and_set (GstPngDec * pngdec);

static gboolean gst_pngdec_start (GstVideoDecoder * decoder);
static gboolean gst_pngdec_stop (GstVideoDecoder * decoder);
static gboolean gst_pngdec_flush (GstVideoDecoder * decoder);
static gboolean gst_pngdec_set_format (GstVideoDecoder * Decoder,
    GstVideoCodecState * state);
static GstFlowReturn gst_pngdec_parse (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame, GstAdapter * adapter, gboolean at_eos);
static GstFlowReturn gst_pngdec_handle_frame (GstVideoDecoder * decoder,
    GstVideoCodecFrame * frame);
static gboolean gst_pngdec_decide_allocation (GstVideoDecoder * decoder,
    GstQuery * query);
static gboolean gst_pngdec_sink_event (GstVideoDecoder * bdec,
    GstEvent * event);

#define parent_class gst_pngdec_parent_class
G_DEFINE_TYPE (GstPngDec, gst_pngdec, GST_TYPE_VIDEO_DECODER);

static GstStaticPadTemplate gst_pngdec_src_pad_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE
        ("{ RGBA, RGB, ARGB64, GRAY8, GRAY16_BE }"))
    );

static GstStaticPadTemplate gst_pngdec_sink_pad_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/png")
    );

static void
gst_pngdec_class_init (GstPngDecClass * klass)
{
  GstElementClass *element_class = (GstElementClass *) klass;
  GstVideoDecoderClass *vdec_class = (GstVideoDecoderClass *) klass;

  gst_element_class_add_static_pad_template (element_class,
      &gst_pngdec_src_pad_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_pngdec_sink_pad_template);
  gst_element_class_set_static_metadata (element_class, "PNG image decoder",
      "Codec/Decoder/Image", "Decode a png video frame to a raw image",
      "Wim Taymans <wim@fluendo.com>");

  vdec_class->start = gst_pngdec_start;
  vdec_class->stop = gst_pngdec_stop;
  vdec_class->flush = gst_pngdec_flush;
  vdec_class->set_format = gst_pngdec_set_format;
  vdec_class->parse = gst_pngdec_parse;
  vdec_class->handle_frame = gst_pngdec_handle_frame;
  vdec_class->decide_allocation = gst_pngdec_decide_allocation;
  vdec_class->sink_event = gst_pngdec_sink_event;

  GST_DEBUG_CATEGORY_INIT (pngdec_debug, "pngdec", 0, "PNG image decoder");
}

static void
gst_pngdec_init (GstPngDec * pngdec)
{
  pngdec->png = NULL;
  pngdec->info = NULL;
  pngdec->endinfo = NULL;

  pngdec->color_type = -1;

  pngdec->image_ready = FALSE;
  pngdec->read_data = 0;

  gst_video_decoder_set_use_default_pad_acceptcaps (GST_VIDEO_DECODER_CAST
      (pngdec), TRUE);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_DECODER_SINK_PAD (pngdec));
}

static void
user_error_fn (png_structp png_ptr, png_const_charp error_msg)
{
  GST_ERROR ("%s", error_msg);
}

static void
user_warning_fn (png_structp png_ptr, png_const_charp warning_msg)
{
  GST_WARNING ("%s", warning_msg);
}

static void
user_info_callback (png_structp png_ptr, png_infop info)
{
  GstPngDec *pngdec = NULL;
  GstFlowReturn ret;

  GST_LOG ("info ready");

  pngdec = GST_PNGDEC (png_get_io_ptr (png_ptr));
  /* Generate the caps and configure */
  ret = gst_pngdec_caps_create_and_set (pngdec);
  if (ret != GST_FLOW_OK) {
    goto beach;
  }

  /* Allocate output buffer */
  ret =
      gst_video_decoder_allocate_output_frame (GST_VIDEO_DECODER (pngdec),
      pngdec->current_frame);
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    GST_DEBUG_OBJECT (pngdec, "failed to acquire buffer");

beach:
  pngdec->ret = ret;
}

static gboolean
gst_pngdec_set_format (GstVideoDecoder * decoder, GstVideoCodecState * state)
{
  GstPngDec *pngdec = (GstPngDec *) decoder;

  if (pngdec->input_state)
    gst_video_codec_state_unref (pngdec->input_state);
  pngdec->input_state = gst_video_codec_state_ref (state);

  /* We'll set format later on */

  return TRUE;
}

static void
user_endrow_callback (png_structp png_ptr, png_bytep new_row,
    png_uint_32 row_num, int pass)
{
  GstPngDec *pngdec = NULL;

  pngdec = GST_PNGDEC (png_get_io_ptr (png_ptr));

  /* If buffer_out doesn't exist, it means buffer_alloc failed, which 
   * will already have set the return code */
  if (new_row && GST_IS_BUFFER (pngdec->current_frame->output_buffer)) {
    GstVideoFrame frame;
    GstBuffer *buffer = pngdec->current_frame->output_buffer;
    size_t offset;
    guint8 *data;

    if (!gst_video_frame_map (&frame, &pngdec->output_state->info, buffer,
            GST_MAP_WRITE)) {
      pngdec->ret = GST_FLOW_ERROR;
      return;
    }

    data = GST_VIDEO_FRAME_COMP_DATA (&frame, 0);
    offset = row_num * GST_VIDEO_FRAME_COMP_STRIDE (&frame, 0);
    GST_LOG ("got row %u at pass %d, copying in buffer %p at offset %"
        G_GSIZE_FORMAT, (guint) row_num, pass,
        pngdec->current_frame->output_buffer, offset);
    png_progressive_combine_row (pngdec->png, data + offset, new_row);
    gst_video_frame_unmap (&frame);
    pngdec->ret = GST_FLOW_OK;
  } else
    pngdec->ret = GST_FLOW_OK;
}

static void
user_end_callback (png_structp png_ptr, png_infop info)
{
  GstPngDec *pngdec = NULL;

  pngdec = GST_PNGDEC (png_get_io_ptr (png_ptr));

  GST_LOG_OBJECT (pngdec, "and we are done reading this image");

  if (!pngdec->current_frame->output_buffer)
    return;

  gst_buffer_unmap (pngdec->current_frame->input_buffer,
      &pngdec->current_frame_map);

  pngdec->ret =
      gst_video_decoder_finish_frame (GST_VIDEO_DECODER (pngdec),
      pngdec->current_frame);

  pngdec->image_ready = TRUE;
}


static GstFlowReturn
gst_pngdec_caps_create_and_set (GstPngDec * pngdec)
{
  GstFlowReturn ret = GST_FLOW_OK;
  gint bpc = 0, color_type;
  png_uint_32 width, height;
  GstVideoFormat format = GST_VIDEO_FORMAT_UNKNOWN;

  g_return_val_if_fail (GST_IS_PNGDEC (pngdec), GST_FLOW_ERROR);

  /* Get bits per channel */
  bpc = png_get_bit_depth (pngdec->png, pngdec->info);

  /* Get Color type */
  color_type = png_get_color_type (pngdec->png, pngdec->info);

  /* Add alpha channel if 16-bit depth, but not for GRAY images */
  if ((bpc > 8) && (color_type != PNG_COLOR_TYPE_GRAY)) {
    png_set_add_alpha (pngdec->png, 0xffff, PNG_FILLER_BEFORE);
    png_set_swap (pngdec->png);
  }
#if 0
  /* We used to have this HACK to reverse the outgoing bytes, but the problem
   * that originally required the hack seems to have been in videoconvert's
   * RGBA descriptions. It doesn't seem needed now that's fixed, but might
   * still be needed on big-endian systems, I'm not sure. J.S. 6/7/2007 */
  if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    png_set_bgr (pngdec->png);
#endif

  /* Gray scale with alpha channel converted to RGB */
  if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    GST_LOG_OBJECT (pngdec,
        "converting grayscale png with alpha channel to RGB");
    png_set_gray_to_rgb (pngdec->png);
  }

  /* Gray scale converted to upscaled to 8 bits */
  if ((color_type == PNG_COLOR_TYPE_GRAY_ALPHA) ||
      (color_type == PNG_COLOR_TYPE_GRAY)) {
    if (bpc < 8) {              /* Convert to 8 bits */
      GST_LOG_OBJECT (pngdec, "converting grayscale image to 8 bits");
#if PNG_LIBPNG_VER < 10400
      png_set_gray_1_2_4_to_8 (pngdec->png);
#else
      png_set_expand_gray_1_2_4_to_8 (pngdec->png);
#endif
    }
  }

  /* Palette converted to RGB */
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    GST_LOG_OBJECT (pngdec, "converting palette png to RGB");
    png_set_palette_to_rgb (pngdec->png);
  }

  png_set_interlace_handling (pngdec->png);

  /* Update the info structure */
  png_read_update_info (pngdec->png, pngdec->info);

  /* Get IHDR header again after transformation settings */
  png_get_IHDR (pngdec->png, pngdec->info, &width, &height,
      &bpc, &pngdec->color_type, NULL, NULL, NULL);

  GST_LOG_OBJECT (pngdec, "this is a %dx%d PNG image", (gint) width,
      (gint) height);

  switch (pngdec->color_type) {
    case PNG_COLOR_TYPE_RGB:
      GST_LOG_OBJECT (pngdec, "we have no alpha channel, depth is 24 bits");
      if (bpc == 8)
        format = GST_VIDEO_FORMAT_RGB;
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
      GST_LOG_OBJECT (pngdec,
          "we have an alpha channel, depth is 32 or 64 bits");
      if (bpc == 8)
        format = GST_VIDEO_FORMAT_RGBA;
      else if (bpc == 16)
        format = GST_VIDEO_FORMAT_ARGB64;
      break;
    case PNG_COLOR_TYPE_GRAY:
      GST_LOG_OBJECT (pngdec,
          "We have an gray image, depth is 8 or 16 (be) bits");
      if (bpc == 8)
        format = GST_VIDEO_FORMAT_GRAY8;
      else if (bpc == 16)
        format = GST_VIDEO_FORMAT_GRAY16_BE;
      break;
    default:
      break;
  }

  if (format == GST_VIDEO_FORMAT_UNKNOWN) {
    GST_ELEMENT_ERROR (pngdec, STREAM, NOT_IMPLEMENTED, (NULL),
        ("pngdec does not support this color type"));
    ret = GST_FLOW_NOT_SUPPORTED;
    goto beach;
  }

  /* Check if output state changed */
  if (pngdec->output_state) {
    GstVideoInfo *info = &pngdec->output_state->info;

    if (width == GST_VIDEO_INFO_WIDTH (info) &&
        height == GST_VIDEO_INFO_HEIGHT (info) &&
        GST_VIDEO_INFO_FORMAT (info) == format) {
      goto beach;
    }
    gst_video_codec_state_unref (pngdec->output_state);
  }
#ifdef HAVE_LIBPNG_1_5
  if ((pngdec->color_type & PNG_COLOR_MASK_COLOR)
      && !(pngdec->color_type & PNG_COLOR_MASK_PALETTE)
      && png_get_valid (pngdec->png, pngdec->info, PNG_INFO_iCCP)) {
    png_charp icc_name;
    png_bytep icc_profile;
    int icc_compression_type;
    png_uint_32 icc_proflen = 0;
    png_uint_32 ret = png_get_iCCP (pngdec->png, pngdec->info, &icc_name,
        &icc_compression_type, &icc_profile, &icc_proflen);

    if ((ret & PNG_INFO_iCCP)) {
      gpointer gst_icc_prof = g_memdup (icc_profile, icc_proflen);
      GstBuffer *tagbuffer = NULL;
      GstSample *tagsample = NULL;
      GstTagList *taglist = NULL;
      GstStructure *info = NULL;
      GstCaps *caps;

      GST_DEBUG_OBJECT (pngdec, "extracted ICC profile '%s' length=%i",
          icc_name, (guint32) icc_proflen);

      tagbuffer = gst_buffer_new_wrapped (gst_icc_prof, icc_proflen);

      caps = gst_caps_new_empty_simple ("application/vnd.iccprofile");
      info = gst_structure_new_empty ("application/vnd.iccprofile");

      if (icc_name)
        gst_structure_set (info, "icc-name", G_TYPE_STRING, icc_name, NULL);

      tagsample = gst_sample_new (tagbuffer, caps, NULL, info);

      gst_buffer_unref (tagbuffer);
      gst_caps_unref (caps);

      taglist = gst_tag_list_new_empty ();
      gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, GST_TAG_ATTACHMENT,
          tagsample, NULL);
      gst_sample_unref (tagsample);

      gst_video_decoder_merge_tags (GST_VIDEO_DECODER (pngdec), taglist,
          GST_TAG_MERGE_APPEND);
      gst_tag_list_unref (taglist);
    }
  }
#endif

  pngdec->output_state =
      gst_video_decoder_set_output_state (GST_VIDEO_DECODER (pngdec), format,
      width, height, pngdec->input_state);
  gst_video_decoder_negotiate (GST_VIDEO_DECODER (pngdec));
  GST_DEBUG ("Final %d %d", GST_VIDEO_INFO_WIDTH (&pngdec->output_state->info),
      GST_VIDEO_INFO_HEIGHT (&pngdec->output_state->info));

beach:
  return ret;
}

static GstFlowReturn
gst_pngdec_handle_frame (GstVideoDecoder * decoder, GstVideoCodecFrame * frame)
{
  GstPngDec *pngdec = (GstPngDec *) decoder;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_LOG_OBJECT (pngdec, "Got buffer, size=%u",
      (guint) gst_buffer_get_size (frame->input_buffer));

  /* Let libpng come back here on error */
  if (setjmp (png_jmpbuf (pngdec->png))) {
    GST_WARNING_OBJECT (pngdec, "error during decoding");
    ret = GST_FLOW_ERROR;
    goto beach;
  }

  pngdec->current_frame = frame;

  /* Progressive loading of the PNG image */
  if (!gst_buffer_map (frame->input_buffer, &pngdec->current_frame_map,
          GST_MAP_READ)) {
    GST_WARNING_OBJECT (pngdec, "Failed to map input buffer");
    ret = GST_FLOW_ERROR;
    goto beach;
  }

  png_process_data (pngdec->png, pngdec->info,
      pngdec->current_frame_map.data, pngdec->current_frame_map.size);

  if (pngdec->image_ready) {
    /* Reset ourselves for the next frame */
    gst_pngdec_flush (decoder);
    GST_LOG_OBJECT (pngdec, "setting up callbacks for next frame");
    png_set_progressive_read_fn (pngdec->png, pngdec,
        user_info_callback, user_endrow_callback, user_end_callback);
    pngdec->image_ready = FALSE;
  } else {
    /* An error happened and we have to unmap */
    gst_buffer_unmap (pngdec->current_frame->input_buffer,
        &pngdec->current_frame_map);
  }

  ret = pngdec->ret;
beach:

  return ret;
}

/* Based on pngparse */
#define PNG_SIGNATURE G_GUINT64_CONSTANT (0x89504E470D0A1A0A)

static GstFlowReturn
gst_pngdec_parse (GstVideoDecoder * decoder, GstVideoCodecFrame * frame,
    GstAdapter * adapter, gboolean at_eos)
{
  gsize toadd = 0;
  GstByteReader reader;
  gconstpointer data;
  guint64 signature;
  gsize size;
  GstPngDec *pngdec = (GstPngDec *) decoder;

  GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);

  /* FIXME : The overhead of using scan_uint32 is massive */

  size = gst_adapter_available (adapter);
  GST_DEBUG ("Parsing PNG image data (%" G_GSIZE_FORMAT " bytes)", size);

  if (size < 8)
    goto need_more_data;

  data = gst_adapter_map (adapter, size);
  gst_byte_reader_init (&reader, data, size);

  if (pngdec->read_data == 0) {
    if (!gst_byte_reader_peek_uint64_be (&reader, &signature))
      goto need_more_data;

    if (signature != PNG_SIGNATURE) {
      for (;;) {
        guint offset;

        offset = gst_byte_reader_masked_scan_uint32 (&reader, 0xffffffff,
            0x89504E47, 0, gst_byte_reader_get_remaining (&reader));

        if (offset == -1) {
          gst_adapter_flush (adapter,
              gst_byte_reader_get_remaining (&reader) - 4);
          goto need_more_data;
        }

        if (!gst_byte_reader_skip (&reader, offset))
          goto need_more_data;

        if (!gst_byte_reader_peek_uint64_be (&reader, &signature))
          goto need_more_data;

        if (signature == PNG_SIGNATURE) {
          /* We're skipping, go out, we'll be back */
          gst_adapter_flush (adapter, gst_byte_reader_get_pos (&reader));
          goto need_more_data;
        }
        if (!gst_byte_reader_skip (&reader, 4))
          goto need_more_data;
      }
    }
    pngdec->read_data = 8;
  }

  if (!gst_byte_reader_skip (&reader, pngdec->read_data))
    goto need_more_data;

  for (;;) {
    guint32 length;
    guint32 code;

    if (!gst_byte_reader_get_uint32_be (&reader, &length))
      goto need_more_data;
    if (!gst_byte_reader_get_uint32_le (&reader, &code))
      goto need_more_data;

    if (!gst_byte_reader_skip (&reader, length + 4))
      goto need_more_data;

    if (code == GST_MAKE_FOURCC ('I', 'E', 'N', 'D')) {
      /* Have complete frame */
      toadd = gst_byte_reader_get_pos (&reader);
      GST_DEBUG_OBJECT (decoder, "Have complete frame of size %" G_GSIZE_FORMAT,
          toadd);
      pngdec->read_data = 0;
      goto have_full_frame;
    } else
      pngdec->read_data += length + 12;
  }

  g_assert_not_reached ();
  return GST_FLOW_ERROR;

need_more_data:
  return GST_VIDEO_DECODER_FLOW_NEED_DATA;

have_full_frame:
  if (toadd)
    gst_video_decoder_add_to_frame (decoder, toadd);
  return gst_video_decoder_have_frame (decoder);
}

static gboolean
gst_pngdec_decide_allocation (GstVideoDecoder * bdec, GstQuery * query)
{
  GstBufferPool *pool = NULL;
  GstStructure *config;

  if (!GST_VIDEO_DECODER_CLASS (parent_class)->decide_allocation (bdec, query))
    return FALSE;

  if (gst_query_get_n_allocation_pools (query) > 0)
    gst_query_parse_nth_allocation_pool (query, 0, &pool, NULL, NULL, NULL);

  if (pool == NULL)
    return FALSE;

  config = gst_buffer_pool_get_config (pool);
  if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }
  gst_buffer_pool_set_config (pool, config);
  gst_object_unref (pool);

  return TRUE;
}

static gboolean
gst_pngdec_sink_event (GstVideoDecoder * bdec, GstEvent * event)
{
  const GstSegment *segment;

  if (GST_EVENT_TYPE (event) != GST_EVENT_SEGMENT)
    goto done;

  gst_event_parse_segment (event, &segment);

  if (segment->format == GST_FORMAT_TIME)
    gst_video_decoder_set_packetized (bdec, TRUE);
  else
    gst_video_decoder_set_packetized (bdec, FALSE);

done:
  return GST_VIDEO_DECODER_CLASS (parent_class)->sink_event (bdec, event);
}

static gboolean
gst_pngdec_libpng_init (GstPngDec * pngdec)
{
  g_return_val_if_fail (GST_IS_PNGDEC (pngdec), FALSE);

  GST_LOG ("init libpng structures");

  /* initialize png struct stuff */
  pngdec->png = png_create_read_struct (PNG_LIBPNG_VER_STRING,
      (png_voidp) NULL, user_error_fn, user_warning_fn);

  if (pngdec->png == NULL)
    goto init_failed;

  pngdec->info = png_create_info_struct (pngdec->png);
  if (pngdec->info == NULL)
    goto info_failed;

  pngdec->endinfo = png_create_info_struct (pngdec->png);
  if (pngdec->endinfo == NULL)
    goto endinfo_failed;

  png_set_progressive_read_fn (pngdec->png, pngdec,
      user_info_callback, user_endrow_callback, user_end_callback);

  return TRUE;

  /* ERRORS */
init_failed:
  {
    GST_ELEMENT_ERROR (pngdec, LIBRARY, INIT, (NULL),
        ("Failed to initialize png structure"));
    return FALSE;
  }
info_failed:
  {
    GST_ELEMENT_ERROR (pngdec, LIBRARY, INIT, (NULL),
        ("Failed to initialize info structure"));
    return FALSE;
  }
endinfo_failed:
  {
    GST_ELEMENT_ERROR (pngdec, LIBRARY, INIT, (NULL),
        ("Failed to initialize endinfo structure"));
    return FALSE;
  }
}


static void
gst_pngdec_libpng_clear (GstPngDec * pngdec)
{
  png_infopp info = NULL, endinfo = NULL;

  GST_LOG ("cleaning up libpng structures");

  if (pngdec->info) {
    info = &pngdec->info;
  }

  if (pngdec->endinfo) {
    endinfo = &pngdec->endinfo;
  }

  if (pngdec->png) {
    png_destroy_read_struct (&(pngdec->png), info, endinfo);
    pngdec->png = NULL;
    pngdec->info = NULL;
    pngdec->endinfo = NULL;
  }

  pngdec->color_type = -1;
  pngdec->read_data = 0;
}

static gboolean
gst_pngdec_start (GstVideoDecoder * decoder)
{
  GstPngDec *pngdec = (GstPngDec *) decoder;

  gst_video_decoder_set_packetized (GST_VIDEO_DECODER (pngdec), FALSE);
  gst_pngdec_libpng_init (pngdec);

  return TRUE;
}

static gboolean
gst_pngdec_stop (GstVideoDecoder * decoder)
{
  GstPngDec *pngdec = (GstPngDec *) decoder;

  gst_pngdec_libpng_clear (pngdec);

  if (pngdec->input_state) {
    gst_video_codec_state_unref (pngdec->input_state);
    pngdec->input_state = NULL;
  }
  if (pngdec->output_state) {
    gst_video_codec_state_unref (pngdec->output_state);
    pngdec->output_state = NULL;
  }

  return TRUE;
}

/* Clean up the libpng structures */
static gboolean
gst_pngdec_flush (GstVideoDecoder * decoder)
{
  gst_pngdec_libpng_clear ((GstPngDec *) decoder);
  gst_pngdec_libpng_init ((GstPngDec *) decoder);

  return TRUE;
}
