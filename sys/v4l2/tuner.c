/* GStreamer Tuner
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * tuner.c: tuner design virtual class function wrappers
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

#include "tuner.h"

#include <string.h>

/**
 * SECTION:gsttuner
 * @title: TunEr.h
 * @short_description: Interface for elements providing tuner operations
 *
 * The GstTuner interface is provided by elements that have the ability to
 * tune into multiple input signals, for example TV or radio capture cards.
 *
 * The interpretation of 'tuning into' an input stream depends on the element
 * implementing the interface. For v4lsrc, it might imply selection of an
 * input source and/or frequency to be configured on a TV card. Another
 * GstTuner implementation might be to allow selection of an active input pad
 * from multiple input pads.
 *
 * That said, the GstTuner interface functions are biased toward the
 * TV capture scenario.
 *
 * The general parameters provided are for configuration are:
 *
 * * Selection of a current #GstTunerChannel. The current channel
 *   represents the input source (e.g. Composite, S-Video etc for TV capture).
 * * The #GstTunerNorm for the channel. The norm chooses the
 *   interpretation of the incoming signal for the current channel. For example,
 *   PAL or NTSC, or more specific variants there-of.
 * * Channel frequency. If the current channel has the ability to tune
 *   between multiple frequencies (if it has the GST_TUNER_CHANNEL_FREQUENCY flag)
 *   then the frequency can be changed/retrieved via the
 *   gst_tuner_set_frequency() and gst_tuner_get_frequency() methods.
 *
 * Where applicable, the signal strength can be retrieved and/or monitored
 * via a signal.
 *
 */

/* FIXME 0.11: check if we need to add API for sometimes-supportedness
 * (aka making up for GstImplementsInterface removal) */

/* FIXME 0.11: replace signals with messages (+ make API thread-safe) */

enum
{
  NORM_CHANGED,
  CHANNEL_CHANGED,
  FREQUENCY_CHANGED,
  SIGNAL_CHANGED,
  LAST_SIGNAL
};

static guint gst_tuner_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_INTERFACE (GstTuner, gst_tuner, G_TYPE_INVALID);

static void
gst_tuner_default_init (GstTunerInterface * iface)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /**
     * GstTuner::norm-changed:
     * @tuner: The element providing the GstTuner interface
     * @norm: The new configured norm.
     *
     * Reports that the current #GstTunerNorm has changed.
     */
    gst_tuner_signals[NORM_CHANGED] =
        g_signal_new ("norm-changed",
        GST_TYPE_TUNER, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GstTunerInterface, norm_changed),
        NULL, NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_TUNER_NORM);
    /**
     * GstTuner::channel-changed:
     * @tuner: The element providing the GstTuner interface
     * @channel: The new configured channel.
     *
     * Reports that the current #GstTunerChannel has changed.
     */
    gst_tuner_signals[CHANNEL_CHANGED] =
        g_signal_new ("channel-changed",
        GST_TYPE_TUNER, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GstTunerInterface, channel_changed),
        NULL, NULL, NULL, G_TYPE_NONE, 1, GST_TYPE_TUNER_CHANNEL);
    /**
     * GstTuner::frequency-changed:
     * @tuner: The element providing the GstTuner interface
     * @frequency: The new frequency (an unsigned long)
     *
     * Reports that the current frequency has changed.
     */
    gst_tuner_signals[FREQUENCY_CHANGED] =
        g_signal_new ("frequency-changed",
        GST_TYPE_TUNER, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GstTunerInterface, frequency_changed),
        NULL, NULL, NULL, G_TYPE_NONE, 2, GST_TYPE_TUNER_CHANNEL, G_TYPE_ULONG);
    /**
     * GstTuner::signal-changed:
     * @tuner: The element providing the GstTuner interface
     * @channel: The current #GstTunerChannel
     * @signal: The new signal strength (an integer)
     *
     * Reports that the signal strength has changed.
     *
     * See Also: gst_tuner_signal_strength()
     */
    gst_tuner_signals[SIGNAL_CHANGED] =
        g_signal_new ("signal-changed",
        GST_TYPE_TUNER, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (GstTunerInterface, signal_changed),
        NULL, NULL, NULL, G_TYPE_NONE, 2, GST_TYPE_TUNER_CHANNEL, G_TYPE_INT);

    initialized = TRUE;
  }

  /* default virtual functions */
  iface->list_channels = NULL;
  iface->set_channel = NULL;
  iface->get_channel = NULL;

  iface->list_norms = NULL;
  iface->set_norm = NULL;
  iface->get_norm = NULL;

  iface->set_frequency = NULL;
  iface->get_frequency = NULL;
  iface->signal_strength = NULL;
}

