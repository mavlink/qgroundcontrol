/* GStreamer
* Copyright (C) 2005 Sebastien Moutte <sebastien@moutte.net>
*
* gstwaveformsink.c:
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
 * SECTION:element-waveformsink
 * @title: waveformsink
 *
 * This element lets you output sound using the Windows WaveForm API.
 *
 * Note that you should almost always use generic audio conversion elements
 * like audioconvert and audioresample in front of an audiosink to make sure
 * your pipeline works under all circumstances (those conversion elements will
 * act in passthrough-mode if no conversion is necessary).
 *
 * ## Example pipelines
 * |[
 * gst-launch-1.0 -v audiotestsrc ! audioconvert ! volume volume=0.1 ! waveformsink
 * ]| will output a sine wave (continuous beep sound) to your sound card (with
 * a very low volume as precaution).
 * |[
 * gst-launch-1.0 -v filesrc location=music.ogg ! decodebin ! audioconvert ! audioresample ! waveformsink
 * ]| will play an Ogg/Vorbis audio file and output it.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstwaveformsink.h"

GST_DEBUG_CATEGORY_STATIC (waveformsink_debug);

static void gst_waveform_sink_finalise (GObject * object);
static void gst_waveform_sink_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_waveform_sink_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static GstCaps *gst_waveform_sink_getcaps (GstBaseSink * bsink,
    GstCaps * filter);

/************************************************************************/
/* GstAudioSink functions                                               */
/************************************************************************/
static gboolean gst_waveform_sink_prepare (GstAudioSink * asink,
    GstAudioRingBufferSpec * spec);
static gboolean gst_waveform_sink_unprepare (GstAudioSink * asink);
static gboolean gst_waveform_sink_open (GstAudioSink * asink);
static gboolean gst_waveform_sink_close (GstAudioSink * asink);
static gint gst_waveform_sink_write (GstAudioSink * asink, gpointer data,
    guint length);
static guint gst_waveform_sink_delay (GstAudioSink * asink);
static void gst_waveform_sink_reset (GstAudioSink * asink);

/************************************************************************/
/* Utils                                                                */
/************************************************************************/
GstCaps *gst_waveform_sink_create_caps (gint rate, gint channels,
    const gchar * format);
WAVEHDR *bufferpool_get_buffer (GstWaveFormSink * wfsink, gpointer data,
    guint length);
void CALLBACK waveOutProc (HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
    DWORD_PTR dwParam1, DWORD_PTR dwParam2);

static GstStaticPadTemplate waveformsink_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) { " GST_AUDIO_NE (S16) ", S8 }, "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [ 1, 2 ]"));

#define gst_waveform_sink_parent_class parent_class
G_DEFINE_TYPE (GstWaveFormSink, gst_waveform_sink, GST_TYPE_AUDIO_SINK);

static void
gst_waveform_sink_class_init (GstWaveFormSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSinkClass *gstbasesink_class;
  GstAudioSinkClass *gstaudiosink_class;
  GstElementClass *element_class;

  gobject_class = (GObjectClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstaudiosink_class = (GstAudioSinkClass *) klass;
  element_class = GST_ELEMENT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gst_waveform_sink_finalise;
  gobject_class->get_property = gst_waveform_sink_get_property;
  gobject_class->set_property = gst_waveform_sink_set_property;

  gstbasesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_waveform_sink_getcaps);

  gstaudiosink_class->prepare = GST_DEBUG_FUNCPTR (gst_waveform_sink_prepare);
  gstaudiosink_class->unprepare =
      GST_DEBUG_FUNCPTR (gst_waveform_sink_unprepare);
  gstaudiosink_class->open = GST_DEBUG_FUNCPTR (gst_waveform_sink_open);
  gstaudiosink_class->close = GST_DEBUG_FUNCPTR (gst_waveform_sink_close);
  gstaudiosink_class->write = GST_DEBUG_FUNCPTR (gst_waveform_sink_write);
  gstaudiosink_class->delay = GST_DEBUG_FUNCPTR (gst_waveform_sink_delay);
  gstaudiosink_class->reset = GST_DEBUG_FUNCPTR (gst_waveform_sink_reset);

  GST_DEBUG_CATEGORY_INIT (waveformsink_debug, "waveformsink", 0,
      "Waveform sink");

  gst_element_class_set_static_metadata (element_class, "WaveForm Audio Sink",
      "Sink/Audio",
      "Output to a sound card via WaveForm API",
      "Sebastien Moutte <sebastien@moutte.net>");

  gst_element_class_add_static_pad_template (element_class,
      &waveformsink_sink_factory);
}

