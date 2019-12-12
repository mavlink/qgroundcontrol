/* GStreamer
 *
 * Copyright (C) 2001-2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2object.h: base class for V4L2 elements
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

#ifndef __GST_V4L2_OBJECT_H__
#define __GST_V4L2_OBJECT_H__

#include "ext/videodev2.h"
#ifdef HAVE_LIBV4L2
#  include <libv4l2.h>
#endif

#include "v4l2-utils.h"

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

#include <gst/video/video.h>
#include <unistd.h>

typedef struct _GstV4l2Object GstV4l2Object;
typedef struct _GstV4l2ObjectClassHelper GstV4l2ObjectClassHelper;

#include <gstv4l2bufferpool.h>

/* size of v4l2 buffer pool in streaming case */
#define GST_V4L2_MIN_BUFFERS 2

/* max frame width/height */
#define GST_V4L2_MAX_SIZE (1<<15) /* 2^15 == 32768 */

G_BEGIN_DECLS

#define GST_TYPE_V4L2_IO_MODE (gst_v4l2_io_mode_get_type ())
GType gst_v4l2_io_mode_get_type (void);

#define GST_V4L2_OBJECT(obj) (GstV4l2Object *)(obj)

typedef enum {
  GST_V4L2_IO_AUTO          = 0,
  GST_V4L2_IO_RW            = 1,
  GST_V4L2_IO_MMAP          = 2,
  GST_V4L2_IO_USERPTR       = 3,
  GST_V4L2_IO_DMABUF        = 4,
  GST_V4L2_IO_DMABUF_IMPORT = 5
} GstV4l2IOMode;

typedef gboolean  (*GstV4l2GetInOutFunction)  (GstV4l2Object * v4l2object, gint * input);
typedef gboolean  (*GstV4l2SetInOutFunction)  (GstV4l2Object * v4l2object, gint input);
typedef gboolean  (*GstV4l2UpdateFpsFunction) (GstV4l2Object * v4l2object);

/* On Android NDK r18b the ioctl() signature uses 'unsigned' instead of
 * 'unsigned long' for the 2nd parameter */
#ifdef __ANDROID__
typedef unsigned ioctl_req_t;
#else
typedef gulong ioctl_req_t;
#endif

#define GST_V4L2_WIDTH(o)        (GST_VIDEO_INFO_WIDTH (&(o)->info))
#define GST_V4L2_HEIGHT(o)       (GST_VIDEO_INFO_HEIGHT (&(o)->info))
#define GST_V4L2_PIXELFORMAT(o)  ((o)->fmtdesc->pixelformat)
#define GST_V4L2_FPS_N(o)        (GST_VIDEO_INFO_FPS_N (&(o)->info))
#define GST_V4L2_FPS_D(o)        (GST_VIDEO_INFO_FPS_D (&(o)->info))

/* simple check whether the device is open */
#define GST_V4L2_IS_OPEN(o)      ((o)->video_fd > 0)

/* check whether the device is 'active' */
#define GST_V4L2_IS_ACTIVE(o)    ((o)->active)
#define GST_V4L2_SET_ACTIVE(o)   ((o)->active = TRUE)
#define GST_V4L2_SET_INACTIVE(o) ((o)->active = FALSE)

/* checks whether the current v4lv4l2object has already been open()'ed or not */
#define GST_V4L2_CHECK_OPEN(v4l2object)				\
  if (!GST_V4L2_IS_OPEN(v4l2object))				\
  {								\
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,	\
      (_("Device is not open.")), (NULL));                      \
    return FALSE;						\
  }

/* checks whether the current v4lv4l2object is close()'ed or whether it is still open */
#define GST_V4L2_CHECK_NOT_OPEN(v4l2object)			\
  if (GST_V4L2_IS_OPEN(v4l2object))				\
  {								\
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,	\
      (_("Device is open.")), (NULL));                          \
    return FALSE;						\
  }

/* checks whether we're out of capture mode or not */
#define GST_V4L2_CHECK_NOT_ACTIVE(v4l2object)			\
  if (GST_V4L2_IS_ACTIVE(v4l2object))				\
  {								\
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS, \
      (NULL), ("Device is in streaming mode"));                 \
    return FALSE;						\
  }


struct _GstV4l2Object {
  GstElement * element;
  GstObject * dbg_obj;

  enum v4l2_buf_type type;   /* V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_BUF_TYPE_VIDEO_OUTPUT */

  /* the video device */
  char *videodev;

  /* the video-device's file descriptor */
  gint video_fd;
  GstV4l2IOMode mode;

