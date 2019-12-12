/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * (c) 2006 Tim-Philipp Müller <tim centricular net>
 * (c) 2008 Sebastian Dröge <slomo@circular-chaos.org>
 * (c) 2011 Debarshi Ray <rishi@gnu.org>
 *
 * matroska-read-common.c: shared by matroska file/stream demuxer and parser
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#ifdef HAVE_BZ2
#include <bzlib.h>
#endif

#include <gst/tag/tag.h>
#include <gst/base/gsttypefindhelper.h>
#include <gst/base/gstbytewriter.h>

#include "lzo.h"

#include "ebml-read.h"
#include "matroska-read-common.h"
#include "matroska-ids.h"

GST_DEBUG_CATEGORY (matroskareadcommon_debug);
#define GST_CAT_DEFAULT matroskareadcommon_debug

#define DEBUG_ELEMENT_START(common, ebml, element) \
    GST_DEBUG_OBJECT (common->sinkpad, "Parsing " element " element at offset %" \
        G_GUINT64_FORMAT, gst_ebml_read_get_pos (ebml))

#define DEBUG_ELEMENT_STOP(common, ebml, element, ret) \
    GST_DEBUG_OBJECT (common->sinkpad, "Parsing " element " element " \
        " finished with '%s'", gst_flow_get_name (ret))

#define GST_MATROSKA_TOC_UID_CHAPTER "chapter"
#define GST_MATROSKA_TOC_UID_EDITION "edition"
#define GST_MATROSKA_TOC_UID_EMPTY "empty"

typedef struct
{
  GstTagList *result;
  guint64 target_type_value;
  gchar *target_type;
  gboolean audio_only;
} TargetTypeContext;


static gboolean
gst_matroska_decompress_data (GstMatroskaTrackEncoding * enc,
    gpointer * data_out, gsize * size_out,
    GstMatroskaTrackCompressionAlgorithm algo)
{
  guint8 *new_data = NULL;
  guint new_size = 0;
  guint8 *data = *data_out;
  guint size = *size_out;
  gboolean ret = TRUE;

  if (algo == GST_MATROSKA_TRACK_COMPRESSION_ALGORITHM_ZLIB) {
#ifdef HAVE_ZLIB
    /* zlib encoded data */
    z_stream zstream;
    guint orig_size;
    int result;

    orig_size = size;
    zstream.zalloc = (alloc_func) 0;
    zstream.zfree = (free_func) 0;
    zstream.opaque = (voidpf) 0;
    if (inflateInit (&zstream) != Z_OK) {
      GST_WARNING ("zlib initialization failed.");
      ret = FALSE;
      goto out;
    }
    zstream.next_in = (Bytef *) data;
    zstream.avail_in = orig_size;
    new_size = orig_size;
    new_data = g_malloc (new_size);
    zstream.avail_out = new_size;
    zstream.next_out = (Bytef *) new_data;

    do {
      result = inflate (&zstream, Z_NO_FLUSH);
      if (result == Z_STREAM_END) {
        break;
      } else if (result != Z_OK) {
        GST_WARNING ("inflate() returned %d", result);
        break;
      }

      new_size += 4096;
      new_data = g_realloc (new_data, new_size);
      zstream.next_out = (Bytef *) (new_data + zstream.total_out);
      zstream.avail_out += 4096;
    } while (zstream.avail_in > 0);

    if (result != Z_STREAM_END) {
      ret = FALSE;
      g_free (new_data);
    } else {
      new_size = zstream.total_out;
    }
    inflateEnd (&zstream);

#else
    GST_WARNING ("zlib encoded tracks not supported.");
    ret = FALSE;
    goto out;
#endif
  } else if (algo == GST_MATROSKA_TRACK_COMPRESSION_ALGORITHM_BZLIB) {
#ifdef HAVE_BZ2
    /* bzip2 encoded data */
    bz_stream bzstream;
    guint orig_size;
    int result;

    bzstream.bzalloc = NULL;
    bzstream.bzfree = NULL;
    bzstream.opaque = NULL;
    orig_size = size;

    if (BZ2_bzDecompressInit (&bzstream, 0, 0) != BZ_OK) {
      GST_WARNING ("bzip2 initialization failed.");
      ret = FALSE;
      goto out;
    }

    bzstream.next_in = (char *) data;
    bzstream.avail_in = orig_size;
    new_size = orig_size;
    new_data = g_malloc (new_size);
    bzstream.avail_out = new_size;
    bzstream.next_out = (char *) new_data;

    do {
      result = BZ2_bzDecompress (&bzstream);
      if (result == BZ_STREAM_END) {
        break;
      } else if (result != BZ_OK) {
        GST_WARNING ("BZ2_bzDecompress() returned %d", result);
        break;
      }

      new_size += 4096;
      new_data = g_realloc (new_data, new_size);
      bzstream.next_out = (char *) (new_data + bzstream.total_out_lo32);
      bzstream.avail_out += 4096;
    } while (bzstream.avail_in > 0);

    if (result != BZ_STREAM_END) {
      ret = FALSE;
      g_free (new_data);
    } else {
      new_size = bzstream.total_out_lo32;
    }
    BZ2_bzDecompressEnd (&bzstream);

#else
    GST_WARNING ("bzip2 encoded tracks not supported.");
    ret = FALSE;
    goto out;
#endif
  } else if (algo == GST_MATROSKA_TRACK_COMPRESSION_ALGORITHM_LZO1X) {
    /* lzo encoded data */
    int result;
    int orig_size, out_size;

    orig_size = size;
    out_size = size;
    new_size = size;
    new_data = g_malloc (new_size);

    do {
      orig_size = size;
      out_size = new_size;

      result = lzo1x_decode (new_data, &out_size, data, &orig_size);

      if (orig_size > 0) {
        new_size += 4096;
        new_data = g_realloc (new_data, new_size);
      }
    } while (orig_size > 0 && result == LZO_OUTPUT_FULL);

    new_size -= out_size;

    if (result != LZO_OUTPUT_FULL) {
      GST_WARNING ("lzo decompression failed");
      g_free (new_data);

      ret = FALSE;
      goto out;
    }

  } else if (algo == GST_MATROSKA_TRACK_COMPRESSION_ALGORITHM_HEADERSTRIP) {
    /* header stripped encoded data */
    if (enc->comp_settings_length > 0) {
      new_data = g_malloc (size + enc->comp_settings_length);
      new_size = size + enc->comp_settings_length;

      memcpy (new_data, enc->comp_settings, enc->comp_settings_length);
      memcpy (new_data + enc->comp_settings_length, data, size);
    }
  } else {
    GST_ERROR ("invalid compression algorithm %d", algo);
    ret = FALSE;
  }

out:

  if (!ret) {
    *data_out = NULL;
    *size_out = 0;
  } else {
    *data_out = new_data;
    *size_out = new_size;
  }

  return ret;
}

GstFlowReturn
gst_matroska_decode_content_encodings (GArray * encodings)
{
  gint i;

  if (encodings == NULL)
    return GST_FLOW_OK;

  for (i = 0; i < encodings->len; i++) {
    GstMatroskaTrackEncoding *enc =
        &g_array_index (encodings, GstMatroskaTrackEncoding, i);
    gpointer data = NULL;
    gsize size;

    if ((enc->scope & GST_MATROSKA_TRACK_ENCODING_SCOPE_NEXT_CONTENT_ENCODING)
        == 0)
      continue;

    /* Other than ENCODING_COMPRESSION not handled here */
    if (enc->type != GST_MATROSKA_ENCODING_COMPRESSION)
      continue;

    if (i + 1 >= encodings->len)
      return GST_FLOW_ERROR;

    if (enc->comp_settings_length == 0)
      continue;

    data = enc->comp_settings;
    size = enc->comp_settings_length;

    if (!gst_matroska_decompress_data (enc, &data, &size, enc->comp_algo))
      return GST_FLOW_ERROR;

    g_free (enc->comp_settings);

    enc->comp_settings = data;
    enc->comp_settings_length = size;
  }

  return GST_FLOW_OK;
}

gboolean
gst_matroska_decode_data (GArray * encodings, gpointer * data_out,
    gsize * size_out, GstMatroskaTrackEncodingScope scope, gboolean free)
{
  gpointer data;
  gsize size;
  gboolean ret = TRUE;
  gint i;

  g_return_val_if_fail (encodings != NULL, FALSE);
  g_return_val_if_fail (data_out != NULL && *data_out != NULL, FALSE);
  g_return_val_if_fail (size_out != NULL, FALSE);

  data = *data_out;
  size = *size_out;

  for (i = 0; i < encodings->len; i++) {
    GstMatroskaTrackEncoding *enc =
        &g_array_index (encodings, GstMatroskaTrackEncoding, i);
    gpointer new_data = NULL;
    gsize new_size = 0;

    if ((enc->scope & scope) == 0)
      continue;

    /* Encryption not handled here */
    if (enc->type != GST_MATROSKA_ENCODING_COMPRESSION) {
      ret = TRUE;
      break;
    }

    new_data = data;
    new_size = size;

    ret =
        gst_matroska_decompress_data (enc, &new_data, &new_size,
        enc->comp_algo);

    if (!ret)
      break;

    if ((data == *data_out && free) || (data != *data_out))
      g_free (data);

    data = new_data;
    size = new_size;
  }

  if (!ret) {
    if ((data == *data_out && free) || (data != *data_out))
      g_free (data);

    *data_out = NULL;
    *size_out = 0;
  } else {
    *data_out = data;
    *size_out = size;
  }

  return ret;
}

/* This function parses the protection info of Block/SimpleBlock and extracts the
 * IV and partitioning format (subsample) information.
 * Set those parsed information into protection info structure @info_protect which
 * will be added in protection metadata of the Gstbuffer.
 * The subsamples format follows the same pssh box format in Common Encryption spec:
 * subsample number + clear subsample size (16bit bigendian) | encrypted subsample size (32bit bigendian) | ...
 * @encrypted is an output argument: TRUE if the current Block/SimpleBlock is encrypted else FALSE
 */
gboolean
gst_matroska_parse_protection_meta (gpointer * data_out, gsize * size_out,
    GstStructure * info_protect, gboolean * encrypted)
{
  guint8 *data;
  GstBuffer *buf_iv;
  guint8 *data_iv;
  guint8 *subsamples;
  guint8 signal_byte;
  gint i;
  GstByteReader reader;

  g_return_val_if_fail (data_out != NULL && *data_out != NULL, FALSE);
  g_return_val_if_fail (size_out != NULL, FALSE);
  g_return_val_if_fail (info_protect != NULL, FALSE);
  g_return_val_if_fail (encrypted != NULL, FALSE);

  *encrypted = FALSE;
  data = *data_out;
  gst_byte_reader_init (&reader, data, *size_out);

  /* WebM spec:
   * 4.7 Signal Byte Format
   *  0 1 2 3 4 5 6 7
   * +-+-+-+-+-+-+-+-+
   * |X|   RSV   |P|E|
   * +-+-+-+-+-+-+-+-+
   *
   * Extension bit (X)
   * If set, another signal byte will follow this byte. Reserved for future expansion (currently MUST be set to 0).
   * RSV bits (RSV)
   * Bits reserved for future use. MUST be set to 0 and MUST be ignored.
   * Encrypted bit (E)
   * If set, the Block MUST contain an IV immediately followed by an encrypted frame. If not set, the Block MUST NOT include an IV and the frame MUST be unencrypted. The unencrypted frame MUST immediately follow the Signal Byte.
   * Partitioned bit (P)
   * Used to indicate that the sample has subsample partitions. If set, the IV will be followed by a num_partitions byte, and num_partitions * 32-bit partition offsets. This bit can only be set if the E bit is also set.
   */
  if (!gst_byte_reader_get_uint8 (&reader, &signal_byte)) {
    GST_ERROR ("Error reading the signal byte");
    return FALSE;
  }

  /* Unencrypted buffer */
  if (!(signal_byte & GST_MATROSKA_BLOCK_ENCRYPTED)) {
    return TRUE;
  }

  /* Encrypted buffer */
  *encrypted = TRUE;
  /* Create IV buffer */
  if (!gst_byte_reader_dup_data (&reader, sizeof (guint64), &data_iv)) {
    GST_ERROR ("Error reading the IV data");
    return FALSE;
  }
  buf_iv = gst_buffer_new_wrapped ((gpointer) data_iv, sizeof (guint64));
  gst_structure_set (info_protect, "iv", GST_TYPE_BUFFER, buf_iv, NULL);
  gst_buffer_unref (buf_iv);

  /* Partitioned in subsample */
  if (signal_byte & GST_MATROSKA_BLOCK_PARTITIONED) {
    guint nb_subsample;
    guint32 offset = 0;
    guint32 offset_prev;
    guint32 encrypted_bytes = 0;
    guint16 clear_bytes = 0;
    GstBuffer *buf_sub_sample;
    guint8 nb_part;
    GstByteWriter writer;

    /* Read the number of partitions (1 byte) */
    if (!gst_byte_reader_get_uint8 (&reader, &nb_part)) {
      GST_ERROR ("Error reading the partition number");
      return FALSE;
    }

    if (nb_part == 0) {
      GST_ERROR ("Partitioned, but the subsample number equal to zero");
      return FALSE;
    }

    nb_subsample = (nb_part + 2) >> 1;

    gst_structure_set (info_protect, "subsample_count", G_TYPE_UINT,
        nb_subsample, NULL);

    /* WebM Spec:
     *
     * 4.6 Subsample Encrypted Block Format
     *
     * The Subsample Encrypted Block format extends the Full-sample format by setting a "partitioned" (P) bit in the Signal Byte.
     * If this bit is set, the EncryptedBlock header shall include an
     * 8-bit integer indicating the number of sample partitions (dividers between clear/encrypted sections),
     * and a series of 32-bit integers in big-endian encoding indicating the byte offsets of such partitions.
     *
     *  0                   1                   2                   3
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |  Signal Byte  |                                               |
     * +-+-+-+-+-+-+-+-+             IV                                |
     * |                                                               |
     * |               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |               | num_partition |     Partition 0 offset ->     |
     * |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
     * |     -> Partition 0 offset     |              ...              |
     * |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
     * |             ...               |     Partition n-1 offset ->   |
     * |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
     * |     -> Partition n-1 offset   |                               |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
     * |                    Clear/encrypted sample data                |
     * |                                                               |
     * |                                                               |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *
     * 4.6.1 SAMPLE PARTITIONS
     *
     * The samples shall be partitioned into alternating clear and encrypted sections,
     * always starting with a clear section.
     * Generally for n clear/encrypted sections there shall be n-1 partition offsets.
     * However, if it is required that the first section be encrypted, then the first partition shall be at byte offset 0
     * (indicating a zero-size clear section), and there shall be n partition offsets.
     * Please refer to the "Sample Encryption" description of the "Common Encryption"
     * section of the VP Codec ISO Media File Format Binding Specification for more
     * detail on how subsample encryption is implemented.
     */
    subsamples =
        g_malloc (nb_subsample * (sizeof (guint16) + sizeof (guint32)));

    gst_byte_writer_init_with_data (&writer, subsamples,
        nb_subsample * (sizeof (guint16) + sizeof (guint32)), FALSE);

    for (i = 0; i <= nb_part; i++) {
      offset_prev = offset;
      if (i == nb_part) {
        offset = gst_byte_reader_get_remaining (&reader);
      } else {
        if (!gst_byte_reader_get_uint32_be (&reader, &offset)) {
          GST_ERROR ("Error reading the partition offset");
          goto release_err;
        }
      }

      if (offset < offset_prev) {
        GST_ERROR ("Partition offsets should not decrease");
        goto release_err;
      }

      if (i % 2 == 0) {
        if ((offset - offset_prev) & 0xFFFF0000) {
          GST_ERROR
              ("The Clear Partition exceed 64KB in encrypted subsample format");
          goto release_err;
        }
        /* We set the Clear partition size in 16 bits, in order to
         * follow the same format of the box PSSH in CENC spec */
        clear_bytes = offset - offset_prev;
        if (i == nb_part)
          encrypted_bytes = 0;
      } else {
        encrypted_bytes = offset - offset_prev;
      }

      if ((i % 2 == 1) || (i == nb_part)) {
        if (clear_bytes == 0 && encrypted_bytes == 0) {
          GST_ERROR ("Found 2 partitions with the same offsets.");
          goto release_err;
        }
        if (!gst_byte_writer_put_uint16_be (&writer, clear_bytes)) {
          GST_ERROR ("Error writing the number of clear bytes");
          goto release_err;
        }
        if (!gst_byte_writer_put_uint32_be (&writer, encrypted_bytes)) {
          GST_ERROR ("Error writing the number of encrypted bytes");
          goto release_err;
        }
      }
    }

    buf_sub_sample =
        gst_buffer_new_wrapped (subsamples,
        nb_subsample * (sizeof (guint16) + sizeof (guint32)));
    gst_structure_set (info_protect, "subsamples", GST_TYPE_BUFFER,
        buf_sub_sample, NULL);
    gst_buffer_unref (buf_sub_sample);
  } else {
    gst_structure_set (info_protect, "subsample_count", G_TYPE_UINT, 0, NULL);
  }

  gst_byte_reader_get_data (&reader, 0, (const guint8 **) data_out);
  *size_out = gst_byte_reader_get_remaining (&reader);
  return TRUE;

release_err:
  g_free (subsamples);
  return FALSE;
}

