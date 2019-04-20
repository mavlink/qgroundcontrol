/* GStreamer
 * Copyright (C) 2010 Wim Taymans <wim.taymans at gmail.com>
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

#include <gst/gst.h>

#ifndef __GST_RTSP_THREAD_POOL_H__
#define __GST_RTSP_THREAD_POOL_H__

typedef struct _GstRTSPThread GstRTSPThread;
typedef struct _GstRTSPThreadPool GstRTSPThreadPool;
typedef struct _GstRTSPThreadPoolClass GstRTSPThreadPoolClass;
typedef struct _GstRTSPThreadPoolPrivate GstRTSPThreadPoolPrivate;

#include "rtsp-client.h"

G_BEGIN_DECLS

#define GST_TYPE_RTSP_THREAD_POOL              (gst_rtsp_thread_pool_get_type ())
#define GST_IS_RTSP_THREAD_POOL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_RTSP_THREAD_POOL))
#define GST_IS_RTSP_THREAD_POOL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_RTSP_THREAD_POOL))
#define GST_RTSP_THREAD_POOL_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_RTSP_THREAD_POOL, GstRTSPThreadPoolClass))
#define GST_RTSP_THREAD_POOL(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_RTSP_THREAD_POOL, GstRTSPThreadPool))
#define GST_RTSP_THREAD_POOL_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_RTSP_THREAD_POOL, GstRTSPThreadPoolClass))
#define GST_RTSP_THREAD_POOL_CAST(obj)         ((GstRTSPThreadPool*)(obj))
#define GST_RTSP_THREAD_POOL_CLASS_CAST(klass) ((GstRTSPThreadPoolClass*)(klass))

GType gst_rtsp_thread_get_type (void);

#define GST_TYPE_RTSP_THREAD        (gst_rtsp_thread_get_type ())
#define GST_IS_RTSP_THREAD(obj)     (GST_IS_MINI_OBJECT_TYPE (obj, GST_TYPE_RTSP_THREAD))
#define GST_RTSP_THREAD_CAST(obj)   ((GstRTSPThread*)(obj))
#define GST_RTSP_THREAD(obj)        (GST_RTSP_THREAD_CAST(obj))

/**
 * GstRTSPThreadType:
 * @GST_RTSP_THREAD_TYPE_CLIENT: a thread to handle the client communication
 * @GST_RTSP_THREAD_TYPE_MEDIA: a thread to handle media 
 *
 * Different thread types
 */
typedef enum
{
  GST_RTSP_THREAD_TYPE_CLIENT,
  GST_RTSP_THREAD_TYPE_MEDIA
} GstRTSPThreadType;

/**
 * GstRTSPThread:
 * @mini_object: parent #GstMiniObject
 * @type: the thread type
 * @context: a #GMainContext
 * @loop: a #GMainLoop
 *
 * Structure holding info about a mainloop running in a thread
 */
struct _GstRTSPThread {
  GstMiniObject mini_object;

  GstRTSPThreadType type;
  GMainContext *context;
  GMainLoop *loop;
};

GstRTSPThread *   gst_rtsp_thread_new      (GstRTSPThreadType type);

gboolean          gst_rtsp_thread_reuse    (GstRTSPThread * thread);
void              gst_rtsp_thread_stop     (GstRTSPThread * thread);

/**
 * gst_rtsp_thread_ref:
 * @thread: The thread to refcount
 *
 * Increase the refcount of this thread.
 *
 * Returns: (transfer full): @thread (for convenience when doing assignments)
 */
#ifdef _FOOL_GTK_DOC_
G_INLINE_FUNC GstRTSPThread * gst_rtsp_thread_ref (GstRTSPThread * thread);
#endif

static inline GstRTSPThread *
gst_rtsp_thread_ref (GstRTSPThread * thread)
{
  return (GstRTSPThread *) gst_mini_object_ref (GST_MINI_OBJECT_CAST (thread));
}

/**
 * gst_rtsp_thread_unref:
 * @thread: (transfer full): the thread to refcount
 *
 * Decrease the refcount of an thread, freeing it if the refcount reaches 0.
 */
#ifdef _FOOL_GTK_DOC_
G_INLINE_FUNC void gst_rtsp_thread_unref (GstRTSPPermissions * thread);
#endif


static inline void
gst_rtsp_thread_unref (GstRTSPThread * thread)
{
  gst_mini_object_unref (GST_MINI_OBJECT_CAST (thread));
}

/**
 * GstRTSPThreadPool:
 *
 * The thread pool structure.
 */
struct _GstRTSPThreadPool {
  GObject       parent;

  /*< private >*/
  GstRTSPThreadPoolPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

/**
 * GstRTSPThreadPoolClass:
 * @pool: a #GThreadPool used internally
 * @get_thread: this function should make or reuse an existing thread that runs
 *        a mainloop.
 * @configure_thread: configure a thread object. this vmethod is called when
 *       a new thread has been created and should be configured.
 * @thread_enter: called from the thread when it is entered
 * @thread_leave: called from the thread when it is left
 *
 * Class for managing threads.
 */
struct _GstRTSPThreadPoolClass {
  GObjectClass  parent_class;

  GThreadPool *pool;

  GstRTSPThread * (*get_thread)        (GstRTSPThreadPool *pool,
                                        GstRTSPThreadType type,
                                        GstRTSPContext *ctx);
  void            (*configure_thread)  (GstRTSPThreadPool *pool,
                                        GstRTSPThread * thread,
                                        GstRTSPContext *ctx);

  void            (*thread_enter)      (GstRTSPThreadPool *pool,
                                        GstRTSPThread *thread);
  void            (*thread_leave)      (GstRTSPThreadPool *pool,
                                        GstRTSPThread *thread);

  /*< private >*/
  gpointer         _gst_reserved[GST_PADDING];
};

GType               gst_rtsp_thread_pool_get_type        (void);

GstRTSPThreadPool * gst_rtsp_thread_pool_new             (void);

void                gst_rtsp_thread_pool_set_max_threads (GstRTSPThreadPool * pool, gint max_threads);
gint                gst_rtsp_thread_pool_get_max_threads (GstRTSPThreadPool * pool);

GstRTSPThread *     gst_rtsp_thread_pool_get_thread      (GstRTSPThreadPool *pool,
                                                          GstRTSPThreadType type,
                                                          GstRTSPContext *ctx);
void                gst_rtsp_thread_pool_cleanup         (void);
G_END_DECLS

#endif /* __GST_RTSP_THREAD_POOL_H__ */
