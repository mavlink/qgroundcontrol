/* GStreamer Editing Services
 * Copyright (C) <2013> Thibault Saunier <thibault.saunier@collabora.com>
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

#ifndef _GES_AUDIO_TRACK_H_
#define _GES_AUDIO_TRACK_H_

#include <glib-object.h>

#include "ges-track.h"
#include "ges-types.h"

G_BEGIN_DECLS

#define GES_TYPE_AUDIO_TRACK             (ges_audio_track_get_type ())
#define GES_AUDIO_TRACK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_AUDIO_TRACK, GESAudioTrack))
#define GES_AUDIO_TRACK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_AUDIO_TRACK, GESAudioTrackClass))
#define GES_IS_AUDIO_TRACK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_AUDIO_TRACK))
#define GES_IS_AUDIO_TRACK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_AUDIO_TRACK))
#define GES_AUDIO_TRACK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_AUDIO_TRACK, GESAudioTrackClass))

typedef struct _GESAudioTrackPrivate GESAudioTrackPrivate;

struct _GESAudioTrackClass
{
  GESTrackClass parent_class;

  /* Padding for API extension */
  gpointer    _ges_reserved[GES_PADDING];
};

struct _GESAudioTrack
{
  GESTrack parent_instance;

  /*< private >*/
  GESAudioTrackPrivate *priv;
  /* Padding for API extension */
  gpointer         _ges_reserved[GES_PADDING];
};

GType          ges_audio_track_get_type (void) G_GNUC_CONST;
GESAudioTrack* ges_audio_track_new (void);

G_END_DECLS
#endif /* _GES_AUDIO_TRACK_H_ */
