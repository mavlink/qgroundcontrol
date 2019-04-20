/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *               2009 Nokia Corporation
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

#ifndef _GES_AUDIO_SOURCE
#define _GES_AUDIO_SOURCE

#include <glib-object.h>
#include <gst/gst.h>
#include <ges/ges-types.h>
#include <ges/ges-track-element.h>
#include <ges/ges-source.h>

G_BEGIN_DECLS

#define GES_TYPE_AUDIO_SOURCE ges_audio_source_get_type()

#define GES_AUDIO_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_AUDIO_SOURCE, GESAudioSource))

#define GES_AUDIO_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_AUDIO_SOURCE, GESAudioSourceClass))

#define GES_IS_AUDIO_SOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_AUDIO_SOURCE))

#define GES_IS_AUDIO_SOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_AUDIO_SOURCE))

#define GES_AUDIO_SOURCE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_AUDIO_SOURCE, GESAudioSourceClass))

typedef struct _GESAudioSourcePrivate GESAudioSourcePrivate;

/**
 * GESAudioSource:
 *
 * Base class for audio sources
 */

struct _GESAudioSource {
  /*< private >*/
  GESSource parent;

  GESAudioSourcePrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESAudioSourceClass:
 * @create_source: method to return the GstElement to put in the source topbin.
 * Other elements will be queued, like a volume.
 * In the case of a AudioUriSource, for example, the subclass will return a decodebin,
 * and we will append a volume.
 */
struct _GESAudioSourceClass {
  /*< private >*/
  GESSourceClass parent_class;

  /*< public >*/
  GstElement*  (*create_source)           (GESTrackElement * object);

  /*< private >*/
  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_audio_source_get_type (void);

G_END_DECLS

#endif /* _GES_AUDIO_SOURCE */