static gint
gst_matroska_index_compare (GstMatroskaIndex * i1, GstMatroskaIndex * i2)
{
  if (i1->time < i2->time)
    return -1;
  else if (i1->time > i2->time)
    return 1;
  else if (i1->block < i2->block)
    return -1;
  else if (i1->block > i2->block)
    return 1;
  else
    return 0;
}

gint
gst_matroska_index_seek_find (GstMatroskaIndex * i1, GstClockTime * time,
    gpointer user_data)
{
  if (i1->time < *time)
    return -1;
  else if (i1->time > *time)
    return 1;
  else
    return 0;
}

GstMatroskaIndex *
gst_matroska_read_common_do_index_seek (GstMatroskaReadCommon * common,
    GstMatroskaTrackContext * track, gint64 seek_pos, GArray ** _index,
    gint * _entry_index, GstSearchMode snap_dir)
{
  GstMatroskaIndex *entry = NULL;
  GArray *index;

  /* find entry just before or at the requested position */
  if (track && track->index_table)
    index = track->index_table;
  else
    index = common->index;

  if (!index || !index->len)
    return NULL;

  entry =
      gst_util_array_binary_search (index->data, index->len,
      sizeof (GstMatroskaIndex),
      (GCompareDataFunc) gst_matroska_index_seek_find, snap_dir, &seek_pos,
      NULL);

  if (entry == NULL) {
    if (snap_dir == GST_SEARCH_MODE_AFTER) {
      /* Can only happen with a reverse seek past the end */
      entry = &g_array_index (index, GstMatroskaIndex, index->len - 1);
    } else {
      /* Can only happen with a forward seek before the start */
      entry = &g_array_index (index, GstMatroskaIndex, 0);
    }
  }

  if (_index)
    *_index = index;
  if (_entry_index)
    *_entry_index = entry - (GstMatroskaIndex *) index->data;

  return entry;
}

static gint
gst_matroska_read_common_encoding_cmp (GstMatroskaTrackEncoding * a,
    GstMatroskaTrackEncoding * b)
{
  if (b->order > a->order)
    return 1;
  else if (b->order < a->order)
    return -1;
  else
    return 0;
}

static gboolean
gst_matroska_read_common_encoding_order_unique (GArray * encodings, guint64
    order)
{
  gint i;

  if (encodings == NULL || encodings->len == 0)
    return TRUE;

  for (i = 0; i < encodings->len; i++)
    if (g_array_index (encodings, GstMatroskaTrackEncoding, i).order == order)
      return FALSE;

  return TRUE;
}

/* takes ownership of taglist */
void
gst_matroska_read_common_found_global_tag (GstMatroskaReadCommon * common,
    GstElement * el, GstTagList * taglist)
{
  if (common->global_tags) {
    gst_tag_list_insert (common->global_tags, taglist, GST_TAG_MERGE_APPEND);
    gst_tag_list_unref (taglist);
  } else {
    common->global_tags = taglist;
  }
  common->global_tags_changed = TRUE;
}

gint64
gst_matroska_read_common_get_length (GstMatroskaReadCommon * common)
{
  gint64 end = -1;

  if (!gst_pad_peer_query_duration (common->sinkpad, GST_FORMAT_BYTES,
          &end) || end < 0)
    GST_DEBUG_OBJECT (common->sinkpad, "no upstream length");

  return end;
}

/* determine track to seek in */
GstMatroskaTrackContext *
gst_matroska_read_common_get_seek_track (GstMatroskaReadCommon * common,
    GstMatroskaTrackContext * track)
{
  gint i;

  if (track && track->type == GST_MATROSKA_TRACK_TYPE_VIDEO)
    return track;

  for (i = 0; i < common->src->len; i++) {
    GstMatroskaTrackContext *stream;

    stream = g_ptr_array_index (common->src, i);
    if (stream->type == GST_MATROSKA_TRACK_TYPE_VIDEO && stream->index_table)
      track = stream;
  }

  return track;
}

/* skip unknown or alike element */
GstFlowReturn
gst_matroska_read_common_parse_skip (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, const gchar * parent_name, guint id)
{
  if (id == GST_EBML_ID_VOID) {
    GST_DEBUG_OBJECT (common->sinkpad, "Skipping EBML Void element");
  } else if (id == GST_EBML_ID_CRC32) {
    GST_DEBUG_OBJECT (common->sinkpad, "Skipping EBML CRC32 element");
  } else {
    GST_WARNING_OBJECT (common->sinkpad,
        "Unknown %s subelement 0x%x - ignoring", parent_name, id);
  }

  return gst_ebml_read_skip (ebml);
}

static GstFlowReturn
gst_matroska_read_common_parse_attached_file (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, GstTagList * taglist)
{
  guint32 id;
  GstFlowReturn ret;
  gchar *description = NULL;
  gchar *filename = NULL;
  gchar *mimetype = NULL;
  guint8 *data = NULL;
  guint64 datalen = 0;

  DEBUG_ELEMENT_START (common, ebml, "AttachedFile");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "AttachedFile", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    /* read all sub-entries */

    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_FILEDESCRIPTION:
        if (description) {
          GST_WARNING_OBJECT (common->sinkpad,
              "FileDescription can only appear once");
          break;
        }

        ret = gst_ebml_read_utf8 (ebml, &id, &description);
        GST_DEBUG_OBJECT (common->sinkpad, "FileDescription: %s",
            GST_STR_NULL (description));
        break;
      case GST_MATROSKA_ID_FILENAME:
        if (filename) {
          GST_WARNING_OBJECT (common->sinkpad, "FileName can only appear once");
          break;
        }

        ret = gst_ebml_read_utf8 (ebml, &id, &filename);

        GST_DEBUG_OBJECT (common->sinkpad, "FileName: %s",
            GST_STR_NULL (filename));
        break;
      case GST_MATROSKA_ID_FILEMIMETYPE:
        if (mimetype) {
          GST_WARNING_OBJECT (common->sinkpad,
              "FileMimeType can only appear once");
          break;
        }

        ret = gst_ebml_read_ascii (ebml, &id, &mimetype);
        GST_DEBUG_OBJECT (common->sinkpad, "FileMimeType: %s",
            GST_STR_NULL (mimetype));
        break;
      case GST_MATROSKA_ID_FILEDATA:
        if (data) {
          GST_WARNING_OBJECT (common->sinkpad, "FileData can only appear once");
          break;
        }

        ret = gst_ebml_read_binary (ebml, &id, &data, &datalen);
        GST_DEBUG_OBJECT (common->sinkpad,
            "FileData of size %" G_GUINT64_FORMAT, datalen);
        break;

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml,
            "AttachedFile", id);
        break;
      case GST_MATROSKA_ID_FILEUID:
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "AttachedFile", ret);

  if (filename && mimetype && data && datalen > 0) {
    GstTagImageType image_type = GST_TAG_IMAGE_TYPE_NONE;
    GstBuffer *tagbuffer = NULL;
    GstSample *tagsample = NULL;
    GstStructure *info = NULL;
    GstCaps *caps = NULL;
    gchar *filename_lc = g_utf8_strdown (filename, -1);

    GST_DEBUG_OBJECT (common->sinkpad, "Creating tag for attachment with "
        "filename '%s', mimetype '%s', description '%s', "
        "size %" G_GUINT64_FORMAT, filename, mimetype,
        GST_STR_NULL (description), datalen);

    /* TODO: better heuristics for different image types */
    if (strstr (filename_lc, "cover")) {
      if (strstr (filename_lc, "back"))
        image_type = GST_TAG_IMAGE_TYPE_BACK_COVER;
      else
        image_type = GST_TAG_IMAGE_TYPE_FRONT_COVER;
    } else if (g_str_has_prefix (mimetype, "image/") ||
        g_str_has_suffix (filename_lc, "png") ||
        g_str_has_suffix (filename_lc, "jpg") ||
        g_str_has_suffix (filename_lc, "jpeg") ||
        g_str_has_suffix (filename_lc, "gif") ||
        g_str_has_suffix (filename_lc, "bmp")) {
      image_type = GST_TAG_IMAGE_TYPE_UNDEFINED;
    }
    g_free (filename_lc);

    /* First try to create an image tag buffer from this */
    if (image_type != GST_TAG_IMAGE_TYPE_NONE) {
      tagsample =
          gst_tag_image_data_to_image_sample (data, datalen, image_type);

      if (!tagsample) {
        image_type = GST_TAG_IMAGE_TYPE_NONE;
      } else {
        tagbuffer = gst_buffer_ref (gst_sample_get_buffer (tagsample));
        caps = gst_caps_ref (gst_sample_get_caps (tagsample));
        info = gst_structure_copy (gst_sample_get_info (tagsample));
        gst_sample_unref (tagsample);
      }
    }

    /* if this failed create an attachment buffer */
    if (!tagbuffer) {
      tagbuffer = gst_buffer_new_wrapped (g_memdup (data, datalen), datalen);

      caps = gst_type_find_helper_for_buffer (NULL, tagbuffer, NULL);
      if (caps == NULL)
        caps = gst_caps_new_empty_simple (mimetype);
    }

    /* Set filename and description in the info */
    if (info == NULL)
      info = gst_structure_new_empty ("GstTagImageInfo");

    gst_structure_set (info, "filename", G_TYPE_STRING, filename, NULL);
    if (description)
      gst_structure_set (info, "description", G_TYPE_STRING, description, NULL);

    tagsample = gst_sample_new (tagbuffer, caps, NULL, info);

    gst_buffer_unref (tagbuffer);
    gst_caps_unref (caps);

    GST_DEBUG_OBJECT (common->sinkpad,
        "Created attachment sample: %" GST_PTR_FORMAT, tagsample);

    /* and append to the tag list */
    if (image_type != GST_TAG_IMAGE_TYPE_NONE)
      gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, GST_TAG_IMAGE, tagsample,
          NULL);
    else
      gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, GST_TAG_ATTACHMENT,
          tagsample, NULL);

    /* the list adds it own ref */
    gst_sample_unref (tagsample);
  }

  g_free (filename);
  g_free (mimetype);
  g_free (data);
  g_free (description);

  return ret;
}

GstFlowReturn
gst_matroska_read_common_parse_attachments (GstMatroskaReadCommon * common,
    GstElement * el, GstEbmlRead * ebml)
{
  guint32 id;
  GstFlowReturn ret = GST_FLOW_OK;
  GstTagList *taglist;

  DEBUG_ELEMENT_START (common, ebml, "Attachments");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "Attachments", ret);
    return ret;
  }

  taglist = gst_tag_list_new_empty ();
  gst_tag_list_set_scope (taglist, GST_TAG_SCOPE_GLOBAL);

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_ATTACHEDFILE:
        ret = gst_matroska_read_common_parse_attached_file (common, ebml,
            taglist);
        break;

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml,
            "Attachments", id);
        break;
    }
  }
  DEBUG_ELEMENT_STOP (common, ebml, "Attachments", ret);

  if (gst_tag_list_n_tags (taglist) > 0) {
    GST_DEBUG_OBJECT (common->sinkpad, "Storing attachment tags");
    gst_matroska_read_common_found_global_tag (common, el, taglist);
  } else {
    GST_DEBUG_OBJECT (common->sinkpad, "No valid attachments found");
    gst_tag_list_unref (taglist);
  }

  common->attachments_parsed = TRUE;

  return ret;
}

