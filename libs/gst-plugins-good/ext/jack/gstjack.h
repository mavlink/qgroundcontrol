/* GStreamer
 * Copyright (C) 2006 Wim Taymans <wim@fluendo.com>
 *
 * gstjack.h:
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

#ifndef _GST_JACK_H_
#define _GST_JACK_H_

#include <jack/jack.h>
#include <gst/audio/audio.h>

/**
 * GstJackConnect:
 * @GST_JACK_CONNECT_NONE: Don't automatically connect to physical ports.
 *     In this mode, the element will accept any number of input channels and will
 *     create (but not connect) an output port for each channel.
 * @GST_JACK_CONNECT_AUTO: In this mode, the element will try to connect each
 *     output port to a random physical jack input pin. The sink will
 *     expose the number of physical channels on its pad caps.
 * @GST_JACK_CONNECT_AUTO_FORCED: In this mode, the element will try to connect each
 *     output port to a random physical jack input pin. The  element will accept any number
 *     of input channels.
 *
 * Specify how the output ports will be connected.
 */
typedef enum {
  GST_JACK_CONNECT_NONE,
  GST_JACK_CONNECT_AUTO,
  GST_JACK_CONNECT_AUTO_FORCED
} GstJackConnect;

/**
 * GstJackTransport:
 * @GST_JACK_TRANSPORT_AUTONOMOUS: no transport support
 * @GST_JACK_TRANSPORT_MASTER: start and stop transport with state-changes
 * @GST_JACK_TRANSPORT_SLAVE: follow transport state changes
 *
 * The jack transport state allow to sync multiple clients. This enum defines a
 * client behaviour regarding to the transport mechanism.
 */
typedef enum {
  GST_JACK_TRANSPORT_AUTONOMOUS = 0,
  GST_JACK_TRANSPORT_MASTER = (1 << 0),
  GST_JACK_TRANSPORT_SLAVE = (1 << 1),
} GstJackTransport;

typedef jack_default_audio_sample_t sample_t;

#define GST_TYPE_JACK_CONNECT   (gst_jack_connect_get_type ())
#define GST_TYPE_JACK_TRANSPORT (gst_jack_transport_get_type ())
#define GST_TYPE_JACK_CLIENT    (gst_jack_client_get_type ())

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define GST_JACK_FORMAT_STR "F32LE"
#else
#define GST_JACK_FORMAT_STR "F32BE"
#endif

GType gst_jack_client_get_type(void);
GType gst_jack_connect_get_type(void);
GType gst_jack_transport_get_type(void);

#endif  // _GST_JACK_H_
