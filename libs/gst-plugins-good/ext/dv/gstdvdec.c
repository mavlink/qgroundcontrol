/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *               <2005> Wim Taymans <wim@fluendo.com>
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
 * SECTION:element-dvdec
 * @title: dvdec
 *
 * dvdec decodes DV video into raw video. The element expects a full DV frame
 * as input, which is 120000 bytes for NTSC and 144000 for PAL video.
 *
 * This element can perform simple frame dropping with the #GstDVDec:drop-factor
 * property. Setting this property to a value N > 1 will only decode every
 * Nth frame.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=test.dv ! dvdemux name=demux ! dvdec ! xvimagesink
 * ]| This pipeline decodes and renders the raw DV stream to a videosink.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <math.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include "gstdvdec.h"

/* sizes of one input buffer */
#define NTSC_HEIGHT 480
#define NTSC_BUFFER 120000
#define NTSC_FRAMERATE_NUMERATOR 30000
#define NTSC_FRAMERATE_DENOMINATOR 1001

#define PAL_HEIGHT 576
#define PAL_BUFFER 144000
#define PAL_FRAMERATE_NUMERATOR 25
#define PAL_FRAMERATE_DENOMINATOR 1

#define PAL_NORMAL_PAR_X        16
#define PAL_NORMAL_PAR_Y        15
#define PAL_WIDE_PAR_X          64
#define PAL_WIDE_PAR_Y          45

#define NTSC_NORMAL_PAR_X       8
#define NTSC_NORMAL_PAR_Y       9
#define NTSC_WIDE_PAR_X         32
#define NTSC_WIDE_PAR_Y         27

#define DV_DEFAULT_QUALITY DV_QUALITY_BEST
#define DV_DEFAULT_DECODE_NTH 1

GST_DEBUG_CATEGORY_STATIC (dvdec_debug);
#define GST_CAT_DEFAULT dvdec_debug

enum
{
  PROP_0,
  PROP_CLAMP_LUMA,
  PROP_CLAMP_CHROMA,
  PROP_QUALITY,
  PROP_DECODE_NTH
};

const gint qualities[] = {
  DV_QUALITY_DC,
  DV_QUALITY_AC_1,
  DV_QUALITY_AC_2,
  DV_QUALITY_DC | DV_QUALITY_COLOR,
  DV_QUALITY_AC_1 | DV_QUALITY_COLOR,
  DV_QUALITY_AC_2 | DV_QUALITY_COLOR
};

static GstStaticPadTemplate sink_temp = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-dv, systemstream = (boolean) false")
    );

static GstStaticPadTemplate src_temp = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) { YUY2, BGRx, RGB }, "
        "framerate = (fraction) [ 1/1, 60/1 ], "
        "width = (int) 720, " "height = (int) { 576, 480 }")
    );

#define GST_TYPE_DVDEC_QUALITY (gst_dvdec_quality_get_type())
static GType
gst_dvdec_quality_get_type (void)
{
  static GType qtype = 0;

  if (qtype == 0) {
    static const GEnumValue values[] = {
      {0, "Monochrome, DC (Fastest)", "fastest"},
      {1, "Monochrome, first AC coefficient", "monochrome-ac"},
      {2, "Monochrome, highest quality", "monochrome-best"},
      {3, "Colour, DC, fastest", "colour-fastest"},
      {4, "Colour, using only the first AC coefficient", "colour-ac"},
      {5, "Highest quality colour decoding", "best"},
      {0, NULL, NULL},
    };

    qtype = g_enum_register_static ("GstDVDecQualityEnum", values);
  }
  return qtype;
}

#define gst_dvdec_parent_class parent_class
G_DEFINE_TYPE (GstDVDec, gst_dvdec, GST_TYPE_ELEMENT);

static GstFlowReturn gst_dvdec_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer);
static gboolean gst_dvdec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event);

static GstStateChangeReturn gst_dvdec_change_state (GstElement * element,
    GstStateChange transition);

