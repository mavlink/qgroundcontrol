/* GStreamer Tuner
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * tuner.h: tuner interface design
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

#ifndef __GST_TUNER_H__
#define __GST_TUNER_H__

#include <gst/gst.h>

#include "tunernorm.h"
#include "tunerchannel.h"

G_BEGIN_DECLS

#define GST_TYPE_TUNER \
  (gst_tuner_get_type ())
#define GST_TUNER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_TUNER, GstTuner))
#define GST_IS_TUNER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_TUNER))
#define GST_TUNER_GET_INTERFACE(inst) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GST_TYPE_TUNER, GstTunerInterface))

typedef struct _GstTuner GstTuner;
typedef struct _GstTunerInterface GstTunerInterface;

/**
 * GstTunerInterface:
 * @iface: the parent interface
 * @list_channels: list available channels
 * @set_channel: set to a channel
 * @get_channel: return the current channel
 * @list_norms: list available norms
 * @set_norm: set a norm
 * @get_norm: return the current norm
 * @set_frequency: set the frequency
 * @get_frequency: return the current frequency
 * @signal_strength: get the signal strength
 * @channel_changed: default handler for channel changed notification
 * @norm_changed: default handler for norm changed notification
 * @frequency_changed: default handler for frequency changed notification
 * @signal_changed: default handler for signal-strength changed notification
 *
 * Tuner interface.
 */
struct _GstTunerInterface {
  GTypeInterface iface;

  /* virtual functions */
  const GList * (* list_channels)   (GstTuner        *tuner);
  void          (* set_channel)     (GstTuner        *tuner,
                                     GstTunerChannel *channel);
  GstTunerChannel *
                (* get_channel)     (GstTuner        *tuner);

  const GList * (* list_norms)      (GstTuner        *tuner);
  void          (* set_norm)        (GstTuner        *tuner,
                                     GstTunerNorm    *norm);
  GstTunerNorm *(* get_norm)        (GstTuner        *tuner);

  void          (* set_frequency)   (GstTuner        *tuner,
                                     GstTunerChannel *channel,
                                     gulong           frequency);
  gulong        (* get_frequency)   (GstTuner        *tuner,
                                     GstTunerChannel *channel);
  gint          (* signal_strength) (GstTuner        *tuner,
                                     GstTunerChannel *channel);

  /* signals */
  void (*channel_changed)   (GstTuner        *tuner,
                             GstTunerChannel *channel);
  void (*norm_changed)      (GstTuner        *tuner,
                             GstTunerNorm    *norm);
  void (*frequency_changed) (GstTuner        *tuner,
                             GstTunerChannel *channel,
                             gulong           frequency);
  void (*signal_changed)    (GstTuner        *tuner,
                             GstTunerChannel *channel,
                             gint             signal);
};

GType           gst_tuner_get_type              (void);

/* virtual class function wrappers */
const GList *   gst_tuner_list_channels         (GstTuner        *tuner);
void            gst_tuner_set_channel           (GstTuner        *tuner,
                                                 GstTunerChannel *channel);
GstTunerChannel *
                gst_tuner_get_channel           (GstTuner        *tuner);

const GList *   gst_tuner_list_norms            (GstTuner        *tuner);
void            gst_tuner_set_norm              (GstTuner        *tuner,
                                                 GstTunerNorm    *norm);
GstTunerNorm *  gst_tuner_get_norm              (GstTuner        *tuner);

void            gst_tuner_set_frequency         (GstTuner        *tuner,
                                                 GstTunerChannel *channel,
                                                 gulong           frequency);
gulong          gst_tuner_get_frequency         (GstTuner        *tuner,
                                                 GstTunerChannel *channel);
gint            gst_tuner_signal_strength       (GstTuner        *tuner,
                                                 GstTunerChannel *channel);

/* helper functions */
GstTunerNorm *  gst_tuner_find_norm_by_name     (GstTuner        *tuner,
                                                 gchar           *norm);
GstTunerChannel *gst_tuner_find_channel_by_name (GstTuner        *tuner,
                                                 gchar           *channel);

/* trigger signals */
void            gst_tuner_channel_changed       (GstTuner        *tuner,
                                                 GstTunerChannel *channel);
void            gst_tuner_norm_changed          (GstTuner        *tuner,
                                                 GstTunerNorm    *norm);
void            gst_tuner_frequency_changed     (GstTuner        *tuner,
                                                 GstTunerChannel *channel,
                                                 gulong           frequency);
void            gst_tuner_signal_changed        (GstTuner        *tuner,
                                                 GstTunerChannel *channel,
                                                 gint             signal);

G_END_DECLS

#endif /* __GST_TUNER_H__ */
