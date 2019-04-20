/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * gst-editing-services
 * Copyright (C) 2013 Thibault Saunier <tsaunier@gnome.org>
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

#ifndef _GES_SMART_ADDER_H_
#define _GES_SMART_ADDER_H_

#include <glib-object.h>
#include <gst/gst.h>

#include "ges-track.h"

G_BEGIN_DECLS

#define GES_TYPE_SMART_ADDER             (ges_smart_adder_get_type ())
#define GES_SMART_ADDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_SMART_ADDER, GESSmartAdder))
#define GES_SMART_ADDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_SMART_ADDER, GESSmartAdderClass))
#define GES_IS_SMART_ADDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_SMART_ADDER))
#define GES_IS_SMART_ADDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_SMART_ADDER))
#define GES_SMART_ADDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_SMART_ADDER, GESSmartAdderClass))

typedef struct _GESSmartAdderClass GESSmartAdderClass;
typedef struct _GESSmartAdder GESSmartAdder;

struct _GESSmartAdderClass
{
  GstBinClass parent_class;

  gpointer _ges_reserved[GES_PADDING];
};

struct _GESSmartAdder
{
  GstBin parent_instance;

  GHashTable *pads_infos;
  GstPad *srcpad;
  GstElement *adder;
  GMutex lock;

  GstCaps *caps;

  GESTrack *track;

  gpointer _ges_reserved[GES_PADDING];
};

GType         ges_smart_adder_get_type (void) G_GNUC_CONST;
GstElement*   ges_smart_adder_new      (GESTrack *track);

G_END_DECLS
#endif /* _GES_SMART_ADDER_H_ */
