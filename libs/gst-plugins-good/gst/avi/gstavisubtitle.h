
#ifndef __GSTAVISUBTITLE_H__
#define __GSTAVISUBTITLE_H__

#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _GstAviSubtitle GstAviSubtitle;
typedef struct _GstAviSubtitleClass GstAviSubtitleClass;

#define GST_TYPE_AVI_SUBTITLE (gst_avi_subtitle_get_type ())
#define GST_AVI_SUBTITLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_AVI_SUBTITLE, GstAviSubtitle))
#define GST_AVI_SUBTITLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_AVI_SUBTITLE, GstAviSubtitleClass))
#define GST_IS_AVI_SUBTITLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_AVI_SUBTITLE))
#define GST_IS_AVI_SUBTITLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_AVI_SUBTITLE))
#define GST_AVI_SUBTITLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_AVI_SUBTITLE, GstAviSubtitleClass))

GType gst_avi_subtitle_get_type (void);

struct _GstAviSubtitle
{
  GstElement parent;

  GstPad    *src;
  GstPad    *sink;

  GstBuffer *subfile;  /* the complete subtitle file in one buffer */
};

struct _GstAviSubtitleClass
{
  GstElementClass parent;
};

G_END_DECLS
#endif
