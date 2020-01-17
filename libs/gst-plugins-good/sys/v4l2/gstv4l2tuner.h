/* GStreamer
 *
 * Copyright (C) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2tuner.h: tuner interface implementation for V4L2
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

#ifndef __GST_V4L2_TUNER_H__
#define __GST_V4L2_TUNER_H__

#include <gst/gst.h>

#include "tuner.h"
#include "gstv4l2object.h"

G_BEGIN_DECLS

#define GST_TYPE_V4L2_TUNER_CHANNEL \
  (gst_v4l2_tuner_channel_get_type ())
#define GST_V4L2_TUNER_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_V4L2_TUNER_CHANNEL, \
          GstV4l2TunerChannel))
#define GST_V4L2_TUNER_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_V4L2_TUNER_CHANNEL, \
       GstV4l2TunerChannelClass))
#define GST_IS_V4L2_TUNER_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_V4L2_TUNER_CHANNEL))
#define GST_IS_V4L2_TUNER_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_V4L2_TUNER_CHANNEL))

typedef struct _GstV4l2TunerChannel {
  GstTunerChannel parent;

  guint32         index;
  guint32         tuner;
  guint32         audio;
} GstV4l2TunerChannel;

typedef struct _GstV4l2TunerChannelClass {
  GstTunerChannelClass parent;
} GstV4l2TunerChannelClass;

#define GST_TYPE_V4L2_TUNER_NORM \
  (gst_v4l2_tuner_norm_get_type ())
#define GST_V4L2_TUNER_NORM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_V4L2_TUNER_NORM, \
          GstV4l2TunerNorm))
#define GST_V4L2_TUNER_NORM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_V4L2_TUNER_NORM, \
       GstV4l2TunerNormClass))
#define GST_IS_V4L2_TUNER_NORM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_V4L2_TUNER_NORM))
#define GST_IS_V4L2_TUNER_NORM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_V4L2_TUNER_NORM))

typedef struct _GstV4l2TunerNorm {
  GstTunerNorm parent;

  v4l2_std_id  index;
} GstV4l2TunerNorm;

typedef struct _GstV4l2TunerNormClass {
  GstTunerNormClass parent;
} GstV4l2TunerNormClass;

GType gst_v4l2_tuner_channel_get_type (void);
GType gst_v4l2_tuner_norm_get_type (void);

/* channels */
const GList*      gst_v4l2_tuner_list_channels            (GstV4l2Object * v4l2object);
GstTunerChannel*  gst_v4l2_tuner_get_channel              (GstV4l2Object * v4l2object);
gboolean          gst_v4l2_tuner_set_channel              (GstV4l2Object * v4l2object,
		                                           GstTunerChannel * channel);
/* norms */
const GList*      gst_v4l2_tuner_list_norms               (GstV4l2Object * v4l2object);
void              gst_v4l2_tuner_set_norm_and_notify      (GstV4l2Object * v4l2object,
		                                           GstTunerNorm * norm);
GstTunerNorm*     gst_v4l2_tuner_get_norm                 (GstV4l2Object * v4l2object);
gboolean          gst_v4l2_tuner_set_norm                 (GstV4l2Object * v4l2object,
		                                           GstTunerNorm * norm);
GstTunerNorm*     gst_v4l2_tuner_get_norm_by_std_id       (GstV4l2Object * v4l2object,
                                               v4l2_std_id norm);
v4l2_std_id       gst_v4l2_tuner_get_std_id_by_norm       (GstV4l2Object * v4l2object,
                                               GstTunerNorm * norm);

/* frequency */
void              gst_v4l2_tuner_set_frequency_and_notify (GstV4l2Object * v4l2object,
                                                           GstTunerChannel * channel, 
							   gulong frequency);
gint              gst_v4l2_tuner_signal_strength          (GstV4l2Object * v4l2object,
		                                           GstTunerChannel * channel);
