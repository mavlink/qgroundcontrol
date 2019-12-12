/* GStreamer taglib-based APEv2 muxer
 * Copyright (C) 2006 Christophe Fergeau <teuf@gnome.org>
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2006 Sebastian Dröge <slomo@circular-chaos.org>
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
 * SECTION:element-apev2mux
 * @see_also: #GstTagSetter
 *
 * This element adds APEv2 tags to the beginning of a stream using the taglib
 * library.
 *
 * Applications can set the tags to write using the #GstTagSetter interface.
 * Tags sent by upstream elements will be picked up automatically (and merged
 * according to the merge mode set via the tag setter interface).
 *
 * ## Example pipelines
 *
 * |[
 * gst-launch-1.0 -v filesrc location=foo.ogg ! decodebin ! audioconvert ! lame ! apev2mux ! filesink location=foo.mp3
 * ]| A pipeline that transcodes a file from Ogg/Vorbis to mp3 format with an
 * APEv2 that contains the same as the the Ogg/Vorbis file. Make sure the
 * Ogg/Vorbis file actually has comments to preserve.
 * |[
 * gst-launch-1.0 -m filesrc location=foo.mp3 ! apedemux ! fakesink silent=TRUE 2&gt; /dev/null | grep taglist
 * ]| Verify that tags have been written.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gstapev2mux.h"

#include <string.h>

#include <apetag.h>
#include <gst/tag/tag.h>

using namespace TagLib;

GST_DEBUG_CATEGORY_STATIC (gst_apev2_mux_debug);
#define GST_CAT_DEFAULT gst_apev2_mux_debug

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-apetag"));

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY"));

G_DEFINE_TYPE (GstApev2Mux, gst_apev2_mux, GST_TYPE_TAG_MUX);

static GstBuffer *gst_apev2_mux_render_start_tag (GstTagMux * mux,
    const GstTagList * taglist);
static GstBuffer *gst_apev2_mux_render_end_tag (GstTagMux * mux,
    const GstTagList * taglist);

static void
gst_apev2_mux_class_init (GstApev2MuxClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_TAG_MUX_CLASS (klass)->render_start_tag =
      GST_DEBUG_FUNCPTR (gst_apev2_mux_render_start_tag);
  GST_TAG_MUX_CLASS (klass)->render_end_tag =
      GST_DEBUG_FUNCPTR (gst_apev2_mux_render_end_tag);

  gst_element_class_add_static_pad_template (element_class, &sink_template);
  gst_element_class_add_static_pad_template (element_class, &src_template);

  gst_element_class_set_static_metadata (element_class,
      "TagLib-based APEv2 Muxer", "Formatter/Metadata",
      "Adds an APEv2 header to the beginning of files using taglib",
      "Sebastian Dröge <slomo@circular-chaos.org>");

  GST_DEBUG_CATEGORY_INIT (gst_apev2_mux_debug, "apev2mux", 0,
      "taglib-based APEv2 tag muxer");
}

static void
gst_apev2_mux_init (GstApev2Mux * apev2mux)
{
  /* nothing to do */
}

static gboolean
gst_apev2_mux_have_wavpack (GstApev2Mux * apev2mux)
{
  const GstStructure *s;
  gboolean ret;
  GstCaps *caps;
  GstPad *sink;

  sink = gst_element_get_static_pad (GST_ELEMENT_CAST (apev2mux), "sink");
  caps = gst_pad_get_current_caps (sink);
  gst_object_unref (sink);

  if (caps == NULL)
    return FALSE;

  s = gst_caps_get_structure (caps, 0);
  ret = gst_structure_has_name (s, "audio/x-wavpack");
  gst_caps_unref (caps);

  GST_LOG_OBJECT (apev2mux, "got wavpack input: %s", ret ? "yes" : "no");
  return ret;
}

