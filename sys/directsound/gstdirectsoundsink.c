/* GStreamer
* Copyright (C) 2005 Sebastien Moutte <sebastien@moutte.net>
* Copyright (C) 2007 Pioneers of the Inevitable <songbird@songbirdnest.com>
* Copyright (C) 2010 Fluendo S.A. <support@fluendo.com>
*
* gstdirectsoundsink.c:
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
*
*
* The development of this code was made possible due to the involvement
* of Pioneers of the Inevitable, the creators of the Songbird Music player
*
*/

/**
 * SECTION:element-directsoundsink
 * @title: directsoundsink
 *
 * This element lets you output sound using the DirectSound API.
 *
 * Note that you should almost always use generic audio conversion elements
 * like audioconvert and audioresample in front of an audiosink to make sure
 * your pipeline works under all circumstances (those conversion elements will
 * act in passthrough-mode if no conversion is necessary).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v audiotestsrc ! audioconvert ! volume volume=0.1 ! directsoundsink
 * ]| will output a sine wave (continuous beep sound) to your sound card (with
 * a very low volume as precaution).
 * |[
 * gst-launch-1.0 -v filesrc location=music.ogg ! decodebin ! audioconvert ! audioresample ! directsoundsink
 * ]| will play an Ogg/Vorbis audio file and output it.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/base/gstbasesink.h>
#include "gstdirectsoundsink.h"
#include <gst/audio/gstaudioiec61937.h>

#include <math.h>

#ifdef __CYGWIN__
#include <unistd.h>
#ifndef _swab
#define _swab swab
#endif
#endif

#define DEFAULT_MUTE FALSE

GST_DEBUG_CATEGORY_STATIC (directsoundsink_debug);
#define GST_CAT_DEFAULT directsoundsink_debug

static void gst_directsound_sink_finalize (GObject * object);

static void gst_directsound_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_directsound_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstCaps *gst_directsound_sink_getcaps (GstBaseSink * bsink,
    GstCaps * filter);
static GstBuffer *gst_directsound_sink_payload (GstAudioBaseSink * sink,
    GstBuffer * buf);
static gboolean gst_directsound_sink_prepare (GstAudioSink * asink,
    GstAudioRingBufferSpec * spec);
static gboolean gst_directsound_sink_unprepare (GstAudioSink * asink);
static gboolean gst_directsound_sink_open (GstAudioSink * asink);
static gboolean gst_directsound_sink_close (GstAudioSink * asink);
static gint gst_directsound_sink_write (GstAudioSink * asink,
    gpointer data, guint length);
static guint gst_directsound_sink_delay (GstAudioSink * asink);
static void gst_directsound_sink_reset (GstAudioSink * asink);
static GstCaps *gst_directsound_probe_supported_formats (GstDirectSoundSink *
    dsoundsink, const GstCaps * template_caps);
static gboolean gst_directsound_sink_query (GstBaseSink * pad,
    GstQuery * query);

static void gst_directsound_sink_set_volume (GstDirectSoundSink * sink,
    gdouble volume, gboolean store);
static gdouble gst_directsound_sink_get_volume (GstDirectSoundSink * sink);
static void gst_directsound_sink_set_mute (GstDirectSoundSink * sink,
    gboolean mute);
static gboolean gst_directsound_sink_get_mute (GstDirectSoundSink * sink);
static const gchar *gst_directsound_sink_get_device (GstDirectSoundSink *
    dsoundsink);
static void gst_directsound_sink_set_device (GstDirectSoundSink * dsoundsink,
    const gchar * device_id);

static gboolean gst_directsound_sink_is_spdif_format (GstAudioRingBufferSpec *
    spec);

static gchar *gst_hres_to_string (HRESULT hRes);

static GstStaticPadTemplate directsoundsink_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_DIRECTSOUND_SINK_CAPS));

enum
{
  PROP_0,
  PROP_VOLUME,
  PROP_MUTE,
  PROP_DEVICE
};

#define gst_directsound_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstDirectSoundSink, gst_directsound_sink,
    GST_TYPE_AUDIO_SINK, G_IMPLEMENT_INTERFACE (GST_TYPE_STREAM_VOLUME, NULL)
    );

static void
gst_directsound_sink_finalize (GObject * object)
{
  GstDirectSoundSink *dsoundsink = GST_DIRECTSOUND_SINK (object);

  g_free (dsoundsink->device_id);
  dsoundsink->device_id = NULL;

  g_mutex_clear (&dsoundsink->dsound_lock);
  gst_object_unref (dsoundsink->system_clock);
  if (dsoundsink->write_wait_clock_id != NULL) {
    gst_clock_id_unref (dsoundsink->write_wait_clock_id);
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_directsound_sink_class_init (GstDirectSoundSinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseSinkClass *gstbasesink_class = GST_BASE_SINK_CLASS (klass);
  GstAudioSinkClass *gstaudiosink_class = GST_AUDIO_SINK_CLASS (klass);
  GstAudioBaseSinkClass *gstaudiobasesink_class =
      GST_AUDIO_BASE_SINK_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (directsoundsink_debug, "directsoundsink", 0,
      "DirectSound sink");

  gobject_class->finalize = gst_directsound_sink_finalize;
  gobject_class->set_property = gst_directsound_sink_set_property;
  gobject_class->get_property = gst_directsound_sink_get_property;

  gstbasesink_class->get_caps =
      GST_DEBUG_FUNCPTR (gst_directsound_sink_getcaps);

  gstbasesink_class->query = GST_DEBUG_FUNCPTR (gst_directsound_sink_query);

  gstaudiobasesink_class->payload =
      GST_DEBUG_FUNCPTR (gst_directsound_sink_payload);

  gstaudiosink_class->prepare =
      GST_DEBUG_FUNCPTR (gst_directsound_sink_prepare);
  gstaudiosink_class->unprepare =
      GST_DEBUG_FUNCPTR (gst_directsound_sink_unprepare);
  gstaudiosink_class->open = GST_DEBUG_FUNCPTR (gst_directsound_sink_open);
  gstaudiosink_class->close = GST_DEBUG_FUNCPTR (gst_directsound_sink_close);
  gstaudiosink_class->write = GST_DEBUG_FUNCPTR (gst_directsound_sink_write);
  gstaudiosink_class->delay = GST_DEBUG_FUNCPTR (gst_directsound_sink_delay);
  gstaudiosink_class->reset = GST_DEBUG_FUNCPTR (gst_directsound_sink_reset);

  g_object_class_install_property (gobject_class,
      PROP_VOLUME,
      g_param_spec_double ("volume", "Volume",
          "Volume of this stream", 0.0, 1.0, 1.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_MUTE,
      g_param_spec_boolean ("mute", "Mute",
          "Mute state of this stream", DEFAULT_MUTE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_DEVICE,
      g_param_spec_string ("device", "Device",
          "DirectSound playback device as a GUID string",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (element_class,
      "Direct Sound Audio Sink", "Sink/Audio",
      "Output to a sound card via Direct Sound",
      "Sebastien Moutte <sebastien@moutte.net>");

  gst_element_class_add_static_pad_template (element_class,
      &directsoundsink_sink_factory);
}

static void
gst_directsound_sink_init (GstDirectSoundSink * dsoundsink)
{
  dsoundsink->volume = 100;
  dsoundsink->mute = FALSE;
  dsoundsink->device_id = NULL;
  dsoundsink->pDS = NULL;
  dsoundsink->cached_caps = NULL;
  dsoundsink->pDSBSecondary = NULL;
  dsoundsink->current_circular_offset = 0;
  dsoundsink->buffer_size = DSBSIZE_MIN;
  g_mutex_init (&dsoundsink->dsound_lock);
  dsoundsink->system_clock = gst_system_clock_obtain ();
  dsoundsink->write_wait_clock_id = NULL;
  dsoundsink->first_buffer_after_reset = FALSE;
}

static void
gst_directsound_sink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstDirectSoundSink *sink = GST_DIRECTSOUND_SINK (object);

  switch (prop_id) {
    case PROP_VOLUME:
      gst_directsound_sink_set_volume (sink, g_value_get_double (value), TRUE);
      break;
    case PROP_MUTE:
      gst_directsound_sink_set_mute (sink, g_value_get_boolean (value));
      break;
    case PROP_DEVICE:
      gst_directsound_sink_set_device (sink, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_directsound_sink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstDirectSoundSink *sink = GST_DIRECTSOUND_SINK (object);

  switch (prop_id) {
    case PROP_VOLUME:
      g_value_set_double (value, gst_directsound_sink_get_volume (sink));
      break;
    case PROP_MUTE:
      g_value_set_boolean (value, gst_directsound_sink_get_mute (sink));
      break;
    case PROP_DEVICE:
      g_value_set_string (value, gst_directsound_sink_get_device (sink));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstCaps *
gst_directsound_sink_getcaps (GstBaseSink * bsink, GstCaps * filter)
{
  GstElementClass *element_class;
  GstPadTemplate *pad_template;
  GstDirectSoundSink *dsoundsink = GST_DIRECTSOUND_SINK (bsink);
  GstCaps *caps;

  if (dsoundsink->pDS == NULL) {
    GST_DEBUG_OBJECT (dsoundsink, "device not open, using template caps");
    return NULL;                /* base class will get template caps for us */
  }

  if (dsoundsink->cached_caps) {
    caps = gst_caps_ref (dsoundsink->cached_caps);
  } else {
    element_class = GST_ELEMENT_GET_CLASS (dsoundsink);
    pad_template = gst_element_class_get_pad_template (element_class, "sink");
    g_return_val_if_fail (pad_template != NULL, NULL);

    caps = gst_directsound_probe_supported_formats (dsoundsink,
        gst_pad_template_get_caps (pad_template));
    if (caps)
      dsoundsink->cached_caps = gst_caps_ref (caps);
  }

  if (caps && filter) {
    GstCaps *tmp =
        gst_caps_intersect_full (filter, caps, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (caps);
    caps = tmp;
  }

  if (caps) {
    gchar *caps_string = gst_caps_to_string (caps);
    GST_DEBUG_OBJECT (dsoundsink, "returning caps %s", caps_string);
    g_free (caps_string);
  }

  return caps;
}

