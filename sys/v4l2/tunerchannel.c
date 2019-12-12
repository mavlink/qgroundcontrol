/* GStreamer Tuner
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * tunerchannel.c: tuner channel object design
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

#include "tunerchannel.h"

/**
 * SECTION:gsttunerchannel
 * @title: GstTunerChannel
 * @short_description: A channel from an element implementing the #GstTuner
 * interface.
 *
 * The #GstTunerChannel object is provided by an element implementing
 * the #GstTuner interface.
 *
 * GstTunerChannel provides a name and flags to determine the type and
 * capabilities of the channel. If the GST_TUNER_CHANNEL_FREQUENCY flag is
 * set, then the channel also information about the minimum and maximum
 * frequency, and range of the reported signal strength.
 */

enum
{
  /* FILL ME */
  SIGNAL_FREQUENCY_CHANGED,
  SIGNAL_SIGNAL_CHANGED,
  LAST_SIGNAL
};

G_DEFINE_TYPE (GstTunerChannel, gst_tuner_channel, G_TYPE_OBJECT);

static void gst_tuner_channel_dispose (GObject * object);

static guint signals[LAST_SIGNAL] = { 0 };

static void
gst_tuner_channel_class_init (GstTunerChannelClass * klass)
{
  GObjectClass *object_klass = (GObjectClass *) klass;

  /**
   * GstTunerChannel::frequency-changed:
   * @tunerchannel: The #GstTunerChannel
   * @frequency: The new frequency (an unsigned long)
   *
   * Reports that the current frequency has changed.
   */
  signals[SIGNAL_FREQUENCY_CHANGED] =
      g_signal_new ("frequency-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstTunerChannelClass, frequency_changed),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_ULONG);
  /**
   * GstTunerChannel::signal-changed:
   * @tunerchannel: The #GstTunerChannel
   * @signal: The new signal strength (an integer)
   *
   * Reports that the signal strength has changed.
   *
   * See Also: gst_tuner_signal_strength()
   */
  signals[SIGNAL_SIGNAL_CHANGED] =
      g_signal_new ("signal-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GstTunerChannelClass, signal_changed),
      NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT);

  object_klass->dispose = gst_tuner_channel_dispose;
}

static void
gst_tuner_channel_init (GstTunerChannel * channel)
{
  channel->label = NULL;
  channel->flags = 0;
  channel->min_frequency = channel->max_frequency = 0;
  channel->min_signal = channel->max_signal = 0;
}

static void
gst_tuner_channel_dispose (GObject * object)
{
  GstTunerChannel *channel = GST_TUNER_CHANNEL (object);

  if (channel->label) {
    g_free (channel->label);
    channel->label = NULL;
  }

  G_OBJECT_CLASS (gst_tuner_channel_parent_class)->dispose (object);
}
