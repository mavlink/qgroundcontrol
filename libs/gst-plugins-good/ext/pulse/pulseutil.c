/*
 *  GStreamer pulseaudio plugin
 *
 *  Copyright (c) 2004-2008 Lennart Poettering
 *
 *  gst-pulse is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  gst-pulse is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with gst-pulse; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 *  USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/audio/audio.h>

#include "pulseutil.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>            /* getpid on UNIX */
#endif
#ifdef HAVE_PROCESS_H
# include <process.h>           /* getpid on win32 */
#endif

static const struct
{
  GstAudioChannelPosition gst_pos;
  pa_channel_position_t pa_pos;
} gst_pa_pos_table[] = {
  {
  GST_AUDIO_CHANNEL_POSITION_MONO, PA_CHANNEL_POSITION_MONO}, {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT, PA_CHANNEL_POSITION_FRONT_LEFT}, {
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT, PA_CHANNEL_POSITION_FRONT_RIGHT}, {
  GST_AUDIO_CHANNEL_POSITION_REAR_CENTER, PA_CHANNEL_POSITION_REAR_CENTER}, {
  GST_AUDIO_CHANNEL_POSITION_REAR_LEFT, PA_CHANNEL_POSITION_REAR_LEFT}, {
  GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT, PA_CHANNEL_POSITION_REAR_RIGHT}, {
  GST_AUDIO_CHANNEL_POSITION_LFE1, PA_CHANNEL_POSITION_LFE}, {
  GST_AUDIO_CHANNEL_POSITION_FRONT_CENTER, PA_CHANNEL_POSITION_FRONT_CENTER}, {
  GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER,
        PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER}, {
  GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER,
        PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER}, {
  GST_AUDIO_CHANNEL_POSITION_SIDE_LEFT, PA_CHANNEL_POSITION_SIDE_LEFT}, {
  GST_AUDIO_CHANNEL_POSITION_SIDE_RIGHT, PA_CHANNEL_POSITION_SIDE_RIGHT}, {
  GST_AUDIO_CHANNEL_POSITION_TOP_CENTER, PA_CHANNEL_POSITION_TOP_CENTER}, {
  GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_LEFT,
        PA_CHANNEL_POSITION_TOP_FRONT_LEFT}, {
  GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_RIGHT,
        PA_CHANNEL_POSITION_TOP_FRONT_RIGHT}, {
  GST_AUDIO_CHANNEL_POSITION_TOP_FRONT_CENTER,
        PA_CHANNEL_POSITION_TOP_FRONT_CENTER}, {
  GST_AUDIO_CHANNEL_POSITION_TOP_REAR_LEFT, PA_CHANNEL_POSITION_TOP_REAR_LEFT}, {
  GST_AUDIO_CHANNEL_POSITION_TOP_REAR_RIGHT,
        PA_CHANNEL_POSITION_TOP_REAR_RIGHT}, {
  GST_AUDIO_CHANNEL_POSITION_TOP_REAR_CENTER,
        PA_CHANNEL_POSITION_TOP_REAR_CENTER}, {
  GST_AUDIO_CHANNEL_POSITION_NONE, PA_CHANNEL_POSITION_INVALID}
};

static gboolean
gstaudioformat_to_pasampleformat (GstAudioFormat format,
    pa_sample_format_t * sf)
{
  switch (format) {
    case GST_AUDIO_FORMAT_U8:
      *sf = PA_SAMPLE_U8;
      break;
    case GST_AUDIO_FORMAT_S16LE:
      *sf = PA_SAMPLE_S16LE;
      break;
    case GST_AUDIO_FORMAT_S16BE:
      *sf = PA_SAMPLE_S16BE;
      break;
    case GST_AUDIO_FORMAT_F32LE:
      *sf = PA_SAMPLE_FLOAT32LE;
      break;
    case GST_AUDIO_FORMAT_F32BE:
      *sf = PA_SAMPLE_FLOAT32BE;
      break;
    case GST_AUDIO_FORMAT_S32LE:
      *sf = PA_SAMPLE_S32LE;
      break;
    case GST_AUDIO_FORMAT_S32BE:
      *sf = PA_SAMPLE_S32BE;
      break;
    case GST_AUDIO_FORMAT_S24LE:
      *sf = PA_SAMPLE_S24LE;
      break;
    case GST_AUDIO_FORMAT_S24BE:
      *sf = PA_SAMPLE_S24BE;
      break;
    case GST_AUDIO_FORMAT_S24_32LE:
      *sf = PA_SAMPLE_S24_32LE;
      break;
    case GST_AUDIO_FORMAT_S24_32BE:
      *sf = PA_SAMPLE_S24_32BE;
      break;
    default:
      return FALSE;
  }
  return TRUE;
}