static void
gst_matroska_read_common_parse_toc_tag (GstTocEntry * entry,
    GstTocEntry * internal_entry, GArray * edition_targets,
    GArray * chapter_targets, GstTagList * tags)
{
  gchar *uid;
  guint i;
  guint64 tgt;
  GArray *targets;
  GList *cur, *internal_cur;
  GstTagList *etags;

  targets =
      (gst_toc_entry_get_entry_type (entry) ==
      GST_TOC_ENTRY_TYPE_EDITION) ? edition_targets : chapter_targets;

  etags = gst_tag_list_new_empty ();

  for (i = 0; i < targets->len; ++i) {
    tgt = g_array_index (targets, guint64, i);

    if (tgt == 0)
      gst_tag_list_insert (etags, tags, GST_TAG_MERGE_APPEND);
    else {
      uid = g_strdup_printf ("%" G_GUINT64_FORMAT, tgt);
      if (g_strcmp0 (gst_toc_entry_get_uid (internal_entry), uid) == 0)
        gst_tag_list_insert (etags, tags, GST_TAG_MERGE_APPEND);
      g_free (uid);
    }
  }

  gst_toc_entry_merge_tags (entry, etags, GST_TAG_MERGE_APPEND);
  gst_tag_list_unref (etags);

  cur = gst_toc_entry_get_sub_entries (entry);
  internal_cur = gst_toc_entry_get_sub_entries (internal_entry);
  while (cur != NULL && internal_cur != NULL) {
    gst_matroska_read_common_parse_toc_tag (cur->data, internal_cur->data,
        edition_targets, chapter_targets, tags);
    cur = cur->next;
    internal_cur = internal_cur->next;
  }
}

static GstFlowReturn
gst_matroska_read_common_parse_metadata_targets (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, GArray * edition_targets, GArray * chapter_targets,
    GArray * track_targets, guint64 * target_type_value, gchar ** target_type)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 id;
  guint64 uid;
  guint64 tmp;
  gchar *str;

  DEBUG_ELEMENT_START (common, ebml, "TagTargets");

  *target_type_value = 50;
  *target_type = NULL;

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "TagTargets", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_TARGETCHAPTERUID:
        if ((ret = gst_ebml_read_uint (ebml, &id, &uid)) == GST_FLOW_OK)
          g_array_append_val (chapter_targets, uid);
        break;

      case GST_MATROSKA_ID_TARGETEDITIONUID:
        if ((ret = gst_ebml_read_uint (ebml, &id, &uid)) == GST_FLOW_OK)
          g_array_append_val (edition_targets, uid);
        break;

      case GST_MATROSKA_ID_TARGETTRACKUID:
        if ((ret = gst_ebml_read_uint (ebml, &id, &uid)) == GST_FLOW_OK)
          g_array_append_val (track_targets, uid);
        break;

      case GST_MATROSKA_ID_TARGETTYPEVALUE:
        if ((ret = gst_ebml_read_uint (ebml, &id, &tmp)) == GST_FLOW_OK)
          *target_type_value = tmp;
        break;

      case GST_MATROSKA_ID_TARGETTYPE:
        if ((ret = gst_ebml_read_ascii (ebml, &id, &str)) == GST_FLOW_OK) {
          g_free (*target_type);
          *target_type = str;
        }
        break;

      default:
        ret =
            gst_matroska_read_common_parse_skip (common, ebml, "TagTargets",
            id);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "TagTargets", ret);

  return ret;
}

static void
gst_matroska_read_common_postprocess_toc_entries (GList * toc_entries,
    guint64 max, const gchar * parent_uid)
{
  GstTocEntry *cur_info, *prev_info, *next_info;
  GList *cur_list, *prev_list, *next_list;
  gint64 cur_start, prev_start, stop;

  cur_list = toc_entries;
  while (cur_list != NULL) {
    cur_info = cur_list->data;

    switch (gst_toc_entry_get_entry_type (cur_info)) {
      case GST_TOC_ENTRY_TYPE_ANGLE:
      case GST_TOC_ENTRY_TYPE_VERSION:
      case GST_TOC_ENTRY_TYPE_EDITION:
        /* in Matroska terms edition has duration of full track */
        gst_toc_entry_set_start_stop_times (cur_info, 0, max);

        gst_matroska_read_common_postprocess_toc_entries
            (gst_toc_entry_get_sub_entries (cur_info), max,
            gst_toc_entry_get_uid (cur_info));
        break;

      case GST_TOC_ENTRY_TYPE_TITLE:
      case GST_TOC_ENTRY_TYPE_TRACK:
      case GST_TOC_ENTRY_TYPE_CHAPTER:
        prev_list = cur_list->prev;
        next_list = cur_list->next;

        if (prev_list != NULL)
          prev_info = prev_list->data;
        else
          prev_info = NULL;

        if (next_list != NULL)
          next_info = next_list->data;
        else
          next_info = NULL;

        /* updated stop time in previous chapter and it's subchapters */
        if (prev_info != NULL) {
          gst_toc_entry_get_start_stop_times (prev_info, &prev_start, &stop);
          gst_toc_entry_get_start_stop_times (cur_info, &cur_start, &stop);

          stop = cur_start;
          gst_toc_entry_set_start_stop_times (prev_info, prev_start, stop);

          gst_matroska_read_common_postprocess_toc_entries
              (gst_toc_entry_get_sub_entries (prev_info), cur_start,
              gst_toc_entry_get_uid (prev_info));
        }

        /* updated stop time in current chapter and it's subchapters */
        if (next_info == NULL) {
          gst_toc_entry_get_start_stop_times (cur_info, &cur_start, &stop);

          if (stop == -1) {
            stop = max;
            gst_toc_entry_set_start_stop_times (cur_info, cur_start, stop);
          }

          gst_matroska_read_common_postprocess_toc_entries
              (gst_toc_entry_get_sub_entries (cur_info), stop,
              gst_toc_entry_get_uid (cur_info));
        }
        break;
      case GST_TOC_ENTRY_TYPE_INVALID:
        break;
    }
    cur_list = cur_list->next;
  }
}

static GstFlowReturn
gst_matroska_read_common_parse_chapter_titles (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, GstTagList * titles)
{
  guint32 id;
  gchar *title = NULL;
  GstFlowReturn ret = GST_FLOW_OK;

  DEBUG_ELEMENT_START (common, ebml, "ChaptersTitles");


  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "ChaptersTitles", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_CHAPSTRING:
        ret = gst_ebml_read_utf8 (ebml, &id, &title);
        break;

      default:
        ret =
            gst_matroska_read_common_parse_skip (common, ebml, "ChaptersTitles",
            id);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "ChaptersTitles", ret);

  if (title != NULL && ret == GST_FLOW_OK)
    gst_tag_list_add (titles, GST_TAG_MERGE_APPEND, GST_TAG_TITLE, title, NULL);

  g_free (title);
  return ret;
}

static GstFlowReturn
gst_matroska_read_common_parse_chapter_element (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, GList ** subentries, GList ** internal_subentries)
{
  guint32 id;
  guint64 start_time = -1, stop_time = -1;
  guint64 is_hidden = 0, is_enabled = 1, uid = 0;
  GstFlowReturn ret = GST_FLOW_OK;
  GstTocEntry *chapter_info, *internal_chapter_info;
  GstTagList *tags;
  gchar *uid_str, *string_uid = NULL;
  GList *subsubentries = NULL, *internal_subsubentries = NULL, *l, *il;

  DEBUG_ELEMENT_START (common, ebml, "ChaptersElement");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "ChaptersElement", ret);
    return ret;
  }

  tags = gst_tag_list_new_empty ();

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_CHAPTERUID:
        ret = gst_ebml_read_uint (ebml, &id, &uid);
        break;

      case GST_MATROSKA_ID_CHAPTERSTRINGUID:
        ret = gst_ebml_read_utf8 (ebml, &id, &string_uid);
        break;

      case GST_MATROSKA_ID_CHAPTERTIMESTART:
        ret = gst_ebml_read_uint (ebml, &id, &start_time);
        break;

      case GST_MATROSKA_ID_CHAPTERTIMESTOP:
        ret = gst_ebml_read_uint (ebml, &id, &stop_time);
        break;

      case GST_MATROSKA_ID_CHAPTERATOM:
        ret = gst_matroska_read_common_parse_chapter_element (common, ebml,
            &subsubentries, &internal_subsubentries);
        break;

      case GST_MATROSKA_ID_CHAPTERDISPLAY:
        ret =
            gst_matroska_read_common_parse_chapter_titles (common, ebml, tags);
        break;

      case GST_MATROSKA_ID_CHAPTERFLAGHIDDEN:
        ret = gst_ebml_read_uint (ebml, &id, &is_hidden);
        break;

      case GST_MATROSKA_ID_CHAPTERFLAGENABLED:
        ret = gst_ebml_read_uint (ebml, &id, &is_enabled);
        break;

      default:
        ret =
            gst_matroska_read_common_parse_skip (common, ebml,
            "ChaptersElement", id);
        break;
    }
  }

  if (uid == 0)
    uid = (((guint64) g_random_int ()) << 32) | g_random_int ();
  uid_str = g_strdup_printf ("%" G_GUINT64_FORMAT, uid);
  if (string_uid != NULL) {
    /* init toc with provided String UID */
    chapter_info = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_CHAPTER, string_uid);
    g_free (string_uid);
  } else {
    /* No String UID provided => use the internal UID instead */
    chapter_info = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_CHAPTER, uid_str);
  }
  /* init internal toc with internal UID */
  internal_chapter_info = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_CHAPTER,
      uid_str);
  g_free (uid_str);

  gst_toc_entry_set_tags (chapter_info, tags);
  gst_toc_entry_set_start_stop_times (chapter_info, start_time, stop_time);

  for (l = subsubentries, il = internal_subsubentries;
      l && il; l = l->next, il = il->next) {
    gst_toc_entry_append_sub_entry (chapter_info, l->data);
    gst_toc_entry_append_sub_entry (internal_chapter_info, il->data);
  }
  g_list_free (subsubentries);
  g_list_free (internal_subsubentries);

  DEBUG_ELEMENT_STOP (common, ebml, "ChaptersElement", ret);

  /* start time is mandatory and has no default value,
   * so we should skip chapters without it */
  if (is_hidden == 0 && is_enabled > 0 &&
      start_time != -1 && ret == GST_FLOW_OK) {
    *subentries = g_list_append (*subentries, chapter_info);
    *internal_subentries = g_list_append (*internal_subentries,
        internal_chapter_info);
  } else {
    gst_toc_entry_unref (chapter_info);
    gst_toc_entry_unref (internal_chapter_info);
  }

  return ret;
}

static GstFlowReturn
gst_matroska_read_common_parse_chapter_edition (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, GstToc * toc, GstToc * internal_toc)
{
  guint32 id;
  guint64 is_hidden = 0, uid = 0;
  GstFlowReturn ret = GST_FLOW_OK;
  GstTocEntry *edition_info, *internal_edition_info;
  GList *subentries = NULL, *internal_subentries = NULL, *l, *il;
  gchar *uid_str;

  DEBUG_ELEMENT_START (common, ebml, "ChaptersEdition");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "ChaptersEdition", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_EDITIONUID:
        ret = gst_ebml_read_uint (ebml, &id, &uid);
        break;

      case GST_MATROSKA_ID_CHAPTERATOM:
        ret = gst_matroska_read_common_parse_chapter_element (common, ebml,
            &subentries, &internal_subentries);
        break;

      case GST_MATROSKA_ID_EDITIONFLAGHIDDEN:
        ret = gst_ebml_read_uint (ebml, &id, &is_hidden);
        break;

      default:
        ret =
            gst_matroska_read_common_parse_skip (common, ebml,
            "ChaptersEdition", id);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "ChaptersEdition", ret);

  if (uid == 0)
    uid = (((guint64) g_random_int ()) << 32) | g_random_int ();
  uid_str = g_strdup_printf ("%" G_GUINT64_FORMAT, uid);
  edition_info = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_EDITION, uid_str);
  gst_toc_entry_set_start_stop_times (edition_info, -1, -1);
  internal_edition_info = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_EDITION,
      uid_str);
  g_free (uid_str);

  for (l = subentries, il = internal_subentries; l && il;
      l = l->next, il = il->next) {
    gst_toc_entry_append_sub_entry (edition_info, l->data);
    gst_toc_entry_append_sub_entry (internal_edition_info, il->data);
  }
  g_list_free (subentries);
  g_list_free (internal_subentries);

  if (is_hidden == 0 && subentries != NULL && ret == GST_FLOW_OK) {
    gst_toc_append_entry (toc, edition_info);
    gst_toc_append_entry (internal_toc, internal_edition_info);
  } else {
    GST_DEBUG_OBJECT (common->sinkpad,
        "Skipping empty or hidden edition in the chapters TOC");
    gst_toc_entry_unref (edition_info);
    gst_toc_entry_unref (internal_edition_info);
  }

  return ret;
}

GstFlowReturn
gst_matroska_read_common_parse_chapters (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml)
{
  guint32 id;
  GstFlowReturn ret = GST_FLOW_OK;
  GstToc *toc, *internal_toc;

  DEBUG_ELEMENT_START (common, ebml, "Chapters");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "Chapters", ret);
    return ret;
  }

  /* FIXME: create CURRENT toc as well */
  toc = gst_toc_new (GST_TOC_SCOPE_GLOBAL);
  internal_toc = gst_toc_new (GST_TOC_SCOPE_GLOBAL);

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_EDITIONENTRY:
        ret = gst_matroska_read_common_parse_chapter_edition (common, ebml,
            toc, internal_toc);
        break;

      default:
        ret =
            gst_matroska_read_common_parse_skip (common, ebml, "Chapters", id);
        break;
    }
  }

  if (gst_toc_get_entries (toc) != NULL) {
    gst_matroska_read_common_postprocess_toc_entries (gst_toc_get_entries (toc),
        common->segment.duration, "");
    /* no need to postprocess internal_toc as we don't need to keep track
     * of start / end and tags (only UIDs) */

    common->toc = toc;
    common->internal_toc = internal_toc;
  } else {
    gst_toc_unref (toc);
    gst_toc_unref (internal_toc);
  }

  common->chapters_parsed = TRUE;

  DEBUG_ELEMENT_STOP (common, ebml, "Chapters", ret);
  return ret;
}