static void
gst_waveform_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  /* GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (object); */

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_waveform_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  /* GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (object); */

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_waveform_sink_init (GstWaveFormSink * wfsink)
{
  /* initialize members */
  wfsink->hwaveout = NULL;
  wfsink->cached_caps = NULL;
  wfsink->wave_buffers = NULL;
  wfsink->write_buffer = 0;
  wfsink->buffer_count = BUFFER_COUNT;
  wfsink->buffer_size = BUFFER_SIZE;
  wfsink->free_buffers_count = wfsink->buffer_count;
  wfsink->bytes_in_queue = 0;

  InitializeCriticalSection (&wfsink->critic_wave);
}

static void
gst_waveform_sink_finalise (GObject * object)
{
  GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (object);

  if (wfsink->cached_caps) {
    gst_caps_unref (wfsink->cached_caps);
    wfsink->cached_caps = NULL;
  }

  DeleteCriticalSection (&wfsink->critic_wave);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstCaps *
gst_waveform_sink_getcaps (GstBaseSink * bsink, GstCaps * filter)
{
  GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (bsink);
  MMRESULT mmresult;
  WAVEOUTCAPS wocaps;
  GstCaps *caps, *caps_temp;

  /* return the cached caps if already defined */
  if (wfsink->cached_caps) {
    return gst_caps_ref (wfsink->cached_caps);
  }

  /* get the default device caps */
  mmresult = waveOutGetDevCaps (WAVE_MAPPER, &wocaps, sizeof (wocaps));
  if (mmresult != MMSYSERR_NOERROR) {
    waveOutGetErrorText (mmresult, wfsink->error_string, ERROR_LENGTH - 1);
    GST_ELEMENT_ERROR (wfsink, RESOURCE, SETTINGS,
        ("gst_waveform_sink_getcaps: waveOutGetDevCaps failed error=>%s",
            wfsink->error_string), (NULL));
    return NULL;
  }

  caps = gst_caps_new_empty ();

  /* create a caps for all wave formats supported by the device 
     starting by the best quality format */
  if (wocaps.dwFormats & WAVE_FORMAT_96S16) {
    caps_temp = gst_waveform_sink_create_caps (96000, 2, GST_AUDIO_NE (S16));
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_96S08) {
    caps_temp = gst_waveform_sink_create_caps (96000, 2, "S8");
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_96M16) {
    caps_temp = gst_waveform_sink_create_caps (96000, 1, GST_AUDIO_NE (S16));
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_96M08) {
    caps_temp = gst_waveform_sink_create_caps (96000, 1, "S8");
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_4S16) {
    caps_temp = gst_waveform_sink_create_caps (44100, 2, GST_AUDIO_NE (S16));
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_4S08) {
    caps_temp = gst_waveform_sink_create_caps (44100, 2, "S8");
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_4M16) {
    caps_temp = gst_waveform_sink_create_caps (44100, 1, GST_AUDIO_NE (S16));
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_4M08) {
    caps_temp = gst_waveform_sink_create_caps (44100, 1, "S8");
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_2S16) {
    caps_temp = gst_waveform_sink_create_caps (22050, 2, GST_AUDIO_NE (S16));
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_2S08) {
    caps_temp = gst_waveform_sink_create_caps (22050, 2, "S8");
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_2M16) {
    caps_temp = gst_waveform_sink_create_caps (22050, 1, GST_AUDIO_NE (S16));
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_2M08) {
    caps_temp = gst_waveform_sink_create_caps (22050, 1, "S8");
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_1S16) {
    caps_temp = gst_waveform_sink_create_caps (11025, 2, GST_AUDIO_NE (S16));
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_1S08) {
    caps_temp = gst_waveform_sink_create_caps (11025, 2, "S8");
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_1M16) {
    caps_temp = gst_waveform_sink_create_caps (11025, 1, GST_AUDIO_NE (S16));
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }
  if (wocaps.dwFormats & WAVE_FORMAT_1M08) {
    caps_temp = gst_waveform_sink_create_caps (11025, 1, "S8");
    if (caps_temp) {
      gst_caps_append (caps, caps_temp);
    }
  }

  if (gst_caps_is_empty (caps)) {
    gst_caps_unref (caps);
    caps = NULL;
  } else {
    wfsink->cached_caps = gst_caps_ref (caps);
  }

  GST_CAT_LOG_OBJECT (waveformsink_debug, wfsink,
      "Returning caps %" GST_PTR_FORMAT, caps);

  return caps;
}

static gboolean
gst_waveform_sink_open (GstAudioSink * asink)
{
  /* GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (asink); */

  /* nothing to do here as the device needs to be opened with the format we will use */

  return TRUE;
}

static gboolean
gst_waveform_sink_prepare (GstAudioSink * asink, GstAudioRingBufferSpec * spec)
{
  GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (asink);
  WAVEFORMATEX wfx;
  MMRESULT mmresult;
  guint index;

  /* setup waveformex structure with the input ringbuffer specs */
  memset (&wfx, 0, sizeof (wfx));
  wfx.cbSize = 0;
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = spec->info.channels;
  wfx.nSamplesPerSec = spec->info.rate;
  wfx.wBitsPerSample = (spec->info.bpf * 8) / wfx.nChannels;
  wfx.nBlockAlign = spec->info.bpf;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

  /* save bytes per sample to use it in delay */
  wfsink->bytes_per_sample = spec->info.bpf;

  /* open the default audio device with the given caps */
  mmresult = waveOutOpen (&wfsink->hwaveout, WAVE_MAPPER,
      &wfx, (DWORD_PTR) waveOutProc, (DWORD_PTR) wfsink, CALLBACK_FUNCTION);
  if (mmresult != MMSYSERR_NOERROR) {
    waveOutGetErrorText (mmresult, wfsink->error_string, ERROR_LENGTH - 1);
    GST_ELEMENT_ERROR (wfsink, RESOURCE, OPEN_WRITE,
        ("gst_waveform_sink_prepare: waveOutOpen failed error=>%s",
            wfsink->error_string), (NULL));
    return FALSE;
  }

  /* evaluate the buffer size and the number of buffers needed */
  wfsink->free_buffers_count = wfsink->buffer_count;

  /* allocate wave buffers */
  wfsink->wave_buffers = (WAVEHDR *) g_new0 (WAVEHDR, wfsink->buffer_count);
  if (!wfsink->wave_buffers) {
    GST_ELEMENT_ERROR (wfsink, RESOURCE, OPEN_WRITE,
        ("gst_waveform_sink_prepare: Failed to allocate wave buffer headers (buffer count=%d)",
            wfsink->buffer_count), (NULL));
    return FALSE;
  }
  memset (wfsink->wave_buffers, 0, sizeof (WAVEHDR) * wfsink->buffer_count);

  /* setup headers */
  for (index = 0; index < wfsink->buffer_count; index++) {
    wfsink->wave_buffers[index].dwBufferLength = wfsink->buffer_size;
    wfsink->wave_buffers[index].lpData = g_new0 (gchar, wfsink->buffer_size);
  }

  return TRUE;
}

static gboolean
gst_waveform_sink_unprepare (GstAudioSink * asink)
{
  GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (asink);

  /* free wave buffers */
  if (wfsink->wave_buffers) {
    guint index;

    for (index = 0; index < wfsink->buffer_count; index++) {
      if (wfsink->wave_buffers[index].dwFlags & WHDR_PREPARED) {
        MMRESULT mmresult = waveOutUnprepareHeader (wfsink->hwaveout,
            &wfsink->wave_buffers[index], sizeof (WAVEHDR));
        if (mmresult != MMSYSERR_NOERROR) {
          waveOutGetErrorText (mmresult, wfsink->error_string,
              ERROR_LENGTH - 1);
          GST_CAT_WARNING_OBJECT (waveformsink_debug, wfsink,
              "gst_waveform_sink_unprepare: Error unpreparing buffer => %s",
              wfsink->error_string);
        }
      }
      g_free (wfsink->wave_buffers[index].lpData);
    }
    g_free (wfsink->wave_buffers);
    wfsink->wave_buffers = NULL;
  }

  /* close waveform-audio output device */
  if (wfsink->hwaveout) {
    waveOutClose (wfsink->hwaveout);
    wfsink->hwaveout = NULL;
  }

  return TRUE;
}

static gboolean
gst_waveform_sink_close (GstAudioSink * asink)
{
  /* GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (asink); */

  return TRUE;
}

static gint
gst_waveform_sink_write (GstAudioSink * asink, gpointer data, guint length)
{
  GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (asink);
  WAVEHDR *waveheader;
  MMRESULT mmresult;
  guint bytes_to_write = length;
  guint remaining_length = length;

  wfsink->bytes_in_queue += length;

  while (remaining_length > 0) {
    if (wfsink->free_buffers_count == 0) {
      /* no free buffer available, wait for one */
      Sleep (10);
      continue;
    }

    /* get the current write buffer header */
    waveheader = &wfsink->wave_buffers[wfsink->write_buffer];

    /* unprepare the header if needed */
    if (waveheader->dwFlags & WHDR_PREPARED) {
      mmresult =
          waveOutUnprepareHeader (wfsink->hwaveout, waveheader,
          sizeof (WAVEHDR));
      if (mmresult != MMSYSERR_NOERROR) {
        waveOutGetErrorText (mmresult, wfsink->error_string, ERROR_LENGTH - 1);
        GST_CAT_WARNING_OBJECT (waveformsink_debug, wfsink,
            "Error unpreparing buffer => %s", wfsink->error_string);
      }
    }

    if (wfsink->buffer_size - waveheader->dwUser >= remaining_length)
      bytes_to_write = remaining_length;
    else
      bytes_to_write = wfsink->buffer_size - waveheader->dwUser;

    memcpy (waveheader->lpData + waveheader->dwUser, data, bytes_to_write);
    waveheader->dwUser += bytes_to_write;
    remaining_length -= bytes_to_write;
    data = (guint8 *) data + bytes_to_write;

    if (waveheader->dwUser == wfsink->buffer_size) {
      /* we have filled a buffer, let's prepare it and next write it to the device */
      mmresult =
          waveOutPrepareHeader (wfsink->hwaveout, waveheader, sizeof (WAVEHDR));
      if (mmresult != MMSYSERR_NOERROR) {
        waveOutGetErrorText (mmresult, wfsink->error_string, ERROR_LENGTH - 1);
        GST_CAT_WARNING_OBJECT (waveformsink_debug, wfsink,
            "gst_waveform_sink_write: Error preparing header => %s",
            wfsink->error_string);
      }
      mmresult = waveOutWrite (wfsink->hwaveout, waveheader, sizeof (WAVEHDR));
      if (mmresult != MMSYSERR_NOERROR) {
        waveOutGetErrorText (mmresult, wfsink->error_string, ERROR_LENGTH - 1);
        GST_CAT_WARNING_OBJECT (waveformsink_debug, wfsink,
            "gst_waveform_sink_write: Error writing buffer to the device => %s",
            wfsink->error_string);
      }

      EnterCriticalSection (&wfsink->critic_wave);
      wfsink->free_buffers_count--;
      LeaveCriticalSection (&wfsink->critic_wave);

      wfsink->write_buffer++;
      wfsink->write_buffer %= wfsink->buffer_count;
      waveheader->dwUser = 0;
      wfsink->bytes_in_queue = 0;
      GST_CAT_LOG_OBJECT (waveformsink_debug, wfsink,
          "gst_waveform_sink_write: Writing a buffer to the device (free buffers remaining=%d, write buffer=%d)",
          wfsink->free_buffers_count, wfsink->write_buffer);
    }
  }

  return length;
}

static guint
gst_waveform_sink_delay (GstAudioSink * asink)
{
  /* return the number of samples in queue (device+internal queue) */
  GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (asink);
  guint bytes_in_device =
      (wfsink->buffer_count - wfsink->free_buffers_count) * wfsink->buffer_size;
  guint delay =
      (bytes_in_device + wfsink->bytes_in_queue) / wfsink->bytes_per_sample;
  return delay;
}

static void
gst_waveform_sink_reset (GstAudioSink * asink)
{
  GstWaveFormSink *wfsink = GST_WAVEFORM_SINK (asink);
  MMRESULT mmresult = waveOutReset (wfsink->hwaveout);

  if (mmresult != MMSYSERR_NOERROR) {
    waveOutGetErrorText (mmresult, wfsink->error_string, ERROR_LENGTH - 1);
    GST_CAT_WARNING_OBJECT (waveformsink_debug, wfsink,
        "gst_waveform_sink_reset: Error resetting waveform-audio device => %s",
        wfsink->error_string);
  }
}

GstCaps *
gst_waveform_sink_create_caps (gint rate, gint channels, const gchar * format)
{
  GstCaps *caps = NULL;

  caps = gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, format,
      "layout", G_TYPE_STRING, "interleaved",
      "channels", G_TYPE_INT, channels, "rate", G_TYPE_INT, rate, NULL);
  return caps;
}

void CALLBACK
waveOutProc (HWAVEOUT hwo,
    UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
  GstWaveFormSink *wfsink = (GstWaveFormSink *) dwInstance;

  if (uMsg == WOM_DONE) {
    EnterCriticalSection (&wfsink->critic_wave);
    wfsink->free_buffers_count++;
    LeaveCriticalSection (&wfsink->critic_wave);
  }
}
