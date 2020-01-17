/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *
 * Filter:
 * Copyright (C) 2000 Donald A. Graft
 *
 * Copyright (C) 2012 Collabora Ltd.
 *	Author : Edward Hervey <edward@collabora.com>
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
 * SECTION:element-pngenc
 * @title: pngenc
 *
 * Encodes png images.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <zlib.h>

#include "gstpngenc.h"

GST_DEBUG_CATEGORY_STATIC (pngenc_debug);
#define GST_CAT_DEFAULT pngenc_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

#define DEFAULT_SNAPSHOT                FALSE
#define DEFAULT_COMPRESSION_LEVEL       6

enum
{
  ARG_0,
  ARG_SNAPSHOT,
  ARG_COMPRESSION_LEVEL
};

static GstStaticPadTemplate pngenc_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("image/png, "
        "width = (int) [ 16, 1000000 ], "
        "height = (int) [ 16, 1000000 ], " "framerate = " GST_VIDEO_FPS_RANGE)
    );

static GstStaticPadTemplate pngenc_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ RGBA, RGB, GRAY8, GRAY16_BE }"))
    );

#define parent_class gst_pngenc_parent_class
G_DEFINE_TYPE (GstPngEnc, gst_pngenc, GST_TYPE_VIDEO_ENCODER);

static void gst_pngenc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_pngenc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_pngenc_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);
static gboolean gst_pngenc_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state);
static gboolean gst_pngenc_propose_allocation (GstVideoEncoder * encoder,
    GstQuery * query);

static void gst_pngenc_finalize (GObject * object);

static void
user_error_fn (png_structp png_ptr, png_const_charp error_msg)
{
  g_warning ("%s", error_msg);
}

static void
user_warning_fn (png_structp png_ptr, png_const_charp warning_msg)
{
  g_warning ("%s", warning_msg);
}

