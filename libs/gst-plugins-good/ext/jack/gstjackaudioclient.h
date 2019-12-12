/* GStreamer
 * Copyright (C) 2006 Wim Taymans <wim@fluendo.com>
 *
 * gstjackaudioclient.h:
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

#ifndef __GST_JACK_AUDIO_CLIENT_H__
#define __GST_JACK_AUDIO_CLIENT_H__

#include <jack/jack.h>

#include <gst/gst.h>

G_BEGIN_DECLS

typedef enum 
{
  GST_JACK_CLIENT_SOURCE,
  GST_JACK_CLIENT_SINK
} GstJackClientType;

typedef struct _GstJackAudioClient GstJackAudioClient;

void                  gst_jack_audio_client_init           (void);


GstJackAudioClient *  gst_jack_audio_client_new            (const gchar *id, const gchar *server,
                                                            jack_client_t *jclient,
                                                            GstJackClientType type,
                                                            void (*shutdown) (void *arg),
                                                            JackProcessCallback    process,
                                                            JackBufferSizeCallback buffer_size,
                                                            JackSampleRateCallback sample_rate,
							    gpointer user_data,
							    jack_status_t *status);
void                  gst_jack_audio_client_free           (GstJackAudioClient *client);

jack_client_t *       gst_jack_audio_client_get_client     (GstJackAudioClient *client);

gboolean              gst_jack_audio_client_set_active     (GstJackAudioClient *client, gboolean active);

GstState              gst_jack_audio_client_get_transport_state (GstJackAudioClient *client);

G_END_DECLS

#endif /* __GST_JACK_AUDIO_CLIENT_H__ */