/**
 * gst_tuner_list_channels:
 * @tuner: the #GstTuner (a #GstElement) to get the channels from.
 *
 * Retrieve a #GList of #GstTunerChannels available
 * (e.g. 'composite', 's-video', ...) from the given tuner object.
 *
 * Returns: A list of channels available on this tuner. The list is
 *          owned by the GstTuner and must not be freed.
 */
const GList *
gst_tuner_list_channels (GstTuner * tuner)
{
  GstTunerInterface *iface;

  g_return_val_if_fail (GST_IS_TUNER (tuner), NULL);

  iface = GST_TUNER_GET_INTERFACE (tuner);
  if (iface->list_channels) {
    return iface->list_channels (tuner);
  }

  return NULL;
}

/**
 * gst_tuner_set_channel:
 * @tuner: the #GstTuner (a #GstElement) that owns the channel.
 * @channel: the channel to tune to.
 *
 * Tunes the object to the given channel, which should be one of the
 * channels returned by gst_tuner_list_channels().
 */

void
gst_tuner_set_channel (GstTuner * tuner, GstTunerChannel * channel)
{
  GstTunerInterface *iface;

  g_return_if_fail (GST_IS_TUNER (tuner));

  iface = GST_TUNER_GET_INTERFACE (tuner);
  if (iface->set_channel) {
    iface->set_channel (tuner, channel);
  }
}

/**
 * gst_tuner_get_channel:
 * @tuner: the #GstTuner (a #GstElement) to get the current channel from.
 *
 * Retrieve the current channel from the tuner.
 *
 * Returns: the current channel of the tuner object.
 */

GstTunerChannel *
gst_tuner_get_channel (GstTuner * tuner)
{
  GstTunerInterface *iface;

  g_return_val_if_fail (GST_IS_TUNER (tuner), NULL);

  iface = GST_TUNER_GET_INTERFACE (tuner);
  if (iface->get_channel) {
    return iface->get_channel (tuner);
  }

  return NULL;
}

/**
 * gst_tuner_list_norms:
 * @tuner: the #GstTuner (*a #GstElement) to get the list of norms from.
 *
 * Retrieve a GList of available #GstTunerNorm settings for the currently
 * tuned channel on the given tuner object.
 *
 * Returns: A list of norms available on the current channel for this
 *          tuner object. The list is owned by the GstTuner and must not
 *          be freed.
 */

const GList *
gst_tuner_list_norms (GstTuner * tuner)
{
  GstTunerInterface *iface;

  g_return_val_if_fail (GST_IS_TUNER (tuner), NULL);

  iface = GST_TUNER_GET_INTERFACE (tuner);
  if (iface->list_norms) {
    return iface->list_norms (tuner);
  }

  return NULL;
}

/**
 * gst_tuner_set_norm:
 * @tuner: the #GstTuner (a #GstElement) to set the norm on.
 * @norm: the norm to use for the current channel.
 *
 * Changes the video norm on this tuner to the given norm, which should be
 * one of the norms returned by gst_tuner_list_norms().
 */

void
gst_tuner_set_norm (GstTuner * tuner, GstTunerNorm * norm)
{
  GstTunerInterface *iface;

  g_return_if_fail (GST_IS_TUNER (tuner));

  iface = GST_TUNER_GET_INTERFACE (tuner);
  if (iface->set_norm) {
    iface->set_norm (tuner, norm);
  }
}

/**
 * gst_tuner_get_norm:
 * @tuner: the #GstTuner (a #GstElement) to get the current norm from.
 *
 * Get the current video norm from the given tuner object for the
 * currently selected channel.
 *
 * Returns: the current norm.
 */

GstTunerNorm *
gst_tuner_get_norm (GstTuner * tuner)
{
  GstTunerInterface *iface;

  g_return_val_if_fail (GST_IS_TUNER (tuner), NULL);

  iface = GST_TUNER_GET_INTERFACE (tuner);
  if (iface->get_norm) {
    return iface->get_norm (tuner);
  }

  return NULL;
}

