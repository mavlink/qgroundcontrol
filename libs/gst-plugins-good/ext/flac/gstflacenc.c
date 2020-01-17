/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
 * SECTION:element-flacenc
 * @title: flacenc
 * @see_also: #GstFlacDec
 *
 * flacenc encodes FLAC streams.
 * [FLAC](http://flac.sourceforge.net/) is a Free Lossless Audio Codec.
 * FLAC audio can directly be written into a file, or embedded into containers
 * such as oggmux or matroskamux.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 audiotestsrc num-buffers=100 ! flacenc ! filesink location=beep.flac
 * ]| Encode a short sine wave into FLAC
 * |[
 * gst-launch-1.0 cdparanoiasrc mode=continuous ! queue ! audioconvert ! flacenc ! filesink location=cd.flac
 * ]| Rip a whole audio CD into a single FLAC file, with the track table saved as a CUE sheet inside the FLAC file
 * |[
 * gst-launch-1.0 cdparanoiasrc track=5 ! queue ! audioconvert ! flacenc ! filesink location=track5.flac
 * ]| Rip track 5 of an audio CD and encode it losslessly to a FLAC file
 *
 */

/* TODO: - We currently don't handle discontinuities in the stream in a useful
 *         way and instead rely on the developer plugging in audiorate if
 *         the stream contains discontinuities.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>

#include <gstflacenc.h>
#include <gst/audio/audio.h>
#include <gst/tag/tag.h>
#include <gst/gsttagsetter.h>

/* Taken from http://flac.sourceforge.net/format.html#frame_header */
static const GstAudioChannelPosition channel_positions[8][8] = {
  {GST_AUDIO_CHANNEL_POSITION_MONO},
  {GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
      GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
      GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
      GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
      GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
      GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT},
  /* FIXME: 7/8 channel layouts are not defined in the FLAC specs */
  {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
      GST_AUDIO_CHANNEL_POSITION_REAR_CENTER}, {
        GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_REAR_LEFT,
        GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT,
        GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER,
        GST_AUDIO_CHANNEL_POSITION_LFE1,
        GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT,
      GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT}
};

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-flac")
    );

enum
{
  PROP_0,
  PROP_QUALITY,
  PROP_STREAMABLE_SUBSET,
  PROP_MID_SIDE_STEREO,
  PROP_LOOSE_MID_SIDE_STEREO,
  PROP_BLOCKSIZE,
  PROP_MAX_LPC_ORDER,
  PROP_QLP_COEFF_PRECISION,
  PROP_QLP_COEFF_PREC_SEARCH,
  PROP_ESCAPE_CODING,
  PROP_EXHAUSTIVE_MODEL_SEARCH,
  PROP_MIN_RESIDUAL_PARTITION_ORDER,
  PROP_MAX_RESIDUAL_PARTITION_ORDER,
  PROP_RICE_PARAMETER_SEARCH_DIST,
  PROP_PADDING,
  PROP_SEEKPOINTS
};

GST_DEBUG_CATEGORY_STATIC (flacenc_debug);
#define GST_CAT_DEFAULT flacenc_debug

#define gst_flac_enc_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstFlacEnc, gst_flac_enc, GST_TYPE_AUDIO_ENCODER,
    G_IMPLEMENT_INTERFACE (GST_TYPE_TAG_SETTER, NULL)
    G_IMPLEMENT_INTERFACE (GST_TYPE_TOC_SETTER, NULL)
    );

static gboolean gst_flac_enc_start (GstAudioEncoder * enc);
static gboolean gst_flac_enc_stop (GstAudioEncoder * enc);
static gboolean gst_flac_enc_set_format (GstAudioEncoder * enc,
    GstAudioInfo * info);
static GstFlowReturn gst_flac_enc_handle_frame (GstAudioEncoder * enc,
    GstBuffer * in_buf);
static GstCaps *gst_flac_enc_getcaps (GstAudioEncoder * enc, GstCaps * filter);
static gboolean gst_flac_enc_sink_event (GstAudioEncoder * enc,
    GstEvent * event);
static gboolean gst_flac_enc_sink_query (GstAudioEncoder * enc,
    GstQuery * query);

static void gst_flac_enc_finalize (GObject * object);

static GstCaps *gst_flac_enc_generate_sink_caps (void);

static gboolean gst_flac_enc_update_quality (GstFlacEnc * flacenc,
    gint quality);
static void gst_flac_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_flac_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static FLAC__StreamEncoderWriteStatus
gst_flac_enc_write_callback (const FLAC__StreamEncoder * encoder,
    const FLAC__byte buffer[], size_t bytes,
    unsigned samples, unsigned current_frame, void *client_data);