  gboolean active;
  gboolean streaming;

  /* the current format */
  struct v4l2_fmtdesc *fmtdesc;
  struct v4l2_format format;
  GstVideoInfo info;
  GstVideoAlignment align;

  /* Features */
  gboolean need_video_meta;
  gboolean has_alpha_component;

  /* only used if the device supports MPLANE
   * nb planes is meaning of v4l2 planes
   * the gstreamer equivalent is gst_buffer_n_memory
   */
  gint n_v4l2_planes;

  /* We cache the frame duration if known */
  GstClockTime duration;

  /* if the MPLANE device support both contiguous and non contiguous
   * it allows to select which one we want. But we prefered_non_contiguous
   * non contiguous mode.
   */
  gboolean prefered_non_contiguous;

  /* This will be set if supported in decide_allocation. It can be used to
   * calculate the minimum latency. */
  guint32 min_buffers;

  /* wanted mode */
  GstV4l2IOMode req_mode;

  /* optional pool */
  GstBufferPool *pool;

  /* the video device's capabilities */
  struct v4l2_capability vcap;
  /* opened device specific capabilities */
  guint32 device_caps;

  /* lists... */
  GSList *formats;              /* list of available capture formats */
  GstCaps *probed_caps;

  GList *colors;
  GList *norms;
  GList *channels;
  GData *controls;

  /* properties */
  v4l2_std_id tv_norm;
  gchar *channel;
  gulong frequency;
  GstStructure *extra_controls;
  gboolean keep_aspect;
  GValue *par;

  /* funcs */
  GstV4l2GetInOutFunction  get_in_out_func;
  GstV4l2SetInOutFunction  set_in_out_func;
  GstV4l2UpdateFpsFunction update_fps_func;

  /* syscalls */
  gint (*fd_open) (gint fd, gint v4l2_flags);
  gint (*close) (gint fd);
  gint (*dup) (gint fd);
  gint (*ioctl) (gint fd, ioctl_req_t request, ...);
  gssize (*read) (gint fd, gpointer buffer, gsize n);
  gpointer (*mmap) (gpointer start, gsize length, gint prot, gint flags,
      gint fd,  off_t offset);
  gint (*munmap) (gpointer _start, gsize length);

  /* Quirks */
  /* Skips interlacing probes */
  gboolean never_interlaced;
  /* Allow to skip reading initial format through G_FMT. Some devices
   * just fails if you don't call S_FMT first. (ex: M2M decoders) */
  gboolean no_initial_format;
  /* Avoid any try_fmt probe. This is used by v4l2src to speedup start up time
   * on slow USB firmwares. When this is set, gst_v4l2_set_format() will modify
   * the caps to reflect what was negotiated during fixation */
  gboolean skip_try_fmt_probes;
};

struct _GstV4l2ObjectClassHelper {
  /* probed devices */
  GList *devices;
};

GType gst_v4l2_object_get_type (void);

#define V4L2_STD_OBJECT_PROPS \
    PROP_DEVICE,              \
    PROP_DEVICE_NAME,         \
    PROP_DEVICE_FD,           \
    PROP_FLAGS,               \
    PROP_BRIGHTNESS,          \
    PROP_CONTRAST,            \
    PROP_SATURATION,          \
    PROP_HUE,                 \
    PROP_TV_NORM,             \
    PROP_IO_MODE,             \
    PROP_OUTPUT_IO_MODE,      \
    PROP_CAPTURE_IO_MODE,     \
    PROP_EXTRA_CONTROLS,      \
    PROP_PIXEL_ASPECT_RATIO,  \
    PROP_FORCE_ASPECT_RATIO

/* create/destroy */
GstV4l2Object*  gst_v4l2_object_new       (GstElement * element,
                                           GstObject * dbg_obj,
                                           enum v4l2_buf_type  type,
                                           const char * default_device,
                                           GstV4l2GetInOutFunction get_in_out_func,
                                           GstV4l2SetInOutFunction set_in_out_func,
                                           GstV4l2UpdateFpsFunction update_fps_func);

void         gst_v4l2_object_destroy   (GstV4l2Object * v4l2object);

/* properties */

void         gst_v4l2_object_install_properties_helper (GObjectClass * gobject_class,
                                                        const char * default_device);

void         gst_v4l2_object_install_m2m_properties_helper (GObjectClass * gobject_class);

gboolean     gst_v4l2_object_set_property_helper       (GstV4l2Object * v4l2object,
                                                        guint prop_id,
                                                        const GValue * value,
                                                        GParamSpec * pspec);
