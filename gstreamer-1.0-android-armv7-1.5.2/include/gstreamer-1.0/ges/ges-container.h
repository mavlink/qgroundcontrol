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

#ifndef _GES_CONTAINER
#define _GES_CONTAINER

#include <glib-object.h>
#include <gst/gst.h>
#include <ges/ges-timeline-element.h>
#include <ges/ges-types.h>
#include <ges/ges-track.h>

G_BEGIN_DECLS

#define GES_TYPE_CONTAINER             ges_container_get_type()
#define GES_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_CONTAINER, GESContainer))
#define GES_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_CONTAINER, GESContainerClass))
#define GES_IS_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_CONTAINER))
#define GES_IS_CONTAINER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_CONTAINER))
#define GES_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_CONTAINER, GESContainerClass))

typedef struct _GESContainerPrivate GESContainerPrivate;

/* To be used by sublcasses only */
typedef enum
{
  GES_CHILDREN_UPDATE,
  GES_CHILDREN_IGNORE_NOTIFIES,
  GES_CHILDREN_UPDATE_OFFSETS,
  GES_CHILDREN_UPDATE_ALL_VALUES,
  GES_CHILDREN_LAST
} GESChildrenControlMode;

/**
 * GES_CONTAINER_HEIGHT:
 * @obj: a #GESContainer
 *
 * The span of priorities this object occupies.
 */
#define GES_CONTAINER_HEIGHT(obj) (((GESContainer*)obj)->height)

/**
 * GES_CONTAINER_CHILDREN:
 * @obj: a #GESContainer
 *
 * A #GList containing the children of @object
 */
#define GES_CONTAINER_CHILDREN(obj) (((GESContainer*)obj)->children)

/**
 * GESContainer:
 * @children: (element-type GES.TimelineElement): A list of TimelineElement
 * controlled by this Container. NOTE: Do not modify.
 * @height: The span of priorities this container occupies
 *
 *
 * The #GESContainer base class.
 */
struct _GESContainer
{
  GESTimelineElement parent;

  /*< public > */
  /*< readonly >*/
  GList *children;

  /* We don't add those properties to the priv struct for optimization and code
   * readability purposes */
  guint32 height;       /* the span of priorities this object needs */

  /* <protected> */
  GESChildrenControlMode children_control_mode;
  /*< readonly >*/
  GESTimelineElement *initiated_move;

  /*< private >*/
  GESContainerPrivate *priv;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING_LARGE];
};

/**
 * GESContainerClass:
 * @child_added: Virtual method that is called right after a #GESTimelineElement is added
 * @child_removed: Virtual method that is called right after a #GESTimelineElement is removed
 * @remove_child: Virtual method to remove a child
 * @add_child: Virtual method to add a child
 * @ungroup: Ungroups the #GESTimelineElement contained in this #GESContainer, creating new
 * @group: Groups the #GESContainers together
 * #GESContainer containing those #GESTimelineElement apropriately.
 */
struct _GESContainerClass
{
  /*< private > */
  GESTimelineElementClass parent_class;

  /*< public > */
  /* signals */
  void (*child_added)             (GESContainer *container, GESTimelineElement *element);
  void (*child_removed)           (GESContainer *container, GESTimelineElement *element);
  gboolean (*add_child)           (GESContainer *container, GESTimelineElement *element);
  gboolean (*remove_child)        (GESContainer *container, GESTimelineElement *element);
  GList* (*ungroup)               (GESContainer *container, gboolean recursive);
  GESContainer * (*group)         (GList *containers);
  gboolean (*edit)                (GESContainer * container,
                                   GList * layers, gint new_layer_priority,
                                   GESEditMode mode,
                                   GESEdge edge,
                                   guint64 position);



  /*< private >*/
  guint grouping_priority;

  /* Padding for API extension */
  gpointer _ges_reserved[GES_PADDING_LARGE];
};

GType ges_container_get_type (void);

/* Children handling */
GList* ges_container_get_children (GESContainer *container, gboolean recursive);
gboolean ges_container_add        (GESContainer *container, GESTimelineElement *child);
gboolean ges_container_remove     (GESContainer *container, GESTimelineElement *child);
GList * ges_container_ungroup     (GESContainer * container, gboolean recursive);
GESContainer *ges_container_group (GList *containers);

/* To be used by subclasses only */
void _ges_container_set_height                (GESContainer * container,
                                               guint32 height);
gboolean ges_container_edit                   (GESContainer * container,
                                               GList * layers, gint new_layer_priority,
                                               GESEditMode mode,
                                               GESEdge edge,
                                               guint64 position);
gint  _ges_container_get_priority_offset      (GESContainer * container,
                                               GESTimelineElement *elem);
void _ges_container_set_priority_offset       (GESContainer * container,
                                               GESTimelineElement *elem,
                                               gint32 priority_offset);


G_END_DECLS
#endif /* _GES_CONTAINER */