static void gst_dvdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dvdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void
gst_dvdec_class_init (GstDVDecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_dvdec_set_property;
  gobject_class->get_property = gst_dvdec_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CLAMP_LUMA,
      g_param_spec_boolean ("clamp-luma", "Clamp luma", "Clamp luma",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CLAMP_CHROMA,
      g_param_spec_boolean ("clamp-chroma", "Clamp chroma", "Clamp chroma",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_QUALITY,
      g_param_spec_enum ("quality", "Quality", "Decoding quality",
          GST_TYPE_DVDEC_QUALITY, DV_DEFAULT_QUALITY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DECODE_NTH,
      g_param_spec_int ("drop-factor", "Drop Factor", "Only decode Nth frame",
          1, G_MAXINT, DV_DEFAULT_DECODE_NTH,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_dvdec_change_state);

  gst_element_class_add_static_pad_template (gstelement_class, &sink_temp);
  gst_element_class_add_static_pad_template (gstelement_class, &src_temp);

  gst_element_class_set_static_metadata (gstelement_class, "DV video decoder",
      "Codec/Decoder/Video",
      "Uses libdv to decode DV video (smpte314) (libdv.sourceforge.net)",
      "Erik Walthinsen <omega@cse.ogi.edu>," "Wim Taymans <wim@fluendo.com>");

  GST_DEBUG_CATEGORY_INIT (dvdec_debug, "dvdec", 0, "DV decoding element");
}

static void
gst_dvdec_init (GstDVDec * dvdec)
{
  dvdec->sinkpad = gst_pad_new_from_static_template (&sink_temp, "sink");
  gst_pad_set_chain_function (dvdec->sinkpad, gst_dvdec_chain);
  gst_pad_set_event_function (dvdec->sinkpad, gst_dvdec_sink_event);
  gst_element_add_pad (GST_ELEMENT (dvdec), dvdec->sinkpad);

  dvdec->srcpad = gst_pad_new_from_static_template (&src_temp, "src");
  gst_pad_use_fixed_caps (dvdec->srcpad);
  gst_element_add_pad (GST_ELEMENT (dvdec), dvdec->srcpad);

  dvdec->framerate_numerator = 0;
  dvdec->framerate_denominator = 0;
  dvdec->wide = FALSE;
  dvdec->drop_factor = 1;

  dvdec->clamp_luma = FALSE;
  dvdec->clamp_chroma = FALSE;
  dvdec->quality = DV_DEFAULT_QUALITY;
}

static gboolean
gst_dvdec_sink_setcaps (GstDVDec * dvdec, GstCaps * caps)
{
  GstStructure *s;
  const GValue *par = NULL, *rate = NULL;

  /* first parse the caps */
  s = gst_caps_get_structure (caps, 0);

  /* we allow framerate and PAR to be overwritten. framerate is mandatory. */
  if (!(rate = gst_structure_get_value (s, "framerate")))
    goto no_framerate;
  par = gst_structure_get_value (s, "pixel-aspect-ratio");

  if (par) {
    dvdec->par_x = gst_value_get_fraction_numerator (par);
    dvdec->par_y = gst_value_get_fraction_denominator (par);
    dvdec->need_par = FALSE;
  } else {
    dvdec->par_x = 0;
    dvdec->par_y = 0;
    dvdec->need_par = TRUE;
  }
  dvdec->framerate_numerator = gst_value_get_fraction_numerator (rate);
  dvdec->framerate_denominator = gst_value_get_fraction_denominator (rate);
  dvdec->sink_negotiated = TRUE;
  dvdec->src_negotiated = FALSE;

  return TRUE;

  /* ERRORS */
no_framerate:
  {
    GST_DEBUG_OBJECT (dvdec, "no framerate specified in caps");
    return FALSE;
  }
}

static void
gst_dvdec_negotiate_pool (GstDVDec * dec, GstCaps * caps, GstVideoInfo * info)
{
  GstQuery *query;
  GstBufferPool *pool;
  guint size, min, max;
  GstStructure *config;

  /* find a pool for the negotiated caps now */
  query = gst_query_new_allocation (caps, TRUE);

  if (!gst_pad_peer_query (dec->srcpad, query)) {
    GST_DEBUG_OBJECT (dec, "didn't get downstream ALLOCATION hints");
  }

  if (gst_query_get_n_allocation_pools (query) > 0) {
    /* we got configuration from our peer, parse them */
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
    size = MAX (size, info->size);
  } else {
    pool = NULL;
    size = info->size;
    min = max = 0;
  }

  if (pool == NULL) {
    /* we did not get a pool, make one ourselves then */
    pool = gst_video_buffer_pool_new ();
  }

  if (dec->pool) {
    gst_buffer_pool_set_active (dec->pool, FALSE);
    gst_object_unref (dec->pool);
  }
  dec->pool = pool;

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_set_params (config, caps, size, min, max);

  if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
    /* just set the option, if the pool can support it we will transparently use
     * it through the video info API. We could also see if the pool support this
     * option and only activate it then. */
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }
  gst_buffer_pool_set_config (pool, config);

  /* and activate */
  gst_buffer_pool_set_active (pool, TRUE);

  gst_query_unref (query);
}

static gboolean
gst_dvdec_src_negotiate (GstDVDec * dvdec)
{
  GstCaps *othercaps;
  gboolean ret;

  /* no PAR was specified in input, derive from encoded data */
  if (dvdec->need_par) {
    if (dvdec->PAL) {
      if (dvdec->wide) {
        dvdec->par_x = PAL_WIDE_PAR_X;
        dvdec->par_y = PAL_WIDE_PAR_Y;
      } else {
        dvdec->par_x = PAL_NORMAL_PAR_X;
        dvdec->par_y = PAL_NORMAL_PAR_Y;
      }
    } else {
      if (dvdec->wide) {
        dvdec->par_x = NTSC_WIDE_PAR_X;
        dvdec->par_y = NTSC_WIDE_PAR_Y;
      } else {
        dvdec->par_x = NTSC_NORMAL_PAR_X;
        dvdec->par_y = NTSC_NORMAL_PAR_Y;
      }
    }
    GST_DEBUG_OBJECT (dvdec, "Inferred PAR %d/%d from video format",
        dvdec->par_x, dvdec->par_y);
  }

  /* ignoring rgb, bgr0 for now */
  dvdec->bpp = 2;

  gst_video_info_set_format (&dvdec->vinfo, GST_VIDEO_FORMAT_YUY2,
      720, dvdec->height);
  dvdec->vinfo.fps_n = dvdec->framerate_numerator;
  dvdec->vinfo.fps_d = dvdec->framerate_denominator;
  dvdec->vinfo.par_n = dvdec->par_x;
  dvdec->vinfo.par_d = dvdec->par_y;
  if (dvdec->interlaced) {
    dvdec->vinfo.interlace_mode = GST_VIDEO_INTERLACE_MODE_INTERLEAVED;
  } else {
    dvdec->vinfo.interlace_mode = GST_VIDEO_INTERLACE_MODE_PROGRESSIVE;
  }

  othercaps = gst_video_info_to_caps (&dvdec->vinfo);
  ret = gst_pad_set_caps (dvdec->srcpad, othercaps);

  gst_dvdec_negotiate_pool (dvdec, othercaps, &dvdec->vinfo);
  gst_caps_unref (othercaps);

  dvdec->src_negotiated = TRUE;

  return ret;
}

static gboolean
gst_dvdec_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstDVDec *dvdec;
  gboolean res = TRUE;

  dvdec = GST_DVDEC (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_segment_init (&dvdec->segment, GST_FORMAT_UNDEFINED);
      dvdec->need_segment = FALSE;
      break;
    case GST_EVENT_SEGMENT:{
      const GstSegment *segment;

      gst_event_parse_segment (event, &segment);

      GST_DEBUG_OBJECT (dvdec, "Got SEGMENT %" GST_SEGMENT_FORMAT, &segment);

      gst_segment_copy_into (segment, &dvdec->segment);
      if (!gst_pad_has_current_caps (dvdec->srcpad)) {
        dvdec->need_segment = TRUE;
        gst_event_unref (event);
        event = NULL;
        res = TRUE;
      } else {
        dvdec->need_segment = FALSE;
      }
      break;
    }
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      res = gst_dvdec_sink_setcaps (dvdec, caps);
      gst_event_unref (event);
      event = NULL;
      break;
    }

    default:
      break;
  }

  if (event)
    res = gst_pad_push_event (dvdec->srcpad, event);

  return res;
}