gulong            gst_v4l2_tuner_get_frequency            (GstV4l2Object * v4l2object,
		                                           GstTunerChannel * channel);
gboolean          gst_v4l2_tuner_set_frequency            (GstV4l2Object * v4l2object,
                                                           GstTunerChannel * channel, 
							   gulong frequency);

#define GST_IMPLEMENT_V4L2_TUNER_METHODS(Type, interface_as_function)                 \
                                                                                      \
static const GList *                                                                  \
interface_as_function ## _tuner_list_channels (GstTuner * mixer)                      \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  return gst_v4l2_tuner_list_channels (this->v4l2object);                             \
}                                                                                     \
                                                                                      \
static void                                                                           \
interface_as_function ## _tuner_set_channel (GstTuner * mixer,                        \
                                             GstTunerChannel * channel)               \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  gst_v4l2_tuner_set_channel (this->v4l2object, channel);                             \
}                                                                                     \
static GstTunerChannel *                                                              \
interface_as_function ## _tuner_get_channel (GstTuner * mixer)                        \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  return gst_v4l2_tuner_get_channel (this->v4l2object);                               \
}                                                                                     \
static const GList *                                                                  \
interface_as_function ## _tuner_list_norms (GstTuner * mixer)                         \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  return gst_v4l2_tuner_list_norms (this->v4l2object);                                \
}                                                                                     \
static void                                                                           \
interface_as_function ## _tuner_set_norm_and_notify (GstTuner * mixer,                \
                                                     GstTunerNorm * norm)             \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  gst_v4l2_tuner_set_norm_and_notify (this->v4l2object, norm);                        \
}                                                                                     \
static GstTunerNorm *                                                                 \
interface_as_function ## _tuner_get_norm (GstTuner * mixer)                           \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  return gst_v4l2_tuner_get_norm (this->v4l2object);                                  \
}                                                                                     \
                                                                                      \
static void                                                                           \
interface_as_function ## _tuner_set_frequency_and_notify (GstTuner * mixer,           \
                                                          GstTunerChannel * channel,  \
                                                          gulong frequency)           \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  gst_v4l2_tuner_set_frequency_and_notify (this->v4l2object, channel, frequency);     \
}                                                                                     \
                                                                                      \
static gulong                                                                         \
interface_as_function ## _tuner_get_frequency (GstTuner * mixer,                      \
                                               GstTunerChannel * channel)             \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  return gst_v4l2_tuner_get_frequency (this->v4l2object, channel);                    \
}                                                                                     \
                                                                                      \
static gint                                                                           \
interface_as_function ## _tuner_signal_strength (GstTuner * mixer,                    \
                                                 GstTunerChannel * channel)           \
{                                                                                     \
  Type *this = (Type*) mixer;                                                         \
  return gst_v4l2_tuner_signal_strength (this->v4l2object, channel);                  \
}                                                                                     \
                                                                                      \
static void                                                                           \
interface_as_function ## _tuner_interface_init (GstTunerInterface * iface)                \
{                                                                                     \
  /* default virtual functions */                                                     \
  iface->list_channels = interface_as_function ## _tuner_list_channels;               \
  iface->set_channel = interface_as_function ## _tuner_set_channel;                   \
  iface->get_channel = interface_as_function ## _tuner_get_channel;                   \
                                                                                      \
  iface->list_norms = interface_as_function ## _tuner_list_norms;                     \
  iface->set_norm = interface_as_function ## _tuner_set_norm_and_notify;              \
  iface->get_norm = interface_as_function ## _tuner_get_norm;                         \
                                                                                      \
  iface->set_frequency = interface_as_function ## _tuner_set_frequency_and_notify;    \
  iface->get_frequency = interface_as_function ## _tuner_get_frequency;               \
  iface->signal_strength = interface_as_function ## _tuner_signal_strength;           \
}                                                                                     \

G_END_DECLS

#endif /* __GST_V4L2_TUNER_H__ */