/**
 * gst_tuner_set_frequency:
 * @tuner: The #GstTuner (a #GstElement) that owns the given channel.
 * @channel: The #GstTunerChannel to set the frequency on.
 * @frequency: The frequency to tune in to.
 *
 * Sets a tuning frequency on the given tuner/channel. Note that this
 * requires the given channel to be a "tuning" channel, which can be
 * checked using GST_TUNER_CHANNEL_HAS_FLAG (), with the proper flag
 * being GST_TUNER_CHANNEL_FREQUENCY.
 *
 * The frequency is in Hz, with minimum steps indicated by the
 * frequency_multiplicator provided in the #GstTunerChannel. The
 * valid range is provided in the min_frequency and max_frequency properties
 * of the #GstTunerChannel.
 */

void
gst_tuner_set_frequency (GstTuner * tuner,
    GstTunerChannel * channel, gulong frequency)
{
  GstTunerInterface *iface;

  g_return_if_fail (GST_IS_TUNER (tuner));
  g_return_if_fail (GST_IS_TUNER_CHANNEL (channel));
  g_return_if_fail (GST_TUNER_CHANNEL_HAS_FLAG (channel,
          GST_TUNER_CHANNEL_FREQUENCY));

  iface = GST_TUNER_GET_INTERFACE (tuner);
  if (iface->set_frequency) {
    iface->set_frequency (tuner, channel, frequency);
  }
}

/**
 * gst_tuner_get_frequency:
 * @tuner: The #GstTuner (a #GstElement) that owns the given channel.
 * @channel: The #GstTunerChannel to retrieve the frequency from.
 *
 * Retrieve the current frequency from the given channel. As for
 * gst_tuner_set_frequency(), the #GstTunerChannel must support frequency
 * operations, as indicated by the GST_TUNER_CHANNEL_FREQUENCY flag.
 *
 * Returns: The current frequency, or 0 on error.
 */

gulong
gst_tuner_get_frequency (GstTuner * tuner, GstTunerChannel * channel)
{
  GstTunerInterface *iface;

  g_return_val_if_fail (GST_IS_TUNER (tuner), 0);
  g_return_val_if_fail (GST_IS_TUNER_CHANNEL (channel), 0);
  g_return_val_if_fail (GST_TUNER_CHANNEL_HAS_FLAG (channel,
          GST_TUNER_CHANNEL_FREQUENCY), 0);

  iface = GST_TUNER_GET_INTERFACE (tuner);

  if (iface->get_frequency) {
    return iface->get_frequency (tuner, channel);
  }

  return 0;
}

/**
 * gst_tuner_signal_strength:
 * @tuner: the #GstTuner (a #GstElement) that owns the given channel.
 * @channel: the #GstTunerChannel to get the signal strength from.
 *
 * Get the strength of the signal on this channel. Note that this
 * requires the current channel to be a "tuning" channel, i.e. a
 * channel on which frequency can be set. This can be checked using
 * GST_TUNER_CHANNEL_HAS_FLAG (), and the appropriate flag to check
 * for is GST_TUNER_CHANNEL_FREQUENCY.
 *
 * The valid range of the signal strength is indicated in the
 * min_signal and max_signal properties of the #GstTunerChannel.
 *
 * Returns: Signal strength, or 0 on error.
 */
gint
gst_tuner_signal_strength (GstTuner * tuner, GstTunerChannel * channel)
{
  GstTunerInterface *iface;

  g_return_val_if_fail (GST_IS_TUNER (tuner), 0);
  g_return_val_if_fail (GST_IS_TUNER_CHANNEL (channel), 0);
  g_return_val_if_fail (GST_TUNER_CHANNEL_HAS_FLAG (channel,
          GST_TUNER_CHANNEL_FREQUENCY), 0);

  iface = GST_TUNER_GET_INTERFACE (tuner);
  if (iface->signal_strength) {
    return iface->signal_strength (tuner, channel);
  }

  return 0;
}

/**
 * gst_tuner_find_norm_by_name:
 * @tuner: A #GstTuner instance
 * @norm: A string containing the name of a #GstTunerNorm
 *
 * Look up a #GstTunerNorm by name.
 *
 * Returns: A #GstTunerNorm, or NULL if no norm with the provided name
 * is available.
 */