static GstFlowReturn
gst_dvdec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstDVDec *dvdec;
  guint8 *inframe;
  guint8 *outframe_ptrs[3];
  gint outframe_pitches[3];
  GstMapInfo map;
  GstVideoFrame frame;
  GstBuffer *outbuf;
  GstFlowReturn ret = GST_FLOW_OK;
  guint length;
  guint64 cstart = GST_CLOCK_TIME_NONE, cstop = GST_CLOCK_TIME_NONE;
  gboolean PAL, wide;

  dvdec = GST_DVDEC (parent);

  gst_buffer_map (buf, &map, GST_MAP_READ);
  inframe = map.data;

  /* buffer should be at least the size of one NTSC frame, this should
   * be enough to decode the header. */
  if (G_UNLIKELY (map.size < NTSC_BUFFER))
    goto wrong_size;

  /* preliminary dropping. unref and return if outside of configured segment */
  if ((dvdec->segment.format == GST_FORMAT_TIME) &&
      (!(gst_segment_clip (&dvdec->segment, GST_FORMAT_TIME,
                  GST_BUFFER_TIMESTAMP (buf),
                  GST_BUFFER_TIMESTAMP (buf) + GST_BUFFER_DURATION (buf),
                  &cstart, &cstop))))
    goto dropping;

  if (G_UNLIKELY (dv_parse_header (dvdec->decoder, inframe) < 0))
    goto parse_header_error;

  /* get size */
  PAL = dv_system_50_fields (dvdec->decoder);
  wide = dv_format_wide (dvdec->decoder);

  /* check the buffer is of right size after we know if we are
   * dealing with PAL or NTSC */
  length = (PAL ? PAL_BUFFER : NTSC_BUFFER);
  if (G_UNLIKELY (map.size < length))
    goto wrong_size;

  dv_parse_packs (dvdec->decoder, inframe);

  if (dvdec->video_offset % dvdec->drop_factor != 0)
    goto skip;

  /* renegotiate on change */
  if (PAL != dvdec->PAL || wide != dvdec->wide) {
    dvdec->src_negotiated = FALSE;
    dvdec->PAL = PAL;
    dvdec->wide = wide;
  }

  dvdec->height = (dvdec->PAL ? PAL_HEIGHT : NTSC_HEIGHT);

  dvdec->interlaced = !dv_is_progressive (dvdec->decoder);

  /* negotiate if not done yet */
  if (!dvdec->src_negotiated) {
    if (!gst_dvdec_src_negotiate (dvdec))
      goto not_negotiated;
  }

  if (gst_pad_check_reconfigure (dvdec->srcpad)) {
    GstCaps *caps;

    caps = gst_pad_get_current_caps (dvdec->srcpad);
    if (!caps)
      goto flushing;

    gst_dvdec_negotiate_pool (dvdec, caps, &dvdec->vinfo);
    gst_caps_unref (caps);
  }

  if (dvdec->need_segment) {
    gst_pad_push_event (dvdec->srcpad, gst_event_new_segment (&dvdec->segment));
    dvdec->need_segment = FALSE;
  }

  ret = gst_buffer_pool_acquire_buffer (dvdec->pool, &outbuf, NULL);
  if (G_UNLIKELY (ret != GST_FLOW_OK))
    goto no_buffer;

  gst_video_frame_map (&frame, &dvdec->vinfo, outbuf, GST_MAP_WRITE);

  outframe_ptrs[0] = GST_VIDEO_FRAME_COMP_DATA (&frame, 0);
  outframe_pitches[0] = GST_VIDEO_FRAME_COMP_STRIDE (&frame, 0);

  /* the rest only matters for YUY2 */
  if (dvdec->bpp < 3) {
    outframe_ptrs[1] = GST_VIDEO_FRAME_COMP_DATA (&frame, 1);
    outframe_ptrs[2] = GST_VIDEO_FRAME_COMP_DATA (&frame, 2);

    outframe_pitches[1] = GST_VIDEO_FRAME_COMP_STRIDE (&frame, 1);
    outframe_pitches[2] = GST_VIDEO_FRAME_COMP_STRIDE (&frame, 2);
  }

  GST_DEBUG_OBJECT (dvdec, "decoding and pushing buffer");
  dv_decode_full_frame (dvdec->decoder, inframe,
      e_dv_color_yuv, outframe_ptrs, outframe_pitches);

  gst_video_frame_unmap (&frame);

  GST_BUFFER_FLAG_UNSET (outbuf, GST_VIDEO_BUFFER_FLAG_TFF);

  GST_BUFFER_OFFSET (outbuf) = GST_BUFFER_OFFSET (buf);
  GST_BUFFER_OFFSET_END (outbuf) = GST_BUFFER_OFFSET_END (buf);

  /* FIXME : Compute values when using non-TIME segments,
   * but for the moment make sure we at least don't set bogus values
   */
  if (GST_CLOCK_TIME_IS_VALID (cstart)) {
    GST_BUFFER_TIMESTAMP (outbuf) = cstart;
    if (GST_CLOCK_TIME_IS_VALID (cstop))
      GST_BUFFER_DURATION (outbuf) = cstop - cstart;
  }

  ret = gst_pad_push (dvdec->srcpad, outbuf);