static FLAC__StreamEncoderSeekStatus
gst_flac_enc_seek_callback (const FLAC__StreamEncoder * encoder,
    FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamEncoderTellStatus
gst_flac_enc_tell_callback (const FLAC__StreamEncoder * encoder,
    FLAC__uint64 * absolute_byte_offset, void *client_data);

typedef struct
{
  gboolean exhaustive_model_search;
  gboolean escape_coding;
  gboolean mid_side;
  gboolean loose_mid_side;
  guint qlp_coeff_precision;
  gboolean qlp_coeff_prec_search;
  guint min_residual_partition_order;
  guint max_residual_partition_order;
  guint rice_parameter_search_dist;
  guint max_lpc_order;
  guint blocksize;
}
GstFlacEncParams;

static const GstFlacEncParams flacenc_params[] = {
  {FALSE, FALSE, FALSE, FALSE, 0, FALSE, 2, 2, 0, 0, 1152},
  {FALSE, FALSE, TRUE, TRUE, 0, FALSE, 2, 2, 0, 0, 1152},
  {FALSE, FALSE, TRUE, FALSE, 0, FALSE, 0, 3, 0, 0, 1152},
  {FALSE, FALSE, FALSE, FALSE, 0, FALSE, 3, 3, 0, 6, 4608},
  {FALSE, FALSE, TRUE, TRUE, 0, FALSE, 3, 3, 0, 8, 4608},
  {FALSE, FALSE, TRUE, FALSE, 0, FALSE, 3, 3, 0, 8, 4608},
  {FALSE, FALSE, TRUE, FALSE, 0, FALSE, 0, 4, 0, 8, 4608},
  {TRUE, FALSE, TRUE, FALSE, 0, FALSE, 0, 6, 0, 8, 4608},
  {TRUE, FALSE, TRUE, FALSE, 0, FALSE, 0, 6, 0, 12, 4608},
  {TRUE, TRUE, TRUE, FALSE, 0, FALSE, 0, 16, 0, 32, 4608},
};

#define DEFAULT_QUALITY 5
#define DEFAULT_PADDING 0
#define DEFAULT_SEEKPOINTS -10

#define GST_TYPE_FLAC_ENC_QUALITY (gst_flac_enc_quality_get_type ())
static GType
gst_flac_enc_quality_get_type (void)
{
  static GType qtype = 0;

  if (qtype == 0) {
    static const GEnumValue values[] = {
      {0, "0 - Fastest compression", "0"},
      {1, "1", "1"},
      {2, "2", "2"},
      {3, "3", "3"},
      {4, "4", "4"},
      {5, "5 - Default", "5"},
      {6, "6", "6"},
      {7, "7", "7"},
      {8, "8 - Highest compression", "8"},
      {9, "9 - Insane", "9"},
      {0, NULL, NULL}
    };

    qtype = g_enum_register_static ("GstFlacEncQuality", values);
  }
  return qtype;
}

static void
gst_flac_enc_class_init (GstFlacEncClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstAudioEncoderClass *base_class;
  GstCaps *sink_caps;
  GstPadTemplate *sink_templ;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  base_class = (GstAudioEncoderClass *) (klass);

  GST_DEBUG_CATEGORY_INIT (flacenc_debug, "flacenc", 0,
      "Flac encoding element");

  gobject_class->set_property = gst_flac_enc_set_property;
  gobject_class->get_property = gst_flac_enc_get_property;
  gobject_class->finalize = gst_flac_enc_finalize;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_QUALITY,
      g_param_spec_enum ("quality",
          "Quality",
          "Speed versus compression tradeoff",
          GST_TYPE_FLAC_ENC_QUALITY, DEFAULT_QUALITY,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_STREAMABLE_SUBSET, g_param_spec_boolean ("streamable-subset",
          "Streamable subset",
          "true to limit encoder to generating a Subset stream, else false",
          TRUE,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_MID_SIDE_STEREO,
      g_param_spec_boolean ("mid-side-stereo", "Do mid side stereo",
          "Do mid side stereo (only for stereo input)",
          flacenc_params[DEFAULT_QUALITY].mid_side,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_LOOSE_MID_SIDE_STEREO, g_param_spec_boolean ("loose-mid-side-stereo",
          "Loose mid side stereo", "Loose mid side stereo",
          flacenc_params[DEFAULT_QUALITY].loose_mid_side,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BLOCKSIZE,
      g_param_spec_uint ("blocksize", "Blocksize", "Blocksize in samples",
          FLAC__MIN_BLOCK_SIZE, FLAC__MAX_BLOCK_SIZE,
          flacenc_params[DEFAULT_QUALITY].blocksize,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_MAX_LPC_ORDER,
      g_param_spec_uint ("max-lpc-order", "Max LPC order",
          "Max LPC order; 0 => use only fixed predictors", 0,
          FLAC__MAX_LPC_ORDER, flacenc_params[DEFAULT_QUALITY].max_lpc_order,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_QLP_COEFF_PRECISION, g_param_spec_uint ("qlp-coeff-precision",
          "QLP coefficients precision",
          "Precision in bits of quantized linear-predictor coefficients; 0 = automatic",
          0, 32, flacenc_params[DEFAULT_QUALITY].qlp_coeff_precision,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_QLP_COEFF_PREC_SEARCH, g_param_spec_boolean ("qlp-coeff-prec-search",
          "Do QLP coefficients precision search",
          "false = use qlp_coeff_precision, "
          "true = search around qlp_coeff_precision, take best",
          flacenc_params[DEFAULT_QUALITY].qlp_coeff_prec_search,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ESCAPE_CODING,
      g_param_spec_boolean ("escape-coding", "Do Escape coding",
          "search for escape codes in the entropy coding stage "
          "for slightly better compression",
          flacenc_params[DEFAULT_QUALITY].escape_coding,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_EXHAUSTIVE_MODEL_SEARCH,
      g_param_spec_boolean ("exhaustive-model-search",
          "Do exhaustive model search",
          "do exhaustive search of LP coefficient quantization (expensive!)",
          flacenc_params[DEFAULT_QUALITY].exhaustive_model_search,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_MIN_RESIDUAL_PARTITION_ORDER,
      g_param_spec_uint ("min-residual-partition-order",
          "Min residual partition order",
          "Min residual partition order (above 4 doesn't usually help much)", 0,
          16, flacenc_params[DEFAULT_QUALITY].min_residual_partition_order,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_MAX_RESIDUAL_PARTITION_ORDER,
      g_param_spec_uint ("max-residual-partition-order",
          "Max residual partition order",
          "Max residual partition order (above 4 doesn't usually help much)", 0,
          16, flacenc_params[DEFAULT_QUALITY].max_residual_partition_order,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_RICE_PARAMETER_SEARCH_DIST,
      g_param_spec_uint ("rice-parameter-search-dist",
          "rice_parameter_search_dist",
          "0 = try only calc'd parameter k; else try all [k-dist..k+dist] "
          "parameters, use best", 0, FLAC__MAX_RICE_PARTITION_ORDER,
          flacenc_params[DEFAULT_QUALITY].rice_parameter_search_dist,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_PADDING,
      g_param_spec_uint ("padding",
          "Padding",
          "Write a PADDING block with this length in bytes", 0, G_MAXUINT,
          DEFAULT_PADDING,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_SEEKPOINTS,
      g_param_spec_int ("seekpoints",
          "Seekpoints",
          "Add SEEKTABLE metadata (if > 0, number of entries, if < 0, interval in sec)",
          -G_MAXINT, G_MAXINT,
          DEFAULT_SEEKPOINTS,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);

  sink_caps = gst_flac_enc_generate_sink_caps ();
  sink_templ = gst_pad_template_new ("sink",
      GST_PAD_SINK, GST_PAD_ALWAYS, sink_caps);
  gst_element_class_add_pad_template (gstelement_class, sink_templ);
  gst_caps_unref (sink_caps);

  gst_element_class_set_static_metadata (gstelement_class, "FLAC audio encoder",
      "Codec/Encoder/Audio",
      "Encodes audio with the FLAC lossless audio encoder",
      "Wim Taymans <wim.taymans@chello.be>");

  base_class->start = GST_DEBUG_FUNCPTR (gst_flac_enc_start);
  base_class->stop = GST_DEBUG_FUNCPTR (gst_flac_enc_stop);
  base_class->set_format = GST_DEBUG_FUNCPTR (gst_flac_enc_set_format);
  base_class->handle_frame = GST_DEBUG_FUNCPTR (gst_flac_enc_handle_frame);
  base_class->getcaps = GST_DEBUG_FUNCPTR (gst_flac_enc_getcaps);
  base_class->sink_event = GST_DEBUG_FUNCPTR (gst_flac_enc_sink_event);
  base_class->sink_query = GST_DEBUG_FUNCPTR (gst_flac_enc_sink_query);
}

static void
gst_flac_enc_init (GstFlacEnc * flacenc)
{
  GstAudioEncoder *enc = GST_AUDIO_ENCODER (flacenc);

  flacenc->encoder = FLAC__stream_encoder_new ();
  gst_flac_enc_update_quality (flacenc, DEFAULT_QUALITY);

  /* arrange granulepos marking (and required perfect ts) */
  gst_audio_encoder_set_mark_granule (enc, TRUE);
  gst_audio_encoder_set_perfect_timestamp (enc, TRUE);
}

static void
gst_flac_enc_finalize (GObject * object)
{
  GstFlacEnc *flacenc = GST_FLAC_ENC (object);

  FLAC__stream_encoder_delete (flacenc->encoder);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_flac_enc_start (GstAudioEncoder * enc)
{
  GstFlacEnc *flacenc = GST_FLAC_ENC (enc);

  GST_DEBUG_OBJECT (enc, "start");
  flacenc->stopped = TRUE;
  flacenc->got_headers = FALSE;
  flacenc->last_flow = GST_FLOW_OK;
  flacenc->offset = 0;
  flacenc->eos = FALSE;
  flacenc->tags = gst_tag_list_new_empty ();
  flacenc->toc = NULL;
  flacenc->samples_in = 0;
  flacenc->samples_out = 0;

  return TRUE;
}

static gboolean
gst_flac_enc_stop (GstAudioEncoder * enc)
{
  GstFlacEnc *flacenc = GST_FLAC_ENC (enc);

  GST_DEBUG_OBJECT (enc, "stop");
  gst_tag_list_unref (flacenc->tags);
  flacenc->tags = NULL;
  if (flacenc->toc)
    gst_toc_unref (flacenc->toc);
  flacenc->toc = NULL;
  if (FLAC__stream_encoder_get_state (flacenc->encoder) !=
      FLAC__STREAM_ENCODER_UNINITIALIZED) {
    flacenc->stopped = TRUE;
    FLAC__stream_encoder_finish (flacenc->encoder);
  }
  if (flacenc->meta) {
    FLAC__metadata_object_delete (flacenc->meta[0]);

    if (flacenc->meta[1])
      FLAC__metadata_object_delete (flacenc->meta[1]);

    if (flacenc->meta[2])
      FLAC__metadata_object_delete (flacenc->meta[2]);

    if (flacenc->meta[3])
      FLAC__metadata_object_delete (flacenc->meta[3]);

    g_free (flacenc->meta);
    flacenc->meta = NULL;
  }
  g_list_foreach (flacenc->headers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (flacenc->headers);
  flacenc->headers = NULL;

  gst_tag_setter_reset_tags (GST_TAG_SETTER (enc));
  gst_toc_setter_reset (GST_TOC_SETTER (enc));

  return TRUE;
}

static void
add_one_tag (const GstTagList * list, const gchar * tag, gpointer user_data)
{
  GList *comments;
  GList *it;
  GstFlacEnc *flacenc = GST_FLAC_ENC (user_data);

  /* IMAGE and PREVIEW_IMAGE tags are already written
   * differently, no need to store them inside the
   * vorbiscomments too */
  if (strcmp (tag, GST_TAG_IMAGE) == 0
      || strcmp (tag, GST_TAG_PREVIEW_IMAGE) == 0)
    return;

  comments = gst_tag_to_vorbis_comments (list, tag);
  for (it = comments; it != NULL; it = it->next) {
    FLAC__StreamMetadata_VorbisComment_Entry commment_entry;

    commment_entry.length = strlen (it->data);
    commment_entry.entry = it->data;
    FLAC__metadata_object_vorbiscomment_insert_comment (flacenc->meta[0],
        flacenc->meta[0]->data.vorbis_comment.num_comments,
        commment_entry, TRUE);
    g_free (it->data);
  }
  g_list_free (comments);
}

static gboolean
add_cuesheet (const GstToc * toc, guint sample_rate,
    FLAC__StreamMetadata * cuesheet)
{
  gint8 track_num = 0;
  gint64 start, stop;
  gchar *isrc = NULL;
  const gchar *is_legal;
  GList *list;
  GstTagList *tags;
  GstTocEntry *entry, *subentry = NULL;
  FLAC__StreamMetadata_CueSheet *cs;
  FLAC__StreamMetadata_CueSheet_Track *track;

  cs = &cuesheet->data.cue_sheet;
  if (!cs)
    return FALSE;

  /* check if the TOC entries is valid */
  list = gst_toc_get_entries (toc);
  entry = list->data;
  if (gst_toc_entry_is_alternative (entry)) {
    list = gst_toc_entry_get_sub_entries (entry);
    while (list) {
      subentry = list->data;
      if (!gst_toc_entry_is_sequence (subentry))
        return FALSE;
      list = g_list_next (list);
    }
    list = gst_toc_entry_get_sub_entries (entry);
  }
  if (gst_toc_entry_is_sequence (entry)) {
    while (list) {
      entry = list->data;
      if (!gst_toc_entry_is_sequence (entry))
        return FALSE;
      list = g_list_next (list);
    }
    list = gst_toc_get_entries (toc);
  }

  /* add tracks in cuesheet */
  while (list) {
    entry = list->data;
    gst_toc_entry_get_start_stop_times (entry, &start, &stop);
    tags = gst_toc_entry_get_tags (entry);
    if (tags)
      gst_tag_list_get_string (tags, GST_TAG_ISRC, &isrc);
    track = FLAC__metadata_object_cuesheet_track_new ();
    track->offset =
        (FLAC__uint64) gst_util_uint64_scale_round (start, sample_rate,
        GST_SECOND);
    track->number = (FLAC__byte) track_num + 1;
    if (isrc != NULL && strlen (isrc) <= 12)
      g_strlcpy (track->isrc, isrc, 13);
    if (track->number <= 0)
      return FALSE;
    if (!FLAC__metadata_object_cuesheet_insert_track (cuesheet, track_num,
            track, FALSE))
      return FALSE;
    if (!FLAC__metadata_object_cuesheet_track_insert_blank_index (cuesheet,
            track_num, 0))
      return FALSE;
    track_num++;
    list = g_list_next (list);
  }

  if (cs->num_tracks <= 0)
    return FALSE;

  /* add lead-out track in cuesheet */
  track = FLAC__metadata_object_cuesheet_track_new ();
  track->offset =
      (FLAC__uint64) gst_util_uint64_scale_round (stop, sample_rate,
      GST_SECOND);
  track->number = 255;
  if (!FLAC__metadata_object_cuesheet_insert_track (cuesheet, cs->num_tracks,
          track, FALSE))
    return FALSE;

  /* check if the cuesheet is valid */
  if (!FLAC__metadata_object_cuesheet_is_legal (cuesheet, FALSE, &is_legal)) {
    g_warning ("%s\n", is_legal);
    return FALSE;
  }

  return TRUE;
}

static void
gst_flac_enc_set_metadata (GstFlacEnc * flacenc, GstAudioInfo * info,
    guint64 total_samples)
{
  const GstTagList *user_tags;
  GstTagList *copy;
  gint entries = 1;
  gint n_images, n_preview_images;
  FLAC__StreamMetadata *cuesheet;

  g_return_if_fail (flacenc != NULL);

  user_tags = gst_tag_setter_get_tag_list (GST_TAG_SETTER (flacenc));
  if ((flacenc->tags == NULL) && (user_tags == NULL)) {
    return;
  }
  copy = gst_tag_list_merge (user_tags, flacenc->tags,
      gst_tag_setter_get_tag_merge_mode (GST_TAG_SETTER (flacenc)));
  n_images = gst_tag_list_get_tag_size (copy, GST_TAG_IMAGE);
  n_preview_images = gst_tag_list_get_tag_size (copy, GST_TAG_PREVIEW_IMAGE);

  flacenc->meta =
      g_new0 (FLAC__StreamMetadata *, 4 + n_images + n_preview_images);

  flacenc->meta[0] =
      FLAC__metadata_object_new (FLAC__METADATA_TYPE_VORBIS_COMMENT);
  gst_tag_list_foreach (copy, add_one_tag, flacenc);

  if (!flacenc->toc)
    flacenc->toc = gst_toc_setter_get_toc (GST_TOC_SETTER (flacenc));

  if (flacenc->toc) {
    cuesheet = FLAC__metadata_object_new (FLAC__METADATA_TYPE_CUESHEET);
    if (add_cuesheet (flacenc->toc, GST_AUDIO_INFO_RATE (info), cuesheet)) {
      flacenc->meta[entries] = cuesheet;
      entries++;
    } else {
      FLAC__metadata_object_delete (cuesheet);
      flacenc->meta[entries] = NULL;
    }
  }

  if (n_images + n_preview_images > 0) {
    GstSample *sample;
    GstBuffer *buffer;
    GstCaps *caps;
    const GstStructure *structure;
    GstTagImageType image_type = GST_TAG_IMAGE_TYPE_NONE;
    gint i, width = 0, height = 0, png_icon_count = 0, other_icon_count = 0;
    GstMapInfo map;

    for (i = 0; i < n_images + n_preview_images; i++) {
      gboolean is_preview_image = (i >= n_images);

      if (i < n_images) {
        if (!gst_tag_list_get_sample_index (copy, GST_TAG_IMAGE, i, &sample))
          continue;
      } else {
        if (!gst_tag_list_get_sample_index (copy, GST_TAG_PREVIEW_IMAGE,
                i - n_images, &sample))
          continue;
      }

      structure = gst_sample_get_info (sample);
      caps = gst_sample_get_caps (sample);
      if (!caps) {
        GST_FIXME_OBJECT (flacenc, "Image tag without caps");
        gst_sample_unref (sample);
        continue;
      }

      flacenc->meta[entries] =
          FLAC__metadata_object_new (FLAC__METADATA_TYPE_PICTURE);

      GST_LOG_OBJECT (flacenc, "image info: %" GST_PTR_FORMAT, structure);

      if (structure)
        gst_structure_get (structure, "image-type", GST_TYPE_TAG_IMAGE_TYPE,
            &image_type, NULL);
      else
        image_type = GST_TAG_IMAGE_TYPE_NONE;

      GST_LOG_OBJECT (flacenc, "image caps: %" GST_PTR_FORMAT, caps);

      structure = gst_caps_get_structure (caps, 0);
      gst_structure_get (structure, "width", G_TYPE_INT, &width,
          "height", G_TYPE_INT, &height, NULL);

      /* Convert to ID3v2 APIC image type */
      if (image_type == GST_TAG_IMAGE_TYPE_NONE) {
        if (is_preview_image) {
          /* 1 - 32x32 pixels 'file icon' (PNG only)
           * 2 - Other file icon
           * There may only be one each of picture type 1 and 2 in a file. */
          if (width == 32 && height == 32
              && gst_structure_has_name (structure, "image/png")
              && png_icon_count++ == 0) {
            image_type = 1;
          } else if (width <= 32 && height <= 32 && other_icon_count++ == 0) {
            image_type = 2;
          } else {
            image_type = 0;     /* Other */
          }
        } else {
          image_type = 0;       /* Other */
        }
      } else {
        /* GStreamer enum is the same but without the two icon types 1+2 */
        image_type = image_type + 2;
      }

      buffer = gst_sample_get_buffer (sample);
      gst_buffer_map (buffer, &map, GST_MAP_READ);
      FLAC__metadata_object_picture_set_data (flacenc->meta[entries],
          map.data, map.size, TRUE);
      gst_buffer_unmap (buffer, &map);

      GST_LOG_OBJECT (flacenc, "Setting picture type %d", image_type);
      flacenc->meta[entries]->data.picture.type = image_type;

      if (width > 0 && height > 0) {
        flacenc->meta[entries]->data.picture.width = width;
        flacenc->meta[entries]->data.picture.height = height;
      }

      FLAC__metadata_object_picture_set_mime_type (flacenc->meta[entries],
          (char *) gst_structure_get_name (structure), TRUE);

      gst_sample_unref (sample);
      entries++;
    }
  }

  if (flacenc->seekpoints && total_samples != GST_CLOCK_TIME_NONE) {
    gboolean res;
    guint samples;

    flacenc->meta[entries] =
        FLAC__metadata_object_new (FLAC__METADATA_TYPE_SEEKTABLE);
    if (flacenc->seekpoints > 0) {
      res =
          FLAC__metadata_object_seektable_template_append_spaced_points
          (flacenc->meta[entries], flacenc->seekpoints, total_samples);
    } else {
      samples = -flacenc->seekpoints * GST_AUDIO_INFO_RATE (info);
      res =
          FLAC__metadata_object_seektable_template_append_spaced_points_by_samples
          (flacenc->meta[entries], samples, total_samples);
    }
    if (!res) {
      GST_DEBUG_OBJECT (flacenc, "adding seekpoint template %d failed",
          flacenc->seekpoints);
      FLAC__metadata_object_delete (flacenc->meta[1]);
      flacenc->meta[entries] = NULL;
    } else {
      entries++;
    }
  } else if (flacenc->seekpoints && total_samples == GST_CLOCK_TIME_NONE) {
    GST_WARNING_OBJECT (flacenc, "total time unknown; can not add seekpoints");
  }

  if (flacenc->padding > 0) {
    flacenc->meta[entries] =
        FLAC__metadata_object_new (FLAC__METADATA_TYPE_PADDING);
    flacenc->meta[entries]->length = flacenc->padding;
    entries++;
  }

  if (FLAC__stream_encoder_set_metadata (flacenc->encoder,
          flacenc->meta, entries) != true)
    g_warning ("Dude, i'm already initialized!");

  gst_tag_list_unref (copy);
}

static GstCaps *
gst_flac_enc_generate_sink_caps (void)
{
  GstCaps *ret;
  gint i;
  GValue v_list = { 0, };
  GValue v = { 0, };
  GstStructure *s, *s2;

  g_value_init (&v_list, GST_TYPE_LIST);
  g_value_init (&v, G_TYPE_STRING);

  /* Use system's endianness */
  g_value_set_static_string (&v, "S8");
  gst_value_list_append_value (&v_list, &v);
  g_value_set_static_string (&v, GST_AUDIO_NE (S16));
  gst_value_list_append_value (&v_list, &v);
  g_value_set_static_string (&v, GST_AUDIO_NE (S24));
  gst_value_list_append_value (&v_list, &v);
  g_value_set_static_string (&v, GST_AUDIO_NE (S24_32));
  gst_value_list_append_value (&v_list, &v);
  g_value_unset (&v);

  s = gst_structure_new_empty ("audio/x-raw");
  gst_structure_take_value (s, "format", &v_list);

  gst_structure_set (s, "layout", G_TYPE_STRING, "interleaved",
      "rate", GST_TYPE_INT_RANGE, 1, 655350, NULL);

  ret = gst_caps_new_empty ();
  s2 = gst_structure_copy (s);
  gst_structure_set (s2, "channels", G_TYPE_INT, 1, NULL);
  gst_caps_append_structure (ret, s2);
  for (i = 2; i <= 8; i++) {
    guint64 channel_mask;

    s2 = gst_structure_copy (s);
    gst_audio_channel_positions_to_mask (channel_positions[i - 1], i,
        FALSE, &channel_mask);
    gst_structure_set (s2, "channels", G_TYPE_INT, i, "channel-mask",
        GST_TYPE_BITMASK, channel_mask, NULL);

    gst_caps_append_structure (ret, s2);
  }
  gst_structure_free (s);

  return ret;
}

static GstCaps *
gst_flac_enc_getcaps (GstAudioEncoder * enc, GstCaps * filter)
{
  GstCaps *ret = NULL, *caps = NULL;
  GstPad *pad;

  pad = GST_AUDIO_ENCODER_SINK_PAD (enc);

  ret = gst_pad_get_current_caps (pad);
  if (ret == NULL) {
    ret = gst_pad_get_pad_template_caps (pad);
  }

  GST_DEBUG_OBJECT (pad, "Return caps %" GST_PTR_FORMAT, ret);

  caps = gst_audio_encoder_proxy_getcaps (enc, ret, filter);
  gst_caps_unref (ret);

  return caps;
}

static guint64
gst_flac_enc_peer_query_total_samples (GstFlacEnc * flacenc, GstPad * pad)
{
  gint64 duration;
  GstAudioInfo *info =
      gst_audio_encoder_get_audio_info (GST_AUDIO_ENCODER (flacenc));

  GST_DEBUG_OBJECT (flacenc, "querying peer for DEFAULT format duration");
  if (gst_pad_peer_query_duration (pad, GST_FORMAT_DEFAULT, &duration)
      && duration != GST_CLOCK_TIME_NONE)
    goto done;

  GST_DEBUG_OBJECT (flacenc, "querying peer for TIME format duration");

  if (gst_pad_peer_query_duration (pad, GST_FORMAT_TIME, &duration)
      && duration != GST_CLOCK_TIME_NONE) {
    GST_DEBUG_OBJECT (flacenc, "peer reported duration %" GST_TIME_FORMAT,
        GST_TIME_ARGS (duration));
    duration = GST_CLOCK_TIME_TO_FRAMES (duration, GST_AUDIO_INFO_RATE (info));

    goto done;
  }

  GST_DEBUG_OBJECT (flacenc, "Upstream reported no total samples");
  return GST_CLOCK_TIME_NONE;

done:
  GST_DEBUG_OBJECT (flacenc,
      "Upstream reported %" G_GUINT64_FORMAT " total samples", duration);

  return duration;
}

static gint64
gst_flac_enc_get_latency (GstFlacEnc * flacenc)
{
  /* The FLAC specification states that the data is processed in blocks,
   * regardless of the number of channels. Thus, The latency can be calculated
   * using the blocksize and rate. For example a 1 second block sampled at
   * 44.1KHz has a blocksize of 44100 */

  /* Get the blocksize */
  const guint blocksize = FLAC__stream_encoder_get_blocksize (flacenc->encoder);

  /* Get the sample rate in KHz */
  const guint rate = FLAC__stream_encoder_get_sample_rate (flacenc->encoder);
  if (!rate)
    return 0;

  /* Calculate the latecy */
  return (blocksize * GST_SECOND) / rate;
}

static gboolean
gst_flac_enc_set_format (GstAudioEncoder * enc, GstAudioInfo * info)
{
  GstFlacEnc *flacenc;
  guint64 total_samples = GST_CLOCK_TIME_NONE;
  FLAC__StreamEncoderInitStatus init_status;

  flacenc = GST_FLAC_ENC (enc);

  /* if configured again, means something changed, can't handle that */
  if (FLAC__stream_encoder_get_state (flacenc->encoder) !=
      FLAC__STREAM_ENCODER_UNINITIALIZED)
    goto encoder_already_initialized;

  /* delay setting output caps/format until we have all headers */

  gst_audio_get_channel_reorder_map (GST_AUDIO_INFO_CHANNELS (info),
      channel_positions[GST_AUDIO_INFO_CHANNELS (info) - 1], info->position,
      flacenc->channel_reorder_map);

  total_samples = gst_flac_enc_peer_query_total_samples (flacenc,
      GST_AUDIO_ENCODER_SINK_PAD (enc));

  FLAC__stream_encoder_set_bits_per_sample (flacenc->encoder,
      GST_AUDIO_INFO_DEPTH (info));
  FLAC__stream_encoder_set_sample_rate (flacenc->encoder,
      GST_AUDIO_INFO_RATE (info));
  FLAC__stream_encoder_set_channels (flacenc->encoder,
      GST_AUDIO_INFO_CHANNELS (info));

  if (total_samples != GST_CLOCK_TIME_NONE)
    FLAC__stream_encoder_set_total_samples_estimate (flacenc->encoder,
        MIN (total_samples, G_GUINT64_CONSTANT (0x0FFFFFFFFF)));

  gst_flac_enc_set_metadata (flacenc, info, total_samples);

  /* callbacks clear to go now;
   * write callbacks receives headers during init */
  flacenc->stopped = FALSE;

  init_status = FLAC__stream_encoder_init_stream (flacenc->encoder,
      gst_flac_enc_write_callback, gst_flac_enc_seek_callback,
      gst_flac_enc_tell_callback, NULL, flacenc);
  if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    goto failed_to_initialize;

  /* feedback to base class */
  gst_audio_encoder_set_latency (enc,
      gst_flac_enc_get_latency (flacenc), gst_flac_enc_get_latency (flacenc));

  return TRUE;

encoder_already_initialized:
  {
    g_warning ("flac already initialized -- fixme allow this");
    return FALSE;
  }
failed_to_initialize:
  {
    GST_ELEMENT_ERROR (flacenc, LIBRARY, INIT, (NULL),
        ("could not initialize encoder (wrong parameters?) %d", init_status));
    return FALSE;
  }
}

static gboolean
gst_flac_enc_update_quality (GstFlacEnc * flacenc, gint quality)
{
  GstAudioInfo *info =
      gst_audio_encoder_get_audio_info (GST_AUDIO_ENCODER (flacenc));

  flacenc->quality = quality;

#define DO_UPDATE(name, val, str)                                               \
  G_STMT_START {                                                                \
    if (FLAC__stream_encoder_get_##name (flacenc->encoder) !=                   \
        flacenc_params[quality].val) {                                          \
      FLAC__stream_encoder_set_##name (flacenc->encoder,                        \
          flacenc_params[quality].val);                                         \
      g_object_notify (G_OBJECT (flacenc), str);                                \
    }                                                                           \
  } G_STMT_END

  g_object_freeze_notify (G_OBJECT (flacenc));

  if (GST_AUDIO_INFO_CHANNELS (info) == 2
      || GST_AUDIO_INFO_CHANNELS (info) == 0) {
    DO_UPDATE (do_mid_side_stereo, mid_side, "mid_side_stereo");
    DO_UPDATE (loose_mid_side_stereo, loose_mid_side, "loose_mid_side");
  }

  DO_UPDATE (blocksize, blocksize, "blocksize");
  DO_UPDATE (max_lpc_order, max_lpc_order, "max_lpc_order");
  DO_UPDATE (qlp_coeff_precision, qlp_coeff_precision, "qlp_coeff_precision");
  DO_UPDATE (do_qlp_coeff_prec_search, qlp_coeff_prec_search,
      "qlp_coeff_prec_search");
  DO_UPDATE (do_escape_coding, escape_coding, "escape_coding");
  DO_UPDATE (do_exhaustive_model_search, exhaustive_model_search,
      "exhaustive_model_search");
  DO_UPDATE (min_residual_partition_order, min_residual_partition_order,
      "min_residual_partition_order");
  DO_UPDATE (max_residual_partition_order, max_residual_partition_order,
      "max_residual_partition_order");
  DO_UPDATE (rice_parameter_search_dist, rice_parameter_search_dist,
      "rice_parameter_search_dist");

#undef DO_UPDATE

  g_object_thaw_notify (G_OBJECT (flacenc));

  return TRUE;
}

static FLAC__StreamEncoderSeekStatus
gst_flac_enc_seek_callback (const FLAC__StreamEncoder * encoder,
    FLAC__uint64 absolute_byte_offset, void *client_data)
{
  GstFlacEnc *flacenc;
  GstPad *peerpad;
  GstSegment seg;

  flacenc = GST_FLAC_ENC (client_data);

  if (flacenc->stopped)
    return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;

  if ((peerpad = gst_pad_get_peer (GST_AUDIO_ENCODER_SRC_PAD (flacenc)))) {
    GstEvent *event;
    gboolean ret;
    GstQuery *query;
    gboolean seekable = FALSE;

    /* try to seek to the beginning of the output */
    query = gst_query_new_seeking (GST_FORMAT_BYTES);
    if (gst_pad_query (peerpad, query)) {
      GstFormat format;

      gst_query_parse_seeking (query, &format, &seekable, NULL, NULL);
      if (format != GST_FORMAT_BYTES)
        seekable = FALSE;
    } else {
      GST_LOG_OBJECT (flacenc, "SEEKING query not handled");
    }
    gst_query_unref (query);

    if (!seekable) {
      GST_DEBUG_OBJECT (flacenc, "downstream not seekable; not rewriting");
      gst_object_unref (peerpad);
      return FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;
    }

    gst_segment_init (&seg, GST_FORMAT_BYTES);
    seg.start = absolute_byte_offset;
    seg.stop = GST_BUFFER_OFFSET_NONE;
    seg.time = 0;
    event = gst_event_new_segment (&seg);

    ret = gst_pad_send_event (peerpad, event);
    gst_object_unref (peerpad);

    if (ret) {
      GST_DEBUG ("Seek to %" G_GUINT64_FORMAT " %s",
          (guint64) absolute_byte_offset, "succeeded");
    } else {
      GST_DEBUG ("Seek to %" G_GUINT64_FORMAT " %s",
          (guint64) absolute_byte_offset, "failed");
      return FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED;
    }
  } else {
    GST_DEBUG ("Seek to %" G_GUINT64_FORMAT " failed (no peer pad)",
        (guint64) absolute_byte_offset);
  }

  flacenc->offset = absolute_byte_offset;
  return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

static void
notgst_value_array_append_buffer (GValue * array_val, GstBuffer * buf)
{
  GValue value = { 0, };

  g_value_init (&value, GST_TYPE_BUFFER);
  /* copy buffer to avoid problems with circular refcounts */
  buf = gst_buffer_copy (buf);
  /* again, for good measure */
  GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_HEADER);
  gst_value_set_buffer (&value, buf);
  gst_buffer_unref (buf);
  gst_value_array_append_value (array_val, &value);
  g_value_unset (&value);
}

#define HDR_TYPE_STREAMINFO     0
#define HDR_TYPE_VORBISCOMMENT  4

static GstFlowReturn
gst_flac_enc_process_stream_headers (GstFlacEnc * enc)
{
  GstBuffer *vorbiscomment = NULL;
  GstBuffer *streaminfo = NULL;
  GstBuffer *marker = NULL;
  GValue array = { 0, };
  GstCaps *caps;
  GList *l;
  GstFlowReturn ret = GST_FLOW_OK;
  GstAudioInfo *info =
      gst_audio_encoder_get_audio_info (GST_AUDIO_ENCODER (enc));

  caps = gst_caps_new_simple ("audio/x-flac",
      "channels", G_TYPE_INT, GST_AUDIO_INFO_CHANNELS (info),
      "rate", G_TYPE_INT, GST_AUDIO_INFO_RATE (info), NULL);

  for (l = enc->headers; l != NULL; l = l->next) {
    GstBuffer *buf;
    GstMapInfo map;
    guint8 *data;
    gsize size;

    /* mark buffers so oggmux will ignore them if it already muxed the
     * header buffers from the streamheaders field in the caps */
    l->data = gst_buffer_make_writable (GST_BUFFER_CAST (l->data));

    buf = GST_BUFFER_CAST (l->data);
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_HEADER);

    gst_buffer_map (buf, &map, GST_MAP_READ);
    data = map.data;
    size = map.size;

    /* find initial 4-byte marker which we need to skip later on */
    if (size == 4 && memcmp (data, "fLaC", 4) == 0) {
      marker = buf;
    } else if (size > 1 && (data[0] & 0x7f) == HDR_TYPE_STREAMINFO) {
      streaminfo = buf;
    } else if (size > 1 && (data[0] & 0x7f) == HDR_TYPE_VORBISCOMMENT) {
      vorbiscomment = buf;
    }

    gst_buffer_unmap (buf, &map);
  }

  if (marker == NULL || streaminfo == NULL || vorbiscomment == NULL) {
    GST_WARNING_OBJECT (enc, "missing header %p %p %p, muxing into container "
        "formats may be broken", marker, streaminfo, vorbiscomment);
    goto push_headers;
  }

  g_value_init (&array, GST_TYPE_ARRAY);

  /* add marker including STREAMINFO header */
  {
    GstBuffer *buf;
    guint16 num;
    GstMapInfo map;
    guint8 *bdata;
    gsize slen;

    /* minus one for the marker that is merged with streaminfo here */
    num = g_list_length (enc->headers) - 1;

    slen = gst_buffer_get_size (streaminfo);
    buf = gst_buffer_new_and_alloc (13 + slen);

    gst_buffer_map (buf, &map, GST_MAP_WRITE);
    bdata = map.data;
    bdata[0] = 0x7f;
    memcpy (bdata + 1, "FLAC", 4);
    bdata[5] = 0x01;            /* mapping version major */
    bdata[6] = 0x00;            /* mapping version minor */
    bdata[7] = (num & 0xFF00) >> 8;
    bdata[8] = (num & 0x00FF) >> 0;
    memcpy (bdata + 9, "fLaC", 4);
    gst_buffer_extract (streaminfo, 0, bdata + 13, slen);
    gst_buffer_unmap (buf, &map);

    notgst_value_array_append_buffer (&array, buf);
    gst_buffer_unref (buf);
  }

  /* add VORBISCOMMENT header */
  notgst_value_array_append_buffer (&array, vorbiscomment);

  /* add other headers, if there are any */
  for (l = enc->headers; l != NULL; l = l->next) {
    GstBuffer *buf = GST_BUFFER_CAST (l->data);

    if (buf != marker && buf != streaminfo && buf != vorbiscomment) {
      notgst_value_array_append_buffer (&array, buf);
    }
  }

  gst_structure_set_value (gst_caps_get_structure (caps, 0),
      "streamheader", &array);
  g_value_unset (&array);

push_headers:
  gst_audio_encoder_set_output_format (GST_AUDIO_ENCODER (enc), caps);

  gst_audio_encoder_set_headers (GST_AUDIO_ENCODER (enc), enc->headers);
  enc->headers = NULL;

  gst_caps_unref (caps);

  return ret;
}

static FLAC__StreamEncoderWriteStatus
gst_flac_enc_write_callback (const FLAC__StreamEncoder * encoder,
    const FLAC__byte buffer[], size_t bytes,
    unsigned samples, unsigned current_frame, void *client_data)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstFlacEnc *flacenc;
  GstBuffer *outbuf;
  GstSegment *segment;
  GstClockTime duration;

  flacenc = GST_FLAC_ENC (client_data);

  if (flacenc->stopped)
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;

  outbuf = gst_buffer_new_and_alloc (bytes);
  gst_buffer_fill (outbuf, 0, buffer, bytes);

  /* we assume libflac passes us stuff neatly framed */
  if (!flacenc->got_headers) {
    if (samples == 0) {
      GST_DEBUG_OBJECT (flacenc, "Got header, queueing (%u bytes)",
          (guint) bytes);
      flacenc->headers = g_list_append (flacenc->headers, outbuf);
      /* note: it's important that we increase our byte offset */
      goto out;
    } else {
      GST_INFO_OBJECT (flacenc, "Non-header packet, we have all headers now");
      ret = gst_flac_enc_process_stream_headers (flacenc);
      flacenc->got_headers = TRUE;
    }
  }

  if (flacenc->got_headers && samples == 0) {
    /* header fixup, push downstream directly */
    GST_DEBUG_OBJECT (flacenc, "Fixing up headers at pos=%" G_GUINT64_FORMAT
        ", size=%u", flacenc->offset, (guint) bytes);
#if 0
    GST_MEMDUMP_OBJECT (flacenc, "Presumed header fragment",
        GST_BUFFER_DATA (outbuf), GST_BUFFER_SIZE (outbuf));
#endif
    ret = gst_pad_push (GST_AUDIO_ENCODER_SRC_PAD (flacenc), outbuf);
  } else {
    /* regular frame data, pass to base class */
    if (flacenc->eos && flacenc->samples_in == flacenc->samples_out + samples) {
      /* If encoding part of a frame, and we have no set stop time on
       * the output segment, we update the segment stop time to reflect
       * the last sample. This will let oggmux set the last page's
       * granpos to tell a decoder the dummy samples should be clipped.
       */
      segment = &GST_AUDIO_ENCODER_OUTPUT_SEGMENT (flacenc);
      if (!GST_CLOCK_TIME_IS_VALID (segment->stop)) {
        GST_DEBUG_OBJECT (flacenc,
            "No stop time and partial frame, updating segment");
        duration =
            gst_util_uint64_scale (flacenc->samples_out + samples,
            GST_SECOND,
            FLAC__stream_encoder_get_sample_rate (flacenc->encoder));
        segment->stop = segment->start + duration;
        GST_DEBUG_OBJECT (flacenc, "new output segment %" GST_SEGMENT_FORMAT,
            segment);
        gst_pad_push_event (GST_AUDIO_ENCODER_SRC_PAD (flacenc),
            gst_event_new_segment (segment));
      }
    }

    GST_LOG ("Pushing buffer: samples=%u, size=%u, pos=%" G_GUINT64_FORMAT,
        samples, (guint) bytes, flacenc->offset);
    ret = gst_audio_encoder_finish_frame (GST_AUDIO_ENCODER (flacenc),
        outbuf, samples);
  }

  if (ret != GST_FLOW_OK)
    GST_DEBUG_OBJECT (flacenc, "flow: %s", gst_flow_get_name (ret));

  flacenc->last_flow = ret;

out:
  flacenc->offset += bytes;

  if (ret != GST_FLOW_OK)
    return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

  return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static FLAC__StreamEncoderTellStatus
gst_flac_enc_tell_callback (const FLAC__StreamEncoder * encoder,
    FLAC__uint64 * absolute_byte_offset, void *client_data)
{
  GstFlacEnc *flacenc = GST_FLAC_ENC (client_data);

  *absolute_byte_offset = flacenc->offset;

  return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}

static gboolean
gst_flac_enc_sink_event (GstAudioEncoder * enc, GstEvent * event)
{
  GstFlacEnc *flacenc;
  GstTagList *taglist;
  GstToc *toc;
  gboolean ret = FALSE;

  flacenc = GST_FLAC_ENC (enc);

  GST_DEBUG ("Received %s event on sinkpad, %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      flacenc->eos = TRUE;
      ret = GST_AUDIO_ENCODER_CLASS (parent_class)->sink_event (enc, event);
      break;
    case GST_EVENT_TAG:
      if (flacenc->tags) {
        gst_event_parse_tag (event, &taglist);
        gst_tag_list_insert (flacenc->tags, taglist,
            gst_tag_setter_get_tag_merge_mode (GST_TAG_SETTER (flacenc)));
      } else {
        g_assert_not_reached ();
      }
      ret = GST_AUDIO_ENCODER_CLASS (parent_class)->sink_event (enc, event);
      break;
    case GST_EVENT_TOC:
      gst_event_parse_toc (event, &toc, NULL);
      if (toc) {
        if (flacenc->toc != toc) {
          if (flacenc->toc)
            gst_toc_unref (flacenc->toc);
          flacenc->toc = toc;
        }
      }
      ret = GST_AUDIO_ENCODER_CLASS (parent_class)->sink_event (enc, event);
      break;
    case GST_EVENT_SEGMENT:
      flacenc->samples_in = 0;
      flacenc->samples_out = 0;
      ret = GST_AUDIO_ENCODER_CLASS (parent_class)->sink_event (enc, event);
      break;
    default:
      ret = GST_AUDIO_ENCODER_CLASS (parent_class)->sink_event (enc, event);
      break;
  }

  return ret;
}

static gboolean
gst_flac_enc_sink_query (GstAudioEncoder * enc, GstQuery * query)
{
  GstPad *pad = GST_AUDIO_ENCODER_SINK_PAD (enc);
  gboolean ret = FALSE;

  GST_DEBUG ("Received %s query on sinkpad, %" GST_PTR_FORMAT,
      GST_QUERY_TYPE_NAME (query), query);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ACCEPT_CAPS:{
      GstCaps *acceptable, *caps;

      acceptable = gst_pad_get_current_caps (pad);
      if (acceptable == NULL) {
        acceptable = gst_pad_get_pad_template_caps (pad);
      }

      gst_query_parse_accept_caps (query, &caps);

      gst_query_set_accept_caps_result (query,
          gst_caps_is_subset (caps, acceptable));
      gst_caps_unref (acceptable);
      ret = TRUE;
    }
      break;
    default:
      ret = GST_AUDIO_ENCODER_CLASS (parent_class)->sink_query (enc, query);
      break;
  }

  return ret;
}

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define READ_INT24 GST_READ_UINT24_LE
#else
#define READ_INT24 GST_READ_UINT24_BE
#endif

static GstFlowReturn
gst_flac_enc_handle_frame (GstAudioEncoder * enc, GstBuffer * buffer)
{
  GstFlacEnc *flacenc;
  FLAC__int32 *data;
  gint samples, width, channels;
  gulong i;
  gint j;
  FLAC__bool res;
  GstMapInfo map;
  GstAudioInfo *info =
      gst_audio_encoder_get_audio_info (GST_AUDIO_ENCODER (enc));
  gint *reorder_map;

  flacenc = GST_FLAC_ENC (enc);

  /* base class ensures configuration */
  g_return_val_if_fail (GST_AUDIO_INFO_WIDTH (info) != 0,
      GST_FLOW_NOT_NEGOTIATED);

  width = GST_AUDIO_INFO_WIDTH (info);
  channels = GST_AUDIO_INFO_CHANNELS (info);
  reorder_map = flacenc->channel_reorder_map;

  if (G_UNLIKELY (!buffer)) {
    if (flacenc->eos) {
      GST_DEBUG_OBJECT (flacenc, "finish encoding");
      FLAC__stream_encoder_finish (flacenc->encoder);
    } else {
      /* can't handle intermittent draining/resyncing */
      GST_ELEMENT_WARNING (flacenc, STREAM, FORMAT, (NULL),
          ("Stream discontinuity detected. "
              "The output may have wrong timestamps, "
              "consider using audiorate to handle discontinuities"));
    }
    return flacenc->last_flow;
  }

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  samples = map.size / (width >> 3);

  data = g_malloc (samples * sizeof (FLAC__int32));

  samples /= channels;
  GST_LOG_OBJECT (flacenc, "processing %d samples, %d channels", samples,
      channels);
  if (width == 8) {
    gint8 *indata = (gint8 *) map.data;

    for (i = 0; i < samples; i++)
      for (j = 0; j < channels; j++)
        data[i * channels + reorder_map[j]] =
            (FLAC__int32) indata[i * channels + j];
  } else if (width == 16) {
    gint16 *indata = (gint16 *) map.data;

    for (i = 0; i < samples; i++)
      for (j = 0; j < channels; j++)
        data[i * channels + reorder_map[j]] =
            (FLAC__int32) indata[i * channels + j];
  } else if (width == 24) {
    guint8 *indata = (guint8 *) map.data;
    guint32 val;

    for (i = 0; i < samples; i++)
      for (j = 0; j < channels; j++) {
        val = READ_INT24 (&indata[3 * (i * channels + j)]);
        if (val & 0x00800000)
          val |= 0xff000000;
        data[i * channels + reorder_map[j]] = (FLAC__int32) val;
      }
  } else if (width == 32) {
    gint32 *indata = (gint32 *) map.data;

    for (i = 0; i < samples; i++)
      for (j = 0; j < channels; j++)
        data[i * channels + reorder_map[j]] =
            (FLAC__int32) indata[i * channels + j];
  } else {
    g_assert_not_reached ();
  }
  gst_buffer_unmap (buffer, &map);

  res = FLAC__stream_encoder_process_interleaved (flacenc->encoder,
      (const FLAC__int32 *) data, samples);
  flacenc->samples_in += samples;

  g_free (data);

  if (!res) {
    if (flacenc->last_flow == GST_FLOW_OK)
      return GST_FLOW_ERROR;
    else
      return flacenc->last_flow;
  }

  return GST_FLOW_OK;
}

static void
gst_flac_enc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstFlacEnc *this = GST_FLAC_ENC (object);
  GstAudioEncoder *enc = GST_AUDIO_ENCODER (object);
  guint64 curr_latency = 0, old_latency = gst_flac_enc_get_latency (this);

  GST_OBJECT_LOCK (this);

  switch (prop_id) {
    case PROP_QUALITY:
      gst_flac_enc_update_quality (this, g_value_get_enum (value));
      break;
    case PROP_STREAMABLE_SUBSET:
      FLAC__stream_encoder_set_streamable_subset (this->encoder,
          g_value_get_boolean (value));
      break;
    case PROP_MID_SIDE_STEREO:
      FLAC__stream_encoder_set_do_mid_side_stereo (this->encoder,
          g_value_get_boolean (value));
      break;
    case PROP_LOOSE_MID_SIDE_STEREO:
      FLAC__stream_encoder_set_loose_mid_side_stereo (this->encoder,
          g_value_get_boolean (value));
      break;
    case PROP_BLOCKSIZE:
      FLAC__stream_encoder_set_blocksize (this->encoder,
          g_value_get_uint (value));
      break;
    case PROP_MAX_LPC_ORDER:
      FLAC__stream_encoder_set_max_lpc_order (this->encoder,
          g_value_get_uint (value));
      break;
    case PROP_QLP_COEFF_PRECISION:
      FLAC__stream_encoder_set_qlp_coeff_precision (this->encoder,
          g_value_get_uint (value));
      break;
    case PROP_QLP_COEFF_PREC_SEARCH:
      FLAC__stream_encoder_set_do_qlp_coeff_prec_search (this->encoder,
          g_value_get_boolean (value));
      break;
    case PROP_ESCAPE_CODING:
      FLAC__stream_encoder_set_do_escape_coding (this->encoder,
          g_value_get_boolean (value));
      break;
    case PROP_EXHAUSTIVE_MODEL_SEARCH:
      FLAC__stream_encoder_set_do_exhaustive_model_search (this->encoder,
          g_value_get_boolean (value));
      break;
    case PROP_MIN_RESIDUAL_PARTITION_ORDER:
      FLAC__stream_encoder_set_min_residual_partition_order (this->encoder,
          g_value_get_uint (value));
      break;
    case PROP_MAX_RESIDUAL_PARTITION_ORDER:
      FLAC__stream_encoder_set_max_residual_partition_order (this->encoder,
          g_value_get_uint (value));
      break;
    case PROP_RICE_PARAMETER_SEARCH_DIST:
      FLAC__stream_encoder_set_rice_parameter_search_dist (this->encoder,
          g_value_get_uint (value));
      break;
    case PROP_PADDING:
      this->padding = g_value_get_uint (value);
      break;
    case PROP_SEEKPOINTS:
      this->seekpoints = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (this);

  /* Update latency if it has changed */
  curr_latency = gst_flac_enc_get_latency (this);
  if (old_latency != curr_latency)
    gst_audio_encoder_set_latency (enc, curr_latency, curr_latency);
}

static void
gst_flac_enc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstFlacEnc *this = GST_FLAC_ENC (object);

  GST_OBJECT_LOCK (this);

  switch (prop_id) {
    case PROP_QUALITY:
      g_value_set_enum (value, this->quality);
      break;
    case PROP_STREAMABLE_SUBSET:
      g_value_set_boolean (value,
          FLAC__stream_encoder_get_streamable_subset (this->encoder));
      break;
    case PROP_MID_SIDE_STEREO:
      g_value_set_boolean (value,
          FLAC__stream_encoder_get_do_mid_side_stereo (this->encoder));
      break;
    case PROP_LOOSE_MID_SIDE_STEREO:
      g_value_set_boolean (value,
          FLAC__stream_encoder_get_loose_mid_side_stereo (this->encoder));
      break;
    case PROP_BLOCKSIZE:
      g_value_set_uint (value,
          FLAC__stream_encoder_get_blocksize (this->encoder));
      break;
    case PROP_MAX_LPC_ORDER:
      g_value_set_uint (value,
          FLAC__stream_encoder_get_max_lpc_order (this->encoder));
      break;
    case PROP_QLP_COEFF_PRECISION:
      g_value_set_uint (value,
          FLAC__stream_encoder_get_qlp_coeff_precision (this->encoder));
      break;
    case PROP_QLP_COEFF_PREC_SEARCH:
      g_value_set_boolean (value,
          FLAC__stream_encoder_get_do_qlp_coeff_prec_search (this->encoder));
      break;
    case PROP_ESCAPE_CODING:
      g_value_set_boolean (value,
          FLAC__stream_encoder_get_do_escape_coding (this->encoder));
      break;
    case PROP_EXHAUSTIVE_MODEL_SEARCH:
      g_value_set_boolean (value,
          FLAC__stream_encoder_get_do_exhaustive_model_search (this->encoder));
      break;
    case PROP_MIN_RESIDUAL_PARTITION_ORDER:
      g_value_set_uint (value,
          FLAC__stream_encoder_get_min_residual_partition_order
          (this->encoder));
      break;
    case PROP_MAX_RESIDUAL_PARTITION_ORDER:
      g_value_set_uint (value,
          FLAC__stream_encoder_get_max_residual_partition_order
          (this->encoder));
      break;
    case PROP_RICE_PARAMETER_SEARCH_DIST:
      g_value_set_uint (value,
          FLAC__stream_encoder_get_rice_parameter_search_dist (this->encoder));
      break;
    case PROP_PADDING:
      g_value_set_uint (value, this->padding);
      break;
    case PROP_SEEKPOINTS:
      g_value_set_int (value, this->seekpoints);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (this);
}