gboolean
gst_pulse_fill_sample_spec (GstAudioRingBufferSpec * spec, pa_sample_spec * ss)
{
  if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_RAW) {
    if (!gstaudioformat_to_pasampleformat (GST_AUDIO_INFO_FORMAT (&spec->info),
            &ss->format))
      return FALSE;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MU_LAW) {
    ss->format = PA_SAMPLE_ULAW;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_A_LAW) {
    ss->format = PA_SAMPLE_ALAW;
  } else
    return FALSE;

  ss->channels = GST_AUDIO_INFO_CHANNELS (&spec->info);
  ss->rate = GST_AUDIO_INFO_RATE (&spec->info);

  if (!pa_sample_spec_valid (ss))
    return FALSE;

  return TRUE;
}

gboolean
gst_pulse_fill_format_info (GstAudioRingBufferSpec * spec, pa_format_info ** f,
    guint * channels)
{
  pa_format_info *format;
  pa_sample_format_t sf = PA_SAMPLE_INVALID;
  GstAudioInfo *ainfo = &spec->info;

  format = pa_format_info_new ();

  if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MU_LAW) {
    format->encoding = PA_ENCODING_PCM;
    sf = PA_SAMPLE_ULAW;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_A_LAW) {
    format->encoding = PA_ENCODING_PCM;
    sf = PA_SAMPLE_ALAW;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_RAW) {
    format->encoding = PA_ENCODING_PCM;
    if (!gstaudioformat_to_pasampleformat (GST_AUDIO_INFO_FORMAT (ainfo), &sf))
      goto fail;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_AC3) {
    format->encoding = PA_ENCODING_AC3_IEC61937;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_EAC3) {
    format->encoding = PA_ENCODING_EAC3_IEC61937;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_DTS) {
    format->encoding = PA_ENCODING_DTS_IEC61937;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MPEG) {
    format->encoding = PA_ENCODING_MPEG_IEC61937;
#if PA_CHECK_VERSION(3,99,0)
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MPEG2_AAC) {
    format->encoding = PA_ENCODING_MPEG2_AAC_IEC61937;
  } else if (spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_MPEG4_AAC) {
    /* HACK. treat MPEG4 AAC as MPEG2 AAC for the moment */
    format->encoding = PA_ENCODING_MPEG2_AAC_IEC61937;
#endif
  } else {
    goto fail;
  }

  if (format->encoding == PA_ENCODING_PCM) {
    pa_format_info_set_sample_format (format, sf);
    pa_format_info_set_channels (format, GST_AUDIO_INFO_CHANNELS (ainfo));
  }

  pa_format_info_set_rate (format, GST_AUDIO_INFO_RATE (ainfo));

  if (!pa_format_info_valid (format))
    goto fail;

  *f = format;
  *channels = GST_AUDIO_INFO_CHANNELS (ainfo);

  return TRUE;

fail:
  if (format)
    pa_format_info_free (format);
  return FALSE;
}

const char *
gst_pulse_sample_format_to_caps_format (pa_sample_format_t sf)
{
  switch (sf) {
    case PA_SAMPLE_U8:
      return "U8";

    case PA_SAMPLE_S16LE:
      return "S16LE";

    case PA_SAMPLE_S16BE:
      return "S16BE";

    case PA_SAMPLE_FLOAT32LE:
      return "F32LE";

    case PA_SAMPLE_FLOAT32BE:
      return "F32BE";

    case PA_SAMPLE_S32LE:
      return "S32LE";

    case PA_SAMPLE_S32BE:
      return "S32BE";

    case PA_SAMPLE_S24LE:
      return "S24LE";

    case PA_SAMPLE_S24BE:
      return "S24BE";

    case PA_SAMPLE_S24_32LE:
      return "S24_32LE";

    case PA_SAMPLE_S24_32BE:
      return "S24_32BE";

    default:
      return NULL;
  }
}

/* PATH_MAX is not defined everywhere, e.g. on GNU Hurd */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

gchar *
gst_pulse_client_name (void)
{
  gchar buf[PATH_MAX];

  const char *c;

  if ((c = g_get_application_name ()))
    return g_strdup (c);
  else if (pa_get_binary_name (buf, sizeof (buf)))
    return g_strdup (buf);
  else
    return g_strdup_printf ("GStreamer-pid-%lu", (gulong) getpid ());
}