static void
add_one_tag (const GstTagList * list, const gchar * tag, gpointer user_data)
{
  APE::Tag * apev2tag = (APE::Tag *) user_data;
  gboolean result;

  /* FIXME: if there are several values set for the same tag, this won't
   * work, only the first value will be taken into account
   */
  if (strcmp (tag, GST_TAG_TITLE) == 0) {
    const char *title;

    result = gst_tag_list_peek_string_index (list, tag, 0, &title);
    if (result != FALSE) {
      GST_DEBUG ("Setting title to %s", title);
      apev2tag->setTitle (String (title, String::UTF8));
    }
  } else if (strcmp (tag, GST_TAG_ALBUM) == 0) {
    const char *album;

    result = gst_tag_list_peek_string_index (list, tag, 0, &album);
    if (result != FALSE) {
      GST_DEBUG ("Setting album to %s", album);
      apev2tag->setAlbum (String (album, String::UTF8));
    }
  } else if (strcmp (tag, GST_TAG_ARTIST) == 0) {
    const char *artist;

    result = gst_tag_list_peek_string_index (list, tag, 0, &artist);
    if (result != FALSE) {
      GST_DEBUG ("Setting artist to %s", artist);
      apev2tag->setArtist (String (artist, String::UTF8));
    }
  } else if (strcmp (tag, GST_TAG_COMPOSER) == 0) {
    const char *composer;

    result = gst_tag_list_peek_string_index (list, tag, 0, &composer);
    if (result != FALSE) {
      GST_DEBUG ("Setting composer to %s", composer);
      apev2tag->addValue (String ("COMPOSER", String::UTF8),
          String (composer, String::UTF8));
    }
  } else if (strcmp (tag, GST_TAG_GENRE) == 0) {
    const char *genre;

    result = gst_tag_list_peek_string_index (list, tag, 0, &genre);
    if (result != FALSE) {
      GST_DEBUG ("Setting genre to %s", genre);
      apev2tag->setGenre (String (genre, String::UTF8));
    }
  } else if (strcmp (tag, GST_TAG_COMMENT) == 0) {
    const char *comment;

    result = gst_tag_list_peek_string_index (list, tag, 0, &comment);
    if (result != FALSE) {
      GST_DEBUG ("Setting comment to %s", comment);
      apev2tag->setComment (String (comment, String::UTF8));
    }
  } else if (strcmp (tag, GST_TAG_DATE) == 0) {
    GDate *date;

    result = gst_tag_list_get_date_index (list, tag, 0, &date);
    if (result != FALSE) {
      GDateYear year;

      year = g_date_get_year (date);
      GST_DEBUG ("Setting track year to %d", year);
      apev2tag->setYear (year);
      g_date_free (date);
    }
  } else if (strcmp (tag, GST_TAG_TRACK_NUMBER) == 0) {
    guint track_number;

    result = gst_tag_list_get_uint_index (list, tag, 0, &track_number);
    if (result != FALSE) {
      guint total_tracks;

      result = gst_tag_list_get_uint_index (list, GST_TAG_TRACK_COUNT,
          0, &total_tracks);
      if (result) {
        gchar *tag_str;

        tag_str = g_strdup_printf ("%d/%d", track_number, total_tracks);
        GST_DEBUG ("Setting track number to %s", tag_str);
        apev2tag->addValue (String ("TRACK", String::UTF8),
            String (tag_str, String::UTF8), true);
        g_free (tag_str);
      } else {
        GST_DEBUG ("Setting track number to %d", track_number);
        apev2tag->setTrack (track_number);
      }
    }
  } else if (strcmp (tag, GST_TAG_TRACK_COUNT) == 0) {
    guint n;

    if (gst_tag_list_get_uint_index (list, GST_TAG_TRACK_NUMBER, 0, &n)) {
      GST_DEBUG ("track-count handled with track-number, skipping");
    } else if (gst_tag_list_get_uint_index (list, GST_TAG_TRACK_COUNT, 0, &n)) {
      gchar *tag_str;

      tag_str = g_strdup_printf ("0/%d", n);
      GST_DEBUG ("Setting track number to %s", tag_str);
      apev2tag->addValue (String ("TRACK", String::UTF8),
          String (tag_str, String::UTF8), true);
      g_free (tag_str);
    }
#if 0
  } else if (strcmp (tag, GST_TAG_ALBUM_VOLUME_NUMBER) == 0) {
    guint volume_number;

    result = gst_tag_list_get_uint_index (list, tag, 0, &volume_number);

    if (result != FALSE) {
      guint volume_count;
      gchar *tag_str;

      result = gst_tag_list_get_uint_index (list, GST_TAG_ALBUM_VOLUME_COUNT,
          0, &volume_count);

      if (result) {
        tag_str = g_strdup_printf ("CD %d/%d", volume_number, volume_count);
      } else {
        tag_str = g_strdup_printf ("CD %d", volume_number);
      }

      GST_DEBUG ("Setting album number to %s", tag_str);

      apev2tag->addValue (String ("MEDIA", String::UTF8),
          String (tag_str, String::UTF8), true);
      g_free (tag_str);
    }
  } else if (strcmp (tag, GST_TAG_ALBUM_VOLUME_COUNT) == 0) {
    guint n;

    if (gst_tag_list_get_uint_index (list, GST_TAG_ALBUM_VOLUME_NUMBER, 0, &n)) {
      GST_DEBUG ("volume-count handled with volume-number, skipping");
    } else if (gst_tag_list_get_uint_index (list, GST_TAG_ALBUM_VOLUME_COUNT,
            0, &n)) {
      gchar *tag_str;

      tag_str = g_strdup_printf ("CD 0/%u", n);
      GST_DEBUG ("Setting album volume number/count to %s", tag_str);

      apev2tag->addValue (String ("MEDIA", String::UTF8),
          String (tag_str, String::UTF8), true);
      g_free (tag_str);
    }
#endif
  } else if (strcmp (tag, GST_TAG_COPYRIGHT) == 0) {
    const gchar *copyright;

    result = gst_tag_list_peek_string_index (list, tag, 0, &copyright);

    if (result != FALSE) {
      GST_DEBUG ("Setting copyright to %s", copyright);
      apev2tag->addValue (String ("COPYRIGHT", String::UTF8),
          String (copyright, String::UTF8), true);
    }
  } else if (strcmp (tag, GST_TAG_LOCATION) == 0) {
    const gchar *location;

    result = gst_tag_list_peek_string_index (list, tag, 0, &location);

    if (result != FALSE) {
      GST_DEBUG ("Setting location to %s", location);
      apev2tag->addValue (String ("FILE", String::UTF8),
          String (location, String::UTF8), true);
    }
  } else if (strcmp (tag, GST_TAG_ISRC) == 0) {
    const gchar *isrc;

    result = gst_tag_list_peek_string_index (list, tag, 0, &isrc);

    if (result != FALSE) {
      GST_DEBUG ("Setting ISRC to %s", isrc);
      apev2tag->addValue (String ("ISRC", String::UTF8),
          String (isrc, String::UTF8), true);
    }
  } else if (strcmp (tag, GST_TAG_TRACK_GAIN) == 0) {
    gdouble value;

    result = gst_tag_list_get_double_index (list, tag, 0, &value);

    if (result != FALSE) {
      gchar *track_gain = (gchar *) g_malloc0 (G_ASCII_DTOSTR_BUF_SIZE);

      track_gain = g_ascii_dtostr (track_gain, G_ASCII_DTOSTR_BUF_SIZE, value);
      GST_DEBUG ("Setting track gain to %s", track_gain);
      apev2tag->addValue (String ("REPLAYGAIN_TRACK_GAIN",
              String::UTF8), String (track_gain, String::UTF8), true);
      g_free (track_gain);
    }
  } else if (strcmp (tag, GST_TAG_TRACK_PEAK) == 0) {
    gdouble value;

    result = gst_tag_list_get_double_index (list, tag, 0, &value);

    if (result != FALSE) {
      gchar *track_peak = (gchar *) g_malloc0 (G_ASCII_DTOSTR_BUF_SIZE);

      track_peak = g_ascii_dtostr (track_peak, G_ASCII_DTOSTR_BUF_SIZE, value);
      GST_DEBUG ("Setting track peak to %s", track_peak);
      apev2tag->addValue (String ("REPLAYGAIN_TRACK_PEAK",
              String::UTF8), String (track_peak, String::UTF8), true);
      g_free (track_peak);
    }
  } else if (strcmp (tag, GST_TAG_ALBUM_GAIN) == 0) {
    gdouble value;

    result = gst_tag_list_get_double_index (list, tag, 0, &value);

    if (result != FALSE) {
      gchar *album_gain = (gchar *) g_malloc0 (G_ASCII_DTOSTR_BUF_SIZE);

      album_gain = g_ascii_dtostr (album_gain, G_ASCII_DTOSTR_BUF_SIZE, value);
      GST_DEBUG ("Setting album gain to %s", album_gain);
      apev2tag->addValue (String ("REPLAYGAIN_ALBUM_GAIN",
              String::UTF8), String (album_gain, String::UTF8), true);
      g_free (album_gain);
    }
  } else if (strcmp (tag, GST_TAG_ALBUM_PEAK) == 0) {
    gdouble value;

    result = gst_tag_list_get_double_index (list, tag, 0, &value);

    if (result != FALSE) {
      gchar *album_peak = (gchar *) g_malloc0 (G_ASCII_DTOSTR_BUF_SIZE);

      album_peak = g_ascii_dtostr (album_peak, G_ASCII_DTOSTR_BUF_SIZE, value);
      GST_DEBUG ("Setting album peak to %s", album_peak);
      apev2tag->addValue (String ("REPLAYGAIN_ALBUM_PEAK",
              String::UTF8), String (album_peak, String::UTF8), true);
      g_free (album_peak);
    }
  } else {
    GST_WARNING ("Unsupported tag: %s", tag);
  }
}