GstFlowReturn
gst_matroska_read_common_parse_header (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml)
{
  GstFlowReturn ret;
  gchar *doctype;
  guint version;
  guint32 id;

  /* this function is the first to be called */

  /* default init */
  doctype = NULL;
  version = 1;

  ret = gst_ebml_peek_id (ebml, &id);
  if (ret != GST_FLOW_OK)
    return ret;

  GST_DEBUG_OBJECT (common->sinkpad, "id: %08x", id);

  if (id != GST_EBML_ID_HEADER) {
    GST_ERROR_OBJECT (common->sinkpad, "Failed to read header");
    goto exit;
  }

  ret = gst_ebml_read_master (ebml, &id);
  if (ret != GST_FLOW_OK)
    return ret;

  while (gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    ret = gst_ebml_peek_id (ebml, &id);
    if (ret != GST_FLOW_OK)
      goto exit_error;

    switch (id) {
        /* is our read version up-to-date? */
      case GST_EBML_ID_EBMLREADVERSION:{
        guint64 num;

        ret = gst_ebml_read_uint (ebml, &id, &num);
        if (ret != GST_FLOW_OK)
          goto exit_error;
        if (num != GST_EBML_VERSION) {
          GST_ERROR_OBJECT (common->sinkpad,
              "Unsupported EBML version %" G_GUINT64_FORMAT, num);
          goto exit_error;
        }

        GST_DEBUG_OBJECT (common->sinkpad,
            "EbmlReadVersion: %" G_GUINT64_FORMAT, num);
        break;
      }

        /* we only handle 8 byte lengths at max */
      case GST_EBML_ID_EBMLMAXSIZELENGTH:{
        guint64 num;

        ret = gst_ebml_read_uint (ebml, &id, &num);
        if (ret != GST_FLOW_OK)
          goto exit_error;
        if (num > sizeof (guint64)) {
          GST_ERROR_OBJECT (common->sinkpad,
              "Unsupported EBML maximum size %" G_GUINT64_FORMAT, num);
          return GST_FLOW_ERROR;
        }
        GST_DEBUG_OBJECT (common->sinkpad,
            "EbmlMaxSizeLength: %" G_GUINT64_FORMAT, num);
        break;
      }

        /* we handle 4 byte IDs at max */
      case GST_EBML_ID_EBMLMAXIDLENGTH:{
        guint64 num;

        ret = gst_ebml_read_uint (ebml, &id, &num);
        if (ret != GST_FLOW_OK)
          goto exit_error;
        if (num > sizeof (guint32)) {
          GST_ERROR_OBJECT (common->sinkpad,
              "Unsupported EBML maximum ID %" G_GUINT64_FORMAT, num);
          return GST_FLOW_ERROR;
        }
        GST_DEBUG_OBJECT (common->sinkpad,
            "EbmlMaxIdLength: %" G_GUINT64_FORMAT, num);
        break;
      }

      case GST_EBML_ID_DOCTYPE:{
        gchar *text;

        ret = gst_ebml_read_ascii (ebml, &id, &text);
        if (ret != GST_FLOW_OK)
          goto exit_error;

        GST_DEBUG_OBJECT (common->sinkpad, "EbmlDocType: %s",
            GST_STR_NULL (text));

        g_free (doctype);
        doctype = text;
        break;
      }

      case GST_EBML_ID_DOCTYPEREADVERSION:{
        guint64 num;

        ret = gst_ebml_read_uint (ebml, &id, &num);
        if (ret != GST_FLOW_OK)
          goto exit_error;
        version = num;
        GST_DEBUG_OBJECT (common->sinkpad,
            "EbmlReadVersion: %" G_GUINT64_FORMAT, num);
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml,
            "EBML header", id);
        if (ret != GST_FLOW_OK)
          goto exit_error;
        break;

        /* we ignore these two, as they don't tell us anything we care about */
      case GST_EBML_ID_EBMLVERSION:
      case GST_EBML_ID_DOCTYPEVERSION:
        ret = gst_ebml_read_skip (ebml);
        if (ret != GST_FLOW_OK)
          goto exit_error;
        break;
    }
  }

exit:

  if ((doctype != NULL && !strcmp (doctype, GST_MATROSKA_DOCTYPE_MATROSKA)) ||
      (doctype != NULL && !strcmp (doctype, GST_MATROSKA_DOCTYPE_WEBM)) ||
      (doctype == NULL)) {
    if (version <= 2) {
      if (doctype) {
        GST_INFO_OBJECT (common->sinkpad, "Input is %s version %d", doctype,
            version);
        if (!strcmp (doctype, GST_MATROSKA_DOCTYPE_WEBM))
          common->is_webm = TRUE;
      } else {
        GST_WARNING_OBJECT (common->sinkpad,
            "Input is EBML without doctype, assuming " "matroska (version %d)",
            version);
      }
      ret = GST_FLOW_OK;
    } else {
      GST_ELEMENT_ERROR (common, STREAM, DEMUX, (NULL),
          ("Demuxer version (2) is too old to read %s version %d",
              GST_STR_NULL (doctype), version));
      ret = GST_FLOW_ERROR;
    }
  } else {
    GST_ELEMENT_ERROR (common, STREAM, WRONG_TYPE, (NULL),
        ("Input is not a matroska stream (doctype=%s)", doctype));
    ret = GST_FLOW_ERROR;
  }

exit_error:

  g_free (doctype);

  return ret;
}

static GstFlowReturn
gst_matroska_read_common_parse_index_cuetrack (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, guint * nentries)
{
  guint32 id;
  GstFlowReturn ret;
  GstMatroskaIndex idx;

  idx.pos = (guint64) - 1;
  idx.track = 0;
  idx.time = GST_CLOCK_TIME_NONE;
  idx.block = 1;

  DEBUG_ELEMENT_START (common, ebml, "CueTrackPositions");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "CueTrackPositions", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* track number */
      case GST_MATROSKA_ID_CUETRACK:
      {
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num == 0) {
          idx.track = 0;
          GST_WARNING_OBJECT (common->sinkpad, "Invalid CueTrack 0");
          break;
        }

        GST_DEBUG_OBJECT (common->sinkpad, "CueTrack: %" G_GUINT64_FORMAT, num);
        idx.track = num;
        break;
      }

        /* position in file */
      case GST_MATROSKA_ID_CUECLUSTERPOSITION:
      {
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num > G_MAXINT64) {
          GST_WARNING_OBJECT (common->sinkpad,
              "CueClusterPosition %" G_GUINT64_FORMAT " too large", num);
          break;
        }

        idx.pos = num;
        break;
      }

        /* number of block in the cluster */
      case GST_MATROSKA_ID_CUEBLOCKNUMBER:
      {
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num == 0) {
          GST_WARNING_OBJECT (common->sinkpad, "Invalid CueBlockNumber 0");
          break;
        }

        GST_DEBUG_OBJECT (common->sinkpad, "CueBlockNumber: %" G_GUINT64_FORMAT,
            num);
        idx.block = num;

        /* mild sanity check, disregard strange cases ... */
        if (idx.block > G_MAXUINT16) {
          GST_DEBUG_OBJECT (common->sinkpad, "... looks suspicious, ignoring");
          idx.block = 1;
        }
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml,
            "CueTrackPositions", id);
        break;

      case GST_MATROSKA_ID_CUECODECSTATE:
      case GST_MATROSKA_ID_CUEREFERENCE:
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "CueTrackPositions", ret);

  /* (e.g.) lavf typically creates entries without a block number,
   * which is bogus and leads to contradictory information */
  if (common->index->len) {
    GstMatroskaIndex *last_idx;

    last_idx = &g_array_index (common->index, GstMatroskaIndex,
        common->index->len - 1);
    if (last_idx->block == idx.block && last_idx->pos == idx.pos &&
        last_idx->track == idx.track && idx.time > last_idx->time) {
      GST_DEBUG_OBJECT (common->sinkpad, "Cue entry refers to same location, "
          "but has different time than previous entry; discarding");
      idx.track = 0;
    }
  }

  if ((ret == GST_FLOW_OK || ret == GST_FLOW_EOS)
      && idx.pos != (guint64) - 1 && idx.track > 0) {
    g_array_append_val (common->index, idx);
    (*nentries)++;
  } else if (ret == GST_FLOW_OK || ret == GST_FLOW_EOS) {
    GST_DEBUG_OBJECT (common->sinkpad,
        "CueTrackPositions without valid content");
  }

  return ret;
}

static GstFlowReturn
gst_matroska_read_common_parse_index_pointentry (GstMatroskaReadCommon *
    common, GstEbmlRead * ebml)
{
  guint32 id;
  GstFlowReturn ret;
  GstClockTime time = GST_CLOCK_TIME_NONE;
  guint nentries = 0;

  DEBUG_ELEMENT_START (common, ebml, "CuePoint");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "CuePoint", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* one single index entry ('point') */
      case GST_MATROSKA_ID_CUETIME:
      {
        if ((ret = gst_ebml_read_uint (ebml, &id, &time)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (common->sinkpad, "CueTime: %" G_GUINT64_FORMAT, time);
        time = time * common->time_scale;
        break;
      }

        /* position in the file + track to which it belongs */
      case GST_MATROSKA_ID_CUETRACKPOSITIONS:
      {
        ret = gst_matroska_read_common_parse_index_cuetrack (common, ebml,
            &nentries);
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml, "CuePoint",
            id);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "CuePoint", ret);

  if (nentries > 0) {
    if (time == GST_CLOCK_TIME_NONE) {
      GST_WARNING_OBJECT (common->sinkpad, "CuePoint without valid time");
      g_array_remove_range (common->index, common->index->len - nentries,
          nentries);
    } else {
      gint i;

      for (i = common->index->len - nentries; i < common->index->len; i++) {
        GstMatroskaIndex *idx =
            &g_array_index (common->index, GstMatroskaIndex, i);

        idx->time = time;
        GST_DEBUG_OBJECT (common->sinkpad, "Index entry: pos=%" G_GUINT64_FORMAT
            ", time=%" GST_TIME_FORMAT ", track=%u, block=%u", idx->pos,
            GST_TIME_ARGS (idx->time), (guint) idx->track, (guint) idx->block);
      }
    }
  } else {
    GST_DEBUG_OBJECT (common->sinkpad, "Empty CuePoint");
  }

  return ret;
}

gint
gst_matroska_read_common_stream_from_num (GstMatroskaReadCommon * common,
    guint track_num)
{
  guint n;

  g_assert (common->src->len == common->num_streams);
  for (n = 0; n < common->src->len; n++) {
    GstMatroskaTrackContext *context = g_ptr_array_index (common->src, n);

    if (context->num == track_num) {
      return n;
    }
  }

  if (n == common->num_streams)
    GST_WARNING_OBJECT (common->sinkpad,
        "Failed to find corresponding pad for tracknum %d", track_num);

  return -1;
}

GstFlowReturn
gst_matroska_read_common_parse_index (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml)
{
  guint32 id;
  GstFlowReturn ret = GST_FLOW_OK;
  guint i;

  if (common->index)
    g_array_free (common->index, TRUE);
  common->index =
      g_array_sized_new (FALSE, FALSE, sizeof (GstMatroskaIndex), 128);

  DEBUG_ELEMENT_START (common, ebml, "Cues");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "Cues", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* one single index entry ('point') */
      case GST_MATROSKA_ID_POINTENTRY:
        ret = gst_matroska_read_common_parse_index_pointentry (common, ebml);
        break;

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml, "Cues", id);
        break;
    }
  }
  DEBUG_ELEMENT_STOP (common, ebml, "Cues", ret);

  /* Sort index by time, smallest time first, for easier searching */
  g_array_sort (common->index, (GCompareFunc) gst_matroska_index_compare);

  /* Now sort the track specific index entries into their own arrays */
  for (i = 0; i < common->index->len; i++) {
    GstMatroskaIndex *idx = &g_array_index (common->index, GstMatroskaIndex,
        i);
    gint track_num;
    GstMatroskaTrackContext *ctx;

#if 0
    if (common->element_index) {
      gint writer_id;

      if (idx->track != 0 &&
          (track_num =
              gst_matroska_read_common_stream_from_num (common,
                  idx->track)) != -1) {
        ctx = g_ptr_array_index (common->src, track_num);

        if (ctx->index_writer_id == -1)
          gst_index_get_writer_id (common->element_index,
              GST_OBJECT (ctx->pad), &ctx->index_writer_id);
        writer_id = ctx->index_writer_id;
      } else {
        if (common->element_index_writer_id == -1)
          gst_index_get_writer_id (common->element_index, GST_OBJECT (common),
              &common->element_index_writer_id);
        writer_id = common->element_index_writer_id;
      }

      GST_LOG_OBJECT (common->sinkpad,
          "adding association %" GST_TIME_FORMAT "-> %" G_GUINT64_FORMAT
          " for writer id %d", GST_TIME_ARGS (idx->time), idx->pos, writer_id);
      gst_index_add_association (common->element_index, writer_id,
          GST_ASSOCIATION_FLAG_KEY_UNIT, GST_FORMAT_TIME, idx->time,
          GST_FORMAT_BYTES, idx->pos + common->ebml_segment_start, NULL);
    }
#endif

    if (idx->track == 0)
      continue;

    track_num = gst_matroska_read_common_stream_from_num (common, idx->track);
    if (track_num == -1)
      continue;

    ctx = g_ptr_array_index (common->src, track_num);

    if (ctx->index_table == NULL)
      ctx->index_table =
          g_array_sized_new (FALSE, FALSE, sizeof (GstMatroskaIndex), 128);

    g_array_append_vals (ctx->index_table, idx, 1);
  }

  common->index_parsed = TRUE;

  /* sanity check; empty index normalizes to no index */
  if (common->index->len == 0) {
    g_array_free (common->index, TRUE);
    common->index = NULL;
  }

  return ret;
}

