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

#ifndef __GES_H__
#define __GES_H__
#include <glib.h>
#include <gst/gst.h>

#include <ges/ges-types.h>
#include <ges/ges-enums.h>

#include <ges/ges-timeline.h>
#include <ges/ges-layer.h>
#include <ges/ges-timeline-element.h>
#include <ges/ges-clip.h>
#include <ges/ges-pipeline.h>
#include <ges/ges-source-clip.h>
#include <ges/ges-test-clip.h>
#include <ges/ges-title-clip.h>
#include <ges/ges-operation-clip.h>
#include <ges/ges-base-effect-clip.h>
#include <ges/ges-overlay-clip.h>
#include <ges/ges-text-overlay-clip.h>
#include <ges/ges-base-transition-clip.h>
#include <ges/ges-transition-clip.h>
#include <ges/ges-effect-clip.h>
#include <ges/ges-base-effect-clip.h>
#include <ges/ges-uri-clip.h>
#include <ges/ges-group.h>
#include <ges/ges-screenshot.h>
#include <ges/ges-asset.h>
#include <ges/ges-clip-asset.h>
#include <ges/ges-track-element-asset.h>
#include <ges/ges-uri-asset.h>
#include <ges/ges-project.h>
#include <ges/ges-extractable.h>
#include <ges/ges-base-xml-formatter.h>
#include <ges/ges-xml-formatter.h>

#include <ges/ges-track.h>
#include <ges/ges-track-element.h>
#include <ges/ges-source.h>
#include <ges/ges-operation.h>

#include <ges/ges-video-uri-source.h>
#include <ges/ges-audio-uri-source.h>
#include <ges/ges-image-source.h>
#include <ges/ges-multi-file-source.h>
#include <ges/ges-video-test-source.h>
#include <ges/ges-audio-test-source.h>
#include <ges/ges-title-source.h>
#include <ges/ges-text-overlay.h>
#include <ges/ges-transition.h>
#include <ges/ges-video-transition.h>
#include <ges/ges-audio-transition.h>
#include <ges/ges-base-effect.h>
#include <ges/ges-effect.h>
#include <ges/ges-formatter.h>
#include <ges/ges-pitivi-formatter.h>
#include <ges/ges-command-line-formatter.h>
#include <ges/ges-utils.h>
#include <ges/ges-meta-container.h>
#include <ges/ges-gerror.h>
#include <ges/ges-audio-track.h>
#include <ges/ges-video-track.h>
#include <ges/ges-version.h>

G_BEGIN_DECLS


gboolean ges_init         (void);
gboolean ges_init_check (int *argc, char **argv[], GError ** err);
void     ges_version      (guint * major, guint * minor, guint * micro,
                           guint * nano);
GOptionGroup *
ges_init_get_option_group (void);
gboolean ges_validate_register_action_types (void);

G_END_DECLS

#endif /* __GES_H__ */