skip:
  dvdec->video_offset++;

done:
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);

  return ret;

  /* ERRORS */
wrong_size:
  {
    GST_ELEMENT_ERROR (dvdec, STREAM, DECODE,
        (NULL), ("Input buffer too small"));
    ret = GST_FLOW_ERROR;
    goto done;
  }
parse_header_error:
  {
    GST_ELEMENT_ERROR (dvdec, STREAM, DECODE,
        (NULL), ("Error parsing DV header"));
    ret = GST_FLOW_ERROR;
    goto done;
  }
not_negotiated:
  {
    GST_DEBUG_OBJECT (dvdec, "could not negotiate output");
    if (GST_PAD_IS_FLUSHING (dvdec->srcpad))
      ret = GST_FLOW_FLUSHING;
    else
      ret = GST_FLOW_NOT_NEGOTIATED;
    goto done;
  }
flushing:
  {
    GST_DEBUG_OBJECT (dvdec, "have no current caps");
    ret = GST_FLOW_FLUSHING;
    goto done;
  }
no_buffer:
  {
    GST_DEBUG_OBJECT (dvdec, "could not allocate buffer");
    goto done;
  }

dropping:
  {
    GST_DEBUG_OBJECT (dvdec,
        "dropping buffer since it's out of the configured segment");
    goto done;
  }
}