static gboolean
gst_directsound_sink_acceptcaps (GstBaseSink * sink, GstQuery * query)
{
  GstDirectSoundSink *dsink = GST_DIRECTSOUND_SINK (sink);
  GstPad *pad;
  GstCaps *caps;
  GstCaps *pad_caps;
  GstStructure *st;
  gboolean ret = FALSE;
  GstAudioRingBufferSpec spec = { 0 };

  if (G_UNLIKELY (dsink == NULL))
    return FALSE;

  pad = sink->sinkpad;

  gst_query_parse_accept_caps (query, &caps);
  GST_DEBUG_OBJECT (pad, "caps %" GST_PTR_FORMAT, caps);

  pad_caps = gst_pad_query_caps (pad, NULL);
  if (pad_caps) {
    gboolean cret = gst_caps_is_subset (caps, pad_caps);
    gst_caps_unref (pad_caps);
    if (!cret) {
      GST_DEBUG_OBJECT (dsink,
          "Caps are not a subset of the pad caps, not accepting caps");
      goto done;
    }
  }

  /* If we've not got fixed caps, creating a stream might fail, so let's just
   * return from here with default acceptcaps behaviour */
  if (!gst_caps_is_fixed (caps)) {
    GST_DEBUG_OBJECT (dsink, "Caps are not fixed, not accepting caps");
    goto done;
  }

  spec.latency_time = GST_SECOND;
  if (!gst_audio_ring_buffer_parse_caps (&spec, caps)) {
    GST_DEBUG_OBJECT (dsink, "Failed to parse caps, not accepting");
    goto done;
  }

  /* Make sure input is framed (one frame per buffer) and can be payloaded */
  switch (spec.type) {
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_AC3:
    case GST_AUDIO_RING_BUFFER_FORMAT_TYPE_DTS:
    {
      gboolean framed = FALSE, parsed = FALSE;
      st = gst_caps_get_structure (caps, 0);

      gst_structure_get_boolean (st, "framed", &framed);
      gst_structure_get_boolean (st, "parsed", &parsed);
      if ((!framed && !parsed) || gst_audio_iec61937_frame_size (&spec) <= 0) {
        GST_DEBUG_OBJECT (dsink, "Wrong AC3/DTS caps, not accepting");
        goto done;
      }
    }
    default:
      break;
  }
  ret = TRUE;
  GST_DEBUG_OBJECT (dsink, "Accepting caps");

done:
  gst_query_set_accept_caps_result (query, ret);
  return TRUE;
}