GstFlowReturn
gst_matroska_read_common_parse_info (GstMatroskaReadCommon * common,
    GstElement * el, GstEbmlRead * ebml)
{
  GstFlowReturn ret = GST_FLOW_OK;
  gdouble dur_f = -1.0;
  guint32 id;

  DEBUG_ELEMENT_START (common, ebml, "SegmentInfo");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "SegmentInfo", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
        /* cluster timecode */
      case GST_MATROSKA_ID_TIMECODESCALE:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;


        GST_DEBUG_OBJECT (common->sinkpad, "TimeCodeScale: %" G_GUINT64_FORMAT,
            num);
        common->time_scale = num;
        break;
      }

      case GST_MATROSKA_ID_DURATION:{
        if ((ret = gst_ebml_read_float (ebml, &id, &dur_f)) != GST_FLOW_OK)
          break;

        if (dur_f <= 0.0) {
          GST_WARNING_OBJECT (common->sinkpad, "Invalid duration %lf", dur_f);
          break;
        }

        GST_DEBUG_OBJECT (common->sinkpad, "Duration: %lf", dur_f);
        break;
      }

      case GST_MATROSKA_ID_WRITINGAPP:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (common->sinkpad, "WritingApp: %s",
            GST_STR_NULL (text));
        common->writing_app = text;
        break;
      }

      case GST_MATROSKA_ID_MUXINGAPP:{
        gchar *text;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (common->sinkpad, "MuxingApp: %s",
            GST_STR_NULL (text));
        common->muxing_app = text;
        break;
      }

      case GST_MATROSKA_ID_DATEUTC:{
        gint64 time;
        GstDateTime *datetime;
        GstTagList *taglist;

        if ((ret = gst_ebml_read_date (ebml, &id, &time)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (common->sinkpad, "DateUTC: %" G_GINT64_FORMAT, time);
        common->created = time;
        datetime =
            gst_date_time_new_from_unix_epoch_utc_usecs (time / GST_USECOND);
        taglist = gst_tag_list_new (GST_TAG_DATE_TIME, datetime, NULL);
        gst_tag_list_set_scope (taglist, GST_TAG_SCOPE_GLOBAL);
        gst_matroska_read_common_found_global_tag (common, el, taglist);
        gst_date_time_unref (datetime);
        break;
      }

      case GST_MATROSKA_ID_TITLE:{
        gchar *text;
        GstTagList *taglist;

        if ((ret = gst_ebml_read_utf8 (ebml, &id, &text)) != GST_FLOW_OK)
          break;

        GST_DEBUG_OBJECT (common->sinkpad, "Title: %s", GST_STR_NULL (text));
        taglist = gst_tag_list_new (GST_TAG_TITLE, text, NULL);
        gst_tag_list_set_scope (taglist, GST_TAG_SCOPE_GLOBAL);
        gst_matroska_read_common_found_global_tag (common, el, taglist);
        g_free (text);
        break;
      }

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml,
            "SegmentInfo", id);
        break;

        /* fall through */
      case GST_MATROSKA_ID_SEGMENTUID:
      case GST_MATROSKA_ID_SEGMENTFILENAME:
      case GST_MATROSKA_ID_PREVUID:
      case GST_MATROSKA_ID_PREVFILENAME:
      case GST_MATROSKA_ID_NEXTUID:
      case GST_MATROSKA_ID_NEXTFILENAME:
      case GST_MATROSKA_ID_SEGMENTFAMILY:
      case GST_MATROSKA_ID_CHAPTERTRANSLATE:
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  if (dur_f > 0.0) {
    GstClockTime dur_u;

    dur_u = gst_gdouble_to_guint64 (dur_f *
        gst_guint64_to_gdouble (common->time_scale));
    if (GST_CLOCK_TIME_IS_VALID (dur_u) && dur_u <= G_MAXINT64)
      common->segment.duration = dur_u;
  }

  DEBUG_ELEMENT_STOP (common, ebml, "SegmentInfo", ret);

  common->segmentinfo_parsed = TRUE;

  return ret;
}