static GstBuffer *
gst_apev2_mux_render_tag (GstTagMux * mux, const GstTagList * taglist)
{
  APE::Tag apev2tag;
  ByteVector rendered_tag;
  GstBuffer *buf;
  guint tag_size;

  /* Render the tag */
  gst_tag_list_foreach (taglist, add_one_tag, &apev2tag);

  rendered_tag = apev2tag.render ();
  tag_size = rendered_tag.size ();

  GST_LOG_OBJECT (mux, "tag size = %d bytes", tag_size);

  /* Create buffer with tag */
  buf = gst_buffer_new_and_alloc (tag_size);
  gst_buffer_fill (buf, 0, rendered_tag.data (), tag_size);

  return buf;
}

static GstBuffer *
gst_apev2_mux_render_start_tag (GstTagMux * mux, const GstTagList * taglist)
{
  if (gst_apev2_mux_have_wavpack (GST_APEV2_MUX (mux)))
    return NULL;

  return gst_apev2_mux_render_tag (mux, taglist);
}

static GstBuffer *
gst_apev2_mux_render_end_tag (GstTagMux * mux, const GstTagList * taglist)
{
  if (gst_apev2_mux_have_wavpack (GST_APEV2_MUX (mux)))
    return gst_apev2_mux_render_tag (mux, taglist);

  return NULL;
}
