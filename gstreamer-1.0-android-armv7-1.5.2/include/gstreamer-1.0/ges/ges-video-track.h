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

#ifndef _GES_VIDEO_TRACK_H_
#define _GES_VIDEO_TRACK_H_

#include <glib-object.h>

#include "ges-track.h"
#include "ges-types.h"

G_BEGIN_DECLS
#define GES_TYPE_VIDEO_TRACK             (ges_video_track_get_type ())
#define GES_VIDEO_TRACK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_VIDEO_TRACK, GESVideoTrack))
#define GES_VIDEO_TRACK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_VIDEO_TRACK, GESVideoTrackClass))
#define GES_IS_VIDEO_TRACK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_VIDEO_TRACK))
#define GES_IS_VIDEO_TRACK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_VIDEO_TRACK))
#define GES_VIDEO_TRACK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_VIDEO_TRACK, GESVideoTrackClass))

typedef struct _GESVideoTrackPrivate GESVideoTrackPrivate;

struct _GESVideoTrackClass
{
  GESTrackClass parent_class;

  /* Padding for API extension */
  gpointer    _ges_reserved[GES_PADDING];
};

struct _GESVideoTrack
{
  GESTrack parent_instance;

  /*< private >*/
  GESVideoTrackPrivate *priv;

  /* Padding for API extension */
  gpointer    _ges_reserved[GES_PADDING];
};

GType ges_video_track_get_type (void) G_GNUC_CONST;
GESVideoTrack * ges_video_track_new (void);

G_END_DECLS
#endif /* _GES_VIDEO_TRACK_H_ */