static GstFlowReturn
gst_matroska_read_common_parse_metadata_id_simple_tag (GstMatroskaReadCommon *
    common, GstEbmlRead * ebml, GstTagList ** p_taglist, gchar * parent)
{
  /* FIXME: check if there are more useful mappings */
  static const struct
  {
    const gchar *matroska_tagname;
    const gchar *gstreamer_tagname;
  }

  /* *INDENT-OFF* */
  tag_conv[] = {
    {
      /* The following list has the _same_ order as the one in Matroska spec. Please, don't mess it up. */
      /* TODO: Nesting information:
         ORIGINAL A special tag that is meant to have other tags inside (using nested tags) to describe the original work of art that this item is based on. All tags in this list can be used "under" the ORIGINAL tag like LYRICIST, PERFORMER, etc.
         SAMPLE A tag that contains other tags to describe a sample used in the targeted item taken from another work of art. All tags in this list can be used "under" the SAMPLE tag like TITLE, ARTIST, DATE_RELEASED, etc.
         COUNTRY The name of the country (biblio ISO-639-2) that is meant to have other tags inside (using nested tags) to country specific information about the item. All tags in this list can be used "under" the COUNTRY_SPECIFIC tag like LABEL, PUBLISH_RATING, etc.
       */

      /* Organizational Information */
    GST_MATROSKA_TAG_ID_TOTAL_PARTS, GST_TAG_TRACK_COUNT}, {
    GST_MATROSKA_TAG_ID_PART_NUMBER, GST_TAG_TRACK_NUMBER}, {
      /* TODO: PART_OFFSET A number to add to PART_NUMBER when the parts at that level don't start at 1. (e.g. if TargetType is TRACK, the track number of the second audio CD) */

      /* Titles */
    GST_MATROSKA_TAG_ID_SUBTITLE, GST_TAG_TITLE}, {     /* Sub Title of the entity. Since we're concat'ing all title-like entities anyway, might as well add the sub-title. */
    GST_MATROSKA_TAG_ID_TITLE, GST_TAG_TITLE}, {
    GST_MATROSKA_TAG_ID_ALBUM, GST_TAG_ALBUM}, {        /* Matroska spec does NOT have this tag! Dunno what it was doing here, probably for compatibility. */

      /* TODO: Nested Information:
         URL URL corresponding to the tag it's included in.
         SORT_WITH A child element to indicate what alternative value the parent tag can have to be sorted, for example "Pet Shop Boys" instead of "The Pet Shop Boys". Or "Marley Bob" and "Marley Ziggy" (no comma needed).
         INSTRUMENTS The instruments that are being used/played, separated by a comma. It should be a child of the following tags: ARTIST, LEAD_PERFORMER or ACCOMPANIMENT.
         EMAIL Email corresponding to the tag it's included in.
         ADDRESS The physical address of the entity. The address should include a country code. It can be useful for a recording label.
         FAX The fax number corresponding to the tag it's included in. It can be useful for a recording label.
         PHONE The phone number corresponding to the tag it's included in. It can be useful for a recording label.
       */

      /* Entities */
    GST_MATROSKA_TAG_ID_ARTIST, GST_TAG_ARTIST}, {
    GST_MATROSKA_TAG_ID_LEAD_PERFORMER, GST_TAG_PERFORMER}, {
    GST_MATROSKA_TAG_ID_ACCOMPANIMENT, GST_TAG_PERFORMER}, {    /* Band/orchestra/accompaniment/musician. This is akin to the TPE2 tag in ID3. */
    GST_MATROSKA_TAG_ID_COMPOSER, GST_TAG_COMPOSER}, {
      /* ARRANGER The person who arranged the piece, e.g., Ravel. */
    GST_MATROSKA_TAG_ID_LYRICS, GST_TAG_LYRICS}, {      /* The lyrics corresponding to a song (in case audio synchronization is not known or as a doublon to a subtitle track). Editing this value when subtitles are found should also result in editing the subtitle track for more consistency. */
      /* LYRICIST The person who wrote the lyrics for a musical item. This is akin to the TEXT tag in ID3. */
    GST_MATROSKA_TAG_ID_CONDUCTOR, GST_TAG_PERFORMER}, {        /* Conductor/performer refinement. This is akin to the TPE3 tag in ID3. */
      /* DIRECTOR This is akin to the IART tag in RIFF. */
    GST_MATROSKA_TAG_ID_AUTHOR, GST_TAG_ARTIST}, {
      /* ASSISTANT_DIRECTOR The name of the assistant director. */
      /* DIRECTOR_OF_PHOTOGRAPHY The name of the director of photography, also known as cinematographer. This is akin to the ICNM tag in Extended RIFF. */
      /* SOUND_ENGINEER The name of the sound engineer or sound recordist. */
      /* ART_DIRECTOR The person who oversees the artists and craftspeople who build the sets. */
      /* PRODUCTION_DESIGNER Artist responsible for designing the overall visual appearance of a movie. */
      /* CHOREGRAPHER The name of the choregrapher */
      /* COSTUME_DESIGNER The name of the costume designer */
      /* ACTOR An actor or actress playing a role in this movie. This is the person's real name, not the character's name the person is playing. */
      /* CHARACTER The name of the character an actor or actress plays in this movie. This should be a sub-tag of an ACTOR tag in order not to cause ambiguities. */
      /* WRITTEN_BY The author of the story or script (used for movies and TV shows). */
      /* SCREENPLAY_BY The author of the screenplay or scenario (used for movies and TV shows). */
      /* EDITED_BY This is akin to the IEDT tag in Extended RIFF. */
      /* PRODUCER Produced by. This is akin to the IPRO tag in Extended RIFF. */
      /* COPRODUCER The name of a co-producer. */
      /* EXECUTIVE_PRODUCER The name of an executive producer. */
      /* DISTRIBUTED_BY This is akin to the IDST tag in Extended RIFF. */
      /* MASTERED_BY The engineer who mastered the content for a physical medium or for digital distribution. */
    GST_MATROSKA_TAG_ID_ENCODED_BY, GST_TAG_ENCODED_BY}, {      /* This is akin to the TENC tag in ID3. */
      /* MIXED_BY DJ mix by the artist specified */
      /* REMIXED_BY Interpreted, remixed, or otherwise modified by. This is akin to the TPE4 tag in ID3. */
      /* PRODUCTION_STUDIO This is akin to the ISTD tag in Extended RIFF. */
      /* THANKS_TO A very general tag for everyone else that wants to be listed. */
      /* PUBLISHER This is akin to the TPUB tag in ID3. */
      /* LABEL The record label or imprint on the disc. */
      /* Search / Classification */
    GST_MATROSKA_TAG_ID_GENRE, GST_TAG_GENRE}, {
      /* MOOD Intended to reflect the mood of the item with a few keywords, e.g. "Romantic", "Sad" or "Uplifting". The format follows that of the TMOO tag in ID3. */
      /* ORIGINAL_MEDIA_TYPE Describes the original type of the media, such as, "DVD", "CD", "computer image," "drawing," "lithograph," and so forth. This is akin to the TMED tag in ID3. */
      /* CONTENT_TYPE The type of the item. e.g. Documentary, Feature Film, Cartoon, Music Video, Music, Sound FX, ... */
      /* SUBJECT Describes the topic of the file, such as "Aerial view of Seattle." */
    GST_MATROSKA_TAG_ID_DESCRIPTION, GST_TAG_DESCRIPTION}, {    /* A short description of the content, such as "Two birds flying." */
    GST_MATROSKA_TAG_ID_KEYWORDS, GST_TAG_KEYWORDS}, {  /* Keywords to the item separated by a comma, used for searching. */
      /* SUMMARY A plot outline or a summary of the story. */
      /* SYNOPSIS A description of the story line of the item. */
      /* INITIAL_KEY The initial key that a musical track starts in. The format is identical to ID3. */
      /* PERIOD Describes the period that the piece is from or about. For example, "Renaissance". */
      /* LAW_RATING Depending on the country it's the format of the rating of a movie (P, R, X in the USA, an age in other countries or a URI defining a logo). */
      /* ICRA The ICRA content rating for parental control. (Previously RSACi) */

      /* Temporal Information */
    GST_MATROSKA_TAG_ID_DATE_RELEASED, GST_TAG_DATE}, { /* The time that the item was originally released. This is akin to the TDRL tag in ID3. */
    GST_MATROSKA_TAG_ID_DATE_RECORDED, GST_TAG_DATE}, { /* The time that the recording began. This is akin to the TDRC tag in ID3. */
    GST_MATROSKA_TAG_ID_DATE_ENCODED, GST_TAG_DATE}, {  /* The time that the encoding of this item was completed began. This is akin to the TDEN tag in ID3. */
    GST_MATROSKA_TAG_ID_DATE_TAGGED, GST_TAG_DATE}, {   /* The time that the tags were done for this item. This is akin to the TDTG tag in ID3. */
    GST_MATROSKA_TAG_ID_DATE_DIGITIZED, GST_TAG_DATE}, {        /* The time that the item was transferred to a digital medium. This is akin to the IDIT tag in RIFF. */
    GST_MATROSKA_TAG_ID_DATE_WRITTEN, GST_TAG_DATE}, {  /* The time that the writing of the music/script began. */
    GST_MATROSKA_TAG_ID_DATE_PURCHASED, GST_TAG_DATE}, {        /* Information on when the file was purchased (see also purchase tags). */
    GST_MATROSKA_TAG_ID_DATE, GST_TAG_DATE}, {  /* Matroska spec does NOT have this tag! Dunno what it was doing here, probably for compatibility. */

      /* Spacial Information */
    GST_MATROSKA_TAG_ID_RECORDING_LOCATION, GST_TAG_GEO_LOCATION_NAME}, {       /* The location where the item was recorded. The countries corresponding to the string, same 2 octets as in Internet domains, or possibly ISO-3166. This code is followed by a comma, then more detailed information such as state/province, another comma, and then city. For example, "US, Texas, Austin". This will allow for easy sorting. It is okay to only store the country, or the country and the state/province. More detailed information can be added after the city through the use of additional commas. In cases where the province/state is unknown, but you want to store the city, simply leave a space between the two commas. For example, "US, , Austin". */
      /* COMPOSITION_LOCATION Location that the item was originally designed/written. The countries corresponding to the string, same 2 octets as in Internet domains, or possibly ISO-3166. This code is followed by a comma, then more detailed information such as state/province, another comma, and then city. For example, "US, Texas, Austin". This will allow for easy sorting. It is okay to only store the country, or the country and the state/province. More detailed information can be added after the city through the use of additional commas. In cases where the province/state is unknown, but you want to store the city, simply leave a space between the two commas. For example, "US, , Austin". */
      /* COMPOSER_NATIONALITY Nationality of the main composer of the item, mostly for classical music. The countries corresponding to the string, same 2 octets as in Internet domains, or possibly ISO-3166. */

      /* Personal */
    GST_MATROSKA_TAG_ID_COMMENT, GST_TAG_COMMENT}, {    /* Any comment related to the content. */
    GST_MATROSKA_TAG_ID_COMMENTS, GST_TAG_COMMENT}, {   /* Matroska spec does NOT have this tag! Dunno what it was doing here, probably for compatibility. */
      /* PLAY_COUNTER The number of time the item has been played. */
      /* TODO: RATING A numeric value defining how much a person likes the song/movie. The number is between 0 and 5 with decimal values possible (e.g. 2.7), 5(.0) being the highest possible rating. Other rating systems with different ranges will have to be scaled. */

      /* Technical Information */
    GST_MATROSKA_TAG_ID_ENCODER, GST_TAG_ENCODER}, {
      /* ENCODER_SETTINGS A list of the settings used for encoding this item. No specific format. */
    GST_MATROSKA_TAG_ID_BPS, GST_TAG_BITRATE}, {
    GST_MATROSKA_TAG_ID_BITSPS, GST_TAG_BITRATE}, {     /* Matroska spec does NOT have this tag! Dunno what it was doing here, probably for compatibility. */
      /* WONTFIX (already handled in another way): FPS The average frames per second of the specified item. This is typically the average number of Blocks per second. In the event that lacing is used, each laced chunk is to be counted as a separate frame. */
    GST_MATROSKA_TAG_ID_BPM, GST_TAG_BEATS_PER_MINUTE}, {
      /* MEASURE In music, a measure is a unit of time in Western music like "4/4". It represents a regular grouping of beats, a meter, as indicated in musical notation by the time signature.. The majority of the contemporary rock and pop music you hear on the radio these days is written in the 4/4 time signature. */
      /* TUNING It is saved as a frequency in hertz to allow near-perfect tuning of instruments to the same tone as the musical piece (e.g. "441.34" in Hertz). The default value is 440.0 Hz. */
      /* TODO: REPLAYGAIN_GAIN The gain to apply to reach 89dB SPL on playback. This is based on the Replay Gain standard. Note that ReplayGain information can be found at all TargetType levels (track, album, etc). */
      /* TODO: REPLAYGAIN_PEAK The maximum absolute peak value of the item. This is based on the Replay Gain standard. */

      /* Identifiers */
    GST_MATROSKA_TAG_ID_ISRC, GST_TAG_ISRC}, {
      /* MCDI This is a binary dump of the TOC of the CDROM that this item was taken from. This holds the same information as the MCDI in ID3. */
      /* ISBN International Standard Book Number */
      /* BARCODE EAN-13 (European Article Numbering) or UPC-A (Universal Product Code) bar code identifier */
      /* CATALOG_NUMBER A label-specific string used to identify the release (TIC 01 for example). */
      /* LABEL_CODE A 4-digit or 5-digit number to identify the record label, typically printed as (LC) xxxx or (LC) 0xxxx on CDs medias or covers (only the number is stored). */
      /* LCCN Library of Congress Control Number */

      /* Commercial */
      /* PURCHASE_ITEM URL to purchase this file. This is akin to the WPAY tag in ID3. */
      /* PURCHASE_INFO Information on where to purchase this album. This is akin to the WCOM tag in ID3. */
      /* PURCHASE_OWNER Information on the person who purchased the file. This is akin to the TOWN tag in ID3. */
      /* PURCHASE_PRICE The amount paid for entity. There should only be a numeric value in here. Only numbers, no letters or symbols other than ".". For instance, you would store "15.59" instead of "$15.59USD". */
      /* PURCHASE_CURRENCY The currency type used to pay for the entity. Use ISO-4217 for the 3 letter currency code. */

      /* Legal */
    GST_MATROSKA_TAG_ID_COPYRIGHT, GST_TAG_COPYRIGHT}, {
    GST_MATROSKA_TAG_ID_PRODUCTION_COPYRIGHT, GST_TAG_COPYRIGHT}, {     /* The copyright information as per the production copyright holder. This is akin to the TPRO tag in ID3. */
    GST_MATROSKA_TAG_ID_LICENSE, GST_TAG_LICENSE}, {    /* The license applied to the content (like Creative Commons variants). */
    GST_MATROSKA_TAG_ID_TERMS_OF_USE, GST_TAG_LICENSE}
  };
  /* *INDENT-ON* */
  static const struct
  {
    const gchar *matroska_tagname;
    const gchar *gstreamer_tagname;
  }

  /* *INDENT-OFF* */
  child_tag_conv[] = {
    {
    "TITLE/SORT_WITH=", GST_TAG_TITLE_SORTNAME}, {
    "ARTIST/SORT_WITH=", GST_TAG_ARTIST_SORTNAME}, {
      /* ALBUM-stuff is handled elsewhere */
    "COMPOSER/SORT_WITH=", GST_TAG_TITLE_SORTNAME}, {
    "ORIGINAL/URL=", GST_TAG_LOCATION}, {
      /* EMAIL, PHONE, FAX all can be mapped to GST_TAG_CONTACT, there is special
       * code for that later.
       */
    "TITLE/URL=", GST_TAG_HOMEPAGE}, {
    "ARTIST/URL=", GST_TAG_HOMEPAGE}, {
    "COPYRIGHT/URL=", GST_TAG_COPYRIGHT_URI}, {
    "LICENSE/URL=", GST_TAG_LICENSE_URI}, {
    "LICENSE/URL=", GST_TAG_LICENSE_URI}
  };
  /* *INDENT-ON* */
  GstFlowReturn ret;
  guint32 id;
  gchar *value = NULL;
  gchar *tag = NULL;
  gchar *name_with_parent = NULL;
  GstTagList *child_taglist = NULL;

  DEBUG_ELEMENT_START (common, ebml, "SimpleTag");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "SimpleTag", ret);
    return ret;
  }

  if (parent)
    child_taglist = *p_taglist;
  else
    child_taglist = gst_tag_list_new_empty ();

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    /* read all sub-entries */

    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_TAGNAME:
        g_free (tag);
        tag = NULL;
        ret = gst_ebml_read_ascii (ebml, &id, &tag);
        GST_DEBUG_OBJECT (common->sinkpad, "TagName: %s", GST_STR_NULL (tag));
        g_free (name_with_parent);
        if (parent != NULL)
          name_with_parent = g_strdup_printf ("%s/%s", parent, tag);
        else
          name_with_parent = g_strdup (tag);
        break;

      case GST_MATROSKA_ID_TAGSTRING:
        g_free (value);
        value = NULL;
        ret = gst_ebml_read_utf8 (ebml, &id, &value);
        GST_DEBUG_OBJECT (common->sinkpad, "TagString: %s",
            GST_STR_NULL (value));
        break;

      case GST_MATROSKA_ID_SIMPLETAG:
        /* Recursive SimpleTag */
        /* This implementation requires tag name of _this_ tag to be known
         * in order to read its children. It's not in the spec, just the way
         * the code is written.
         */
        if (name_with_parent != NULL) {
          ret = gst_matroska_read_common_parse_metadata_id_simple_tag (common,
              ebml, &child_taglist, name_with_parent);
          break;
        }
        /* fall-through */

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml, "SimpleTag",
            id);
        break;

      case GST_MATROSKA_ID_TAGLANGUAGE:
      case GST_MATROSKA_ID_TAGDEFAULT:
      case GST_MATROSKA_ID_TAGBINARY:
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "SimpleTag", ret);

  if (parent && tag && value && *value != '\0') {
    /* Don't bother mapping children tags - parent will do that */
    gchar *key_val;
    /* TODO: read LANGUAGE sub-tag, and use "key[lc]=val" form */
    key_val = g_strdup_printf ("%s=%s", name_with_parent, value);
    gst_tag_list_add (*p_taglist, GST_TAG_MERGE_APPEND,
        GST_TAG_EXTENDED_COMMENT, key_val, NULL);
    g_free (key_val);
  } else if (tag && value && *value != '\0') {
    gboolean matched = FALSE;
    guint i;

    for (i = 0; !matched && i < G_N_ELEMENTS (tag_conv); i++) {
      const gchar *tagname_gst = tag_conv[i].gstreamer_tagname;

      const gchar *tagname_mkv = tag_conv[i].matroska_tagname;

      if (strcmp (tagname_mkv, tag) == 0) {
        GValue dest = { 0, };
        GType dest_type = gst_tag_get_type (tagname_gst);

        /* Ensure that any date string is complete */
        if (dest_type == G_TYPE_DATE) {
          guint year = 1901, month = 1, day = 1;

          /* Dates can be yyyy-MM-dd, yyyy-MM or yyyy, but we need
           * the first type */
          if (sscanf (value, "%04u-%02u-%02u", &year, &month, &day) != 0) {
            g_free (value);
            value = g_strdup_printf ("%04u-%02u-%02u", year, month, day);
          }
        }

        g_value_init (&dest, dest_type);
        if (gst_value_deserialize (&dest, value)) {
          gst_tag_list_add_values (*p_taglist, GST_TAG_MERGE_APPEND,
              tagname_gst, &dest, NULL);
        } else {
          GST_WARNING_OBJECT (common->sinkpad, "Can't transform tag '%s' with "
              "value '%s' to target type '%s'", tag, value,
              g_type_name (dest_type));
        }
        g_value_unset (&dest);
        matched = TRUE;
      }
    }
    if (!matched) {
      gchar *key_val;
      /* TODO: read LANGUAGE sub-tag, and use "key[lc]=val" form */
      key_val = g_strdup_printf ("%s=%s", tag, value);
      gst_tag_list_add (*p_taglist, GST_TAG_MERGE_APPEND,
          GST_TAG_EXTENDED_COMMENT, key_val, NULL);
      g_free (key_val);
    }
  }

  if (!parent) {
    /* Map children tags. This only supports top-anchored mapping. That is,
     * we start at toplevel tag (this tag), and see how its combinations
     * with its children can be mapped. Which means that grandchildren
     * are also combined here, with _this_ tag taken into consideration.
     * If grandchildren can be combined only with children, that combination
     * will not happen.
     */
    gint child_tags_n = gst_tag_list_n_tags (child_taglist);
    if (child_tags_n > 0) {
      gint i;
      for (i = 0; i < child_tags_n; i++) {
        gint j;
        const gchar *child_name = gst_tag_list_nth_tag_name (child_taglist, i);
        guint taglen = gst_tag_list_get_tag_size (child_taglist, child_name);
        for (j = 0; j < taglen; j++) {
          gchar *val;
          gboolean matched = FALSE;
          gchar *val_pre, *val_post;
          gint k;

          if (!gst_tag_list_get_string_index (child_taglist, child_name,
                  j, &val))
            continue;
          if (!strchr (val, '=')) {
            g_free (val);
            continue;
          }
          val_post = g_strdup (strchr (val, '=') + 1);
          val_pre = g_strdup (val);
          *(strchr (val_pre, '=') + 1) = '\0';

          for (k = 0; !matched && k < G_N_ELEMENTS (child_tag_conv); k++) {
            const gchar *tagname_gst = child_tag_conv[k].gstreamer_tagname;

            const gchar *tagname_mkv = child_tag_conv[k].matroska_tagname;

            /* TODO: Once "key[lc]=value" form support is implemented,
             * strip [lc] here. It can't be used in combined tags.
             * If a tag is not combined, leave [lc] as it is.
             */
            if (strcmp (tagname_mkv, val_pre) == 0) {
              GValue dest = { 0, };
              GType dest_type = gst_tag_get_type (tagname_gst);

              g_value_init (&dest, dest_type);
              if (gst_value_deserialize (&dest, val_post)) {
                gst_tag_list_add_values (*p_taglist, GST_TAG_MERGE_APPEND,
                    tagname_gst, &dest, NULL);
              } else {
                GST_WARNING_OBJECT (common->sinkpad,
                    "Can't transform complex tag '%s' " "to target type '%s'",
                    val, g_type_name (dest_type));
              }
              g_value_unset (&dest);
              matched = TRUE;
            }
          }
          if (!matched) {
            gchar *last_slash = strrchr (val_pre, '/');
            if (last_slash) {
              last_slash++;
              if (strcmp (last_slash, "EMAIL=") == 0 ||
                  strcmp (last_slash, "PHONE=") == 0 ||
                  strcmp (last_slash, "ADDRESS=") == 0 ||
                  strcmp (last_slash, "FAX=") == 0) {
                gst_tag_list_add (*p_taglist, GST_TAG_MERGE_APPEND,
                    GST_TAG_CONTACT, val_post, NULL);
                matched = TRUE;
              }
            }
          }
          if (!matched)
            gst_tag_list_add (*p_taglist, GST_TAG_MERGE_APPEND,
                GST_TAG_EXTENDED_COMMENT, val, NULL);
          g_free (val_post);
          g_free (val_pre);
          g_free (val);
        }
      }
    }
    gst_tag_list_unref (child_taglist);
  }

  g_free (tag);
  g_free (value);
  g_free (name_with_parent);

  return ret;
}


static void
gst_matroska_read_common_count_streams (GstMatroskaReadCommon * common,
    gint * a, gint * v, gint * s)
{
  gint i;
  gint video_streams = 0, audio_streams = 0, subtitle_streams = 0;

  for (i = 0; i < common->src->len; i++) {
    GstMatroskaTrackContext *stream;

    stream = g_ptr_array_index (common->src, i);
    if (stream->type == GST_MATROSKA_TRACK_TYPE_VIDEO)
      video_streams += 1;
    else if (stream->type == GST_MATROSKA_TRACK_TYPE_AUDIO)
      audio_streams += 1;
    else if (stream->type == GST_MATROSKA_TRACK_TYPE_SUBTITLE)
      subtitle_streams += 1;
  }
  *v = video_streams;
  *a = audio_streams;
  *s = subtitle_streams;
}


