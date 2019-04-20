/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * gst-editing-services
 * Copyright (C) 2013 Mathieu Duponchelle <mduponchelle1@gmail.com>
 *
 * gst-editing-services is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gst-editing-services is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.";
 */

#ifndef _GES_SMART_MIXER_H_
#define _GES_SMART_MIXER_H_

#include <glib-object.h>
#include <gst/gst.h>

#include "ges-track.h"

G_BEGIN_DECLS

#define GES_TYPE_SMART_MIXER             (ges_smart_mixer_get_type ())
#define GES_SMART_MIXER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_SMART_MIXER, GESSmartMixer))
#define GES_SMART_MIXER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_SMART_MIXER, GESSmartMixerClass))
#define GES_IS_SMART_MIXER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_SMART_MIXER))
#define GES_IS_SMART_MIXER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_SMART_MIXER))
#define GES_SMART_MIXER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_SMART_MIXER, GESSmartMixerClass))

typedef struct _GESSmartMixerClass GESSmartMixerClass;
typedef struct _GESSmartMixer GESSmartMixer;

struct _GESSmartMixerClass
{
  GstBinClass parent_class;

  gpointer _ges_reserved[GES_PADDING];
};

struct _GESSmartMixer
{
  GstBin parent_instance;

  GHashTable *pads_infos;
  GstPad *srcpad;
  GstElement *mixer;
  GMutex lock;

  GstCaps *caps;

  GESTrack *track;

  gpointer _ges_reserved[GES_PADDING];
};

GType         ges_smart_mixer_get_type (void) G_GNUC_CONST;
GstElement*   ges_smart_mixer_new      (GESTrack *track);

G_END_DECLS
#endif /* _GES_SMART_MIXER_H_ */