static GstStateChangeReturn
gst_dvdec_change_state (GstElement * element, GstStateChange transition)
{
  GstDVDec *dvdec = GST_DVDEC (element);
  GstStateChangeReturn ret;


  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      dvdec->decoder =
          dv_decoder_new (0, dvdec->clamp_luma, dvdec->clamp_chroma);
      dvdec->decoder->quality = qualities[dvdec->quality];
      dv_set_error_log (dvdec->decoder, NULL);
      gst_video_info_init (&dvdec->vinfo);
      gst_segment_init (&dvdec->segment, GST_FORMAT_UNDEFINED);
      dvdec->src_negotiated = FALSE;
      dvdec->sink_negotiated = FALSE;
      dvdec->need_segment = FALSE;
      /* 
       * Enable this function call when libdv2 0.100 or higher is more
       * common
       */
      /* dv_set_quality (dvdec->decoder, qualities [dvdec->quality]); */
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
      dv_decoder_free (dvdec->decoder);
      dvdec->decoder = NULL;
      if (dvdec->pool) {
        gst_buffer_pool_set_active (dvdec->pool, FALSE);
        gst_object_unref (dvdec->pool);
        dvdec->pool = NULL;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

static void
gst_dvdec_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstDVDec *dvdec = GST_DVDEC (object);

  switch (prop_id) {
    case PROP_CLAMP_LUMA:
      dvdec->clamp_luma = g_value_get_boolean (value);
      break;
    case PROP_CLAMP_CHROMA:
      dvdec->clamp_chroma = g_value_get_boolean (value);
      break;
    case PROP_QUALITY:
      dvdec->quality = g_value_get_enum (value);
      if ((dvdec->quality < 0) || (dvdec->quality > 5))
        dvdec->quality = 0;
      break;
    case PROP_DECODE_NTH:
      dvdec->drop_factor = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dvdec_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstDVDec *dvdec = GST_DVDEC (object);

  switch (prop_id) {
    case PROP_CLAMP_LUMA:
      g_value_set_boolean (value, dvdec->clamp_luma);
      break;
    case PROP_CLAMP_CHROMA:
      g_value_set_boolean (value, dvdec->clamp_chroma);
      break;
    case PROP_QUALITY:
      g_value_set_enum (value, dvdec->quality);
      break;
    case PROP_DECODE_NTH:
      g_value_set_int (value, dvdec->drop_factor);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