GstTunerNorm *
gst_tuner_find_norm_by_name (GstTuner * tuner, gchar * norm)
{
  GList *walk;

  g_return_val_if_fail (GST_IS_TUNER (tuner), NULL);
  g_return_val_if_fail (norm != NULL, NULL);

  walk = (GList *) gst_tuner_list_norms (tuner);
  while (walk) {
    if (strcmp (GST_TUNER_NORM (walk->data)->label, norm) == 0)
      return GST_TUNER_NORM (walk->data);
    walk = g_list_next (walk);
  }
  return NULL;
}

/**
 * gst_tuner_find_channel_by_name:
 * @tuner: A #GstTuner instance
 * @channel: A string containing the name of a #GstTunerChannel
 *
 * Look up a #GstTunerChannel by name.
 *
 * Returns: A #GstTunerChannel, or NULL if no channel with the provided name
 * is available.
 */
GstTunerChannel *
gst_tuner_find_channel_by_name (GstTuner * tuner, gchar * channel)
{
  GList *walk;

  g_return_val_if_fail (GST_IS_TUNER (tuner), NULL);
  g_return_val_if_fail (channel != NULL, NULL);

  walk = (GList *) gst_tuner_list_channels (tuner);
  while (walk) {
    if (strcmp (GST_TUNER_CHANNEL (walk->data)->label, channel) == 0)
      return GST_TUNER_CHANNEL (walk->data);
    walk = g_list_next (walk);
  }
  return NULL;
}

/**
 * gst_tuner_channel_changed:
 * @tuner: A #GstTuner instance
 * @channel: A #GstTunerChannel instance
 *
 * Called by elements implementing the #GstTuner interface when the
 * current channel changes. Fires the #GstTuner::channel-changed signal.
 */
void
gst_tuner_channel_changed (GstTuner * tuner, GstTunerChannel * channel)
{
  g_return_if_fail (GST_IS_TUNER (tuner));
  g_return_if_fail (GST_IS_TUNER_CHANNEL (channel));

  g_signal_emit (G_OBJECT (tuner),
      gst_tuner_signals[CHANNEL_CHANGED], 0, channel);
}

/**
 * gst_tuner_norm_changed:
 * @tuner: A #GstTuner instance
 * @norm: A #GstTunerNorm instance
 *
 * Called by elements implementing the #GstTuner interface when the
 * current norm changes. Fires the #GstTuner::norm-changed signal.
 *
 */
void
gst_tuner_norm_changed (GstTuner * tuner, GstTunerNorm * norm)
{
  g_return_if_fail (GST_IS_TUNER (tuner));
  g_return_if_fail (GST_IS_TUNER_NORM (norm));

  g_signal_emit (G_OBJECT (tuner), gst_tuner_signals[NORM_CHANGED], 0, norm);
}

/**
 * gst_tuner_frequency_changed:
 * @tuner: A #GstTuner instance
 * @channel: The current #GstTunerChannel
 * @frequency: The new frequency setting
 *
 * Called by elements implementing the #GstTuner interface when the
 * configured frequency changes. Fires the #GstTuner::frequency-changed
 * signal on the tuner, and the #GstTunerChannel::frequency-changed signal
 * on the channel.
 */
void
gst_tuner_frequency_changed (GstTuner * tuner,
    GstTunerChannel * channel, gulong frequency)
{
  g_return_if_fail (GST_IS_TUNER (tuner));
  g_return_if_fail (GST_IS_TUNER_CHANNEL (channel));

  g_signal_emit (G_OBJECT (tuner),
      gst_tuner_signals[FREQUENCY_CHANGED], 0, channel, frequency);

  g_signal_emit_by_name (G_OBJECT (channel), "frequency_changed", frequency);
}

/**
 * gst_tuner_signal_changed:
 * @tuner: A #GstTuner instance
 * @channel: The current #GstTunerChannel
 * @signal: The new signal strength
 *
 * Called by elements implementing the #GstTuner interface when the
 * incoming signal strength changes. Fires the #GstTuner::signal-changed
 * signal on the tuner and the #GstTunerChannel::signal-changed signal on
 * the channel.
 */
void
gst_tuner_signal_changed (GstTuner * tuner,
    GstTunerChannel * channel, gint signal)
{
  g_return_if_fail (GST_IS_TUNER (tuner));
  g_return_if_fail (GST_IS_TUNER_CHANNEL (channel));

  g_signal_emit (G_OBJECT (tuner),
      gst_tuner_signals[SIGNAL_CHANGED], 0, channel, signal);

  g_signal_emit_by_name (G_OBJECT (channel), "signal_changed", signal);
}