pa_channel_map *
gst_pulse_gst_to_channel_map (pa_channel_map * map,
    const GstAudioRingBufferSpec * spec)
{
  gint i, j;
  gint channels;
  const GstAudioChannelPosition *pos;

  pa_channel_map_init (map);

  channels = GST_AUDIO_INFO_CHANNELS (&spec->info);
  pos = spec->info.position;

  for (j = 0; j < channels; j++) {
    for (i = 0; i < G_N_ELEMENTS (gst_pa_pos_table); i++) {
      if (pos[j] == gst_pa_pos_table[i].gst_pos) {
        map->map[j] = gst_pa_pos_table[i].pa_pos;
        break;
      }
    }
    if (i == G_N_ELEMENTS (gst_pa_pos_table))
      return NULL;
  }

  if (j != spec->info.channels) {
    return NULL;
  }

  map->channels = spec->info.channels;

  if (!pa_channel_map_valid (map)) {
    return NULL;
  }

  return map;
}

GstAudioRingBufferSpec *
gst_pulse_channel_map_to_gst (const pa_channel_map * map,
    GstAudioRingBufferSpec * spec)
{
  gint i, j;
  gboolean invalid = FALSE;
  gint channels;
  GstAudioChannelPosition *pos;

  channels = GST_AUDIO_INFO_CHANNELS (&spec->info);

  g_return_val_if_fail (map->channels == channels, NULL);

  pos = spec->info.position;

  for (j = 0; j < channels; j++) {
    for (i = 0; j < channels && i < G_N_ELEMENTS (gst_pa_pos_table); i++) {
      if (map->map[j] == gst_pa_pos_table[i].pa_pos) {
        pos[j] = gst_pa_pos_table[i].gst_pos;
        break;
      }
    }
    if (i == G_N_ELEMENTS (gst_pa_pos_table))
      return NULL;
  }

  if (!invalid
      && !gst_audio_check_valid_channel_positions (pos, channels, FALSE))
    invalid = TRUE;

  if (invalid) {
    for (i = 0; i < channels; i++)
      pos[i] = GST_AUDIO_CHANNEL_POSITION_NONE;
  } else {
    if (pos[0] != GST_AUDIO_CHANNEL_POSITION_NONE)
      spec->info.flags &= ~GST_AUDIO_FLAG_UNPOSITIONED;
  }

  return spec;
}

void
gst_pulse_cvolume_from_linear (pa_cvolume * v, unsigned channels,
    gdouble volume)
{
  pa_cvolume_set (v, channels, pa_sw_volume_from_linear (volume));
}

static gboolean
make_proplist_item (GQuark field_id, const GValue * value, gpointer user_data)
{
  pa_proplist *p = (pa_proplist *) user_data;
  gchar *prop_id = (gchar *) g_quark_to_string (field_id);

  /* http://0pointer.de/lennart/projects/pulseaudio/doxygen/proplist_8h.html */

  /* match prop id */

  /* check type */
  switch (G_VALUE_TYPE (value)) {
    case G_TYPE_STRING:
      pa_proplist_sets (p, prop_id, g_value_get_string (value));
      break;
    default:
      GST_WARNING ("unmapped property type %s", G_VALUE_TYPE_NAME (value));
      break;
  }

  return TRUE;
}

pa_proplist *
gst_pulse_make_proplist (const GstStructure * properties)
{
  pa_proplist *proplist = pa_proplist_new ();

  /* iterate the structure and fill the proplist */
  gst_structure_foreach (properties, make_proplist_item, proplist);
  return proplist;
}

GstStructure *
gst_pulse_make_structure (pa_proplist * properties)
{
  GstStructure *str;
  void *state = NULL;

  str = gst_structure_new_empty ("pulse-proplist");

  while (TRUE) {
    const char *key, *val;

    key = pa_proplist_iterate (properties, &state);
    if (key == NULL)
      break;

    val = pa_proplist_gets (properties, key);

    gst_structure_set (str, key, G_TYPE_STRING, val, NULL);
  }
  return str;
}