static void
gst_matroska_read_common_apply_target_type_foreach (const GstTagList * list,
    const gchar * tag, gpointer user_data)
{
  guint vallen;
  guint i;
  TargetTypeContext *ctx = (TargetTypeContext *) user_data;

  vallen = gst_tag_list_get_tag_size (list, tag);
  if (vallen == 0)
    return;

  for (i = 0; i < vallen; i++) {
    const GValue *val_ref;

    val_ref = gst_tag_list_get_value_index (list, tag, i);
    if (val_ref == NULL)
      continue;

    /* TODO: use the optional ctx->target_type somehow */
    if (strcmp (tag, GST_TAG_TITLE) == 0) {
      if (ctx->target_type_value >= 70 && !ctx->audio_only) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_SHOW_NAME, val_ref);
        continue;
      } else if (ctx->target_type_value >= 50) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_TITLE, val_ref);
        continue;
      }
    } else if (strcmp (tag, GST_TAG_TITLE_SORTNAME) == 0) {
      if (ctx->target_type_value >= 70 && !ctx->audio_only) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_SHOW_SORTNAME, val_ref);
        continue;
      } else if (ctx->target_type_value >= 50) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_TITLE_SORTNAME, val_ref);
        continue;
      }
    } else if (strcmp (tag, GST_TAG_ARTIST) == 0) {
      if (ctx->target_type_value >= 50) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_ARTIST, val_ref);
        continue;
      }
    } else if (strcmp (tag, GST_TAG_ARTIST_SORTNAME) == 0) {
      if (ctx->target_type_value >= 50) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_ARTIST_SORTNAME, val_ref);
        continue;
      }
    } else if (strcmp (tag, GST_TAG_TRACK_COUNT) == 0) {
      if (ctx->target_type_value >= 60) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_ALBUM_VOLUME_COUNT, val_ref);
        continue;
      }
    } else if (strcmp (tag, GST_TAG_TRACK_NUMBER) == 0) {
      if (ctx->target_type_value >= 60 && !ctx->audio_only) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_SHOW_SEASON_NUMBER, val_ref);
        continue;
      } else if (ctx->target_type_value >= 50 && !ctx->audio_only) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_SHOW_EPISODE_NUMBER, val_ref);
        continue;
      } else if (ctx->target_type_value >= 50) {
        gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND,
            GST_TAG_ALBUM_VOLUME_NUMBER, val_ref);
        continue;
      }
    }
    gst_tag_list_add_value (ctx->result, GST_TAG_MERGE_APPEND, tag, val_ref);
  }
}


static GstTagList *
gst_matroska_read_common_apply_target_type (GstMatroskaReadCommon * common,
    GstTagList * taglist, guint64 target_type_value, gchar * target_type)
{
  TargetTypeContext ctx;
  gint a = 0;
  gint v = 0;
  gint s = 0;

  gst_matroska_read_common_count_streams (common, &a, &v, &s);

  ctx.audio_only = (a > 0 && v == 0 && s == 0);
  ctx.result = gst_tag_list_new_empty ();
  ctx.target_type_value = target_type_value;
  ctx.target_type = target_type;

  gst_tag_list_foreach (taglist,
      gst_matroska_read_common_apply_target_type_foreach, &ctx);

  gst_tag_list_unref (taglist);
  return ctx.result;
}


static GstFlowReturn
gst_matroska_read_common_parse_metadata_id_tag (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, GstTagList ** p_taglist)
{
  guint32 id;
  GstFlowReturn ret;
  GArray *chapter_targets, *edition_targets, *track_targets;
  GstTagList *taglist;
  GList *cur, *internal_cur;
  guint64 target_type_value = 50;
  gchar *target_type = NULL;

  DEBUG_ELEMENT_START (common, ebml, "Tag");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "Tag", ret);
    return ret;
  }

  edition_targets = g_array_new (FALSE, FALSE, sizeof (guint64));
  chapter_targets = g_array_new (FALSE, FALSE, sizeof (guint64));
  track_targets = g_array_new (FALSE, FALSE, sizeof (guint64));
  taglist = gst_tag_list_new_empty ();
  target_type = NULL;

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    /* read all sub-entries */

    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_SIMPLETAG:
        ret = gst_matroska_read_common_parse_metadata_id_simple_tag (common,
            ebml, &taglist, NULL);
        break;

      case GST_MATROSKA_ID_TARGETS:
        g_free (target_type);
        target_type = NULL;
        target_type_value = 50;
        ret = gst_matroska_read_common_parse_metadata_targets (common, ebml,
            edition_targets, chapter_targets, track_targets,
            &target_type_value, &target_type);
        break;

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml, "Tag", id);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "Tag", ret);

  taglist = gst_matroska_read_common_apply_target_type (common, taglist,
      target_type_value, target_type);
  g_free (target_type);

  /* if tag is chapter/edition specific - try to find that entry */
  if (G_UNLIKELY (chapter_targets->len > 0 || edition_targets->len > 0 ||
          track_targets->len > 0)) {
    gint i;
    if (chapter_targets->len > 0 || edition_targets->len > 0) {
      if (common->toc == NULL)
        GST_WARNING_OBJECT (common->sinkpad,
            "Found chapter/edition specific tag, but TOC is not present");
      else {
        cur = gst_toc_get_entries (common->toc);
        internal_cur = gst_toc_get_entries (common->internal_toc);
        while (cur != NULL && internal_cur != NULL) {
          gst_matroska_read_common_parse_toc_tag (cur->data, internal_cur->data,
              edition_targets, chapter_targets, taglist);
          cur = cur->next;
          internal_cur = internal_cur->next;
        }
        common->toc_updated = TRUE;
      }
    }
    for (i = 0; i < track_targets->len; i++) {
      gint j;
      gboolean found = FALSE;
      guint64 tgt = g_array_index (track_targets, guint64, i);

      for (j = 0; j < common->src->len; j++) {
        GstMatroskaTrackContext *stream = g_ptr_array_index (common->src, j);

        if (stream->uid == tgt) {
          gst_tag_list_insert (stream->tags, taglist, GST_TAG_MERGE_REPLACE);
          stream->tags_changed = TRUE;
          found = TRUE;
        }
      }
      if (!found) {
        /* Cache the track taglist: possibly belongs to a track that will be parsed
           later in gst_matroska_demux.c:gst_matroska_demux_add_stream (...) */
        gpointer track_uid = GUINT_TO_POINTER (tgt);
        GstTagList *cached_taglist =
            g_hash_table_lookup (common->cached_track_taglists, track_uid);
        if (cached_taglist)
          gst_tag_list_insert (cached_taglist, taglist, GST_TAG_MERGE_REPLACE);
        else {
          gst_tag_list_ref (taglist);
          g_hash_table_insert (common->cached_track_taglists, track_uid,
              taglist);
        }
        GST_DEBUG_OBJECT (common->sinkpad,
            "Found track-specific tag(s), but track %" G_GUINT64_FORMAT
            " is not known yet, caching", tgt);
      }
    }
  } else
    gst_tag_list_insert (*p_taglist, taglist, GST_TAG_MERGE_APPEND);

  gst_tag_list_unref (taglist);
  g_array_unref (chapter_targets);
  g_array_unref (edition_targets);
  g_array_unref (track_targets);

  return ret;
}

GstFlowReturn
gst_matroska_read_common_parse_metadata (GstMatroskaReadCommon * common,
    GstElement * el, GstEbmlRead * ebml)
{
  GstTagList *taglist;
  GstFlowReturn ret = GST_FLOW_OK;
  guint32 id;
  GList *l;
  guint64 curpos;

  /* Make sure we don't parse a tags element twice and
   * post it's tags twice */
  curpos = gst_ebml_read_get_pos (ebml);
  for (l = common->tags_parsed; l; l = l->next) {
    guint64 *pos = l->data;

    if (*pos == curpos) {
      GST_DEBUG_OBJECT (common->sinkpad,
          "Skipping already parsed Tags at offset %" G_GUINT64_FORMAT, curpos);
      return GST_FLOW_OK;
    }
  }

  common->tags_parsed =
      g_list_prepend (common->tags_parsed, g_slice_new (guint64));
  *((guint64 *) common->tags_parsed->data) = curpos;
  /* fall-through */

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "Tags", ret);
    return ret;
  }

  taglist = gst_tag_list_new_empty ();
  gst_tag_list_set_scope (taglist, GST_TAG_SCOPE_GLOBAL);
  common->toc_updated = FALSE;

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_TAG:
        ret = gst_matroska_read_common_parse_metadata_id_tag (common, ebml,
            &taglist);
        break;

      default:
        ret = gst_matroska_read_common_parse_skip (common, ebml, "Tags", id);
        break;
        /* FIXME: Use to limit the tags to specific pads */
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "Tags", ret);

  if (G_LIKELY (!gst_tag_list_is_empty (taglist)))
    gst_matroska_read_common_found_global_tag (common, el, taglist);
  else
    gst_tag_list_unref (taglist);

  return ret;
}

static GstFlowReturn
gst_matroska_read_common_peek_adapter (GstMatroskaReadCommon * common, guint
    peek, const guint8 ** data)
{
  /* Caller needs to gst_adapter_unmap. */
  *data = gst_adapter_map (common->adapter, peek);
  if (*data == NULL)
    return GST_FLOW_EOS;

  return GST_FLOW_OK;
}

/*
 * Calls pull_range for (offset,size) without advancing our offset
 */
