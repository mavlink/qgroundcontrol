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

#ifndef _GES_PIPELINE
#define _GES_PIPELINE

#include <glib-object.h>
#include <ges/ges.h>
#include <gst/pbutils/encoding-profile.h>

G_BEGIN_DECLS

#define GES_TYPE_PIPELINE ges_pipeline_get_type()

#define GES_PIPELINE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_PIPELINE, GESPipeline))

#define GES_PIPELINE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_PIPELINE, GESPipelineClass))

#define GES_IS_PIPELINE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_PIPELINE))

#define GES_IS_PIPELINE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_PIPELINE))

#define GES_PIPELINE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_PIPELINE, GESPipelineClass))

typedef struct _GESPipelinePrivate GESPipelinePrivate;

/**
 * GESPipeline:
 *
 */

struct _GESPipeline {
  /*< private >*/
  GstPipeline parent;

  GESPipelinePrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

/**
 * GESPipelineClass:
 * @parent_class: parent class
 *
 */

struct _GESPipelineClass {
  /*< private >*/
  GstPipelineClass parent_class;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING];
};

GType ges_pipeline_get_type (void);

GESPipeline* ges_pipeline_new (void);

gboolean ges_pipeline_set_timeline (GESPipeline * pipeline,
					     GESTimeline * timeline);

gboolean ges_pipeline_set_render_settings (GESPipeline *pipeline,
						    const gchar * output_uri,
						    GstEncodingProfile *profile);
gboolean ges_pipeline_set_mode (GESPipeline *pipeline,
					 GESPipelineFlags mode);

GESPipelineFlags ges_pipeline_get_mode (GESPipeline *pipeline);

GstSample *
ges_pipeline_get_thumbnail(GESPipeline *self, GstCaps *caps);

GstSample *
ges_pipeline_get_thumbnail_rgb24(GESPipeline *self,
    gint width, gint height);

gboolean
ges_pipeline_save_thumbnail(GESPipeline *self,
    int width, int height, const gchar *format, const gchar *location,
    GError **error);

GstElement *
ges_pipeline_preview_get_video_sink (GESPipeline * self);

void
ges_pipeline_preview_set_video_sink (GESPipeline * self,
    GstElement * sink);

GstElement *
ges_pipeline_preview_get_audio_sink (GESPipeline * self);

void
ges_pipeline_preview_set_audio_sink (GESPipeline * self,
    GstElement * sink);

G_END_DECLS

#endif /* _GES_PIPELINE */