static gboolean
gst_pulse_format_info_int_prop_to_value (pa_format_info * format,
    const char *key, GValue * value)
{
  GValue v = { 0, };
  int i;
  int *a, n;
  int min, max;

  if (pa_format_info_get_prop_int (format, key, &i) == 0) {
    g_value_init (value, G_TYPE_INT);
    g_value_set_int (value, i);

  } else if (pa_format_info_get_prop_int_array (format, key, &a, &n) == 0) {
    g_value_init (value, GST_TYPE_LIST);
    g_value_init (&v, G_TYPE_INT);

    for (i = 0; i < n; i++) {
      g_value_set_int (&v, a[i]);
      gst_value_list_append_value (value, &v);
    }

    pa_xfree (a);

  } else if (pa_format_info_get_prop_int_range (format, key, &min, &max) == 0) {
    g_value_init (value, GST_TYPE_INT_RANGE);
    gst_value_set_int_range (value, min, max);

  } else {
    /* Property not available or is not an int type */
    return FALSE;
  }

  return TRUE;
}

GstCaps *
gst_pulse_format_info_to_caps (pa_format_info * format)
{
  GstCaps *ret = NULL;
  GValue v = { 0, };
  pa_sample_spec ss;

  switch (format->encoding) {
    case PA_ENCODING_PCM:{
      char *tmp = NULL;

      pa_format_info_to_sample_spec (format, &ss, NULL);

      if (pa_format_info_get_prop_string (format,
              PA_PROP_FORMAT_SAMPLE_FORMAT, &tmp)) {
        /* No specific sample format means any sample format */
        ret = gst_pulse_fix_pcm_caps (gst_caps_from_string (_PULSE_CAPS_PCM));
        goto out;

      } else if (ss.format == PA_SAMPLE_ALAW) {
        ret = gst_caps_from_string (_PULSE_CAPS_ALAW);

      } else if (ss.format == PA_SAMPLE_ULAW) {
        ret = gst_caps_from_string (_PULSE_CAPS_MULAW);

      } else {
        /* Linear PCM format */
        const char *sformat =
            gst_pulse_sample_format_to_caps_format (ss.format);

        ret = gst_caps_from_string (_PULSE_CAPS_LINEAR);

        if (sformat)
          gst_caps_set_simple (ret, "format", G_TYPE_STRING, sformat, NULL);
      }

      pa_xfree (tmp);
      break;
    }

    case PA_ENCODING_AC3_IEC61937:
      ret = gst_caps_from_string (_PULSE_CAPS_AC3);
      break;

    case PA_ENCODING_EAC3_IEC61937:
      ret = gst_caps_from_string (_PULSE_CAPS_EAC3);
      break;

    case PA_ENCODING_DTS_IEC61937:
      ret = gst_caps_from_string (_PULSE_CAPS_DTS);
      break;

    case PA_ENCODING_MPEG_IEC61937:
      ret = gst_caps_from_string (_PULSE_CAPS_MP3);
      break;

    default:
      GST_WARNING ("Found a PA format that we don't support yet");
      goto out;
  }

  if (gst_pulse_format_info_int_prop_to_value (format, PA_PROP_FORMAT_RATE, &v))
    gst_caps_set_value (ret, "rate", &v);

  g_value_unset (&v);

  if (gst_pulse_format_info_int_prop_to_value (format, PA_PROP_FORMAT_CHANNELS,
          &v))
    gst_caps_set_value (ret, "channels", &v);

out:
  return ret;
}

GstCaps *
gst_pulse_fix_pcm_caps (GstCaps * incaps)
{
  GstCaps *outcaps;
  int i;

  outcaps = gst_caps_make_writable (incaps);

  for (i = 0; i < gst_caps_get_size (outcaps); i++) {
    GstStructure *st = gst_caps_get_structure (outcaps, i);
    const gchar *format = gst_structure_get_name (st);
    const GValue *value;
    GValue new_value = G_VALUE_INIT;
    gint min, max, step;

    if (!(g_str_equal (format, "audio/x-raw") ||
            g_str_equal (format, "audio/x-alaw") ||
            g_str_equal (format, "audio/x-mulaw")))
      continue;

    value = gst_structure_get_value (st, "rate");

    if (!GST_VALUE_HOLDS_INT_RANGE (value))
      continue;

    min = gst_value_get_int_range_min (value);
    max = gst_value_get_int_range_max (value);
    step = gst_value_get_int_range_step (value);

    if (min > PA_RATE_MAX)
      min = PA_RATE_MAX;
    if (max > PA_RATE_MAX)
      max = PA_RATE_MAX;

    g_value_init (&new_value, GST_TYPE_INT_RANGE);
    gst_value_set_int_range_step (&new_value, min, max, step);

    gst_structure_take_value (st, "rate", &new_value);
  }

  return outcaps;
}