static gboolean
gst_directsound_sink_query (GstBaseSink * sink, GstQuery * query)
{
  gboolean res = TRUE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_ACCEPT_CAPS:
      res = gst_directsound_sink_acceptcaps (sink, query);
      break;
    default:
      res = GST_BASE_SINK_CLASS (parent_class)->query (sink, query);
  }

  return res;
}

static LPGUID
string_to_guid (const gchar * str)
{
  HRESULT ret;
  gunichar2 *wstr;
  LPGUID out;

  wstr = g_utf8_to_utf16 (str, -1, NULL, NULL, NULL);
  if (!wstr)
    return NULL;

  out = g_new (GUID, 1);
  ret = CLSIDFromString ((LPOLESTR) wstr, out);
  g_free (wstr);
  if (ret != NOERROR) {
    g_free (out);
    return NULL;
  }

  return out;
}

static gboolean
gst_directsound_sink_open (GstAudioSink * asink)
{
  GstDirectSoundSink *dsoundsink;
  HRESULT hRes;
  LPGUID lpGuid = NULL;

  dsoundsink = GST_DIRECTSOUND_SINK (asink);

  if (dsoundsink->device_id) {
    lpGuid = string_to_guid (dsoundsink->device_id);
    if (lpGuid == NULL) {
      GST_ELEMENT_ERROR (dsoundsink, RESOURCE, OPEN_READ,
          ("device set but guid not found: %s", dsoundsink->device_id), (NULL));
      return FALSE;
    }
  }

  /* create and initialize a DirectSound object */
  if (FAILED (hRes = DirectSoundCreate (lpGuid, &dsoundsink->pDS, NULL))) {
    gchar *error_text = gst_hres_to_string (hRes);
    GST_ELEMENT_ERROR (dsoundsink, RESOURCE, OPEN_READ,
        ("DirectSoundCreate: %s", error_text), (NULL));
    g_free (lpGuid);
    g_free (error_text);
    return FALSE;
  }

  g_free (lpGuid);

  if (FAILED (hRes = IDirectSound_SetCooperativeLevel (dsoundsink->pDS,
              GetDesktopWindow (), DSSCL_PRIORITY))) {
    gchar *error_text = gst_hres_to_string (hRes);
    GST_ELEMENT_ERROR (dsoundsink, RESOURCE, OPEN_READ,
        ("IDirectSound_SetCooperativeLevel: %s", error_text), (NULL));
    g_free (error_text);
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_directsound_sink_is_spdif_format (GstAudioRingBufferSpec * spec)
{
  return spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_AC3 ||
      spec->type == GST_AUDIO_RING_BUFFER_FORMAT_TYPE_DTS;
}

static gboolean
gst_directsound_sink_prepare (GstAudioSink * asink,
    GstAudioRingBufferSpec * spec)
{
  GstDirectSoundSink *dsoundsink;
  HRESULT hRes;
  DSBUFFERDESC descSecondary;
  WAVEFORMATEX wfx;

  dsoundsink = GST_DIRECTSOUND_SINK (asink);

  /*save number of bytes per sample and buffer format */
  dsoundsink->bytes_per_sample = spec->info.bpf;
  dsoundsink->type = spec->type;

  /* fill the WAVEFORMATEX structure with spec params */
  memset (&wfx, 0, sizeof (wfx));
  if (!gst_directsound_sink_is_spdif_format (spec)) {
    wfx.cbSize = sizeof (wfx);
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = spec->info.channels;
    wfx.nSamplesPerSec = spec->info.rate;
    wfx.wBitsPerSample = (spec->info.bpf * 8) / wfx.nChannels;
    wfx.nBlockAlign = spec->info.bpf;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    /* Create directsound buffer with size based on our configured
     * buffer_size (which is 200 ms by default) */
    dsoundsink->buffer_size =
        gst_util_uint64_scale_int (wfx.nAvgBytesPerSec, spec->buffer_time,
        GST_MSECOND);
    /* Make sure we make those numbers multiple of our sample size in bytes */
    dsoundsink->buffer_size -= dsoundsink->buffer_size % spec->info.bpf;

    spec->segsize =
        gst_util_uint64_scale_int (wfx.nAvgBytesPerSec, spec->latency_time,
        GST_MSECOND);
    spec->segsize -= spec->segsize % spec->info.bpf;
    spec->segtotal = dsoundsink->buffer_size / spec->segsize;
  } else {
#ifdef WAVE_FORMAT_DOLBY_AC3_SPDIF
    wfx.cbSize = 0;
    wfx.wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 48000;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    spec->segsize = 6144;
    spec->segtotal = 10;
#else
    g_assert_not_reached ();
#endif
  }

  // Make the final buffer size be an integer number of segments
  dsoundsink->buffer_size = spec->segsize * spec->segtotal;

  GST_INFO_OBJECT (dsoundsink, "channels: %d, rate: %d, bytes_per_sample: %d"
      " WAVEFORMATEX.nSamplesPerSec: %ld, WAVEFORMATEX.wBitsPerSample: %d,"
      " WAVEFORMATEX.nBlockAlign: %d, WAVEFORMATEX.nAvgBytesPerSec: %ld\n"
      "Size of dsound circular buffer=>%d\n",
      GST_AUDIO_INFO_CHANNELS (&spec->info), GST_AUDIO_INFO_RATE (&spec->info),
      GST_AUDIO_INFO_BPF (&spec->info), wfx.nSamplesPerSec, wfx.wBitsPerSample,
      wfx.nBlockAlign, wfx.nAvgBytesPerSec, dsoundsink->buffer_size);

  /* create a secondary directsound buffer */
  memset (&descSecondary, 0, sizeof (DSBUFFERDESC));
  descSecondary.dwSize = sizeof (DSBUFFERDESC);
  descSecondary.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
  if (!gst_directsound_sink_is_spdif_format (spec))
    descSecondary.dwFlags |= DSBCAPS_CTRLVOLUME;

  descSecondary.dwBufferBytes = dsoundsink->buffer_size;
  descSecondary.lpwfxFormat = (WAVEFORMATEX *) & wfx;

  hRes = IDirectSound_CreateSoundBuffer (dsoundsink->pDS, &descSecondary,
      &dsoundsink->pDSBSecondary, NULL);
  if (FAILED (hRes)) {
    gchar *error_text = gst_hres_to_string (hRes);
    GST_ELEMENT_ERROR (dsoundsink, RESOURCE, OPEN_READ,
        ("IDirectSound_CreateSoundBuffer: %s", error_text), (NULL));
    g_free (error_text);
    return FALSE;
  }

  gst_directsound_sink_set_volume (dsoundsink,
      gst_directsound_sink_get_volume (dsoundsink), FALSE);
  gst_directsound_sink_set_mute (dsoundsink, dsoundsink->mute);

  return TRUE;
}

static gboolean
gst_directsound_sink_unprepare (GstAudioSink * asink)
{
  GstDirectSoundSink *dsoundsink;

  dsoundsink = GST_DIRECTSOUND_SINK (asink);

  /* release secondary DirectSound buffer */
  if (dsoundsink->pDSBSecondary) {
    IDirectSoundBuffer_Release (dsoundsink->pDSBSecondary);
    dsoundsink->pDSBSecondary = NULL;
  }

  return TRUE;
}

static gboolean
gst_directsound_sink_close (GstAudioSink * asink)
{
  GstDirectSoundSink *dsoundsink = NULL;

  dsoundsink = GST_DIRECTSOUND_SINK (asink);

  /* release DirectSound object */
  g_return_val_if_fail (dsoundsink->pDS != NULL, FALSE);
  IDirectSound_Release (dsoundsink->pDS);
  dsoundsink->pDS = NULL;

  gst_caps_replace (&dsoundsink->cached_caps, NULL);

  return TRUE;
}

static gint
gst_directsound_sink_write (GstAudioSink * asink, gpointer data, guint length)
{
  GstDirectSoundSink *dsoundsink;
  DWORD dwStatus = 0;
  HRESULT hRes, hRes2;
  LPVOID pLockedBuffer1 = NULL, pLockedBuffer2 = NULL;
  DWORD dwSizeBuffer1, dwSizeBuffer2;
  DWORD dwCurrentPlayCursor;

  dsoundsink = GST_DIRECTSOUND_SINK (asink);

  GST_DSOUND_LOCK (dsoundsink);

  /* get current buffer status */
  hRes = IDirectSoundBuffer_GetStatus (dsoundsink->pDSBSecondary, &dwStatus);

  /* get current play cursor position */
  hRes2 = IDirectSoundBuffer_GetCurrentPosition (dsoundsink->pDSBSecondary,
      &dwCurrentPlayCursor, NULL);

  if (SUCCEEDED (hRes) && SUCCEEDED (hRes2) && (dwStatus & DSBSTATUS_PLAYING)) {
    DWORD dwFreeBufferSize = 0;
    GstClockTime sleep_time_ms = 0, sleep_until;
    GstClockID clock_id;

  calculate_freesize:
    /* Calculate the free space in the circular buffer */
    if (dwCurrentPlayCursor < dsoundsink->current_circular_offset)
      dwFreeBufferSize =
          dsoundsink->buffer_size - (dsoundsink->current_circular_offset -
          dwCurrentPlayCursor);
    else
      dwFreeBufferSize =
          dwCurrentPlayCursor - dsoundsink->current_circular_offset;

    /* Not enough free space, wait for some samples to be played out. We could
     * write out partial data, but that will result in a tight loop in the
     * audioringbuffer write thread, and lead to high CPU usage. */
    if (length > dwFreeBufferSize) {
      gint rate = GST_AUDIO_BASE_SINK (asink)->ringbuffer->spec.info.rate;
      /* Wait for a time proportional to the space needed. In reality, the
       * directsound sink's position does not update frequently enough, so we
       * will end up waiting for much longer. Note that Sleep() has millisecond
       * resolution at best. */
      sleep_time_ms = gst_util_uint64_scale_int ((length - dwFreeBufferSize),
          1000, dsoundsink->bytes_per_sample * rate);
      /* Make sure we don't run in a tight loop unnecessarily */
      sleep_time_ms = MAX (sleep_time_ms, 10);
      sleep_until = gst_clock_get_time (dsoundsink->system_clock) +
          sleep_time_ms * GST_MSECOND;

      GST_DEBUG_OBJECT (dsoundsink,
          "length: %u, FreeBufSiz: %ld, sleep_time_ms: %" G_GUINT64_FORMAT
          ", bps: %i, rate: %i", length, dwFreeBufferSize, sleep_time_ms,
          dsoundsink->bytes_per_sample, rate);

      if (G_UNLIKELY (dsoundsink->write_wait_clock_id == NULL ||
              gst_clock_single_shot_id_reinit (dsoundsink->system_clock,
                  dsoundsink->write_wait_clock_id, sleep_until) == FALSE)) {

        if (dsoundsink->write_wait_clock_id != NULL) {
          gst_clock_id_unref (dsoundsink->write_wait_clock_id);
        }

        dsoundsink->write_wait_clock_id =
            gst_clock_new_single_shot_id (dsoundsink->system_clock,
            sleep_until);
      }

      clock_id = dsoundsink->write_wait_clock_id;
      dsoundsink->reset_while_sleeping = FALSE;

      GST_DSOUND_UNLOCK (dsoundsink);

      /* don't bother with the return value as we'll detect reset separately,
         as reset could happen between when this returns and we obtain the lock
         again -- so we can't use UNSCHEDULED here */
      gst_clock_id_wait (clock_id, NULL);

      GST_DSOUND_LOCK (dsoundsink);

      /* if a reset occurs, exit now */
      if (dsoundsink->reset_while_sleeping == TRUE) {
        GST_DSOUND_UNLOCK (dsoundsink);
        return -1;
      }

      /* May we send out? */
      hRes = IDirectSoundBuffer_GetCurrentPosition (dsoundsink->pDSBSecondary,
          &dwCurrentPlayCursor, NULL);
      hRes2 =
          IDirectSoundBuffer_GetStatus (dsoundsink->pDSBSecondary, &dwStatus);
      if (SUCCEEDED (hRes) && SUCCEEDED (hRes2)
          && (dwStatus & DSBSTATUS_PLAYING))
        goto calculate_freesize;
      else {
        gchar *err1, *err2;

        dsoundsink->first_buffer_after_reset = FALSE;
        GST_DSOUND_UNLOCK (dsoundsink);

        err1 = gst_hres_to_string (hRes);
        err2 = gst_hres_to_string (hRes2);
        GST_ELEMENT_ERROR (dsoundsink, RESOURCE, OPEN_WRITE,
            ("IDirectSoundBuffer_GetStatus %s, "
                "IDirectSoundBuffer_GetCurrentPosition: %s, dwStatus: %lu",
                err2, err1, dwStatus), (NULL));
        g_free (err1);
        g_free (err2);
        return -1;
      }
    }
  }

  if (dwStatus & DSBSTATUS_BUFFERLOST) {
    hRes = IDirectSoundBuffer_Restore (dsoundsink->pDSBSecondary);      /*need a loop waiting the buffer is restored?? */
    dsoundsink->current_circular_offset = 0;
  }

  /* Lock a buffer of length @length for writing */
  hRes = IDirectSoundBuffer_Lock (dsoundsink->pDSBSecondary,
      dsoundsink->current_circular_offset, length, &pLockedBuffer1,
      &dwSizeBuffer1, &pLockedBuffer2, &dwSizeBuffer2, 0L);

  if (SUCCEEDED (hRes)) {
    // Write to pointers without reordering.
    memcpy (pLockedBuffer1, data, dwSizeBuffer1);
    if (pLockedBuffer2 != NULL)
      memcpy (pLockedBuffer2, (LPBYTE) data + dwSizeBuffer1, dwSizeBuffer2);

    hRes = IDirectSoundBuffer_Unlock (dsoundsink->pDSBSecondary, pLockedBuffer1,
        dwSizeBuffer1, pLockedBuffer2, dwSizeBuffer2);

    // Update where the buffer will lock (for next time)
    dsoundsink->current_circular_offset += dwSizeBuffer1 + dwSizeBuffer2;
    dsoundsink->current_circular_offset %= dsoundsink->buffer_size;     /* Circular buffer */
  }

  /* if the buffer was not in playing state yet, call play on the buffer 
     except if this buffer is the fist after a reset (base class call reset and write a buffer when setting the sink to pause) */
  if (!(dwStatus & DSBSTATUS_PLAYING) &&
      dsoundsink->first_buffer_after_reset == FALSE) {
    hRes = IDirectSoundBuffer_Play (dsoundsink->pDSBSecondary, 0, 0,
        DSBPLAY_LOOPING);
  }

  dsoundsink->first_buffer_after_reset = FALSE;

  GST_DSOUND_UNLOCK (dsoundsink);

  return length;
}

static guint
gst_directsound_sink_delay (GstAudioSink * asink)
{
  GstDirectSoundSink *dsoundsink;
  HRESULT hRes;
  DWORD dwCurrentPlayCursor;
  DWORD dwBytesInQueue = 0;
  gint nNbSamplesInQueue = 0;
  DWORD dwStatus;

  dsoundsink = GST_DIRECTSOUND_SINK (asink);

  /* get current buffer status */
  hRes = IDirectSoundBuffer_GetStatus (dsoundsink->pDSBSecondary, &dwStatus);

  if (SUCCEEDED (hRes) && (dwStatus & DSBSTATUS_PLAYING)) {
    /*evaluate the number of samples in queue in the circular buffer */
    hRes = IDirectSoundBuffer_GetCurrentPosition (dsoundsink->pDSBSecondary,
        &dwCurrentPlayCursor, NULL);

    if (hRes == S_OK) {
      if (dwCurrentPlayCursor < dsoundsink->current_circular_offset)
        dwBytesInQueue =
            dsoundsink->current_circular_offset - dwCurrentPlayCursor;
      else
        dwBytesInQueue =
            dsoundsink->current_circular_offset + (dsoundsink->buffer_size -
            dwCurrentPlayCursor);

      nNbSamplesInQueue = dwBytesInQueue / dsoundsink->bytes_per_sample;
    }
  }

  return nNbSamplesInQueue;
}

static void
gst_directsound_sink_reset (GstAudioSink * asink)
{
  GstDirectSoundSink *dsoundsink;
  LPVOID pLockedBuffer = NULL;
  DWORD dwSizeBuffer = 0;

  dsoundsink = GST_DIRECTSOUND_SINK (asink);

  GST_DSOUND_LOCK (dsoundsink);

  if (dsoundsink->pDSBSecondary) {
    /*stop playing */
    HRESULT hRes = IDirectSoundBuffer_Stop (dsoundsink->pDSBSecondary);

    /*reset position */
    hRes = IDirectSoundBuffer_SetCurrentPosition (dsoundsink->pDSBSecondary, 0);
    dsoundsink->current_circular_offset = 0;

    /*reset the buffer */
    hRes = IDirectSoundBuffer_Lock (dsoundsink->pDSBSecondary,
        0, dsoundsink->buffer_size,
        &pLockedBuffer, &dwSizeBuffer, NULL, NULL, 0L);

    if (SUCCEEDED (hRes)) {
      memset (pLockedBuffer, 0, dwSizeBuffer);

      hRes =
          IDirectSoundBuffer_Unlock (dsoundsink->pDSBSecondary, pLockedBuffer,
          dwSizeBuffer, NULL, 0);
    }
  }

  dsoundsink->reset_while_sleeping = TRUE;
  dsoundsink->first_buffer_after_reset = TRUE;
  if (dsoundsink->write_wait_clock_id != NULL) {
    gst_clock_id_unschedule (dsoundsink->write_wait_clock_id);
  }

  GST_DSOUND_UNLOCK (dsoundsink);
}

/*
 * gst_directsound_probe_supported_formats:
 *
 * Takes the template caps and returns the subset which is actually
 * supported by this device.
 *
 */

static GstCaps *
gst_directsound_probe_supported_formats (GstDirectSoundSink * dsoundsink,
    const GstCaps * template_caps)
{
  HRESULT hRes;
  DSBUFFERDESC descSecondary;
  WAVEFORMATEX wfx;
  GstCaps *caps;
  GstCaps *tmp, *tmp2;
  LPDIRECTSOUNDBUFFER tmpBuffer;

  caps = gst_caps_copy (template_caps);

  /*
   * Check availability of digital output by trying to create an SPDIF buffer
   */

#ifdef WAVE_FORMAT_DOLBY_AC3_SPDIF
  /* fill the WAVEFORMATEX structure with some standard AC3 over SPDIF params */
  memset (&wfx, 0, sizeof (wfx));
  wfx.cbSize = 0;
  wfx.wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
  wfx.nChannels = 2;
  wfx.nSamplesPerSec = 48000;
  wfx.wBitsPerSample = 16;
  wfx.nBlockAlign = 4;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

  // create a secondary directsound buffer
  memset (&descSecondary, 0, sizeof (DSBUFFERDESC));
  descSecondary.dwSize = sizeof (DSBUFFERDESC);
  descSecondary.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
  descSecondary.dwBufferBytes = 6144;
  descSecondary.lpwfxFormat = &wfx;

  hRes = IDirectSound_CreateSoundBuffer (dsoundsink->pDS, &descSecondary,
      &tmpBuffer, NULL);
  if (FAILED (hRes)) {
    gchar *error_text = gst_hres_to_string (hRes);
    GST_INFO_OBJECT (dsoundsink, "AC3 passthrough not supported "
        "(IDirectSound_CreateSoundBuffer returned: %s)\n", error_text);
    g_free (error_text);
    tmp = gst_caps_new_empty_simple ("audio/x-ac3");
    tmp2 = gst_caps_subtract (caps, tmp);
    gst_caps_unref (tmp);
    gst_caps_unref (caps);
    caps = tmp2;
    tmp = gst_caps_new_empty_simple ("audio/x-dts");
    tmp2 = gst_caps_subtract (caps, tmp);
    gst_caps_unref (tmp);
    gst_caps_unref (caps);
    caps = tmp2;
  } else {
    GST_INFO_OBJECT (dsoundsink, "AC3 passthrough supported");
    hRes = IDirectSoundBuffer_Release (tmpBuffer);
    if (FAILED (hRes)) {
      gchar *error_text = gst_hres_to_string (hRes);
      GST_DEBUG_OBJECT (dsoundsink,
          "(IDirectSoundBuffer_Release returned: %s)\n", error_text);
      g_free (error_text);
    }
  }
#else
  tmp = gst_caps_new_empty_simple ("audio/x-ac3");
  tmp2 = gst_caps_subtract (caps, tmp);
  gst_caps_unref (tmp);
  gst_caps_unref (caps);
  caps = tmp2;
  tmp = gst_caps_new_empty_simple ("audio/x-dts");
  tmp2 = gst_caps_subtract (caps, tmp);
  gst_caps_unref (tmp);
  gst_caps_unref (caps);
  caps = tmp2;
#endif

  return caps;
}

static GstBuffer *
gst_directsound_sink_payload (GstAudioBaseSink * sink, GstBuffer * buf)
{
  if (gst_directsound_sink_is_spdif_format (&sink->ringbuffer->spec)) {
    gint framesize = gst_audio_iec61937_frame_size (&sink->ringbuffer->spec);
    GstBuffer *out;
    GstMapInfo infobuf, infoout;
    gboolean success;

    if (framesize <= 0)
      return NULL;

    out = gst_buffer_new_and_alloc (framesize);

    if (!gst_buffer_map (buf, &infobuf, GST_MAP_READWRITE)) {
      gst_buffer_unref (out);
      return NULL;
    }
    if (!gst_buffer_map (out, &infoout, GST_MAP_READWRITE)) {
      gst_buffer_unmap (buf, &infobuf);
      gst_buffer_unref (out);
      return NULL;
    }
    success = gst_audio_iec61937_payload (infobuf.data, infobuf.size,
        infoout.data, infoout.size, &sink->ringbuffer->spec, G_BYTE_ORDER);
    if (!success) {
      gst_buffer_unmap (out, &infoout);
      gst_buffer_unmap (buf, &infobuf);
      gst_buffer_unref (out);
      return NULL;
    }

    gst_buffer_copy_into (out, buf, GST_BUFFER_COPY_ALL, 0, -1);
    /* Fix endianness */
    _swab ((gchar *) infoout.data, (gchar *) infoout.data, infobuf.size);
    gst_buffer_unmap (out, &infoout);
    gst_buffer_unmap (buf, &infobuf);
    return out;
  } else
    return gst_buffer_ref (buf);
}

static void
gst_directsound_sink_set_volume (GstDirectSoundSink * dsoundsink,
    gdouble dvolume, gboolean store)
{
  glong volume;

  volume = dvolume * 100;
  if (store)
    dsoundsink->volume = volume;

  if (dsoundsink->pDSBSecondary) {
    /* DirectSound controls volume using units of 100th of a decibel,
     * ranging from -10000 to 0. We use a linear scale of 0 - 100
     * here, so remap.
     */
    long dsVolume;
    if (volume == 0 || dsoundsink->mute)
      dsVolume = -10000;
    else
      dsVolume = 100 * (long) (20 * log10 ((double) volume / 100.));
    dsVolume = CLAMP (dsVolume, -10000, 0);

    GST_DEBUG_OBJECT (dsoundsink,
        "Setting volume on secondary buffer to %d from %d", (int) dsVolume,
        (int) volume);
    IDirectSoundBuffer_SetVolume (dsoundsink->pDSBSecondary, dsVolume);
  }
}

gdouble
gst_directsound_sink_get_volume (GstDirectSoundSink * dsoundsink)
{
  return (gdouble) dsoundsink->volume / 100;
}

static void
gst_directsound_sink_set_mute (GstDirectSoundSink * dsoundsink, gboolean mute)
{
  if (mute) {
    gst_directsound_sink_set_volume (dsoundsink, 0, FALSE);
    dsoundsink->mute = TRUE;
  } else {
    gst_directsound_sink_set_volume (dsoundsink,
        gst_directsound_sink_get_volume (dsoundsink), FALSE);
    dsoundsink->mute = FALSE;
  }

}

static gboolean
gst_directsound_sink_get_mute (GstDirectSoundSink * dsoundsink)
{
  return dsoundsink->mute;
}

static const gchar *
gst_directsound_sink_get_device (GstDirectSoundSink * dsoundsink)
{
  return dsoundsink->device_id;
}

static void
gst_directsound_sink_set_device (GstDirectSoundSink * dsoundsink,
    const gchar * device_id)
{
  g_free (dsoundsink->device_id);
  dsoundsink->device_id = g_strdup (device_id);
}

/* Converts a HRESULT error to a text string
 * LPTSTR is either a */
static gchar *
gst_hres_to_string (HRESULT hRes)
{
  DWORD flags;
  gchar *ret_text;
  LPTSTR error_text = NULL;

  flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER
      | FORMAT_MESSAGE_IGNORE_INSERTS;
  FormatMessage (flags, NULL, hRes, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) & error_text, 0, NULL);

#ifdef UNICODE
  /* If UNICODE is defined, LPTSTR is LPWSTR which is UTF-16 */
  ret_text = g_utf16_to_utf8 (error_text, 0, NULL, NULL, NULL);
#else
  ret_text = g_strdup (error_text);
#endif

  LocalFree (error_text);
  return ret_text;
}
