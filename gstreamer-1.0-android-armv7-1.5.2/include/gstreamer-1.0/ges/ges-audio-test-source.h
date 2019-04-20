/* GStreamer Editing Services
 * Copyright (C) 2010 Brandon Lewis <brandon.lewis@collabora.co.uk>
 *               2010 Nokia Corporation
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

#ifndef _GES_AUDIO_TEST_SOURCE
#define _GES_AUDIO_TEST_SOURCE

#include <glib-object.h>
#include <ges/ges-types.h>
#include <ges/ges-audio-source.h>

G_BEGIN_DECLS

#define GES_TYPE_AUDIO_TEST_SOURCE ges_audio_test_source_get_type()

#define GES_AUDIO_TEST_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_AUDIO_TEST_SOURCE, GESAudioTestSource))

#define GES_AUDIO_TEST_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_AUDIO_TEST_SOURCE, GESAudioTestSourceClass))

#define GES_IS_AUDIO_TEST_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_AUDIO_TEST_SOURCE))

#define GES_IS_AUDIO_TEST_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_AUDIO_TEST_SOURCE))

#define GES_AUDIO_TEST_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_AUDIO_TEST_SOURCE, GESAudioTestSourceClass))

typedef struct _GESAudioTestSourcePrivate GESAudioTestSourcePrivate;


/**
 * GESAudioTestSource:
 *
 */

struct _GESAudioTestSource {
  GESAudioSource parent;

  /*< private >*/
  GESAudioTestSourcePrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

struct _GESAudioTestSourceClass {
  /*< private >*/
  GESAudioSourceClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_audio_test_source_get_type (void);


void ges_audio_test_source_set_freq(GESAudioTestSource *self,
                                          gdouble freq);

void ges_audio_test_source_set_volume(GESAudioTestSource *self,
                                            gdouble volume);

double ges_audio_test_source_get_freq(GESAudioTestSource *self);
double ges_audio_test_source_get_volume(GESAudioTestSource *self);
G_END_DECLS

#endif /* _GES_AUDIO_TEST_SOURCE */