GstFlowReturn
gst_matroska_read_common_peek_bytes (GstMatroskaReadCommon * common, guint64
    offset, guint size, GstBuffer ** p_buf, guint8 ** bytes)
{
  GstFlowReturn ret;

  /* Caching here actually makes much less difference than one would expect.
   * We do it mainly to avoid pulling buffers of 1 byte all the time */
  if (common->cached_buffer) {
    guint64 cache_offset = GST_BUFFER_OFFSET (common->cached_buffer);
    gsize cache_size = gst_buffer_get_size (common->cached_buffer);

    if (cache_offset <= common->offset &&
        (common->offset + size) <= (cache_offset + cache_size)) {
      if (p_buf)
        *p_buf = gst_buffer_copy_region (common->cached_buffer,
            GST_BUFFER_COPY_ALL, common->offset - cache_offset, size);
      if (bytes) {
        if (!common->cached_data) {
          gst_buffer_map (common->cached_buffer, &common->cached_map,
              GST_MAP_READ);
          common->cached_data = common->cached_map.data;
        }
        *bytes = common->cached_data + common->offset - cache_offset;
      }
      return GST_FLOW_OK;
    }
    /* not enough data in the cache, free cache and get a new one */
    if (common->cached_data) {
      gst_buffer_unmap (common->cached_buffer, &common->cached_map);
      common->cached_data = NULL;
    }
    gst_buffer_unref (common->cached_buffer);
    common->cached_buffer = NULL;
  }

  /* refill the cache */
  ret = gst_pad_pull_range (common->sinkpad, common->offset,
      MAX (size, 64 * 1024), &common->cached_buffer);
  if (ret != GST_FLOW_OK) {
    common->cached_buffer = NULL;
    return ret;
  }

  if (gst_buffer_get_size (common->cached_buffer) >= size) {
    if (p_buf)
      *p_buf = gst_buffer_copy_region (common->cached_buffer,
          GST_BUFFER_COPY_ALL, 0, size);
    if (bytes) {
      gst_buffer_map (common->cached_buffer, &common->cached_map, GST_MAP_READ);
      common->cached_data = common->cached_map.data;
      *bytes = common->cached_data;
    }
    return GST_FLOW_OK;
  }

  /* Not possible to get enough data, try a last time with
   * requesting exactly the size we need */
  gst_buffer_unref (common->cached_buffer);
  common->cached_buffer = NULL;

  ret =
      gst_pad_pull_range (common->sinkpad, common->offset, size,
      &common->cached_buffer);
  if (ret != GST_FLOW_OK) {
    GST_DEBUG_OBJECT (common->sinkpad, "pull_range returned %d", ret);
    if (p_buf)
      *p_buf = NULL;
    if (bytes)
      *bytes = NULL;
    return ret;
  }

  if (gst_buffer_get_size (common->cached_buffer) < size) {
    GST_WARNING_OBJECT (common->sinkpad, "Dropping short buffer at offset %"
        G_GUINT64_FORMAT ": wanted %u bytes, got %" G_GSIZE_FORMAT " bytes",
        common->offset, size, gst_buffer_get_size (common->cached_buffer));

    gst_buffer_unref (common->cached_buffer);
    common->cached_buffer = NULL;
    if (p_buf)
      *p_buf = NULL;
    if (bytes)
      *bytes = NULL;
    return GST_FLOW_EOS;
  }

  if (p_buf)
    *p_buf = gst_buffer_copy_region (common->cached_buffer,
        GST_BUFFER_COPY_ALL, 0, size);
  if (bytes) {
    gst_buffer_map (common->cached_buffer, &common->cached_map, GST_MAP_READ);
    common->cached_data = common->cached_map.data;
    *bytes = common->cached_data;
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_matroska_read_common_peek_pull (GstMatroskaReadCommon * common, guint peek,
    guint8 ** data)
{
  return gst_matroska_read_common_peek_bytes (common, common->offset, peek,
      NULL, data);
}

GstFlowReturn
gst_matroska_read_common_peek_id_length_pull (GstMatroskaReadCommon * common,
    GstElement * el, guint32 * _id, guint64 * _length, guint * _needed)
{
  return gst_ebml_peek_id_length (_id, _length, _needed,
      (GstPeekData) gst_matroska_read_common_peek_pull, (gpointer) common, el,
      common->offset);
}

GstFlowReturn
gst_matroska_read_common_peek_id_length_push (GstMatroskaReadCommon * common,
    GstElement * el, guint32 * _id, guint64 * _length, guint * _needed)
{
  GstFlowReturn ret;

  ret = gst_ebml_peek_id_length (_id, _length, _needed,
      (GstPeekData) gst_matroska_read_common_peek_adapter, (gpointer) common,
      el, common->offset);

  gst_adapter_unmap (common->adapter);

  return ret;
}

static GstFlowReturn
gst_matroska_read_common_read_track_encoding (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, GstMatroskaTrackContext * context)
{
  GstMatroskaTrackEncoding enc = { 0, };
  GstFlowReturn ret;
  guint32 id;

  DEBUG_ELEMENT_START (common, ebml, "ContentEncoding");
  /* Set default values */
  enc.scope = 1;
  /* All other default values are 0 */

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "ContentEncoding", ret);
    return ret;
  }

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_CONTENTENCODINGORDER:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (!gst_matroska_read_common_encoding_order_unique (context->encodings,
                num)) {
          GST_ERROR_OBJECT (common->sinkpad,
              "ContentEncodingOrder %" G_GUINT64_FORMAT
              "is not unique for track %" G_GUINT64_FORMAT, num, context->num);
          ret = GST_FLOW_ERROR;
          break;
        }

        GST_DEBUG_OBJECT (common->sinkpad,
            "ContentEncodingOrder: %" G_GUINT64_FORMAT, num);
        enc.order = num;
        break;
      }
      case GST_MATROSKA_ID_CONTENTENCODINGSCOPE:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num > 7 || num == 0) {
          GST_ERROR_OBJECT (common->sinkpad, "Invalid ContentEncodingScope %"
              G_GUINT64_FORMAT, num);
          ret = GST_FLOW_ERROR;
          break;
        }

        GST_DEBUG_OBJECT (common->sinkpad,
            "ContentEncodingScope: %" G_GUINT64_FORMAT, num);
        enc.scope = num;

        break;
      }
      case GST_MATROSKA_ID_CONTENTENCODINGTYPE:{
        guint64 num;

        if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK)
          break;

        if (num > 1) {
          GST_ERROR_OBJECT (common->sinkpad, "Invalid ContentEncodingType %"
              G_GUINT64_FORMAT, num);
          ret = GST_FLOW_ERROR;
          break;
        }

        if ((!common->is_webm) && (num == GST_MATROSKA_ENCODING_ENCRYPTION)) {
          GST_ERROR_OBJECT (common->sinkpad,
              "Encrypted tracks are supported only in WebM");
          ret = GST_FLOW_ERROR;
          break;
        }
        GST_DEBUG_OBJECT (common->sinkpad,
            "ContentEncodingType: %" G_GUINT64_FORMAT, num);
        enc.type = num;
        break;
      }
      case GST_MATROSKA_ID_CONTENTCOMPRESSION:{

        DEBUG_ELEMENT_START (common, ebml, "ContentCompression");

        if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK)
          break;

        while (ret == GST_FLOW_OK &&
            gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
          if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
            break;

          switch (id) {
            case GST_MATROSKA_ID_CONTENTCOMPALGO:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK) {
                break;
              }
              if (num > 3) {
                GST_ERROR_OBJECT (common->sinkpad, "Invalid ContentCompAlgo %"
                    G_GUINT64_FORMAT, num);
                ret = GST_FLOW_ERROR;
                break;
              }
              GST_DEBUG_OBJECT (common->sinkpad,
                  "ContentCompAlgo: %" G_GUINT64_FORMAT, num);
              enc.comp_algo = num;

              break;
            }
            case GST_MATROSKA_ID_CONTENTCOMPSETTINGS:{
              guint8 *data;
              guint64 size;

              if ((ret =
                      gst_ebml_read_binary (ebml, &id, &data,
                          &size)) != GST_FLOW_OK) {
                break;
              }
              enc.comp_settings = data;
              enc.comp_settings_length = size;
              GST_DEBUG_OBJECT (common->sinkpad,
                  "ContentCompSettings of size %" G_GUINT64_FORMAT, size);
              break;
            }
            default:
              GST_WARNING_OBJECT (common->sinkpad,
                  "Unknown ContentCompression subelement 0x%x - ignoring", id);
              ret = gst_ebml_read_skip (ebml);
              break;
          }
        }
        DEBUG_ELEMENT_STOP (common, ebml, "ContentCompression", ret);
        break;
      }

      case GST_MATROSKA_ID_CONTENTENCRYPTION:{

        DEBUG_ELEMENT_START (common, ebml, "ContentEncryption");

        if (enc.type != GST_MATROSKA_ENCODING_ENCRYPTION) {
          GST_WARNING_OBJECT (common->sinkpad,
              "Unexpected to have Content Encryption because it isn't encryption type");
          ret = GST_FLOW_ERROR;
          break;
        }

        if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK)
          break;

        while (ret == GST_FLOW_OK &&
            gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
          if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
            break;

          switch (id) {
            case GST_MATROSKA_ID_CONTENTENCALGO:{
              guint64 num;

              if ((ret = gst_ebml_read_uint (ebml, &id, &num)) != GST_FLOW_OK) {
                break;
              }

              if (num > GST_MATROSKA_TRACK_ENCRYPTION_ALGORITHM_AES) {
                GST_ERROR_OBJECT (common->sinkpad, "Invalid ContentEncAlgo %"
                    G_GUINT64_FORMAT, num);
                ret = GST_FLOW_ERROR;
                break;
              }
              GST_DEBUG_OBJECT (common->sinkpad,
                  "ContentEncAlgo: %" G_GUINT64_FORMAT, num);
              enc.enc_algo = num;

              break;
            }
            case GST_MATROSKA_ID_CONTENTENCAESSETTINGS:{

              DEBUG_ELEMENT_START (common, ebml, "ContentEncAESSettings");

              if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK)
                break;

              while (ret == GST_FLOW_OK &&
                  gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
                if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
                  break;

                switch (id) {
                  case GST_MATROSKA_ID_AESSETTINGSCIPHERMODE:{
                    guint64 num;

                    if ((ret =
                            gst_ebml_read_uint (ebml, &id,
                                &num)) != GST_FLOW_OK) {
                      break;
                    }
                    if (num > 3) {
                      GST_ERROR_OBJECT (common->sinkpad, "Invalid Cipher Mode %"
                          G_GUINT64_FORMAT, num);
                      ret = GST_FLOW_ERROR;
                      break;
                    }
                    GST_DEBUG_OBJECT (common->sinkpad,
                        "ContentEncAESSettings: %" G_GUINT64_FORMAT, num);
                    enc.enc_cipher_mode = num;
                    break;
                  }
                  default:
                    GST_WARNING_OBJECT (common->sinkpad,
                        "Unknown ContentEncAESSettings subelement 0x%x - ignoring",
                        id);
                    ret = gst_ebml_read_skip (ebml);
                    break;
                }
              }
              DEBUG_ELEMENT_STOP (common, ebml, "ContentEncAESSettings", ret);
              break;
            }

            case GST_MATROSKA_ID_CONTENTENCKEYID:{
              guint8 *data;
              guint64 size;
              GstBuffer *keyId_buf;
              GstEvent *event;

              if ((ret =
                      gst_ebml_read_binary (ebml, &id, &data,
                          &size)) != GST_FLOW_OK) {
                break;
              }
              GST_DEBUG_OBJECT (common->sinkpad,
                  "ContentEncrypt KeyID length : %" G_GUINT64_FORMAT, size);
              keyId_buf = gst_buffer_new_wrapped (data, size);

              /* Push an event containing the Key ID into the queues of all streams. */
              /* system_id field is set to GST_PROTECTION_UNSPECIFIED_SYSTEM_ID because it isn't specified neither in WebM nor in Matroska spec. */
              event =
                  gst_event_new_protection
                  (GST_PROTECTION_UNSPECIFIED_SYSTEM_ID, keyId_buf,
                  "matroskademux");
              GST_TRACE_OBJECT (common->sinkpad,
                  "adding protection event for stream %d", context->index);
              g_queue_push_tail (&context->protection_event_queue, event);

              context->protection_info =
                  gst_structure_new ("application/x-cenc", "iv_size",
                  G_TYPE_UINT, 8, "encrypted", G_TYPE_BOOLEAN, TRUE, "kid",
                  GST_TYPE_BUFFER, keyId_buf, NULL);

              gst_buffer_unref (keyId_buf);
              break;
            }
            default:
              GST_WARNING_OBJECT (common->sinkpad,
                  "Unknown ContentEncryption subelement 0x%x - ignoring", id);
              ret = gst_ebml_read_skip (ebml);
              break;
          }
        }
        DEBUG_ELEMENT_STOP (common, ebml, "ContentEncryption", ret);
        break;
      }
      default:
        GST_WARNING_OBJECT (common->sinkpad,
            "Unknown ContentEncoding subelement 0x%x - ignoring", id);
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "ContentEncoding", ret);
  if (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)
    return ret;

  /* TODO: Check if the combination of values is valid */

  g_array_append_val (context->encodings, enc);

  return ret;
}

GstFlowReturn
gst_matroska_read_common_read_track_encodings (GstMatroskaReadCommon * common,
    GstEbmlRead * ebml, GstMatroskaTrackContext * context)
{
  GstFlowReturn ret;
  guint32 id;

  DEBUG_ELEMENT_START (common, ebml, "ContentEncodings");

  if ((ret = gst_ebml_read_master (ebml, &id)) != GST_FLOW_OK) {
    DEBUG_ELEMENT_STOP (common, ebml, "ContentEncodings", ret);
    return ret;
  }

  context->encodings =
      g_array_sized_new (FALSE, FALSE, sizeof (GstMatroskaTrackEncoding), 1);

  while (ret == GST_FLOW_OK && gst_ebml_read_has_remaining (ebml, 1, TRUE)) {
    if ((ret = gst_ebml_peek_id (ebml, &id)) != GST_FLOW_OK)
      break;

    switch (id) {
      case GST_MATROSKA_ID_CONTENTENCODING:
        ret = gst_matroska_read_common_read_track_encoding (common, ebml,
            context);
        break;
      default:
        GST_WARNING_OBJECT (common->sinkpad,
            "Unknown ContentEncodings subelement 0x%x - ignoring", id);
        ret = gst_ebml_read_skip (ebml);
        break;
    }
  }

  DEBUG_ELEMENT_STOP (common, ebml, "ContentEncodings", ret);
  if (ret != GST_FLOW_OK && ret != GST_FLOW_EOS)
    return ret;

  /* Sort encodings according to their order */
  g_array_sort (context->encodings,
      (GCompareFunc) gst_matroska_read_common_encoding_cmp);

  return gst_matroska_decode_content_encodings (context->encodings);
}

void
gst_matroska_read_common_free_parsed_el (gpointer mem, gpointer user_data)
{
  g_slice_free (guint64, mem);
}

void
gst_matroska_read_common_init (GstMatroskaReadCommon * ctx)
{
  ctx->src = NULL;
  ctx->writing_app = NULL;
  ctx->muxing_app = NULL;
  ctx->index = NULL;
  ctx->global_tags = NULL;
  ctx->adapter = gst_adapter_new ();
  ctx->toc = NULL;
  ctx->internal_toc = NULL;
  ctx->toc_updated = FALSE;
  ctx->cached_track_taglists =
      g_hash_table_new_full (NULL, NULL, NULL,
      (GDestroyNotify) gst_tag_list_unref);
}

void
gst_matroska_read_common_finalize (GstMatroskaReadCommon * ctx)
{
  if (ctx->src) {
    g_ptr_array_free (ctx->src, TRUE);
    ctx->src = NULL;
  }

  if (ctx->global_tags) {
    gst_tag_list_unref (ctx->global_tags);
    ctx->global_tags = NULL;
  }

  if (ctx->toc) {
    gst_toc_unref (ctx->toc);
    ctx->toc = NULL;
  }
  if (ctx->internal_toc) {
    gst_toc_unref (ctx->internal_toc);
    ctx->internal_toc = NULL;
  }

  ctx->toc_updated = FALSE;

  g_object_unref (ctx->adapter);
  g_hash_table_remove_all (ctx->cached_track_taglists);
  g_hash_table_unref (ctx->cached_track_taglists);

}

void
gst_matroska_read_common_reset (GstElement * element,
    GstMatroskaReadCommon * ctx)
{
  guint i;

  GST_LOG_OBJECT (ctx->sinkpad, "resetting read context");

  /* reset input */
  ctx->state = GST_MATROSKA_READ_STATE_START;

  /* clean up existing streams if any */
  if (ctx->src) {
    g_assert (ctx->src->len == ctx->num_streams);
    for (i = 0; i < ctx->src->len; i++) {
      GstMatroskaTrackContext *context = g_ptr_array_index (ctx->src, i);

      if (context->pad != NULL)
        gst_element_remove_pad (element, context->pad);

      gst_matroska_track_free (context);
    }
    g_ptr_array_free (ctx->src, TRUE);
  }
  ctx->src = g_ptr_array_new ();
  ctx->num_streams = 0;

  /* reset media info */
  g_free (ctx->writing_app);
  ctx->writing_app = NULL;
  g_free (ctx->muxing_app);
  ctx->muxing_app = NULL;

  /* reset stream type */
  ctx->is_webm = FALSE;
  ctx->has_video = FALSE;

  /* reset indexes */
  if (ctx->index) {
    g_array_free (ctx->index, TRUE);
    ctx->index = NULL;
  }

  /* reset timers */
  ctx->time_scale = 1000000;
  ctx->created = G_MININT64;

  /* cues/tracks/segmentinfo */
  ctx->index_parsed = FALSE;
  ctx->segmentinfo_parsed = FALSE;
  ctx->attachments_parsed = FALSE;
  ctx->chapters_parsed = FALSE;

  /* tags */
  ctx->global_tags_changed = FALSE;
  g_list_foreach (ctx->tags_parsed,
      (GFunc) gst_matroska_read_common_free_parsed_el, NULL);
  g_list_free (ctx->tags_parsed);
  ctx->tags_parsed = NULL;
  if (ctx->global_tags) {
    gst_tag_list_unref (ctx->global_tags);
  }
  ctx->global_tags = gst_tag_list_new_empty ();
  gst_tag_list_set_scope (ctx->global_tags, GST_TAG_SCOPE_GLOBAL);

  gst_segment_init (&ctx->segment, GST_FORMAT_TIME);
  ctx->offset = 0;
  ctx->start_resync_offset = -1;
  ctx->state_to_restore = -1;

  if (ctx->cached_buffer) {
    if (ctx->cached_data) {
      gst_buffer_unmap (ctx->cached_buffer, &ctx->cached_map);
      ctx->cached_data = NULL;
    }
    gst_buffer_unref (ctx->cached_buffer);
    ctx->cached_buffer = NULL;
  }

  /* free chapters TOC if any */
  if (ctx->toc) {
    gst_toc_unref (ctx->toc);
    ctx->toc = NULL;
  }
  if (ctx->internal_toc) {
    gst_toc_unref (ctx->internal_toc);
    ctx->internal_toc = NULL;
  }
  ctx->toc_updated = FALSE;
}

/* call with object lock held */
void
gst_matroska_read_common_reset_streams (GstMatroskaReadCommon * common,
    GstClockTime time, gboolean full)
{
  gint i;

  GST_DEBUG_OBJECT (common->sinkpad, "resetting stream state");

  g_assert (common->src->len == common->num_streams);
  for (i = 0; i < common->src->len; i++) {
    GstMatroskaTrackContext *context = g_ptr_array_index (common->src, i);
    context->pos = time;
    context->set_discont = TRUE;
    context->eos = FALSE;
    context->from_time = GST_CLOCK_TIME_NONE;
    if (context->type == GST_MATROSKA_TRACK_TYPE_VIDEO) {
      GstMatroskaTrackVideoContext *videocontext =
          (GstMatroskaTrackVideoContext *) context;
      /* demux object lock held by caller */
      videocontext->earliest_time = GST_CLOCK_TIME_NONE;
    }
  }
}

gboolean
gst_matroska_read_common_tracknumber_unique (GstMatroskaReadCommon * common,
    guint64 num)
{
  gint i;

  g_assert (common->src->len == common->num_streams);
  for (i = 0; i < common->src->len; i++) {
    GstMatroskaTrackContext *context = g_ptr_array_index (common->src, i);

    if (context->num == num)
      return FALSE;
  }

  return TRUE;
}