gboolean     gst_v4l2_object_get_property_helper       (GstV4l2Object *v4l2object,
                                                        guint prop_id, GValue * value,
                                                        GParamSpec * pspec);
/* open/close */
gboolean     gst_v4l2_object_open            (GstV4l2Object * v4l2object);
gboolean     gst_v4l2_object_open_shared     (GstV4l2Object * v4l2object, GstV4l2Object * other);
gboolean     gst_v4l2_object_close           (GstV4l2Object * v4l2object);

/* probing */

GstCaps*     gst_v4l2_object_get_all_caps (void);

GstCaps*     gst_v4l2_object_get_raw_caps (void);

GstCaps*     gst_v4l2_object_get_codec_caps (void);

gint         gst_v4l2_object_extrapolate_stride (const GstVideoFormatInfo * finfo,
                                                  gint plane, gint stride);

gboolean     gst_v4l2_object_set_format  (GstV4l2Object * v4l2object, GstCaps * caps, GstV4l2Error * error);
gboolean     gst_v4l2_object_try_format  (GstV4l2Object * v4l2object, GstCaps * caps, GstV4l2Error * error);
gboolean     gst_v4l2_object_try_import  (GstV4l2Object * v4l2object, GstBuffer * buffer);

gboolean     gst_v4l2_object_caps_equal       (GstV4l2Object * v4l2object, GstCaps * caps);
gboolean     gst_v4l2_object_caps_is_subset   (GstV4l2Object * v4l2object, GstCaps * caps);
GstCaps *    gst_v4l2_object_get_current_caps (GstV4l2Object * v4l2object);

gboolean     gst_v4l2_object_unlock      (GstV4l2Object * v4l2object);
gboolean     gst_v4l2_object_unlock_stop (GstV4l2Object * v4l2object);

gboolean     gst_v4l2_object_stop        (GstV4l2Object * v4l2object);

GstCaps *    gst_v4l2_object_probe_caps  (GstV4l2Object * v4l2object, GstCaps * filter);
GstCaps *    gst_v4l2_object_get_caps    (GstV4l2Object * v4l2object, GstCaps * filter);

gboolean     gst_v4l2_object_acquire_format (GstV4l2Object * v4l2object, GstVideoInfo * info);

gboolean     gst_v4l2_object_set_crop    (GstV4l2Object * obj);

gboolean     gst_v4l2_object_decide_allocation (GstV4l2Object * v4l2object, GstQuery * query);

gboolean     gst_v4l2_object_propose_allocation (GstV4l2Object * obj, GstQuery * query);

GstStructure * gst_v4l2_object_v4l2fourcc_to_structure (guint32 fourcc);

/* TODO Move to proper namespace */
/* open/close the device */
gboolean     gst_v4l2_open           (GstV4l2Object * v4l2object);
gboolean     gst_v4l2_dup            (GstV4l2Object * v4l2object, GstV4l2Object * other);
gboolean     gst_v4l2_close          (GstV4l2Object * v4l2object);

/* norm/input/output */
gboolean     gst_v4l2_get_norm       (GstV4l2Object * v4l2object, v4l2_std_id * norm);
gboolean     gst_v4l2_set_norm       (GstV4l2Object * v4l2object, v4l2_std_id norm);
gboolean     gst_v4l2_get_input      (GstV4l2Object * v4l2object, gint * input);
gboolean     gst_v4l2_set_input      (GstV4l2Object * v4l2object, gint input);
gboolean     gst_v4l2_get_output     (GstV4l2Object * v4l2object, gint * output);
gboolean     gst_v4l2_set_output     (GstV4l2Object * v4l2object, gint output);

/* frequency control */
gboolean     gst_v4l2_get_frequency   (GstV4l2Object * v4l2object, gint tunernum, gulong * frequency);
gboolean     gst_v4l2_set_frequency   (GstV4l2Object * v4l2object, gint tunernum, gulong frequency);
gboolean     gst_v4l2_signal_strength (GstV4l2Object * v4l2object, gint tunernum, gulong * signal);

/* attribute control */
gboolean     gst_v4l2_get_attribute   (GstV4l2Object * v4l2object, int attribute, int * value);
gboolean     gst_v4l2_set_attribute   (GstV4l2Object * v4l2object, int attribute, const int value);
gboolean     gst_v4l2_set_controls    (GstV4l2Object * v4l2object, GstStructure * controls);

G_END_DECLS

#endif /* __GST_V4L2_OBJECT_H__ */