static void
gst_pngenc_class_init (GstPngEncClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstVideoEncoderClass *venc_class;

  gobject_class = (GObjectClass *) klass;
  element_class = (GstElementClass *) klass;
  venc_class = (GstVideoEncoderClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = gst_pngenc_get_property;
  gobject_class->set_property = gst_pngenc_set_property;

  g_object_class_install_property (gobject_class, ARG_SNAPSHOT,
      g_param_spec_boolean ("snapshot", "Snapshot",
          "Send EOS after encoding a frame, useful for snapshots",
          DEFAULT_SNAPSHOT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, ARG_COMPRESSION_LEVEL,
      g_param_spec_uint ("compression-level", "compression-level",
          "PNG compression level",
          Z_NO_COMPRESSION, Z_BEST_COMPRESSION,
          DEFAULT_COMPRESSION_LEVEL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template
      (element_class, &pngenc_sink_template);
  gst_element_class_add_static_pad_template
      (element_class, &pngenc_src_template);
  gst_element_class_set_static_metadata (element_class, "PNG image encoder",
      "Codec/Encoder/Image",
      "Encode a video frame to a .png image",
      "Jeremy SIMON <jsimon13@yahoo.fr>");

  venc_class->set_format = gst_pngenc_set_format;
  venc_class->handle_frame = gst_pngenc_handle_frame;
  venc_class->propose_allocation = gst_pngenc_propose_allocation;
  gobject_class->finalize = gst_pngenc_finalize;

  GST_DEBUG_CATEGORY_INIT (pngenc_debug, "pngenc", 0, "PNG image encoder");
}


static gboolean
gst_pngenc_set_format (GstVideoEncoder * encoder, GstVideoCodecState * state)
{
  GstPngEnc *pngenc;
  gboolean ret = TRUE;
  GstVideoInfo *info;
  GstVideoCodecState *output_state;

  pngenc = GST_PNGENC (encoder);
  info = &state->info;

  switch (GST_VIDEO_INFO_FORMAT (info)) {
    case GST_VIDEO_FORMAT_RGBA:
      pngenc->png_color_type = PNG_COLOR_TYPE_RGBA;
      break;
    case GST_VIDEO_FORMAT_RGB:
      pngenc->png_color_type = PNG_COLOR_TYPE_RGB;
      break;
    case GST_VIDEO_FORMAT_GRAY8:
    case GST_VIDEO_FORMAT_GRAY16_BE:
      pngenc->png_color_type = PNG_COLOR_TYPE_GRAY;
      break;
    default:
      ret = FALSE;
      goto done;
  }

  switch (GST_VIDEO_INFO_FORMAT (info)) {
    case GST_VIDEO_FORMAT_GRAY16_BE:
      pngenc->depth = 16;
      break;
    default:                   /* GST_VIDEO_FORMAT_RGB and GST_VIDEO_FORMAT_GRAY8 */
      pngenc->depth = 8;
      break;
  }

  if (pngenc->input_state)
    gst_video_codec_state_unref (pngenc->input_state);
  pngenc->input_state = gst_video_codec_state_ref (state);

  output_state =
      gst_video_encoder_set_output_state (encoder,
      gst_caps_new_empty_simple ("image/png"), state);
  gst_video_codec_state_unref (output_state);

done:

  return ret;
}

static void
gst_pngenc_init (GstPngEnc * pngenc)
{
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_ENCODER_SINK_PAD (pngenc));

  /* init settings */
  pngenc->png_struct_ptr = NULL;
  pngenc->png_info_ptr = NULL;

  pngenc->snapshot = DEFAULT_SNAPSHOT;
  pngenc->compression_level = DEFAULT_COMPRESSION_LEVEL;
}

static void
gst_pngenc_finalize (GObject * object)
{
  GstPngEnc *pngenc = GST_PNGENC (object);

  if (pngenc->input_state)
    gst_video_codec_state_unref (pngenc->input_state);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
user_flush_data (png_structp png_ptr G_GNUC_UNUSED)
{
}

static void
user_write_data (png_structp png_ptr, png_bytep data, png_uint_32 length)
{
  GstPngEnc *pngenc;
  GstMemory *mem;
  GstMapInfo minfo;

  pngenc = (GstPngEnc *) png_get_io_ptr (png_ptr);

  mem = gst_allocator_alloc (NULL, length, NULL);
  if (!mem) {
    GST_ERROR_OBJECT (pngenc, "Failed to allocate memory");
    png_error (png_ptr, "Failed to allocate memory");

    /* never reached */
    return;
  }

  if (!gst_memory_map (mem, &minfo, GST_MAP_WRITE)) {
    GST_ERROR_OBJECT (pngenc, "Failed to map memory");
    gst_memory_unref (mem);

    png_error (png_ptr, "Failed to map memory");

    /* never reached */
    return;
  }

  memcpy (minfo.data, data, length);
  gst_memory_unmap (mem, &minfo);

  gst_buffer_append_memory (pngenc->buffer_out, mem);
}

static GstFlowReturn
gst_pngenc_handle_frame (GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  GstPngEnc *pngenc;
  gint row_index;
  png_byte **row_pointers;
  GstFlowReturn ret = GST_FLOW_OK;
  GstVideoInfo *info;
  GstVideoFrame vframe;

  pngenc = GST_PNGENC (encoder);
  info = &pngenc->input_state->info;

  GST_DEBUG_OBJECT (pngenc, "BEGINNING");

  /* initialize png struct stuff */
  pngenc->png_struct_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
      (png_voidp) NULL, user_error_fn, user_warning_fn);
  if (pngenc->png_struct_ptr == NULL)
    goto struct_init_fail;

  pngenc->png_info_ptr = png_create_info_struct (pngenc->png_struct_ptr);
  if (!pngenc->png_info_ptr)
    goto png_info_fail;

  /* non-0 return is from a longjmp inside of libpng */
  if (setjmp (png_jmpbuf (pngenc->png_struct_ptr)) != 0)
    goto longjmp_fail;

  png_set_filter (pngenc->png_struct_ptr, 0,
      PNG_FILTER_NONE | PNG_FILTER_VALUE_NONE);
  png_set_compression_level (pngenc->png_struct_ptr, pngenc->compression_level);

  png_set_IHDR (pngenc->png_struct_ptr,
      pngenc->png_info_ptr,
      GST_VIDEO_INFO_WIDTH (info),
      GST_VIDEO_INFO_HEIGHT (info),
      pngenc->depth,
      pngenc->png_color_type,
      PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_set_write_fn (pngenc->png_struct_ptr, pngenc,
      (png_rw_ptr) user_write_data, user_flush_data);

  if (!gst_video_frame_map (&vframe, &pngenc->input_state->info,
          frame->input_buffer, GST_MAP_READ)) {
    GST_ELEMENT_ERROR (pngenc, STREAM, FORMAT, (NULL),
        ("Failed to map video frame, caps problem?"));
    ret = GST_FLOW_ERROR;
    goto done;
  }

  row_pointers = g_new (png_byte *, GST_VIDEO_INFO_HEIGHT (info));

  for (row_index = 0; row_index < GST_VIDEO_INFO_HEIGHT (info); row_index++) {
    row_pointers[row_index] = GST_VIDEO_FRAME_COMP_DATA (&vframe, 0) +
        (row_index * GST_VIDEO_FRAME_COMP_STRIDE (&vframe, 0));
  }

  /* allocate the output buffer */
  pngenc->buffer_out = gst_buffer_new ();

  png_write_info (pngenc->png_struct_ptr, pngenc->png_info_ptr);
  png_write_image (pngenc->png_struct_ptr, row_pointers);
  png_write_end (pngenc->png_struct_ptr, NULL);

  g_free (row_pointers);
  gst_video_frame_unmap (&vframe);

  png_destroy_info_struct (pngenc->png_struct_ptr, &pngenc->png_info_ptr);
  png_destroy_write_struct (&pngenc->png_struct_ptr, (png_infopp) NULL);

  /* Set final size and store */
  frame->output_buffer = pngenc->buffer_out;

  pngenc->buffer_out = NULL;

  if ((ret = gst_video_encoder_finish_frame (encoder, frame)) != GST_FLOW_OK)
    goto done;

  if (pngenc->snapshot)
    ret = GST_FLOW_EOS;

done:
  GST_DEBUG_OBJECT (pngenc, "END, ret:%d", ret);

  return ret;

  /* ERRORS */
struct_init_fail:
  {
    GST_ELEMENT_ERROR (pngenc, LIBRARY, INIT, (NULL),
        ("Failed to initialize png structure"));
    return GST_FLOW_ERROR;
  }

png_info_fail:
  {
    png_destroy_write_struct (&(pngenc->png_struct_ptr), (png_infopp) NULL);
    GST_ELEMENT_ERROR (pngenc, LIBRARY, INIT, (NULL),
        ("Failed to initialize the png info structure"));
    return GST_FLOW_ERROR;
  }

longjmp_fail:
  {
    png_destroy_write_struct (&pngenc->png_struct_ptr, &pngenc->png_info_ptr);
    GST_ELEMENT_ERROR (pngenc, LIBRARY, FAILED, (NULL),
        ("returning from longjmp"));
    return GST_FLOW_ERROR;
  }
}

static gboolean
gst_pngenc_propose_allocation (GstVideoEncoder * encoder, GstQuery * query)
{
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  return GST_VIDEO_ENCODER_CLASS (parent_class)->propose_allocation (encoder,
      query);
}

static void
gst_pngenc_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstPngEnc *pngenc;

  pngenc = GST_PNGENC (object);

  switch (prop_id) {
    case ARG_SNAPSHOT:
      g_value_set_boolean (value, pngenc->snapshot);
      break;
    case ARG_COMPRESSION_LEVEL:
      g_value_set_uint (value, pngenc->compression_level);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_pngenc_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstPngEnc *pngenc;

  pngenc = GST_PNGENC (object);

  switch (prop_id) {
    case ARG_SNAPSHOT:
      pngenc->snapshot = g_value_get_boolean (value);
      break;
    case ARG_COMPRESSION_LEVEL:
      pngenc->compression_level = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
