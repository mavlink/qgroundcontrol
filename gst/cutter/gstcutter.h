/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_CUTTER_H__
#define __GST_CUTTER_H__


#include <gst/gst.h>
/* #include <gst/meta/audioraw.h> */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GST_TYPE_CUTTER \
  (gst_cutter_get_type())
#define GST_CUTTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CUTTER,GstCutter))
#define GST_CUTTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CUTTER,GstCutterClass))
#define GST_IS_CUTTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CUTTER))
#define GST_IS_CUTTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CUTTER))

typedef struct _GstCutter GstCutter;
typedef struct _GstCutterClass GstCutterClass;

struct _GstCutter
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  double threshold_level;       /* level below which to cut */
  double threshold_length;      /* how long signal has to remain
                                 * below this level before cutting */
  double silent_run_length;     /* how long has it been below threshold ? */
  gboolean silent;
  gboolean silent_prev;

  double pre_length;            /* how long can the pre-record buffer be ? */
  double pre_run_length;        /* how long is it currently ? */
  GList *pre_buffer;            /* list of GstBuffers in pre-record buffer */
  gboolean leaky;               /* do we leak an overflowing prebuffer ? */

  GstAudioInfo info;
};

struct _GstCutterClass
{
  GstElementClass parent_class;
  void (*cut_start) (GstCutter* filter);
  void (*cut_stop) (GstCutter* filter);
};

GType gst_cutter_get_type (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GST_STEREO_H__ */
