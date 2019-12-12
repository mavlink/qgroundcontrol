/* GStreamer
 *
 * Copyright (C) 2001-2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * gstv4l2object.c: base class for V4L2 elements
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. This library is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>


#ifdef HAVE_GUDEV
#include <gudev/gudev.h>
#endif

#include "ext/videodev2.h"
#include "gstv4l2object.h"
#include "gstv4l2tuner.h"
#include "gstv4l2colorbalance.h"

#include "gst/gst-i18n-plugin.h"

#include <gst/video/video.h>
#include <gst/allocators/gstdmabuf.h>

GST_DEBUG_CATEGORY_EXTERN (v4l2_debug);
#define GST_CAT_DEFAULT v4l2_debug

#define DEFAULT_PROP_DEVICE_NAME        NULL
#define DEFAULT_PROP_DEVICE_FD          -1
#define DEFAULT_PROP_FLAGS              0
#define DEFAULT_PROP_TV_NORM            0
#define DEFAULT_PROP_IO_MODE            GST_V4L2_IO_AUTO

#define ENCODED_BUFFER_SIZE             (2 * 1024 * 1024)

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS,
};

/*
 * common format / caps utilities:
 */
typedef enum
{
  GST_V4L2_RAW = 1 << 0,
  GST_V4L2_CODEC = 1 << 1,
  GST_V4L2_TRANSPORT = 1 << 2,
  GST_V4L2_NO_PARSE = 1 << 3,
  GST_V4L2_ALL = 0xffff
} GstV4L2FormatFlags;

typedef struct
{
  guint32 format;
  gboolean dimensions;
  GstV4L2FormatFlags flags;
} GstV4L2FormatDesc;

static const GstV4L2FormatDesc gst_v4l2_formats[] = {
  /* RGB formats */
  {V4L2_PIX_FMT_RGB332, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_ARGB555, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_XRGB555, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_ARGB555X, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_XRGB555X, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB565, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB565X, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGR666, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGR24, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB24, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_ABGR32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_XBGR32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGRA32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGRX32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGBA32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGBX32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_ARGB32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_XRGB32, TRUE, GST_V4L2_RAW},

  /* Deprecated Packed RGB Image Formats (alpha ambiguity) */
  {V4L2_PIX_FMT_RGB444, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB555, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB555X, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_BGR32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_RGB32, TRUE, GST_V4L2_RAW},

  /* Grey formats */
  {V4L2_PIX_FMT_GREY, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y4, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y6, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y10, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y16_BE, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y10BPACK, TRUE, GST_V4L2_RAW},

  /* Palette formats */
  {V4L2_PIX_FMT_PAL8, TRUE, GST_V4L2_RAW},

  /* Chrominance formats */
  {V4L2_PIX_FMT_UV8, TRUE, GST_V4L2_RAW},

  /* Luminance+Chrominance formats */
  {V4L2_PIX_FMT_YVU410, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YVU420, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YVU420M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUYV, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YYUV, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YVYU, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_UYVY, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_VYUY, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV422P, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV411P, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_Y41P, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV444, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV555, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV565, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV32, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV410, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV420, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_YUV420M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_HI240, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_HM12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_M420, TRUE, GST_V4L2_RAW},

  /* two planes -- one Y, one Cr + Cb interleaved  */
  {V4L2_PIX_FMT_NV12, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV12M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV12MT, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV12MT_16X16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV21, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV21M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV16, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV16M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV61, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV61M, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV24, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_NV42, TRUE, GST_V4L2_RAW},

  /* Bayer formats - see http://www.siliconimaging.com/RGB%20Bayer.htm */
  {V4L2_PIX_FMT_SBGGR8, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGBRG8, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SGRBG8, TRUE, GST_V4L2_RAW},
  {V4L2_PIX_FMT_SRGGB8, TRUE, GST_V4L2_RAW},

  /* compressed formats */
  {V4L2_PIX_FMT_MJPEG, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_JPEG, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_PJPG, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_DV, FALSE, GST_V4L2_TRANSPORT},
  {V4L2_PIX_FMT_MPEG, FALSE, GST_V4L2_TRANSPORT},
  {V4L2_PIX_FMT_FWHT, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_H264, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_H264_NO_SC, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_H264_MVC, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_HEVC, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_H263, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_MPEG1, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_MPEG2, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_MPEG4, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_XVID, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_VC1_ANNEX_G, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_VC1_ANNEX_L, FALSE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_VP8, FALSE, GST_V4L2_CODEC | GST_V4L2_NO_PARSE},
  {V4L2_PIX_FMT_VP9, FALSE, GST_V4L2_CODEC | GST_V4L2_NO_PARSE},

  /*  Vendor-specific formats   */
  {V4L2_PIX_FMT_WNVA, TRUE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_SN9C10X, TRUE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_PWC1, TRUE, GST_V4L2_CODEC},
  {V4L2_PIX_FMT_PWC2, TRUE, GST_V4L2_CODEC},
};

#define GST_V4L2_FORMAT_COUNT (G_N_ELEMENTS (gst_v4l2_formats))

static GSList *gst_v4l2_object_get_format_list (GstV4l2Object * v4l2object);


#define GST_TYPE_V4L2_DEVICE_FLAGS (gst_v4l2_device_get_type ())
static GType
gst_v4l2_device_get_type (void)
{
  static GType v4l2_device_type = 0;

  if (v4l2_device_type == 0) {
    static const GFlagsValue values[] = {
      {V4L2_CAP_VIDEO_CAPTURE, "Device supports video capture", "capture"},
      {V4L2_CAP_VIDEO_OUTPUT, "Device supports video playback", "output"},
      {V4L2_CAP_VIDEO_OVERLAY, "Device supports video overlay", "overlay"},

      {V4L2_CAP_VBI_CAPTURE, "Device supports the VBI capture", "vbi-capture"},
      {V4L2_CAP_VBI_OUTPUT, "Device supports the VBI output", "vbi-output"},

      {V4L2_CAP_TUNER, "Device has a tuner or modulator", "tuner"},
      {V4L2_CAP_AUDIO, "Device has audio inputs or outputs", "audio"},

      {0, NULL, NULL}
    };

    v4l2_device_type =
        g_flags_register_static ("GstV4l2DeviceTypeFlags", values);
  }

  return v4l2_device_type;
}

#define GST_TYPE_V4L2_TV_NORM (gst_v4l2_tv_norm_get_type ())
static GType
gst_v4l2_tv_norm_get_type (void)
{
  static GType v4l2_tv_norm = 0;

  if (!v4l2_tv_norm) {
    static const GEnumValue tv_norms[] = {
      {0, "none", "none"},

      {V4L2_STD_NTSC, "NTSC", "NTSC"},
      {V4L2_STD_NTSC_M, "NTSC-M", "NTSC-M"},
      {V4L2_STD_NTSC_M_JP, "NTSC-M-JP", "NTSC-M-JP"},
      {V4L2_STD_NTSC_M_KR, "NTSC-M-KR", "NTSC-M-KR"},
      {V4L2_STD_NTSC_443, "NTSC-443", "NTSC-443"},

      {V4L2_STD_PAL, "PAL", "PAL"},
      {V4L2_STD_PAL_BG, "PAL-BG", "PAL-BG"},
      {V4L2_STD_PAL_B, "PAL-B", "PAL-B"},
      {V4L2_STD_PAL_B1, "PAL-B1", "PAL-B1"},
      {V4L2_STD_PAL_G, "PAL-G", "PAL-G"},
      {V4L2_STD_PAL_H, "PAL-H", "PAL-H"},
      {V4L2_STD_PAL_I, "PAL-I", "PAL-I"},
      {V4L2_STD_PAL_DK, "PAL-DK", "PAL-DK"},
      {V4L2_STD_PAL_D, "PAL-D", "PAL-D"},
      {V4L2_STD_PAL_D1, "PAL-D1", "PAL-D1"},
      {V4L2_STD_PAL_K, "PAL-K", "PAL-K"},
      {V4L2_STD_PAL_M, "PAL-M", "PAL-M"},
      {V4L2_STD_PAL_N, "PAL-N", "PAL-N"},
      {V4L2_STD_PAL_Nc, "PAL-Nc", "PAL-Nc"},
      {V4L2_STD_PAL_60, "PAL-60", "PAL-60"},

      {V4L2_STD_SECAM, "SECAM", "SECAM"},
      {V4L2_STD_SECAM_B, "SECAM-B", "SECAM-B"},
      {V4L2_STD_SECAM_G, "SECAM-G", "SECAM-G"},
      {V4L2_STD_SECAM_H, "SECAM-H", "SECAM-H"},
      {V4L2_STD_SECAM_DK, "SECAM-DK", "SECAM-DK"},
      {V4L2_STD_SECAM_D, "SECAM-D", "SECAM-D"},
      {V4L2_STD_SECAM_K, "SECAM-K", "SECAM-K"},
      {V4L2_STD_SECAM_K1, "SECAM-K1", "SECAM-K1"},
      {V4L2_STD_SECAM_L, "SECAM-L", "SECAM-L"},
      {V4L2_STD_SECAM_LC, "SECAM-Lc", "SECAM-Lc"},

      {0, NULL, NULL}
    };

    v4l2_tv_norm = g_enum_register_static ("V4L2_TV_norms", tv_norms);
  }

  return v4l2_tv_norm;
}

GType
gst_v4l2_io_mode_get_type (void)
{
  static GType v4l2_io_mode = 0;

  if (!v4l2_io_mode) {
    static const GEnumValue io_modes[] = {
      {GST_V4L2_IO_AUTO, "GST_V4L2_IO_AUTO", "auto"},
      {GST_V4L2_IO_RW, "GST_V4L2_IO_RW", "rw"},
      {GST_V4L2_IO_MMAP, "GST_V4L2_IO_MMAP", "mmap"},
      {GST_V4L2_IO_USERPTR, "GST_V4L2_IO_USERPTR", "userptr"},
      {GST_V4L2_IO_DMABUF, "GST_V4L2_IO_DMABUF", "dmabuf"},
      {GST_V4L2_IO_DMABUF_IMPORT, "GST_V4L2_IO_DMABUF_IMPORT",
          "dmabuf-import"},

      {0, NULL, NULL}
    };
    v4l2_io_mode = g_enum_register_static ("GstV4l2IOMode", io_modes);
  }
  return v4l2_io_mode;
}

void
gst_v4l2_object_install_properties_helper (GObjectClass * gobject_class,
    const char *default_device)
{
  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device", "Device location",
          default_device, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Name of the device", DEFAULT_PROP_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DEVICE_FD,
      g_param_spec_int ("device-fd", "File descriptor",
          "File descriptor of the device", -1, G_MAXINT, DEFAULT_PROP_DEVICE_FD,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_FLAGS,
      g_param_spec_flags ("flags", "Flags", "Device type flags",
          GST_TYPE_V4L2_DEVICE_FLAGS, DEFAULT_PROP_FLAGS,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * GstV4l2Src:brightness:
   *
   * Picture brightness, or more precisely, the black level
   */
  g_object_class_install_property (gobject_class, PROP_BRIGHTNESS,
      g_param_spec_int ("brightness", "Brightness",
          "Picture brightness, or more precisely, the black level", G_MININT,
          G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));
  /**
   * GstV4l2Src:contrast:
   *
   * Picture contrast or luma gain
   */
  g_object_class_install_property (gobject_class, PROP_CONTRAST,
      g_param_spec_int ("contrast", "Contrast",
          "Picture contrast or luma gain", G_MININT,
          G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));
  /**
   * GstV4l2Src:saturation:
   *
   * Picture color saturation or chroma gain
   */
  g_object_class_install_property (gobject_class, PROP_SATURATION,
      g_param_spec_int ("saturation", "Saturation",
          "Picture color saturation or chroma gain", G_MININT,
          G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));
  /**
   * GstV4l2Src:hue:
   *
   * Hue or color balance
   */
  g_object_class_install_property (gobject_class, PROP_HUE,
      g_param_spec_int ("hue", "Hue",
          "Hue or color balance", G_MININT,
          G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  /**
   * GstV4l2Src:norm:
   *
   * TV norm
   */
  g_object_class_install_property (gobject_class, PROP_TV_NORM,
      g_param_spec_enum ("norm", "TV norm",
          "video standard",
          GST_TYPE_V4L2_TV_NORM, DEFAULT_PROP_TV_NORM,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstV4l2Src:io-mode:
   *
   * IO Mode
   */
  g_object_class_install_property (gobject_class, PROP_IO_MODE,
      g_param_spec_enum ("io-mode", "IO mode",
          "I/O mode",
          GST_TYPE_V4L2_IO_MODE, DEFAULT_PROP_IO_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstV4l2Src:extra-controls:
   *
   * Additional v4l2 controls for the device. The controls are identified
   * by the control name (lowercase with '_' for any non-alphanumeric
   * characters).
   *
   * Since: 1.2
   */
  g_object_class_install_property (gobject_class, PROP_EXTRA_CONTROLS,
      g_param_spec_boxed ("extra-controls", "Extra Controls",
          "Extra v4l2 controls (CIDs) for the device",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstV4l2Src:pixel-aspect-ratio:
   *
   * The pixel aspect ratio of the device. This overwrites the pixel aspect
   * ratio queried from the device.
   *
   * Since: 1.2
   */
  g_object_class_install_property (gobject_class, PROP_PIXEL_ASPECT_RATIO,
      g_param_spec_string ("pixel-aspect-ratio", "Pixel Aspect Ratio",
          "Overwrite the pixel aspect ratio of the device", "1/1",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstV4l2Src:force-aspect-ratio:
   *
   * When enabled, the pixel aspect ratio queried from the device or set
   * with the pixel-aspect-ratio property will be enforced.
   *
   * Since: 1.2
   */
  g_object_class_install_property (gobject_class, PROP_FORCE_ASPECT_RATIO,
      g_param_spec_boolean ("force-aspect-ratio", "Force aspect ratio",
          "When enabled, the pixel aspect ratio will be enforced", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

void
gst_v4l2_object_install_m2m_properties_helper (GObjectClass * gobject_class)
{
  g_object_class_install_property (gobject_class, PROP_DEVICE,
      g_param_spec_string ("device", "Device", "Device location",
          NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device name",
          "Name of the device", DEFAULT_PROP_DEVICE_NAME,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEVICE_FD,
      g_param_spec_int ("device-fd", "File descriptor",
          "File descriptor of the device", -1, G_MAXINT, DEFAULT_PROP_DEVICE_FD,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_OUTPUT_IO_MODE,
      g_param_spec_enum ("output-io-mode", "Output IO mode",
          "Output side I/O mode (matches sink pad)",
          GST_TYPE_V4L2_IO_MODE, DEFAULT_PROP_IO_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CAPTURE_IO_MODE,
      g_param_spec_enum ("capture-io-mode", "Capture IO mode",
          "Capture I/O mode (matches src pad)",
          GST_TYPE_V4L2_IO_MODE, DEFAULT_PROP_IO_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_EXTRA_CONTROLS,
      g_param_spec_boxed ("extra-controls", "Extra Controls",
          "Extra v4l2 controls (CIDs) for the device",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/* Support for 32bit off_t, this wrapper is casting off_t to gint64 */
#ifdef HAVE_LIBV4L2
#if SIZEOF_OFF_T < 8

static gpointer
v4l2_mmap_wrapper (gpointer start, gsize length, gint prot, gint flags, gint fd,
    off_t offset)
{
  return v4l2_mmap (start, length, prot, flags, fd, (gint64) offset);
}

#define v4l2_mmap v4l2_mmap_wrapper

#endif /* SIZEOF_OFF_T < 8 */
#endif /* HAVE_LIBV4L2 */

GstV4l2Object *
gst_v4l2_object_new (GstElement * element,
    GstObject * debug_object,
    enum v4l2_buf_type type,
    const char *default_device,
    GstV4l2GetInOutFunction get_in_out_func,
    GstV4l2SetInOutFunction set_in_out_func,
    GstV4l2UpdateFpsFunction update_fps_func)
{
  GstV4l2Object *v4l2object;

  /*
   * some default values
   */
  v4l2object = g_new0 (GstV4l2Object, 1);

  v4l2object->type = type;
  v4l2object->formats = NULL;

  v4l2object->element = element;
  v4l2object->dbg_obj = debug_object;
  v4l2object->get_in_out_func = get_in_out_func;
  v4l2object->set_in_out_func = set_in_out_func;
  v4l2object->update_fps_func = update_fps_func;

  v4l2object->video_fd = -1;
  v4l2object->active = FALSE;
  v4l2object->videodev = g_strdup (default_device);

  v4l2object->norms = NULL;
  v4l2object->channels = NULL;
  v4l2object->colors = NULL;

  v4l2object->keep_aspect = TRUE;

  v4l2object->n_v4l2_planes = 0;

  v4l2object->no_initial_format = FALSE;

  /* We now disable libv4l2 by default, but have an env to enable it. */
#ifdef HAVE_LIBV4L2
  if (g_getenv ("GST_V4L2_USE_LIBV4L2")) {
    v4l2object->fd_open = v4l2_fd_open;
    v4l2object->close = v4l2_close;
    v4l2object->dup = v4l2_dup;
    v4l2object->ioctl = v4l2_ioctl;
    v4l2object->read = v4l2_read;
    v4l2object->mmap = v4l2_mmap;
    v4l2object->munmap = v4l2_munmap;
  } else
#endif
  {
    v4l2object->fd_open = NULL;
    v4l2object->close = close;
    v4l2object->dup = dup;
    v4l2object->ioctl = ioctl;
    v4l2object->read = read;
    v4l2object->mmap = mmap;
    v4l2object->munmap = munmap;
  }

  return v4l2object;
}

static gboolean gst_v4l2_object_clear_format_list (GstV4l2Object * v4l2object);


void
gst_v4l2_object_destroy (GstV4l2Object * v4l2object)
{
  g_return_if_fail (v4l2object != NULL);

  g_free (v4l2object->videodev);

  g_free (v4l2object->channel);

  if (v4l2object->formats) {
    gst_v4l2_object_clear_format_list (v4l2object);
  }

  if (v4l2object->probed_caps) {
    gst_caps_unref (v4l2object->probed_caps);
  }

  if (v4l2object->extra_controls) {
    gst_structure_free (v4l2object->extra_controls);
  }

  g_free (v4l2object);
}


static gboolean
gst_v4l2_object_clear_format_list (GstV4l2Object * v4l2object)
{
  g_slist_foreach (v4l2object->formats, (GFunc) g_free, NULL);
  g_slist_free (v4l2object->formats);
  v4l2object->formats = NULL;

  return TRUE;
}

static gint
gst_v4l2_object_prop_to_cid (guint prop_id)
{
  gint cid = -1;

  switch (prop_id) {
    case PROP_BRIGHTNESS:
      cid = V4L2_CID_BRIGHTNESS;
      break;
    case PROP_CONTRAST:
      cid = V4L2_CID_CONTRAST;
      break;
    case PROP_SATURATION:
      cid = V4L2_CID_SATURATION;
      break;
    case PROP_HUE:
      cid = V4L2_CID_HUE;
      break;
    default:
      GST_WARNING ("unmapped property id: %d", prop_id);
  }
  return cid;
}


gboolean
gst_v4l2_object_set_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_DEVICE:
      g_free (v4l2object->videodev);
      v4l2object->videodev = g_value_dup_string (value);
      break;
    case PROP_BRIGHTNESS:
    case PROP_CONTRAST:
    case PROP_SATURATION:
    case PROP_HUE:
    {
      gint cid = gst_v4l2_object_prop_to_cid (prop_id);

      if (cid != -1) {
        if (GST_V4L2_IS_OPEN (v4l2object)) {
          gst_v4l2_set_attribute (v4l2object, cid, g_value_get_int (value));
        }
      }
      return TRUE;
    }
      break;
    case PROP_TV_NORM:
      v4l2object->tv_norm = g_value_get_enum (value);
      break;
#if 0
    case PROP_CHANNEL:
      if (GST_V4L2_IS_OPEN (v4l2object)) {
        GstTuner *tuner = GST_TUNER (v4l2object->element);
        GstTunerChannel *channel = gst_tuner_find_channel_by_name (tuner,
            (gchar *) g_value_get_string (value));

        if (channel) {
          /* like gst_tuner_set_channel (tuner, channel)
             without g_object_notify */
          gst_v4l2_tuner_set_channel (v4l2object, channel);
        }
      } else {
        g_free (v4l2object->channel);
        v4l2object->channel = g_value_dup_string (value);
      }
      break;
    case PROP_FREQUENCY:
      if (GST_V4L2_IS_OPEN (v4l2object)) {
        GstTuner *tuner = GST_TUNER (v4l2object->element);
        GstTunerChannel *channel = gst_tuner_get_channel (tuner);

        if (channel &&
            GST_TUNER_CHANNEL_HAS_FLAG (channel, GST_TUNER_CHANNEL_FREQUENCY)) {
          /* like
             gst_tuner_set_frequency (tuner, channel, g_value_get_ulong (value))
             without g_object_notify */
          gst_v4l2_tuner_set_frequency (v4l2object, channel,
              g_value_get_ulong (value));
        }
      } else {
        v4l2object->frequency = g_value_get_ulong (value);
      }
      break;
#endif

    case PROP_IO_MODE:
      v4l2object->req_mode = g_value_get_enum (value);
      break;
    case PROP_CAPTURE_IO_MODE:
      g_return_val_if_fail (!V4L2_TYPE_IS_OUTPUT (v4l2object->type), FALSE);
      v4l2object->req_mode = g_value_get_enum (value);
      break;
    case PROP_OUTPUT_IO_MODE:
      g_return_val_if_fail (V4L2_TYPE_IS_OUTPUT (v4l2object->type), FALSE);
      v4l2object->req_mode = g_value_get_enum (value);
      break;
    case PROP_EXTRA_CONTROLS:{
      const GstStructure *s = gst_value_get_structure (value);

      if (v4l2object->extra_controls)
        gst_structure_free (v4l2object->extra_controls);

      v4l2object->extra_controls = s ? gst_structure_copy (s) : NULL;
      if (GST_V4L2_IS_OPEN (v4l2object))
        gst_v4l2_set_controls (v4l2object, v4l2object->extra_controls);
      break;
    }
    case PROP_PIXEL_ASPECT_RATIO:
      if (v4l2object->par) {
        g_value_unset (v4l2object->par);
        g_free (v4l2object->par);
      }
      v4l2object->par = g_new0 (GValue, 1);
      g_value_init (v4l2object->par, GST_TYPE_FRACTION);
      if (!g_value_transform (value, v4l2object->par)) {
        g_warning ("Could not transform string to aspect ratio");
        gst_value_set_fraction (v4l2object->par, 1, 1);
      }

      GST_DEBUG_OBJECT (v4l2object->dbg_obj, "set PAR to %d/%d",
          gst_value_get_fraction_numerator (v4l2object->par),
          gst_value_get_fraction_denominator (v4l2object->par));
      break;
    case PROP_FORCE_ASPECT_RATIO:
      v4l2object->keep_aspect = g_value_get_boolean (value);
      break;
    default:
      return FALSE;
      break;
  }
  return TRUE;
}


gboolean
gst_v4l2_object_get_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  switch (prop_id) {
    case PROP_DEVICE:
      g_value_set_string (value, v4l2object->videodev);
      break;
    case PROP_DEVICE_NAME:
    {
      const guchar *name = NULL;

      if (GST_V4L2_IS_OPEN (v4l2object))
        name = v4l2object->vcap.card;

      g_value_set_string (value, (gchar *) name);
      break;
    }
    case PROP_DEVICE_FD:
    {
      if (GST_V4L2_IS_OPEN (v4l2object))
        g_value_set_int (value, v4l2object->video_fd);
      else
        g_value_set_int (value, DEFAULT_PROP_DEVICE_FD);
      break;
    }
    case PROP_FLAGS:
    {
      guint flags = 0;

      if (GST_V4L2_IS_OPEN (v4l2object)) {
        flags |= v4l2object->device_caps &
            (V4L2_CAP_VIDEO_CAPTURE |
            V4L2_CAP_VIDEO_OUTPUT |
            V4L2_CAP_VIDEO_OVERLAY |
            V4L2_CAP_VBI_CAPTURE |
            V4L2_CAP_VBI_OUTPUT | V4L2_CAP_TUNER | V4L2_CAP_AUDIO);

        if (v4l2object->device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
          flags |= V4L2_CAP_VIDEO_CAPTURE;

        if (v4l2object->device_caps & V4L2_CAP_VIDEO_OUTPUT_MPLANE)
          flags |= V4L2_CAP_VIDEO_OUTPUT;
      }
      g_value_set_flags (value, flags);
      break;
    }
    case PROP_BRIGHTNESS:
    case PROP_CONTRAST:
    case PROP_SATURATION:
    case PROP_HUE:
    {
      gint cid = gst_v4l2_object_prop_to_cid (prop_id);

      if (cid != -1) {
        if (GST_V4L2_IS_OPEN (v4l2object)) {
          gint v;
          if (gst_v4l2_get_attribute (v4l2object, cid, &v)) {
            g_value_set_int (value, v);
          }
        }
      }
      return TRUE;
    }
      break;
    case PROP_TV_NORM:
      g_value_set_enum (value, v4l2object->tv_norm);
      break;
    case PROP_IO_MODE:
      g_value_set_enum (value, v4l2object->req_mode);
      break;
    case PROP_CAPTURE_IO_MODE:
      g_return_val_if_fail (!V4L2_TYPE_IS_OUTPUT (v4l2object->type), FALSE);
      g_value_set_enum (value, v4l2object->req_mode);
      break;
    case PROP_OUTPUT_IO_MODE:
      g_return_val_if_fail (V4L2_TYPE_IS_OUTPUT (v4l2object->type), FALSE);
      g_value_set_enum (value, v4l2object->req_mode);
      break;
    case PROP_EXTRA_CONTROLS:
      gst_value_set_structure (value, v4l2object->extra_controls);
      break;
    case PROP_PIXEL_ASPECT_RATIO:
      if (v4l2object->par)
        g_value_transform (v4l2object->par, value);
      break;
    case PROP_FORCE_ASPECT_RATIO:
      g_value_set_boolean (value, v4l2object->keep_aspect);
      break;
    default:
      return FALSE;
      break;
  }
  return TRUE;
}

static void
gst_v4l2_get_driver_min_buffers (GstV4l2Object * v4l2object)
{
  struct v4l2_control control = { 0, };

  g_return_if_fail (GST_V4L2_IS_OPEN (v4l2object));

  if (V4L2_TYPE_IS_OUTPUT (v4l2object->type))
    control.id = V4L2_CID_MIN_BUFFERS_FOR_OUTPUT;
  else
    control.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_CTRL, &control) == 0) {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "driver requires a minimum of %d buffers", control.value);
    v4l2object->min_buffers = control.value;
  } else {
    v4l2object->min_buffers = 0;
  }
}

static void
gst_v4l2_set_defaults (GstV4l2Object * v4l2object)
{
  GstTunerNorm *norm = NULL;
  GstTunerChannel *channel = NULL;
  GstTuner *tuner;

  if (!GST_IS_TUNER (v4l2object->element))
    return;

  tuner = GST_TUNER (v4l2object->element);

  if (v4l2object->tv_norm)
    norm = gst_v4l2_tuner_get_norm_by_std_id (v4l2object, v4l2object->tv_norm);
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "tv_norm=0x%" G_GINT64_MODIFIER "x, "
      "norm=%p", (guint64) v4l2object->tv_norm, norm);
  if (norm) {
    gst_tuner_set_norm (tuner, norm);
  } else {
    norm =
        GST_TUNER_NORM (gst_tuner_get_norm (GST_TUNER (v4l2object->element)));
    if (norm) {
      v4l2object->tv_norm =
          gst_v4l2_tuner_get_std_id_by_norm (v4l2object, norm);
      gst_tuner_norm_changed (tuner, norm);
    }
  }

  if (v4l2object->channel)
    channel = gst_tuner_find_channel_by_name (tuner, v4l2object->channel);
  if (channel) {
    gst_tuner_set_channel (tuner, channel);
  } else {
    channel =
        GST_TUNER_CHANNEL (gst_tuner_get_channel (GST_TUNER
            (v4l2object->element)));
    if (channel) {
      g_free (v4l2object->channel);
      v4l2object->channel = g_strdup (channel->label);
      gst_tuner_channel_changed (tuner, channel);
    }
  }

  if (channel
      && GST_TUNER_CHANNEL_HAS_FLAG (channel, GST_TUNER_CHANNEL_FREQUENCY)) {
    if (v4l2object->frequency != 0) {
      gst_tuner_set_frequency (tuner, channel, v4l2object->frequency);
    } else {
      v4l2object->frequency = gst_tuner_get_frequency (tuner, channel);
      if (v4l2object->frequency == 0) {
        /* guess */
        gst_tuner_set_frequency (tuner, channel, 1000);
      } else {
      }
    }
  }
}

gboolean
gst_v4l2_object_open (GstV4l2Object * v4l2object)
{
  if (gst_v4l2_open (v4l2object))
    gst_v4l2_set_defaults (v4l2object);
  else
    return FALSE;

  return TRUE;
}

gboolean
gst_v4l2_object_open_shared (GstV4l2Object * v4l2object, GstV4l2Object * other)
{
  gboolean ret;

  ret = gst_v4l2_dup (v4l2object, other);

  return ret;
}

gboolean
gst_v4l2_object_close (GstV4l2Object * v4l2object)
{
  if (!gst_v4l2_close (v4l2object))
    return FALSE;

  gst_caps_replace (&v4l2object->probed_caps, NULL);

  /* reset our copy of the device caps */
  v4l2object->device_caps = 0;

  if (v4l2object->formats) {
    gst_v4l2_object_clear_format_list (v4l2object);
  }

  if (v4l2object->par) {
    g_value_unset (v4l2object->par);
    g_free (v4l2object->par);
    v4l2object->par = NULL;
  }

  if (v4l2object->channel) {
    g_free (v4l2object->channel);
    v4l2object->channel = NULL;
  }

  return TRUE;
}

static struct v4l2_fmtdesc *
gst_v4l2_object_get_format_from_fourcc (GstV4l2Object * v4l2object,
    guint32 fourcc)
{
  struct v4l2_fmtdesc *fmt;
  GSList *walk;

  if (fourcc == 0)
    return NULL;

  walk = gst_v4l2_object_get_format_list (v4l2object);
  while (walk) {
    fmt = (struct v4l2_fmtdesc *) walk->data;
    if (fmt->pixelformat == fourcc)
      return fmt;
    /* special case for jpeg */
    if (fmt->pixelformat == V4L2_PIX_FMT_MJPEG ||
        fmt->pixelformat == V4L2_PIX_FMT_JPEG ||
        fmt->pixelformat == V4L2_PIX_FMT_PJPG) {
      if (fourcc == V4L2_PIX_FMT_JPEG || fourcc == V4L2_PIX_FMT_MJPEG ||
          fourcc == V4L2_PIX_FMT_PJPG) {
        return fmt;
      }
    }
    walk = g_slist_next (walk);
  }

  return NULL;
}



/* complete made up ranking, the values themselves are meaningless */
/* These ranks MUST be X such that X<<15 fits on a signed int - see
   the comment at the end of gst_v4l2_object_format_get_rank. */
#define YUV_BASE_RANK     1000
#define JPEG_BASE_RANK     500
#define DV_BASE_RANK       200
#define RGB_BASE_RANK      100
#define YUV_ODD_BASE_RANK   50
#define RGB_ODD_BASE_RANK   25
#define BAYER_BASE_RANK     15
#define S910_BASE_RANK      10
#define GREY_BASE_RANK       5
#define PWC_BASE_RANK        1

static gint
gst_v4l2_object_format_get_rank (const struct v4l2_fmtdesc *fmt)
{
  guint32 fourcc = fmt->pixelformat;
  gboolean emulated = ((fmt->flags & V4L2_FMT_FLAG_EMULATED) != 0);
  gint rank = 0;

  switch (fourcc) {
    case V4L2_PIX_FMT_MJPEG:
    case V4L2_PIX_FMT_PJPG:
      rank = JPEG_BASE_RANK;
      break;
    case V4L2_PIX_FMT_JPEG:
      rank = JPEG_BASE_RANK + 1;
      break;
    case V4L2_PIX_FMT_MPEG:    /* MPEG          */
      rank = JPEG_BASE_RANK + 2;
      break;

    case V4L2_PIX_FMT_RGB332:
    case V4L2_PIX_FMT_ARGB555:
    case V4L2_PIX_FMT_XRGB555:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_ARGB555X:
    case V4L2_PIX_FMT_XRGB555X:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_BGR666:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_RGB444:
    case V4L2_PIX_FMT_Y4:
    case V4L2_PIX_FMT_Y6:
    case V4L2_PIX_FMT_Y10:
    case V4L2_PIX_FMT_Y12:
    case V4L2_PIX_FMT_Y10BPACK:
    case V4L2_PIX_FMT_YUV555:
    case V4L2_PIX_FMT_YUV565:
    case V4L2_PIX_FMT_YUV32:
    case V4L2_PIX_FMT_NV12MT_16X16:
    case V4L2_PIX_FMT_NV42:
    case V4L2_PIX_FMT_H264_MVC:
      rank = RGB_ODD_BASE_RANK;
      break;

    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
      rank = RGB_BASE_RANK - 1;
      break;

    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_BGRA32:
    case V4L2_PIX_FMT_BGRX32:
    case V4L2_PIX_FMT_RGBA32:
    case V4L2_PIX_FMT_RGBX32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_XRGB32:
      rank = RGB_BASE_RANK;
      break;

    case V4L2_PIX_FMT_GREY:    /*  8  Greyscale     */
      rank = GREY_BASE_RANK;
      break;

    case V4L2_PIX_FMT_NV12:    /* 12  Y/CbCr 4:2:0  */
    case V4L2_PIX_FMT_NV12M:   /* Same as NV12      */
    case V4L2_PIX_FMT_NV12MT:  /* NV12 64x32 tile   */
    case V4L2_PIX_FMT_NV21:    /* 12  Y/CrCb 4:2:0  */
    case V4L2_PIX_FMT_NV21M:   /* Same as NV21      */
    case V4L2_PIX_FMT_YYUV:    /* 16  YUV 4:2:2     */
    case V4L2_PIX_FMT_HI240:   /*  8  8-bit color   */
    case V4L2_PIX_FMT_NV16:    /* 16  Y/CbCr 4:2:2  */
    case V4L2_PIX_FMT_NV16M:   /* Same as NV16      */
    case V4L2_PIX_FMT_NV61:    /* 16  Y/CrCb 4:2:2  */
    case V4L2_PIX_FMT_NV61M:   /* Same as NV61      */
    case V4L2_PIX_FMT_NV24:    /* 24  Y/CrCb 4:4:4  */
      rank = YUV_ODD_BASE_RANK;
      break;

    case V4L2_PIX_FMT_YVU410:  /* YVU9,  9 bits per pixel */
      rank = YUV_BASE_RANK + 3;
      break;
    case V4L2_PIX_FMT_YUV410:  /* YUV9,  9 bits per pixel */
      rank = YUV_BASE_RANK + 2;
      break;
    case V4L2_PIX_FMT_YUV420:  /* I420, 12 bits per pixel */
    case V4L2_PIX_FMT_YUV420M:
      rank = YUV_BASE_RANK + 7;
      break;
    case V4L2_PIX_FMT_YUYV:    /* YUY2, 16 bits per pixel */
      rank = YUV_BASE_RANK + 10;
      break;
    case V4L2_PIX_FMT_YVU420:  /* YV12, 12 bits per pixel */
      rank = YUV_BASE_RANK + 6;
      break;
    case V4L2_PIX_FMT_UYVY:    /* UYVY, 16 bits per pixel */
      rank = YUV_BASE_RANK + 9;
      break;
    case V4L2_PIX_FMT_YUV444:
      rank = YUV_BASE_RANK + 6;
      break;
    case V4L2_PIX_FMT_Y41P:    /* Y41P, 12 bits per pixel */
      rank = YUV_BASE_RANK + 5;
      break;
    case V4L2_PIX_FMT_YUV411P: /* Y41B, 12 bits per pixel */
      rank = YUV_BASE_RANK + 4;
      break;
    case V4L2_PIX_FMT_YUV422P: /* Y42B, 16 bits per pixel */
      rank = YUV_BASE_RANK + 8;
      break;

    case V4L2_PIX_FMT_DV:
      rank = DV_BASE_RANK;
      break;

    case V4L2_PIX_FMT_WNVA:    /* Winnov hw compress */
      rank = 0;
      break;

    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
      rank = BAYER_BASE_RANK;
      break;

    case V4L2_PIX_FMT_SN9C10X:
      rank = S910_BASE_RANK;
      break;

    case V4L2_PIX_FMT_PWC1:
      rank = PWC_BASE_RANK;
      break;
    case V4L2_PIX_FMT_PWC2:
      rank = PWC_BASE_RANK;
      break;

    default:
      rank = 0;
      break;
  }

  /* All ranks are below 1<<15 so a shift by 15
   * will a) make all non-emulated formats larger
   * than emulated and b) will not overflow
   */
  if (!emulated)
    rank <<= 15;

  return rank;
}



static gint
format_cmp_func (gconstpointer a, gconstpointer b)
{
  const struct v4l2_fmtdesc *fa = a;
  const struct v4l2_fmtdesc *fb = b;

  if (fa->pixelformat == fb->pixelformat)
    return 0;

  return gst_v4l2_object_format_get_rank (fb) -
      gst_v4l2_object_format_get_rank (fa);
}

/******************************************************
 * gst_v4l2_object_fill_format_list():
 *   create list of supported capture formats
 * return value: TRUE on success, FALSE on error
 ******************************************************/
static gboolean
gst_v4l2_object_fill_format_list (GstV4l2Object * v4l2object,
    enum v4l2_buf_type type)
{
  gint n;
  struct v4l2_fmtdesc *format;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "getting src format enumerations");

  /* format enumeration */
  for (n = 0;; n++) {
    format = g_new0 (struct v4l2_fmtdesc, 1);

    format->index = n;
    format->type = type;

    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_ENUM_FMT, format) < 0) {
      if (errno == EINVAL) {
        g_free (format);
        break;                  /* end of enumeration */
      } else {
        goto failed;
      }
    }

    GST_LOG_OBJECT (v4l2object->dbg_obj, "index:       %u", format->index);
    GST_LOG_OBJECT (v4l2object->dbg_obj, "type:        %d", format->type);
    GST_LOG_OBJECT (v4l2object->dbg_obj, "flags:       %08x", format->flags);
    GST_LOG_OBJECT (v4l2object->dbg_obj, "description: '%s'",
        format->description);
    GST_LOG_OBJECT (v4l2object->dbg_obj, "pixelformat: %" GST_FOURCC_FORMAT,
        GST_FOURCC_ARGS (format->pixelformat));

    /* sort formats according to our preference;  we do this, because caps
     * are probed in the order the formats are in the list, and the order of
     * formats in the final probed caps matters for things like fixation */
    v4l2object->formats = g_slist_insert_sorted (v4l2object->formats, format,
        (GCompareFunc) format_cmp_func);
  }

#ifndef GST_DISABLE_GST_DEBUG
  {
    GSList *l;

    GST_INFO_OBJECT (v4l2object->dbg_obj, "got %d format(s):", n);
    for (l = v4l2object->formats; l != NULL; l = l->next) {
      format = l->data;

      GST_INFO_OBJECT (v4l2object->dbg_obj,
          "  %" GST_FOURCC_FORMAT "%s", GST_FOURCC_ARGS (format->pixelformat),
          ((format->flags & V4L2_FMT_FLAG_EMULATED)) ? " (emulated)" : "");
    }
  }
#endif

  return TRUE;

  /* ERRORS */
failed:
  {
    g_free (format);

    if (v4l2object->element)
      return FALSE;

    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to enumerate possible video formats device '%s' can work "
                "with"), v4l2object->videodev),
        ("Failed to get number %d in pixelformat enumeration for %s. (%d - %s)",
            n, v4l2object->videodev, errno, g_strerror (errno)));

    return FALSE;
  }
}

/*
  * Get the list of supported capture formats, a list of
  * `struct v4l2_fmtdesc`.
  */
static GSList *
gst_v4l2_object_get_format_list (GstV4l2Object * v4l2object)
{
  if (!v4l2object->formats) {

    /* check usual way */
    gst_v4l2_object_fill_format_list (v4l2object, v4l2object->type);

    /* if our driver supports multi-planar
     * and if formats are still empty then we can workaround driver bug
     * by also looking up formats as if our device was not supporting
     * multiplanar */
    if (!v4l2object->formats) {
      switch (v4l2object->type) {
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
          gst_v4l2_object_fill_format_list (v4l2object,
              V4L2_BUF_TYPE_VIDEO_CAPTURE);
          break;

        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
          gst_v4l2_object_fill_format_list (v4l2object,
              V4L2_BUF_TYPE_VIDEO_OUTPUT);
          break;

        default:
          break;
      }
    }
  }
  return v4l2object->formats;
}

static GstVideoFormat
gst_v4l2_object_v4l2fourcc_to_video_format (guint32 fourcc)
{
  GstVideoFormat format;

  switch (fourcc) {
    case V4L2_PIX_FMT_GREY:    /*  8  Greyscale     */
      format = GST_VIDEO_FORMAT_GRAY8;
      break;
    case V4L2_PIX_FMT_Y16:
      format = GST_VIDEO_FORMAT_GRAY16_LE;
      break;
    case V4L2_PIX_FMT_Y16_BE:
      format = GST_VIDEO_FORMAT_GRAY16_BE;
      break;
    case V4L2_PIX_FMT_XRGB555:
    case V4L2_PIX_FMT_RGB555:
      format = GST_VIDEO_FORMAT_RGB15;
      break;
    case V4L2_PIX_FMT_XRGB555X:
    case V4L2_PIX_FMT_RGB555X:
      format = GST_VIDEO_FORMAT_BGR15;
      break;
    case V4L2_PIX_FMT_RGB565:
      format = GST_VIDEO_FORMAT_RGB16;
      break;
    case V4L2_PIX_FMT_RGB24:
      format = GST_VIDEO_FORMAT_RGB;
      break;
    case V4L2_PIX_FMT_BGR24:
      format = GST_VIDEO_FORMAT_BGR;
      break;
    case V4L2_PIX_FMT_XRGB32:
    case V4L2_PIX_FMT_RGB32:
      format = GST_VIDEO_FORMAT_xRGB;
      break;
    case V4L2_PIX_FMT_RGBX32:
      format = GST_VIDEO_FORMAT_RGBx;
      break;
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_BGR32:
      format = GST_VIDEO_FORMAT_BGRx;
      break;
    case V4L2_PIX_FMT_BGRX32:
      format = GST_VIDEO_FORMAT_xBGR;
      break;
    case V4L2_PIX_FMT_ABGR32:
      format = GST_VIDEO_FORMAT_BGRA;
      break;
    case V4L2_PIX_FMT_BGRA32:
      format = GST_VIDEO_FORMAT_ABGR;
      break;
    case V4L2_PIX_FMT_RGBA32:
      format = GST_VIDEO_FORMAT_RGB;
      break;
    case V4L2_PIX_FMT_ARGB32:
      format = GST_VIDEO_FORMAT_ARGB;
      break;
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12M:
      format = GST_VIDEO_FORMAT_NV12;
      break;
    case V4L2_PIX_FMT_NV12MT:
      format = GST_VIDEO_FORMAT_NV12_64Z32;
      break;
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV21M:
      format = GST_VIDEO_FORMAT_NV21;
      break;
    case V4L2_PIX_FMT_YVU410:
      format = GST_VIDEO_FORMAT_YVU9;
      break;
    case V4L2_PIX_FMT_YUV410:
      format = GST_VIDEO_FORMAT_YUV9;
      break;
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YUV420M:
      format = GST_VIDEO_FORMAT_I420;
      break;
    case V4L2_PIX_FMT_YUYV:
      format = GST_VIDEO_FORMAT_YUY2;
      break;
    case V4L2_PIX_FMT_YVU420:
      format = GST_VIDEO_FORMAT_YV12;
      break;
    case V4L2_PIX_FMT_UYVY:
      format = GST_VIDEO_FORMAT_UYVY;
      break;
    case V4L2_PIX_FMT_YUV411P:
      format = GST_VIDEO_FORMAT_Y41B;
      break;
    case V4L2_PIX_FMT_YUV422P:
      format = GST_VIDEO_FORMAT_Y42B;
      break;
    case V4L2_PIX_FMT_YVYU:
      format = GST_VIDEO_FORMAT_YVYU;
      break;
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV16M:
      format = GST_VIDEO_FORMAT_NV16;
      break;
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_NV61M:
      format = GST_VIDEO_FORMAT_NV61;
      break;
    case V4L2_PIX_FMT_NV24:
      format = GST_VIDEO_FORMAT_NV24;
      break;
    default:
      format = GST_VIDEO_FORMAT_UNKNOWN;
      break;
  }

  return format;
}

static gboolean
gst_v4l2_object_v4l2fourcc_is_rgb (guint32 fourcc)
{
  gboolean ret = FALSE;

  switch (fourcc) {
    case V4L2_PIX_FMT_XRGB555:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_XRGB555X:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
    case V4L2_PIX_FMT_XRGB32:
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_RGBA32:
    case V4L2_PIX_FMT_RGBX32:
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_BGRA32:
    case V4L2_PIX_FMT_BGRX32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
      ret = TRUE;
      break;
    default:
      break;
  }

  return ret;
}

static GstStructure *
gst_v4l2_object_v4l2fourcc_to_bare_struct (guint32 fourcc)
{
  GstStructure *structure = NULL;

  switch (fourcc) {
    case V4L2_PIX_FMT_MJPEG:   /* Motion-JPEG */
    case V4L2_PIX_FMT_PJPG:    /* Progressive-JPEG */
    case V4L2_PIX_FMT_JPEG:    /* JFIF JPEG */
      structure = gst_structure_new_empty ("image/jpeg");
      break;
    case V4L2_PIX_FMT_MPEG1:
      structure = gst_structure_new ("video/mpeg",
          "mpegversion", G_TYPE_INT, 1, NULL);
      break;
    case V4L2_PIX_FMT_MPEG2:
      structure = gst_structure_new ("video/mpeg",
          "mpegversion", G_TYPE_INT, 2, NULL);
      break;
    case V4L2_PIX_FMT_MPEG4:
    case V4L2_PIX_FMT_XVID:
      structure = gst_structure_new ("video/mpeg",
          "mpegversion", G_TYPE_INT, 4, "systemstream",
          G_TYPE_BOOLEAN, FALSE, NULL);
      break;
    case V4L2_PIX_FMT_FWHT:
      structure = gst_structure_new_empty ("video/x-fwht");
      break;
    case V4L2_PIX_FMT_H263:
      structure = gst_structure_new ("video/x-h263",
          "variant", G_TYPE_STRING, "itu", NULL);
      break;
    case V4L2_PIX_FMT_H264:    /* H.264 */
      structure = gst_structure_new ("video/x-h264",
          "stream-format", G_TYPE_STRING, "byte-stream", "alignment",
          G_TYPE_STRING, "au", NULL);
      break;
    case V4L2_PIX_FMT_H264_NO_SC:
      structure = gst_structure_new ("video/x-h264",
          "stream-format", G_TYPE_STRING, "avc", "alignment",
          G_TYPE_STRING, "au", NULL);
      break;
    case V4L2_PIX_FMT_HEVC:    /* H.265 */
      structure = gst_structure_new ("video/x-h265",
          "stream-format", G_TYPE_STRING, "byte-stream", "alignment",
          G_TYPE_STRING, "au", NULL);
      break;
    case V4L2_PIX_FMT_VC1_ANNEX_G:
    case V4L2_PIX_FMT_VC1_ANNEX_L:
      structure = gst_structure_new ("video/x-wmv",
          "wmvversion", G_TYPE_INT, 3, "format", G_TYPE_STRING, "WVC1", NULL);
      break;
    case V4L2_PIX_FMT_VP8:
      structure = gst_structure_new_empty ("video/x-vp8");
      break;
    case V4L2_PIX_FMT_VP9:
      structure = gst_structure_new_empty ("video/x-vp9");
      break;
    case V4L2_PIX_FMT_GREY:    /*  8  Greyscale     */
    case V4L2_PIX_FMT_Y16:
    case V4L2_PIX_FMT_Y16_BE:
    case V4L2_PIX_FMT_XRGB555:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_XRGB555X:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_BGR24:
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_XRGB32:
    case V4L2_PIX_FMT_ARGB32:
    case V4L2_PIX_FMT_RGBX32:
    case V4L2_PIX_FMT_RGBA32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_BGRX32:
    case V4L2_PIX_FMT_BGRA32:
    case V4L2_PIX_FMT_XBGR32:
    case V4L2_PIX_FMT_ABGR32:
    case V4L2_PIX_FMT_NV12:    /* 12  Y/CbCr 4:2:0  */
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV12MT:
    case V4L2_PIX_FMT_NV21:    /* 12  Y/CrCb 4:2:0  */
    case V4L2_PIX_FMT_NV21M:
    case V4L2_PIX_FMT_NV16:    /* 16  Y/CbCr 4:2:2  */
    case V4L2_PIX_FMT_NV16M:
    case V4L2_PIX_FMT_NV61:    /* 16  Y/CrCb 4:2:2  */
    case V4L2_PIX_FMT_NV61M:
    case V4L2_PIX_FMT_NV24:    /* 24  Y/CrCb 4:4:4  */
    case V4L2_PIX_FMT_YVU410:
    case V4L2_PIX_FMT_YUV410:
    case V4L2_PIX_FMT_YUV420:  /* I420/IYUV */
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_YUV411P:{
      GstVideoFormat format;
      format = gst_v4l2_object_v4l2fourcc_to_video_format (fourcc);
      if (format != GST_VIDEO_FORMAT_UNKNOWN)
        structure = gst_structure_new ("video/x-raw",
            "format", G_TYPE_STRING, gst_video_format_to_string (format), NULL);
      break;
    }
    case V4L2_PIX_FMT_DV:
      structure =
          gst_structure_new ("video/x-dv", "systemstream", G_TYPE_BOOLEAN, TRUE,
          NULL);
      break;
    case V4L2_PIX_FMT_MPEG:    /* MPEG          */
      structure = gst_structure_new ("video/mpegts",
          "systemstream", G_TYPE_BOOLEAN, TRUE, NULL);
      break;
    case V4L2_PIX_FMT_WNVA:    /* Winnov hw compress */
      break;
    case V4L2_PIX_FMT_SBGGR8:
    case V4L2_PIX_FMT_SGBRG8:
    case V4L2_PIX_FMT_SGRBG8:
    case V4L2_PIX_FMT_SRGGB8:
      structure = gst_structure_new ("video/x-bayer", "format", G_TYPE_STRING,
          fourcc == V4L2_PIX_FMT_SBGGR8 ? "bggr" :
          fourcc == V4L2_PIX_FMT_SGBRG8 ? "gbrg" :
          fourcc == V4L2_PIX_FMT_SGRBG8 ? "grbg" :
          /* fourcc == V4L2_PIX_FMT_SRGGB8 ? */ "rggb", NULL);
      break;
    case V4L2_PIX_FMT_SN9C10X:
      structure = gst_structure_new_empty ("video/x-sonix");
      break;
    case V4L2_PIX_FMT_PWC1:
      structure = gst_structure_new_empty ("video/x-pwc1");
      break;
    case V4L2_PIX_FMT_PWC2:
      structure = gst_structure_new_empty ("video/x-pwc2");
      break;
    case V4L2_PIX_FMT_RGB332:
    case V4L2_PIX_FMT_BGR666:
    case V4L2_PIX_FMT_ARGB555X:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_RGB444:
    case V4L2_PIX_FMT_YYUV:    /* 16  YUV 4:2:2     */
    case V4L2_PIX_FMT_HI240:   /*  8  8-bit color   */
    case V4L2_PIX_FMT_Y4:
    case V4L2_PIX_FMT_Y6:
    case V4L2_PIX_FMT_Y10:
    case V4L2_PIX_FMT_Y12:
    case V4L2_PIX_FMT_Y10BPACK:
    case V4L2_PIX_FMT_YUV444:
    case V4L2_PIX_FMT_YUV555:
    case V4L2_PIX_FMT_YUV565:
    case V4L2_PIX_FMT_Y41P:
    case V4L2_PIX_FMT_YUV32:
    case V4L2_PIX_FMT_NV12MT_16X16:
    case V4L2_PIX_FMT_NV42:
    case V4L2_PIX_FMT_H264_MVC:
    default:
      GST_DEBUG ("Unsupported fourcc 0x%08x %" GST_FOURCC_FORMAT,
          fourcc, GST_FOURCC_ARGS (fourcc));
      break;
  }

  return structure;
}

GstStructure *
gst_v4l2_object_v4l2fourcc_to_structure (guint32 fourcc)
{
  GstStructure *template;
  gint i;

  template = gst_v4l2_object_v4l2fourcc_to_bare_struct (fourcc);

  if (template == NULL)
    goto done;

  for (i = 0; i < GST_V4L2_FORMAT_COUNT; i++) {
    if (gst_v4l2_formats[i].format != fourcc)
      continue;

    if (gst_v4l2_formats[i].dimensions) {
      gst_structure_set (template,
          "width", GST_TYPE_INT_RANGE, 1, GST_V4L2_MAX_SIZE,
          "height", GST_TYPE_INT_RANGE, 1, GST_V4L2_MAX_SIZE,
          "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
    }
    break;
  }

done:
  return template;
}


static GstCaps *
gst_v4l2_object_get_caps_helper (GstV4L2FormatFlags flags)
{
  GstStructure *structure;
  GstCaps *caps;
  guint i;

  caps = gst_caps_new_empty ();
  for (i = 0; i < GST_V4L2_FORMAT_COUNT; i++) {

    if ((gst_v4l2_formats[i].flags & flags) == 0)
      continue;

    structure =
        gst_v4l2_object_v4l2fourcc_to_bare_struct (gst_v4l2_formats[i].format);

    if (structure) {
      GstStructure *alt_s = NULL;

      if (gst_v4l2_formats[i].dimensions) {
        gst_structure_set (structure,
            "width", GST_TYPE_INT_RANGE, 1, GST_V4L2_MAX_SIZE,
            "height", GST_TYPE_INT_RANGE, 1, GST_V4L2_MAX_SIZE,
            "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
      }

      switch (gst_v4l2_formats[i].format) {
        case V4L2_PIX_FMT_RGB32:
          alt_s = gst_structure_copy (structure);
          gst_structure_set (alt_s, "format", G_TYPE_STRING, "ARGB", NULL);
          break;
        case V4L2_PIX_FMT_BGR32:
          alt_s = gst_structure_copy (structure);
          gst_structure_set (alt_s, "format", G_TYPE_STRING, "BGRA", NULL);
        default:
          break;
      }

      gst_caps_append_structure (caps, structure);

      if (alt_s)
        gst_caps_append_structure (caps, alt_s);
    }
  }

  return gst_caps_simplify (caps);
}

GstCaps *
gst_v4l2_object_get_all_caps (void)
{
  static GstCaps *caps = NULL;

  if (g_once_init_enter (&caps)) {
    GstCaps *all_caps = gst_v4l2_object_get_caps_helper (GST_V4L2_ALL);
    GST_MINI_OBJECT_FLAG_SET (all_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
    g_once_init_leave (&caps, all_caps);
  }

  return caps;
}

GstCaps *
gst_v4l2_object_get_raw_caps (void)
{
  static GstCaps *caps = NULL;

  if (g_once_init_enter (&caps)) {
    GstCaps *raw_caps = gst_v4l2_object_get_caps_helper (GST_V4L2_RAW);
    GST_MINI_OBJECT_FLAG_SET (raw_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
    g_once_init_leave (&caps, raw_caps);
  }

  return caps;
}

GstCaps *
gst_v4l2_object_get_codec_caps (void)
{
  static GstCaps *caps = NULL;

  if (g_once_init_enter (&caps)) {
    GstCaps *codec_caps = gst_v4l2_object_get_caps_helper (GST_V4L2_CODEC);
    GST_MINI_OBJECT_FLAG_SET (codec_caps, GST_MINI_OBJECT_FLAG_MAY_BE_LEAKED);
    g_once_init_leave (&caps, codec_caps);
  }

  return caps;
}

/* collect data for the given caps
 * @caps: given input caps
 * @format: location for the v4l format
 * @w/@h: location for width and height
 * @fps_n/@fps_d: location for framerate
 * @size: location for expected size of the frame or 0 if unknown
 */
static gboolean
gst_v4l2_object_get_caps_info (GstV4l2Object * v4l2object, GstCaps * caps,
    struct v4l2_fmtdesc **format, GstVideoInfo * info)
{
  GstStructure *structure;
  guint32 fourcc = 0, fourcc_nc = 0;
  const gchar *mimetype;
  struct v4l2_fmtdesc *fmt = NULL;

  structure = gst_caps_get_structure (caps, 0);

  mimetype = gst_structure_get_name (structure);

  if (!gst_video_info_from_caps (info, caps))
    goto invalid_format;

  if (g_str_equal (mimetype, "video/x-raw")) {
    switch (GST_VIDEO_INFO_FORMAT (info)) {
      case GST_VIDEO_FORMAT_I420:
        fourcc = V4L2_PIX_FMT_YUV420;
        fourcc_nc = V4L2_PIX_FMT_YUV420M;
        break;
      case GST_VIDEO_FORMAT_YUY2:
        fourcc = V4L2_PIX_FMT_YUYV;
        break;
      case GST_VIDEO_FORMAT_UYVY:
        fourcc = V4L2_PIX_FMT_UYVY;
        break;
      case GST_VIDEO_FORMAT_YV12:
        fourcc = V4L2_PIX_FMT_YVU420;
        break;
      case GST_VIDEO_FORMAT_Y41B:
        fourcc = V4L2_PIX_FMT_YUV411P;
        break;
      case GST_VIDEO_FORMAT_Y42B:
        fourcc = V4L2_PIX_FMT_YUV422P;
        break;
      case GST_VIDEO_FORMAT_NV12:
        fourcc = V4L2_PIX_FMT_NV12;
        fourcc_nc = V4L2_PIX_FMT_NV12M;
        break;
      case GST_VIDEO_FORMAT_NV12_64Z32:
        fourcc_nc = V4L2_PIX_FMT_NV12MT;
        break;
      case GST_VIDEO_FORMAT_NV21:
        fourcc = V4L2_PIX_FMT_NV21;
        fourcc_nc = V4L2_PIX_FMT_NV21M;
        break;
      case GST_VIDEO_FORMAT_NV16:
        fourcc = V4L2_PIX_FMT_NV16;
        fourcc_nc = V4L2_PIX_FMT_NV16M;
        break;
      case GST_VIDEO_FORMAT_NV61:
        fourcc = V4L2_PIX_FMT_NV61;
        fourcc_nc = V4L2_PIX_FMT_NV61M;
        break;
      case GST_VIDEO_FORMAT_NV24:
        fourcc = V4L2_PIX_FMT_NV24;
        break;
      case GST_VIDEO_FORMAT_YVYU:
        fourcc = V4L2_PIX_FMT_YVYU;
        break;
      case GST_VIDEO_FORMAT_RGB15:
        fourcc = V4L2_PIX_FMT_RGB555;
        fourcc_nc = V4L2_PIX_FMT_XRGB555;
        break;
      case GST_VIDEO_FORMAT_RGB16:
        fourcc = V4L2_PIX_FMT_RGB565;
        break;
      case GST_VIDEO_FORMAT_RGB:
        fourcc = V4L2_PIX_FMT_RGB24;
        break;
      case GST_VIDEO_FORMAT_BGR:
        fourcc = V4L2_PIX_FMT_BGR24;
        break;
      case GST_VIDEO_FORMAT_xRGB:
        fourcc = V4L2_PIX_FMT_RGB32;
        fourcc_nc = V4L2_PIX_FMT_XRGB32;
        break;
      case GST_VIDEO_FORMAT_RGBx:
        fourcc = V4L2_PIX_FMT_RGBX32;
        break;
      case GST_VIDEO_FORMAT_ARGB:
        fourcc = V4L2_PIX_FMT_RGB32;
        fourcc_nc = V4L2_PIX_FMT_ARGB32;
        break;
      case GST_VIDEO_FORMAT_RGBA:
        fourcc = V4L2_PIX_FMT_RGBA32;
        break;
      case GST_VIDEO_FORMAT_BGRx:
        fourcc = V4L2_PIX_FMT_BGR32;
        fourcc_nc = V4L2_PIX_FMT_XBGR32;
        break;
      case GST_VIDEO_FORMAT_xBGR:
        fourcc = V4L2_PIX_FMT_BGRX32;
        break;
      case GST_VIDEO_FORMAT_BGRA:
        fourcc = V4L2_PIX_FMT_BGR32;
        fourcc_nc = V4L2_PIX_FMT_ABGR32;
        break;
      case GST_VIDEO_FORMAT_ABGR:
        fourcc = V4L2_PIX_FMT_BGRA32;
        break;
      case GST_VIDEO_FORMAT_GRAY8:
        fourcc = V4L2_PIX_FMT_GREY;
        break;
      case GST_VIDEO_FORMAT_GRAY16_LE:
        fourcc = V4L2_PIX_FMT_Y16;
        break;
      case GST_VIDEO_FORMAT_GRAY16_BE:
        fourcc = V4L2_PIX_FMT_Y16_BE;
        break;
      default:
        break;
    }
  } else {
    if (g_str_equal (mimetype, "video/mpegts")) {
      fourcc = V4L2_PIX_FMT_MPEG;
    } else if (g_str_equal (mimetype, "video/x-dv")) {
      fourcc = V4L2_PIX_FMT_DV;
    } else if (g_str_equal (mimetype, "image/jpeg")) {
      fourcc = V4L2_PIX_FMT_JPEG;
    } else if (g_str_equal (mimetype, "video/mpeg")) {
      gint version;
      if (gst_structure_get_int (structure, "mpegversion", &version)) {
        switch (version) {
          case 1:
            fourcc = V4L2_PIX_FMT_MPEG1;
            break;
          case 2:
            fourcc = V4L2_PIX_FMT_MPEG2;
            break;
          case 4:
            fourcc = V4L2_PIX_FMT_MPEG4;
            fourcc_nc = V4L2_PIX_FMT_XVID;
            break;
          default:
            break;
        }
      }
    } else if (g_str_equal (mimetype, "video/x-fwht")) {
      fourcc = V4L2_PIX_FMT_FWHT;
    } else if (g_str_equal (mimetype, "video/x-h263")) {
      fourcc = V4L2_PIX_FMT_H263;
    } else if (g_str_equal (mimetype, "video/x-h264")) {
      const gchar *stream_format =
          gst_structure_get_string (structure, "stream-format");
      if (g_str_equal (stream_format, "avc"))
        fourcc = V4L2_PIX_FMT_H264_NO_SC;
      else
        fourcc = V4L2_PIX_FMT_H264;
    } else if (g_str_equal (mimetype, "video/x-h265")) {
      fourcc = V4L2_PIX_FMT_HEVC;
    } else if (g_str_equal (mimetype, "video/x-vp8")) {
      fourcc = V4L2_PIX_FMT_VP8;
    } else if (g_str_equal (mimetype, "video/x-vp9")) {
      fourcc = V4L2_PIX_FMT_VP9;
    } else if (g_str_equal (mimetype, "video/x-bayer")) {
      const gchar *format = gst_structure_get_string (structure, "format");
      if (format) {
        if (!g_ascii_strcasecmp (format, "bggr"))
          fourcc = V4L2_PIX_FMT_SBGGR8;
        else if (!g_ascii_strcasecmp (format, "gbrg"))
          fourcc = V4L2_PIX_FMT_SGBRG8;
        else if (!g_ascii_strcasecmp (format, "grbg"))
          fourcc = V4L2_PIX_FMT_SGRBG8;
        else if (!g_ascii_strcasecmp (format, "rggb"))
          fourcc = V4L2_PIX_FMT_SRGGB8;
      }
    } else if (g_str_equal (mimetype, "video/x-sonix")) {
      fourcc = V4L2_PIX_FMT_SN9C10X;
    } else if (g_str_equal (mimetype, "video/x-pwc1")) {
      fourcc = V4L2_PIX_FMT_PWC1;
    } else if (g_str_equal (mimetype, "video/x-pwc2")) {
      fourcc = V4L2_PIX_FMT_PWC2;
    }
  }


  /* Prefer the non-contiguous if supported */
  v4l2object->prefered_non_contiguous = TRUE;

  if (fourcc_nc)
    fmt = gst_v4l2_object_get_format_from_fourcc (v4l2object, fourcc_nc);
  else if (fourcc == 0)
    goto unhandled_format;

  if (fmt == NULL) {
    fmt = gst_v4l2_object_get_format_from_fourcc (v4l2object, fourcc);
    v4l2object->prefered_non_contiguous = FALSE;
  }

  if (fmt == NULL)
    goto unsupported_format;

  *format = fmt;

  return TRUE;

  /* ERRORS */
invalid_format:
  {
    GST_DEBUG_OBJECT (v4l2object, "invalid format");
    return FALSE;
  }
unhandled_format:
  {
    GST_DEBUG_OBJECT (v4l2object, "unhandled format");
    return FALSE;
  }
unsupported_format:
  {
    GST_DEBUG_OBJECT (v4l2object, "unsupported format");
    return FALSE;
  }
}

static gboolean
gst_v4l2_object_get_nearest_size (GstV4l2Object * v4l2object,
    guint32 pixelformat, gint * width, gint * height);

static void
gst_v4l2_object_add_aspect_ratio (GstV4l2Object * v4l2object, GstStructure * s)
{
  if (v4l2object->keep_aspect && v4l2object->par)
    gst_structure_set_value (s, "pixel-aspect-ratio", v4l2object->par);
}

/* returns TRUE if the value was changed in place, otherwise FALSE */
static gboolean
gst_v4l2src_value_simplify (GValue * val)
{
  /* simplify list of one value to one value */
  if (GST_VALUE_HOLDS_LIST (val) && gst_value_list_get_size (val) == 1) {
    const GValue *list_val;
    GValue new_val = G_VALUE_INIT;

    list_val = gst_value_list_get_value (val, 0);
    g_value_init (&new_val, G_VALUE_TYPE (list_val));
    g_value_copy (list_val, &new_val);
    g_value_unset (val);
    *val = new_val;
    return TRUE;
  }

  return FALSE;
}

static gboolean
gst_v4l2_object_get_interlace_mode (enum v4l2_field field,
    GstVideoInterlaceMode * interlace_mode)
{
  switch (field) {
    case V4L2_FIELD_ANY:
      GST_ERROR
          ("Driver bug detected - check driver with v4l2-compliance from http://git.linuxtv.org/v4l-utils.git\n");
      /* fallthrough */
    case V4L2_FIELD_NONE:
      *interlace_mode = GST_VIDEO_INTERLACE_MODE_PROGRESSIVE;
      return TRUE;
    case V4L2_FIELD_INTERLACED:
    case V4L2_FIELD_INTERLACED_TB:
    case V4L2_FIELD_INTERLACED_BT:
      *interlace_mode = GST_VIDEO_INTERLACE_MODE_INTERLEAVED;
      return TRUE;
    default:
      GST_ERROR ("Unknown enum v4l2_field %d", field);
      return FALSE;
  }
}

static gboolean
gst_v4l2_object_get_colorspace (struct v4l2_format *fmt,
    GstVideoColorimetry * cinfo)
{
  gboolean is_rgb =
      gst_v4l2_object_v4l2fourcc_is_rgb (fmt->fmt.pix.pixelformat);
  enum v4l2_colorspace colorspace;
  enum v4l2_quantization range;
  enum v4l2_ycbcr_encoding matrix;
  enum v4l2_xfer_func transfer;
  gboolean ret = TRUE;

  if (V4L2_TYPE_IS_MULTIPLANAR (fmt->type)) {
    colorspace = fmt->fmt.pix_mp.colorspace;
    range = fmt->fmt.pix_mp.quantization;
    matrix = fmt->fmt.pix_mp.ycbcr_enc;
    transfer = fmt->fmt.pix_mp.xfer_func;
  } else {
    colorspace = fmt->fmt.pix.colorspace;
    range = fmt->fmt.pix.quantization;
    matrix = fmt->fmt.pix.ycbcr_enc;
    transfer = fmt->fmt.pix.xfer_func;
  }

  /* First step, set the defaults for each primaries */
  switch (colorspace) {
    case V4L2_COLORSPACE_SMPTE170M:
      cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT601;
      cinfo->transfer = GST_VIDEO_TRANSFER_BT709;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_SMPTE170M;
      break;
    case V4L2_COLORSPACE_REC709:
      cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT709;
      cinfo->transfer = GST_VIDEO_TRANSFER_BT709;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_BT709;
      break;
    case V4L2_COLORSPACE_SRGB:
    case V4L2_COLORSPACE_JPEG:
      cinfo->range = GST_VIDEO_COLOR_RANGE_0_255;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT601;
      cinfo->transfer = GST_VIDEO_TRANSFER_SRGB;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_BT709;
      break;
    case V4L2_COLORSPACE_OPRGB:
      cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT601;
      cinfo->transfer = GST_VIDEO_TRANSFER_ADOBERGB;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_ADOBERGB;
      break;
    case V4L2_COLORSPACE_BT2020:
      cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT2020;
      cinfo->transfer = GST_VIDEO_TRANSFER_BT2020_12;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_BT2020;
      break;
    case V4L2_COLORSPACE_SMPTE240M:
      cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_SMPTE240M;
      cinfo->transfer = GST_VIDEO_TRANSFER_SMPTE240M;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_SMPTE240M;
      break;
    case V4L2_COLORSPACE_470_SYSTEM_M:
      cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT601;
      cinfo->transfer = GST_VIDEO_TRANSFER_BT709;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_BT470M;
      break;
    case V4L2_COLORSPACE_470_SYSTEM_BG:
      cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT601;
      cinfo->transfer = GST_VIDEO_TRANSFER_BT709;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_BT470BG;
      break;
    case V4L2_COLORSPACE_RAW:
      /* Explicitly unknown */
      cinfo->range = GST_VIDEO_COLOR_RANGE_UNKNOWN;
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_UNKNOWN;
      cinfo->transfer = GST_VIDEO_TRANSFER_UNKNOWN;
      cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_UNKNOWN;
      break;
    default:
      GST_DEBUG ("Unknown enum v4l2_colorspace %d", colorspace);
      ret = FALSE;
      break;
  }

  if (!ret)
    goto done;

  /* Second step, apply any custom variation */
  switch (range) {
    case V4L2_QUANTIZATION_FULL_RANGE:
      cinfo->range = GST_VIDEO_COLOR_RANGE_0_255;
      break;
    case V4L2_QUANTIZATION_LIM_RANGE:
      cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      break;
    case V4L2_QUANTIZATION_DEFAULT:
      /* replicated V4L2_MAP_QUANTIZATION_DEFAULT macro behavior */
      if (is_rgb && colorspace == V4L2_COLORSPACE_BT2020)
        cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      else if (is_rgb || matrix == V4L2_YCBCR_ENC_XV601
          || matrix == V4L2_YCBCR_ENC_XV709
          || colorspace == V4L2_COLORSPACE_JPEG)
        cinfo->range = GST_VIDEO_COLOR_RANGE_0_255;
      else
        cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
      break;
    default:
      GST_WARNING ("Unknown enum v4l2_quantization value %d", range);
      cinfo->range = GST_VIDEO_COLOR_RANGE_UNKNOWN;
      break;
  }

  switch (matrix) {
    case V4L2_YCBCR_ENC_XV601:
    case V4L2_YCBCR_ENC_SYCC:
      GST_FIXME ("XV601 and SYCC not defined, assuming 601");
      /* fallthrough */
    case V4L2_YCBCR_ENC_601:
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT601;
      break;
    case V4L2_YCBCR_ENC_XV709:
      GST_FIXME ("XV709 not defined, assuming 709");
      /* fallthrough */
    case V4L2_YCBCR_ENC_709:
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT709;
      break;
    case V4L2_YCBCR_ENC_BT2020_CONST_LUM:
      GST_FIXME ("BT2020 with constant luma is not defined, assuming BT2020");
      /* fallthrough */
    case V4L2_YCBCR_ENC_BT2020:
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT2020;
      break;
    case V4L2_YCBCR_ENC_SMPTE240M:
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_SMPTE240M;
      break;
    case V4L2_YCBCR_ENC_DEFAULT:
      /* nothing, just use defaults for colorspace */
      break;
    default:
      GST_WARNING ("Unknown enum v4l2_ycbcr_encoding value %d", matrix);
      cinfo->matrix = GST_VIDEO_COLOR_MATRIX_UNKNOWN;
      break;
  }

  /* Set identity matrix for R'G'B' formats to avoid creating
   * confusion. This though is cosmetic as it's now properly ignored by
   * the video info API and videoconvert. */
  if (is_rgb)
    cinfo->matrix = GST_VIDEO_COLOR_MATRIX_RGB;

  switch (transfer) {
    case V4L2_XFER_FUNC_709:
      if (colorspace == V4L2_COLORSPACE_BT2020 && fmt->fmt.pix.height >= 2160)
        cinfo->transfer = GST_VIDEO_TRANSFER_BT2020_12;
      else
        cinfo->transfer = GST_VIDEO_TRANSFER_BT709;
      break;
    case V4L2_XFER_FUNC_SRGB:
      cinfo->transfer = GST_VIDEO_TRANSFER_SRGB;
      break;
    case V4L2_XFER_FUNC_OPRGB:
      cinfo->transfer = GST_VIDEO_TRANSFER_ADOBERGB;
      break;
    case V4L2_XFER_FUNC_SMPTE240M:
      cinfo->transfer = GST_VIDEO_TRANSFER_SMPTE240M;
      break;
    case V4L2_XFER_FUNC_NONE:
      cinfo->transfer = GST_VIDEO_TRANSFER_GAMMA10;
      break;
    case V4L2_XFER_FUNC_DEFAULT:
      /* nothing, just use defaults for colorspace */
      break;
    default:
      GST_WARNING ("Unknown enum v4l2_xfer_func value %d", transfer);
      cinfo->transfer = GST_VIDEO_TRANSFER_UNKNOWN;
      break;
  }

done:
  return ret;
}

static int
gst_v4l2_object_try_fmt (GstV4l2Object * v4l2object,
    struct v4l2_format *try_fmt)
{
  int fd = v4l2object->video_fd;
  struct v4l2_format fmt;
  int r;

  memcpy (&fmt, try_fmt, sizeof (fmt));
  r = v4l2object->ioctl (fd, VIDIOC_TRY_FMT, &fmt);

  if (r < 0 && errno == ENOTTY) {
    /* The driver might not implement TRY_FMT, in which case we will try
       S_FMT to probe */
    if (GST_V4L2_IS_ACTIVE (v4l2object))
      goto error;

    memcpy (&fmt, try_fmt, sizeof (fmt));
    r = v4l2object->ioctl (fd, VIDIOC_S_FMT, &fmt);
  }
  memcpy (try_fmt, &fmt, sizeof (fmt));

  return r;

error:
  memcpy (try_fmt, &fmt, sizeof (fmt));
  GST_WARNING_OBJECT (v4l2object->dbg_obj,
      "Unable to try format: %s", g_strerror (errno));
  return r;
}


static void
gst_v4l2_object_add_interlace_mode (GstV4l2Object * v4l2object,
    GstStructure * s, guint32 width, guint32 height, guint32 pixelformat)
{
  struct v4l2_format fmt;
  GValue interlace_formats = { 0, };
  enum v4l2_field formats[] = { V4L2_FIELD_NONE, V4L2_FIELD_INTERLACED };
  gsize i;
  GstVideoInterlaceMode interlace_mode, prev = -1;

  if (!g_str_equal (gst_structure_get_name (s), "video/x-raw"))
    return;

  if (v4l2object->never_interlaced) {
    gst_structure_set (s, "interlace-mode", G_TYPE_STRING, "progressive", NULL);
    return;
  }

  g_value_init (&interlace_formats, GST_TYPE_LIST);

  /* Try twice - once for NONE, once for INTERLACED. */
  for (i = 0; i < G_N_ELEMENTS (formats); i++) {
    memset (&fmt, 0, sizeof (fmt));
    fmt.type = v4l2object->type;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = pixelformat;
    fmt.fmt.pix.field = formats[i];

    if (gst_v4l2_object_try_fmt (v4l2object, &fmt) == 0 &&
        gst_v4l2_object_get_interlace_mode (fmt.fmt.pix.field, &interlace_mode)
        && prev != interlace_mode) {
      GValue interlace_enum = { 0, };
      const gchar *mode_string;
      g_value_init (&interlace_enum, G_TYPE_STRING);
      mode_string = gst_video_interlace_mode_to_string (interlace_mode);
      g_value_set_string (&interlace_enum, mode_string);
      gst_value_list_append_and_take_value (&interlace_formats,
          &interlace_enum);
      prev = interlace_mode;
    }
  }

  if (gst_v4l2src_value_simplify (&interlace_formats)
      || gst_value_list_get_size (&interlace_formats) > 0)
    gst_structure_take_value (s, "interlace-mode", &interlace_formats);
  else
    GST_WARNING_OBJECT (v4l2object, "Failed to determine interlace mode");

  return;
}

static void
gst_v4l2_object_fill_colorimetry_list (GValue * list,
    GstVideoColorimetry * cinfo)
{
  GValue colorimetry = G_VALUE_INIT;
  guint size;
  guint i;
  gboolean found = FALSE;

  g_value_init (&colorimetry, G_TYPE_STRING);
  g_value_take_string (&colorimetry, gst_video_colorimetry_to_string (cinfo));

  /* only insert if no duplicate */
  size = gst_value_list_get_size (list);
  for (i = 0; i < size; i++) {
    const GValue *tmp;

    tmp = gst_value_list_get_value (list, i);
    if (gst_value_compare (&colorimetry, tmp) == GST_VALUE_EQUAL) {
      found = TRUE;
      break;
    }
  }

  if (!found)
    gst_value_list_append_and_take_value (list, &colorimetry);
  else
    g_value_unset (&colorimetry);
}

static void
gst_v4l2_object_add_colorspace (GstV4l2Object * v4l2object, GstStructure * s,
    guint32 width, guint32 height, guint32 pixelformat)
{
  struct v4l2_format fmt;
  GValue list = G_VALUE_INIT;
  GstVideoColorimetry cinfo;
  enum v4l2_colorspace req_cspace;

  memset (&fmt, 0, sizeof (fmt));
  fmt.type = v4l2object->type;
  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = pixelformat;

  g_value_init (&list, GST_TYPE_LIST);

  /* step 1: get device default colorspace and insert it first as
   * it should be the preferred one */
  if (gst_v4l2_object_try_fmt (v4l2object, &fmt) == 0) {
    if (gst_v4l2_object_get_colorspace (&fmt, &cinfo))
      gst_v4l2_object_fill_colorimetry_list (&list, &cinfo);
  }

  /* step 2: probe all colorspace other than default
   * We don't probe all colorspace, range, matrix and transfer combination to
   * avoid ioctl flooding which could greatly increase initialization time
   * with low-speed devices (UVC...) */
  for (req_cspace = V4L2_COLORSPACE_SMPTE170M;
      req_cspace <= V4L2_COLORSPACE_RAW; req_cspace++) {
    /* V4L2_COLORSPACE_BT878 is deprecated and shall not be used, so skip */
    if (req_cspace == V4L2_COLORSPACE_BT878)
      continue;

    if (V4L2_TYPE_IS_MULTIPLANAR (v4l2object->type))
      fmt.fmt.pix_mp.colorspace = req_cspace;
    else
      fmt.fmt.pix.colorspace = req_cspace;

    if (gst_v4l2_object_try_fmt (v4l2object, &fmt) == 0) {
      enum v4l2_colorspace colorspace;

      if (V4L2_TYPE_IS_MULTIPLANAR (v4l2object->type))
        colorspace = fmt.fmt.pix_mp.colorspace;
      else
        colorspace = fmt.fmt.pix.colorspace;

      if (colorspace == req_cspace) {
        if (gst_v4l2_object_get_colorspace (&fmt, &cinfo))
          gst_v4l2_object_fill_colorimetry_list (&list, &cinfo);
      }
    }
  }

  if (gst_value_list_get_size (&list) > 0)
    gst_structure_take_value (s, "colorimetry", &list);
  else
    g_value_unset (&list);

  return;
}

/* The frame interval enumeration code first appeared in Linux 2.6.19. */
static GstStructure *
gst_v4l2_object_probe_caps_for_format_and_size (GstV4l2Object * v4l2object,
    guint32 pixelformat,
    guint32 width, guint32 height, const GstStructure * template)
{
  gint fd = v4l2object->video_fd;
  struct v4l2_frmivalenum ival;
  guint32 num, denom;
  GstStructure *s;
  GValue rates = { 0, };

  memset (&ival, 0, sizeof (struct v4l2_frmivalenum));
  ival.index = 0;
  ival.pixel_format = pixelformat;
  ival.width = width;
  ival.height = height;

  GST_LOG_OBJECT (v4l2object->dbg_obj,
      "get frame interval for %ux%u, %" GST_FOURCC_FORMAT, width, height,
      GST_FOURCC_ARGS (pixelformat));

  /* keep in mind that v4l2 gives us frame intervals (durations); we invert the
   * fraction to get framerate */
  if (v4l2object->ioctl (fd, VIDIOC_ENUM_FRAMEINTERVALS, &ival) < 0)
    goto enum_frameintervals_failed;

  if (ival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
    GValue rate = { 0, };

    g_value_init (&rates, GST_TYPE_LIST);
    g_value_init (&rate, GST_TYPE_FRACTION);

    do {
      num = ival.discrete.numerator;
      denom = ival.discrete.denominator;

      if (num > G_MAXINT || denom > G_MAXINT) {
        /* let us hope we don't get here... */
        num >>= 1;
        denom >>= 1;
      }

      GST_LOG_OBJECT (v4l2object->dbg_obj, "adding discrete framerate: %d/%d",
          denom, num);

      /* swap to get the framerate */
      gst_value_set_fraction (&rate, denom, num);
      gst_value_list_append_value (&rates, &rate);

      ival.index++;
    } while (v4l2object->ioctl (fd, VIDIOC_ENUM_FRAMEINTERVALS, &ival) >= 0);
  } else if (ival.type == V4L2_FRMIVAL_TYPE_STEPWISE) {
    GValue min = { 0, };
    GValue step = { 0, };
    GValue max = { 0, };
    gboolean added = FALSE;
    guint32 minnum, mindenom;
    guint32 maxnum, maxdenom;

    g_value_init (&rates, GST_TYPE_LIST);

    g_value_init (&min, GST_TYPE_FRACTION);
    g_value_init (&step, GST_TYPE_FRACTION);
    g_value_init (&max, GST_TYPE_FRACTION);

    /* get the min */
    minnum = ival.stepwise.min.numerator;
    mindenom = ival.stepwise.min.denominator;
    if (minnum > G_MAXINT || mindenom > G_MAXINT) {
      minnum >>= 1;
      mindenom >>= 1;
    }
    GST_LOG_OBJECT (v4l2object->dbg_obj, "stepwise min frame interval: %d/%d",
        minnum, mindenom);
    gst_value_set_fraction (&min, minnum, mindenom);

    /* get the max */
    maxnum = ival.stepwise.max.numerator;
    maxdenom = ival.stepwise.max.denominator;
    if (maxnum > G_MAXINT || maxdenom > G_MAXINT) {
      maxnum >>= 1;
      maxdenom >>= 1;
    }

    GST_LOG_OBJECT (v4l2object->dbg_obj, "stepwise max frame interval: %d/%d",
        maxnum, maxdenom);
    gst_value_set_fraction (&max, maxnum, maxdenom);

    /* get the step */
    num = ival.stepwise.step.numerator;
    denom = ival.stepwise.step.denominator;
    if (num > G_MAXINT || denom > G_MAXINT) {
      num >>= 1;
      denom >>= 1;
    }

    if (num == 0 || denom == 0) {
      /* in this case we have a wrong fraction or no step, set the step to max
       * so that we only add the min value in the loop below */
      num = maxnum;
      denom = maxdenom;
    }

    /* since we only have gst_value_fraction_subtract and not add, negate the
     * numerator */
    GST_LOG_OBJECT (v4l2object->dbg_obj, "stepwise step frame interval: %d/%d",
        num, denom);
    gst_value_set_fraction (&step, -num, denom);

    while (gst_value_compare (&min, &max) != GST_VALUE_GREATER_THAN) {
      GValue rate = { 0, };

      num = gst_value_get_fraction_numerator (&min);
      denom = gst_value_get_fraction_denominator (&min);
      GST_LOG_OBJECT (v4l2object->dbg_obj, "adding stepwise framerate: %d/%d",
          denom, num);

      /* invert to get the framerate */
      g_value_init (&rate, GST_TYPE_FRACTION);
      gst_value_set_fraction (&rate, denom, num);
      gst_value_list_append_value (&rates, &rate);
      added = TRUE;

      /* we're actually adding because step was negated above. This is because
       * there is no _add function... */
      if (!gst_value_fraction_subtract (&min, &min, &step)) {
        GST_WARNING_OBJECT (v4l2object->dbg_obj, "could not step fraction!");
        break;
      }
    }
    if (!added) {
      /* no range was added, leave the default range from the template */
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "no range added, leaving default");
      g_value_unset (&rates);
    }
  } else if (ival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS) {
    guint32 maxnum, maxdenom;

    g_value_init (&rates, GST_TYPE_FRACTION_RANGE);

    num = ival.stepwise.min.numerator;
    denom = ival.stepwise.min.denominator;
    if (num > G_MAXINT || denom > G_MAXINT) {
      num >>= 1;
      denom >>= 1;
    }

    maxnum = ival.stepwise.max.numerator;
    maxdenom = ival.stepwise.max.denominator;
    if (maxnum > G_MAXINT || maxdenom > G_MAXINT) {
      maxnum >>= 1;
      maxdenom >>= 1;
    }

    GST_LOG_OBJECT (v4l2object->dbg_obj,
        "continuous frame interval %d/%d to %d/%d", maxdenom, maxnum, denom,
        num);

    gst_value_set_fraction_range_full (&rates, maxdenom, maxnum, denom, num);
  } else {
    goto unknown_type;
  }

return_data:
  s = gst_structure_copy (template);
  gst_structure_set (s, "width", G_TYPE_INT, (gint) width,
      "height", G_TYPE_INT, (gint) height, NULL);

  gst_v4l2_object_add_aspect_ratio (v4l2object, s);

  if (!v4l2object->skip_try_fmt_probes) {
    gst_v4l2_object_add_interlace_mode (v4l2object, s, width, height,
        pixelformat);
    gst_v4l2_object_add_colorspace (v4l2object, s, width, height, pixelformat);
  }

  if (G_IS_VALUE (&rates)) {
    gst_v4l2src_value_simplify (&rates);
    /* only change the framerate on the template when we have a valid probed new
     * value */
    gst_structure_take_value (s, "framerate", &rates);
  } else if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
      v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
    gst_structure_set (s, "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT,
        1, NULL);
  }
  return s;

  /* ERRORS */
enum_frameintervals_failed:
  {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "Unable to enumerate intervals for %" GST_FOURCC_FORMAT "@%ux%u",
        GST_FOURCC_ARGS (pixelformat), width, height);
    goto return_data;
  }
unknown_type:
  {
    /* I don't see how this is actually an error, we ignore the format then */
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "Unknown frame interval type at %" GST_FOURCC_FORMAT "@%ux%u: %u",
        GST_FOURCC_ARGS (pixelformat), width, height, ival.type);
    return NULL;
  }
}

static gint
sort_by_frame_size (GstStructure * s1, GstStructure * s2)
{
  int w1, h1, w2, h2;

  gst_structure_get_int (s1, "width", &w1);
  gst_structure_get_int (s1, "height", &h1);
  gst_structure_get_int (s2, "width", &w2);
  gst_structure_get_int (s2, "height", &h2);

  /* I think it's safe to assume that this won't overflow for a while */
  return ((w2 * h2) - (w1 * h1));
}

static void
gst_v4l2_object_update_and_append (GstV4l2Object * v4l2object,
    guint32 format, GstCaps * caps, GstStructure * s)
{
  GstStructure *alt_s = NULL;

  /* Encoded stream on output buffer need to be parsed */
  if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT ||
      v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
    gint i = 0;

    for (; i < GST_V4L2_FORMAT_COUNT; i++) {
      if (format == gst_v4l2_formats[i].format &&
          gst_v4l2_formats[i].flags & GST_V4L2_CODEC &&
          !(gst_v4l2_formats[i].flags & GST_V4L2_NO_PARSE)) {
        gst_structure_set (s, "parsed", G_TYPE_BOOLEAN, TRUE, NULL);
        break;
      }
    }
  }

  if (v4l2object->has_alpha_component &&
      (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
          v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
    switch (format) {
      case V4L2_PIX_FMT_RGB32:
        alt_s = gst_structure_copy (s);
        gst_structure_set (alt_s, "format", G_TYPE_STRING, "ARGB", NULL);
        break;
      case V4L2_PIX_FMT_BGR32:
        alt_s = gst_structure_copy (s);
        gst_structure_set (alt_s, "format", G_TYPE_STRING, "BGRA", NULL);
        break;
      default:
        break;
    }
  }

  gst_caps_append_structure (caps, s);

  if (alt_s)
    gst_caps_append_structure (caps, alt_s);
}

static GstCaps *
gst_v4l2_object_probe_caps_for_format (GstV4l2Object * v4l2object,
    guint32 pixelformat, const GstStructure * template)
{
  GstCaps *ret = gst_caps_new_empty ();
  GstStructure *tmp;
  gint fd = v4l2object->video_fd;
  struct v4l2_frmsizeenum size;
  GList *results = NULL;
  guint32 w, h;

  if (pixelformat == GST_MAKE_FOURCC ('M', 'P', 'E', 'G')) {
    gst_caps_append_structure (ret, gst_structure_copy (template));
    return ret;
  }

  memset (&size, 0, sizeof (struct v4l2_frmsizeenum));
  size.index = 0;
  size.pixel_format = pixelformat;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj,
      "Enumerating frame sizes for %" GST_FOURCC_FORMAT,
      GST_FOURCC_ARGS (pixelformat));

  if (v4l2object->ioctl (fd, VIDIOC_ENUM_FRAMESIZES, &size) < 0)
    goto enum_framesizes_failed;

  if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
    do {
      GST_LOG_OBJECT (v4l2object->dbg_obj, "got discrete frame size %dx%d",
          size.discrete.width, size.discrete.height);

      w = MIN (size.discrete.width, G_MAXINT);
      h = MIN (size.discrete.height, G_MAXINT);

      if (w && h) {
        tmp =
            gst_v4l2_object_probe_caps_for_format_and_size (v4l2object,
            pixelformat, w, h, template);

        if (tmp)
          results = g_list_prepend (results, tmp);
      }

      size.index++;
    } while (v4l2object->ioctl (fd, VIDIOC_ENUM_FRAMESIZES, &size) >= 0);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "done iterating discrete frame sizes");
  } else if (size.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
    guint32 maxw, maxh, step_w, step_h;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "we have stepwise frame sizes:");
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min width:   %d",
        size.stepwise.min_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min height:  %d",
        size.stepwise.min_height);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "max width:   %d",
        size.stepwise.max_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min height:  %d",
        size.stepwise.max_height);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "step width:  %d",
        size.stepwise.step_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "step height: %d",
        size.stepwise.step_height);

    w = MAX (size.stepwise.min_width, 1);
    h = MAX (size.stepwise.min_height, 1);
    maxw = MIN (size.stepwise.max_width, G_MAXINT);
    maxh = MIN (size.stepwise.max_height, G_MAXINT);

    step_w = MAX (size.stepwise.step_width, 1);
    step_h = MAX (size.stepwise.step_height, 1);

    /* FIXME: check for sanity and that min/max are multiples of the steps */

    /* we only query details for the max width/height since it's likely the
     * most restricted if there are any resolution-dependent restrictions */
    tmp = gst_v4l2_object_probe_caps_for_format_and_size (v4l2object,
        pixelformat, maxw, maxh, template);

    if (tmp) {
      GValue step_range = G_VALUE_INIT;

      g_value_init (&step_range, GST_TYPE_INT_RANGE);
      gst_value_set_int_range_step (&step_range, w, maxw, step_w);
      gst_structure_set_value (tmp, "width", &step_range);

      gst_value_set_int_range_step (&step_range, h, maxh, step_h);
      gst_structure_take_value (tmp, "height", &step_range);

      /* no point using the results list here, since there's only one struct */
      gst_v4l2_object_update_and_append (v4l2object, pixelformat, ret, tmp);
    }
  } else if (size.type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
    guint32 maxw, maxh;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "we have continuous frame sizes:");
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min width:   %d",
        size.stepwise.min_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min height:  %d",
        size.stepwise.min_height);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "max width:   %d",
        size.stepwise.max_width);
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "min height:  %d",
        size.stepwise.max_height);

    w = MAX (size.stepwise.min_width, 1);
    h = MAX (size.stepwise.min_height, 1);
    maxw = MIN (size.stepwise.max_width, G_MAXINT);
    maxh = MIN (size.stepwise.max_height, G_MAXINT);

    tmp =
        gst_v4l2_object_probe_caps_for_format_and_size (v4l2object, pixelformat,
        w, h, template);
    if (tmp) {
      gst_structure_set (tmp, "width", GST_TYPE_INT_RANGE, (gint) w,
          (gint) maxw, "height", GST_TYPE_INT_RANGE, (gint) h, (gint) maxh,
          NULL);

      /* no point using the results list here, since there's only one struct */
      gst_v4l2_object_update_and_append (v4l2object, pixelformat, ret, tmp);
    }
  } else {
    goto unknown_type;
  }

  /* we use an intermediary list to store and then sort the results of the
   * probing because we can't make any assumptions about the order in which
   * the driver will give us the sizes, but we want the final caps to contain
   * the results starting with the highest resolution and having the lowest
   * resolution last, since order in caps matters for things like fixation. */
  results = g_list_sort (results, (GCompareFunc) sort_by_frame_size);
  while (results != NULL) {
    gst_v4l2_object_update_and_append (v4l2object, pixelformat, ret,
        results->data);
    results = g_list_delete_link (results, results);
  }

  if (gst_caps_is_empty (ret))
    goto enum_framesizes_no_results;

  return ret;

  /* ERRORS */
enum_framesizes_failed:
  {
    /* I don't see how this is actually an error */
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "Failed to enumerate frame sizes for pixelformat %" GST_FOURCC_FORMAT
        " (%s)", GST_FOURCC_ARGS (pixelformat), g_strerror (errno));
    goto default_frame_sizes;
  }
enum_framesizes_no_results:
  {
    /* it's possible that VIDIOC_ENUM_FRAMESIZES is defined but the driver in
     * question doesn't actually support it yet */
    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "No results for pixelformat %" GST_FOURCC_FORMAT
        " enumerating frame sizes, trying fallback",
        GST_FOURCC_ARGS (pixelformat));
    goto default_frame_sizes;
  }
unknown_type:
  {
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "Unknown frame sizeenum type for pixelformat %" GST_FOURCC_FORMAT
        ": %u", GST_FOURCC_ARGS (pixelformat), size.type);
    goto default_frame_sizes;
  }

default_frame_sizes:
  {
    gint min_w, max_w, min_h, max_h, fix_num = 0, fix_denom = 0;

    /* This code is for Linux < 2.6.19 */
    min_w = min_h = 1;
    max_w = max_h = GST_V4L2_MAX_SIZE;
    if (!gst_v4l2_object_get_nearest_size (v4l2object, pixelformat, &min_w,
            &min_h)) {
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Could not probe minimum capture size for pixelformat %"
          GST_FOURCC_FORMAT, GST_FOURCC_ARGS (pixelformat));
    }
    if (!gst_v4l2_object_get_nearest_size (v4l2object, pixelformat, &max_w,
            &max_h)) {
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Could not probe maximum capture size for pixelformat %"
          GST_FOURCC_FORMAT, GST_FOURCC_ARGS (pixelformat));
    }

    /* Since we can't get framerate directly, try to use the current norm */
    if (v4l2object->tv_norm && v4l2object->norms) {
      GList *norms;
      GstTunerNorm *norm = NULL;
      GstTunerNorm *current =
          gst_v4l2_tuner_get_norm_by_std_id (v4l2object, v4l2object->tv_norm);

      for (norms = v4l2object->norms; norms != NULL; norms = norms->next) {
        norm = (GstTunerNorm *) norms->data;
        if (!strcmp (norm->label, current->label))
          break;
      }
      /* If it's possible, set framerate to that (discrete) value */
      if (norm) {
        fix_num = gst_value_get_fraction_numerator (&norm->framerate);
        fix_denom = gst_value_get_fraction_denominator (&norm->framerate);
      }
    }

    tmp = gst_structure_copy (template);
    if (fix_num) {
      gst_structure_set (tmp, "framerate", GST_TYPE_FRACTION, fix_num,
          fix_denom, NULL);
    } else if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
        v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
      /* if norm can't be used, copy the template framerate */
      gst_structure_set (tmp, "framerate", GST_TYPE_FRACTION_RANGE, 0, 1,
          G_MAXINT, 1, NULL);
    }

    if (min_w == max_w)
      gst_structure_set (tmp, "width", G_TYPE_INT, max_w, NULL);
    else
      gst_structure_set (tmp, "width", GST_TYPE_INT_RANGE, min_w, max_w, NULL);

    if (min_h == max_h)
      gst_structure_set (tmp, "height", G_TYPE_INT, max_h, NULL);
    else
      gst_structure_set (tmp, "height", GST_TYPE_INT_RANGE, min_h, max_h, NULL);

    gst_v4l2_object_add_aspect_ratio (v4l2object, tmp);

    if (!v4l2object->skip_try_fmt_probes) {
      /* We could consider setting interlace mode from min and max. */
      gst_v4l2_object_add_interlace_mode (v4l2object, tmp, max_w, max_h,
          pixelformat);
      /* We could consider to check colorspace for min too, in case it depends on
       * the size. But in this case, min and max could not be enough */
      gst_v4l2_object_add_colorspace (v4l2object, tmp, max_w, max_h,
          pixelformat);
    }

    gst_v4l2_object_update_and_append (v4l2object, pixelformat, ret, tmp);
    return ret;
  }
}

static gboolean
gst_v4l2_object_get_nearest_size (GstV4l2Object * v4l2object,
    guint32 pixelformat, gint * width, gint * height)
{
  struct v4l2_format fmt;
  gboolean ret = FALSE;
  GstVideoInterlaceMode interlace_mode;

  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  GST_LOG_OBJECT (v4l2object->dbg_obj,
      "getting nearest size to %dx%d with format %" GST_FOURCC_FORMAT,
      *width, *height, GST_FOURCC_ARGS (pixelformat));

  memset (&fmt, 0, sizeof (struct v4l2_format));

  /* get size delimiters */
  memset (&fmt, 0, sizeof (fmt));
  fmt.type = v4l2object->type;
  fmt.fmt.pix.width = *width;
  fmt.fmt.pix.height = *height;
  fmt.fmt.pix.pixelformat = pixelformat;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;

  if (gst_v4l2_object_try_fmt (v4l2object, &fmt) < 0)
    goto error;

  GST_LOG_OBJECT (v4l2object->dbg_obj,
      "got nearest size %dx%d", fmt.fmt.pix.width, fmt.fmt.pix.height);

  *width = fmt.fmt.pix.width;
  *height = fmt.fmt.pix.height;

  if (!gst_v4l2_object_get_interlace_mode (fmt.fmt.pix.field, &interlace_mode)) {
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "Unsupported field type for %" GST_FOURCC_FORMAT "@%ux%u: %u",
        GST_FOURCC_ARGS (pixelformat), *width, *height, fmt.fmt.pix.field);
    goto error;
  }

  ret = TRUE;

error:
  if (!ret) {
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "Unable to try format: %s", g_strerror (errno));
  }

  return ret;
}

static gboolean
gst_v4l2_object_is_dmabuf_supported (GstV4l2Object * v4l2object)
{
  gboolean ret = TRUE;
  struct v4l2_exportbuffer expbuf = {
    .type = v4l2object->type,
    .index = -1,
    .plane = -1,
    .flags = O_CLOEXEC | O_RDWR,
  };

  if (v4l2object->fmtdesc->flags & V4L2_FMT_FLAG_EMULATED) {
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "libv4l2 converter detected, disabling DMABuf");
    ret = FALSE;
  }

  /* Expected to fail, but ENOTTY tells us that it is not implemented. */
  v4l2object->ioctl (v4l2object->video_fd, VIDIOC_EXPBUF, &expbuf);
  if (errno == ENOTTY)
    ret = FALSE;

  return ret;
}

static gboolean
gst_v4l2_object_setup_pool (GstV4l2Object * v4l2object, GstCaps * caps)
{
  GstV4l2IOMode mode;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "initializing the %s system",
      V4L2_TYPE_IS_OUTPUT (v4l2object->type) ? "output" : "capture");

  GST_V4L2_CHECK_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  /* find transport */
  mode = v4l2object->req_mode;

  if (v4l2object->device_caps & V4L2_CAP_READWRITE) {
    if (v4l2object->req_mode == GST_V4L2_IO_AUTO)
      mode = GST_V4L2_IO_RW;
  } else if (v4l2object->req_mode == GST_V4L2_IO_RW)
    goto method_not_supported;

  if (v4l2object->device_caps & V4L2_CAP_STREAMING) {
    if (v4l2object->req_mode == GST_V4L2_IO_AUTO) {
      if (!V4L2_TYPE_IS_OUTPUT (v4l2object->type) &&
          gst_v4l2_object_is_dmabuf_supported (v4l2object)) {
        mode = GST_V4L2_IO_DMABUF;
      } else {
        mode = GST_V4L2_IO_MMAP;
      }
    }
  } else if (v4l2object->req_mode == GST_V4L2_IO_MMAP ||
      v4l2object->req_mode == GST_V4L2_IO_DMABUF)
    goto method_not_supported;

  /* if still no transport selected, error out */
  if (mode == GST_V4L2_IO_AUTO)
    goto no_supported_capture_method;

  GST_INFO_OBJECT (v4l2object->dbg_obj, "accessing buffers via mode %d", mode);
  v4l2object->mode = mode;

  /* If min_buffers is not set, the driver either does not support the control or
     it has not been asked yet via propose_allocation/decide_allocation. */
  if (!v4l2object->min_buffers)
    gst_v4l2_get_driver_min_buffers (v4l2object);

  /* Map the buffers */
  GST_LOG_OBJECT (v4l2object->dbg_obj, "initiating buffer pool");

  if (!(v4l2object->pool = gst_v4l2_buffer_pool_new (v4l2object, caps)))
    goto buffer_pool_new_failed;

  GST_V4L2_SET_ACTIVE (v4l2object);

  return TRUE;

  /* ERRORS */
buffer_pool_new_failed:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, READ,
        (_("Could not map buffers from device '%s'"),
            v4l2object->videodev),
        ("Failed to create buffer pool: %s", g_strerror (errno)));
    return FALSE;
  }
method_not_supported:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, READ,
        (_("The driver of device '%s' does not support the IO method %d"),
            v4l2object->videodev, mode), (NULL));
    return FALSE;
  }
no_supported_capture_method:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, READ,
        (_("The driver of device '%s' does not support any known IO "
                "method."), v4l2object->videodev), (NULL));
    return FALSE;
  }
}

static void
gst_v4l2_object_set_stride (GstVideoInfo * info, GstVideoAlignment * align,
    gint plane, gint stride)
{
  const GstVideoFormatInfo *finfo = info->finfo;

  if (GST_VIDEO_FORMAT_INFO_IS_TILED (finfo)) {
    gint x_tiles, y_tiles, ws, hs, tile_height, padded_height;


    ws = GST_VIDEO_FORMAT_INFO_TILE_WS (finfo);
    hs = GST_VIDEO_FORMAT_INFO_TILE_HS (finfo);
    tile_height = 1 << hs;

    padded_height = GST_VIDEO_FORMAT_INFO_SCALE_HEIGHT (finfo, plane,
        info->height + align->padding_top + align->padding_bottom);
    padded_height = GST_ROUND_UP_N (padded_height, tile_height);

    x_tiles = stride >> ws;
    y_tiles = padded_height >> hs;
    info->stride[plane] = GST_VIDEO_TILE_MAKE_STRIDE (x_tiles, y_tiles);
  } else {
    info->stride[plane] = stride;
  }
}

static void
gst_v4l2_object_extrapolate_info (GstV4l2Object * v4l2object,
    GstVideoInfo * info, GstVideoAlignment * align, gint stride)
{
  const GstVideoFormatInfo *finfo = info->finfo;
  gint i, estride, padded_height;
  gsize offs = 0;

  g_return_if_fail (v4l2object->n_v4l2_planes == 1);

  padded_height = info->height + align->padding_top + align->padding_bottom;

  for (i = 0; i < finfo->n_planes; i++) {
    estride = gst_v4l2_object_extrapolate_stride (finfo, i, stride);

    gst_v4l2_object_set_stride (info, align, i, estride);

    info->offset[i] = offs;
    offs += estride *
        GST_VIDEO_FORMAT_INFO_SCALE_HEIGHT (finfo, i, padded_height);

    GST_DEBUG_OBJECT (v4l2object->dbg_obj,
        "Extrapolated for plane %d with base stride %d: "
        "stride %d, offset %" G_GSIZE_FORMAT, i, stride, info->stride[i],
        info->offset[i]);
  }

  /* Update the image size according the amount of data we are going to
   * read/write. This workaround bugs in driver where the sizeimage provided
   * by TRY/S_FMT represent the buffer length (maximum size) rather then the expected
   * bytesused (buffer size). */
  if (offs < info->size)
    info->size = offs;
}

static void
gst_v4l2_object_save_format (GstV4l2Object * v4l2object,
    struct v4l2_fmtdesc *fmtdesc, struct v4l2_format *format,
    GstVideoInfo * info, GstVideoAlignment * align)
{
  const GstVideoFormatInfo *finfo = info->finfo;
  gboolean standard_stride = TRUE;
  gint stride, pstride, padded_width, padded_height, i;

  if (GST_VIDEO_INFO_FORMAT (info) == GST_VIDEO_FORMAT_ENCODED) {
    v4l2object->n_v4l2_planes = 1;
    info->size = format->fmt.pix.sizeimage;
    goto store_info;
  }

  /* adjust right padding */
  if (V4L2_TYPE_IS_MULTIPLANAR (v4l2object->type))
    stride = format->fmt.pix_mp.plane_fmt[0].bytesperline;
  else
    stride = format->fmt.pix.bytesperline;

  pstride = GST_VIDEO_FORMAT_INFO_PSTRIDE (finfo, 0);
  if (pstride) {
    padded_width = stride / pstride;
  } else {
    /* pstride can be 0 for complex formats */
    GST_WARNING_OBJECT (v4l2object->element,
        "format %s has a pstride of 0, cannot compute padded with",
        gst_video_format_to_string (GST_VIDEO_INFO_FORMAT (info)));
    padded_width = stride;
  }

  if (padded_width < format->fmt.pix.width)
    GST_WARNING_OBJECT (v4l2object->dbg_obj,
        "Driver bug detected, stride (%d) is too small for the width (%d)",
        padded_width, format->fmt.pix.width);

  align->padding_right = padded_width - info->width - align->padding_left;

  /* adjust bottom padding */
  padded_height = format->fmt.pix.height;

  if (GST_VIDEO_FORMAT_INFO_IS_TILED (finfo)) {
    guint hs, tile_height;

    hs = GST_VIDEO_FORMAT_INFO_TILE_HS (finfo);
    tile_height = 1 << hs;

    padded_height = GST_ROUND_UP_N (padded_height, tile_height);
  }

  align->padding_bottom = padded_height - info->height - align->padding_top;

  /* setup the strides and offset */
  if (V4L2_TYPE_IS_MULTIPLANAR (v4l2object->type)) {
    struct v4l2_pix_format_mplane *pix_mp = &format->fmt.pix_mp;

    /* figure out the frame layout */
    v4l2object->n_v4l2_planes = MAX (1, pix_mp->num_planes);
    info->size = 0;
    for (i = 0; i < v4l2object->n_v4l2_planes; i++) {
      stride = pix_mp->plane_fmt[i].bytesperline;

      if (info->stride[i] != stride)
        standard_stride = FALSE;

      gst_v4l2_object_set_stride (info, align, i, stride);
      info->offset[i] = info->size;
      info->size += pix_mp->plane_fmt[i].sizeimage;
    }

    /* Extrapolate stride if planar format are being set in 1 v4l2 plane */
    if (v4l2object->n_v4l2_planes < finfo->n_planes) {
      stride = format->fmt.pix_mp.plane_fmt[0].bytesperline;
      gst_v4l2_object_extrapolate_info (v4l2object, info, align, stride);
    }
  } else {
    /* only one plane in non-MPLANE mode */
    v4l2object->n_v4l2_planes = 1;
    info->size = format->fmt.pix.sizeimage;
    stride = format->fmt.pix.bytesperline;

    if (info->stride[0] != stride)
      standard_stride = FALSE;

    gst_v4l2_object_extrapolate_info (v4l2object, info, align, stride);
  }

  /* adjust the offset to take into account left and top */
  if (GST_VIDEO_FORMAT_INFO_IS_TILED (finfo)) {
    if ((align->padding_left + align->padding_top) > 0)
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Left and top padding is not permitted for tiled formats");
  } else {
    for (i = 0; i < finfo->n_planes; i++) {
      gint vedge, hedge;

      /* FIXME we assume plane as component as this is true for all supported
       * format we support. */

      hedge = GST_VIDEO_FORMAT_INFO_SCALE_WIDTH (finfo, i, align->padding_left);
      vedge = GST_VIDEO_FORMAT_INFO_SCALE_HEIGHT (finfo, i, align->padding_top);

      info->offset[i] += (vedge * info->stride[i]) +
          (hedge * GST_VIDEO_INFO_COMP_PSTRIDE (info, i));
    }
  }

store_info:
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Got sizeimage %" G_GSIZE_FORMAT,
      info->size);

  /* to avoid copies we need video meta if there is padding */
  v4l2object->need_video_meta =
      ((align->padding_top + align->padding_left + align->padding_right +
          align->padding_bottom) != 0);

  /* ... or if stride is non "standard" */
  if (!standard_stride)
    v4l2object->need_video_meta = TRUE;

  /* ... or also video meta if we use multiple, non-contiguous, planes */
  if (v4l2object->n_v4l2_planes > 1)
    v4l2object->need_video_meta = TRUE;

  v4l2object->info = *info;
  v4l2object->align = *align;
  v4l2object->format = *format;
  v4l2object->fmtdesc = fmtdesc;

  /* if we have a framerate pre-calculate duration */
  if (info->fps_n > 0 && info->fps_d > 0) {
    v4l2object->duration = gst_util_uint64_scale_int (GST_SECOND, info->fps_d,
        info->fps_n);
  } else {
    v4l2object->duration = GST_CLOCK_TIME_NONE;
  }
}

gint
gst_v4l2_object_extrapolate_stride (const GstVideoFormatInfo * finfo,
    gint plane, gint stride)
{
  gint estride;

  switch (finfo->format) {
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV12_64Z32:
    case GST_VIDEO_FORMAT_NV21:
    case GST_VIDEO_FORMAT_NV16:
    case GST_VIDEO_FORMAT_NV61:
    case GST_VIDEO_FORMAT_NV24:
      estride = (plane == 0 ? 1 : 2) *
          GST_VIDEO_FORMAT_INFO_SCALE_WIDTH (finfo, plane, stride);
      break;
    default:
      estride = GST_VIDEO_FORMAT_INFO_SCALE_WIDTH (finfo, plane, stride);
      break;
  }

  return estride;
}

static gboolean
gst_v4l2_video_colorimetry_matches (const GstVideoColorimetry * cinfo,
    const gchar * color)
{
  GstVideoColorimetry ci;
  static const GstVideoColorimetry ci_likely_jpeg = {
    GST_VIDEO_COLOR_RANGE_0_255, GST_VIDEO_COLOR_MATRIX_BT601,
    GST_VIDEO_TRANSFER_UNKNOWN, GST_VIDEO_COLOR_PRIMARIES_UNKNOWN
  };
  static const GstVideoColorimetry ci_jpeg = {
    GST_VIDEO_COLOR_RANGE_0_255, GST_VIDEO_COLOR_MATRIX_BT601,
    GST_VIDEO_TRANSFER_SRGB, GST_VIDEO_COLOR_PRIMARIES_BT709
  };

  if (!gst_video_colorimetry_from_string (&ci, color))
    return FALSE;

  if (gst_video_colorimetry_is_equal (&ci, cinfo))
    return TRUE;

  /* Allow 1:4:0:0 (produced by jpegdec) if the device expects 1:4:7:1 */
  if (gst_video_colorimetry_is_equal (&ci, &ci_likely_jpeg)
      && gst_video_colorimetry_is_equal (cinfo, &ci_jpeg))
    return TRUE;

  return FALSE;
}

static gboolean
gst_v4l2_object_set_format_full (GstV4l2Object * v4l2object, GstCaps * caps,
    gboolean try_only, GstV4l2Error * error)
{
  gint fd = v4l2object->video_fd;
  struct v4l2_format format;
  struct v4l2_streamparm streamparm;
  enum v4l2_field field;
  guint32 pixelformat;
  struct v4l2_fmtdesc *fmtdesc;
  GstVideoInfo info;
  GstVideoAlignment align;
  gint width, height, fps_n, fps_d;
  gint n_v4l_planes;
  gint i = 0;
  gboolean is_mplane;
  enum v4l2_colorspace colorspace = 0;
  enum v4l2_quantization range = 0;
  enum v4l2_ycbcr_encoding matrix = 0;
  enum v4l2_xfer_func transfer = 0;
  GstStructure *s;
  gboolean disable_colorimetry = FALSE;

  g_return_val_if_fail (!v4l2object->skip_try_fmt_probes ||
      gst_caps_is_writable (caps), FALSE);

  GST_V4L2_CHECK_OPEN (v4l2object);
  if (!try_only)
    GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  is_mplane = V4L2_TYPE_IS_MULTIPLANAR (v4l2object->type);

  gst_video_info_init (&info);
  gst_video_alignment_reset (&align);

  if (!gst_v4l2_object_get_caps_info (v4l2object, caps, &fmtdesc, &info))
    goto invalid_caps;

  pixelformat = fmtdesc->pixelformat;
  width = GST_VIDEO_INFO_WIDTH (&info);
  height = GST_VIDEO_INFO_HEIGHT (&info);
  fps_n = GST_VIDEO_INFO_FPS_N (&info);
  fps_d = GST_VIDEO_INFO_FPS_D (&info);

  /* if encoded format (GST_VIDEO_INFO_N_PLANES return 0)
   * or if contiguous is preferred */
  n_v4l_planes = GST_VIDEO_INFO_N_PLANES (&info);
  if (!n_v4l_planes || !v4l2object->prefered_non_contiguous)
    n_v4l_planes = 1;

  if (GST_VIDEO_INFO_IS_INTERLACED (&info)) {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "interlaced video");
    /* ideally we would differentiate between types of interlaced video
     * but there is not sufficient information in the caps..
     */
    field = V4L2_FIELD_INTERLACED;
  } else {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "progressive video");
    field = V4L2_FIELD_NONE;
  }

  /* We first pick the main colorspace from the primaries */
  switch (info.colorimetry.primaries) {
    case GST_VIDEO_COLOR_PRIMARIES_BT709:
      /* There is two colorspaces using these primaries, use the range to
       * differentiate */
      if (info.colorimetry.range == GST_VIDEO_COLOR_RANGE_16_235)
        colorspace = V4L2_COLORSPACE_REC709;
      else
        colorspace = V4L2_COLORSPACE_SRGB;
      break;
    case GST_VIDEO_COLOR_PRIMARIES_BT2020:
      colorspace = V4L2_COLORSPACE_BT2020;
      break;
    case GST_VIDEO_COLOR_PRIMARIES_BT470M:
      colorspace = V4L2_COLORSPACE_470_SYSTEM_M;
      break;
    case GST_VIDEO_COLOR_PRIMARIES_BT470BG:
      colorspace = V4L2_COLORSPACE_470_SYSTEM_BG;
      break;
    case GST_VIDEO_COLOR_PRIMARIES_SMPTE170M:
      colorspace = V4L2_COLORSPACE_SMPTE170M;
      break;
    case GST_VIDEO_COLOR_PRIMARIES_SMPTE240M:
      colorspace = V4L2_COLORSPACE_SMPTE240M;
      break;

    case GST_VIDEO_COLOR_PRIMARIES_FILM:
    case GST_VIDEO_COLOR_PRIMARIES_UNKNOWN:
      /* We don't know, we will guess */
      break;

    default:
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Unknown colorimetry primaries %d", info.colorimetry.primaries);
      break;
  }

  switch (info.colorimetry.range) {
    case GST_VIDEO_COLOR_RANGE_0_255:
      range = V4L2_QUANTIZATION_FULL_RANGE;
      break;
    case GST_VIDEO_COLOR_RANGE_16_235:
      range = V4L2_QUANTIZATION_LIM_RANGE;
      break;
    case GST_VIDEO_COLOR_RANGE_UNKNOWN:
      /* We let the driver pick a default one */
      break;
    default:
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Unknown colorimetry range %d", info.colorimetry.range);
      break;
  }

  switch (info.colorimetry.matrix) {
    case GST_VIDEO_COLOR_MATRIX_RGB:
      /* Unspecified, leave to default */
      break;
      /* FCC is about the same as BT601 with less digit */
    case GST_VIDEO_COLOR_MATRIX_FCC:
    case GST_VIDEO_COLOR_MATRIX_BT601:
      matrix = V4L2_YCBCR_ENC_601;
      break;
    case GST_VIDEO_COLOR_MATRIX_BT709:
      matrix = V4L2_YCBCR_ENC_709;
      break;
    case GST_VIDEO_COLOR_MATRIX_SMPTE240M:
      matrix = V4L2_YCBCR_ENC_SMPTE240M;
      break;
    case GST_VIDEO_COLOR_MATRIX_BT2020:
      matrix = V4L2_YCBCR_ENC_BT2020;
      break;
    case GST_VIDEO_COLOR_MATRIX_UNKNOWN:
      /* We let the driver pick a default one */
      break;
    default:
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Unknown colorimetry matrix %d", info.colorimetry.matrix);
      break;
  }

  switch (info.colorimetry.transfer) {
    case GST_VIDEO_TRANSFER_GAMMA18:
    case GST_VIDEO_TRANSFER_GAMMA20:
    case GST_VIDEO_TRANSFER_GAMMA22:
    case GST_VIDEO_TRANSFER_GAMMA28:
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "GAMMA 18, 20, 22, 28 transfer functions not supported");
      /* fallthrough */
    case GST_VIDEO_TRANSFER_GAMMA10:
      transfer = V4L2_XFER_FUNC_NONE;
      break;
    case GST_VIDEO_TRANSFER_BT2020_12:
    case GST_VIDEO_TRANSFER_BT709:
      transfer = V4L2_XFER_FUNC_709;
      break;
    case GST_VIDEO_TRANSFER_SMPTE240M:
      transfer = V4L2_XFER_FUNC_SMPTE240M;
      break;
    case GST_VIDEO_TRANSFER_SRGB:
      transfer = V4L2_XFER_FUNC_SRGB;
      break;
    case GST_VIDEO_TRANSFER_LOG100:
    case GST_VIDEO_TRANSFER_LOG316:
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "LOG 100, 316 transfer functions not supported");
      /* FIXME No known sensible default, maybe AdobeRGB ? */
      break;
    case GST_VIDEO_TRANSFER_UNKNOWN:
      /* We let the driver pick a default one */
      break;
    default:
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Unknown colorimetry transfer %d", info.colorimetry.transfer);
      break;
  }

  if (colorspace == 0) {
    /* Try to guess colorspace according to pixelformat and size */
    if (GST_VIDEO_INFO_IS_YUV (&info)) {
      if (range == V4L2_QUANTIZATION_FULL_RANGE
          && matrix == V4L2_YCBCR_ENC_601 && transfer == 0) {
        /* Full range BT.601 YCbCr encoding with unknown primaries and transfer
         * function most likely is JPEG */
        colorspace = V4L2_COLORSPACE_JPEG;
        transfer = V4L2_XFER_FUNC_SRGB;
      } else {
        /* SD streams likely use SMPTE170M and HD streams REC709 */
        if (width <= 720 && height <= 576)
          colorspace = V4L2_COLORSPACE_SMPTE170M;
        else
          colorspace = V4L2_COLORSPACE_REC709;
      }
    } else if (GST_VIDEO_INFO_IS_RGB (&info)) {
      colorspace = V4L2_COLORSPACE_SRGB;
      transfer = V4L2_XFER_FUNC_NONE;
    }
  }

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Desired format %dx%d, format "
      "%" GST_FOURCC_FORMAT " stride: %d", width, height,
      GST_FOURCC_ARGS (pixelformat), GST_VIDEO_INFO_PLANE_STRIDE (&info, 0));

  memset (&format, 0x00, sizeof (struct v4l2_format));
  format.type = v4l2object->type;

  if (is_mplane) {
    format.type = v4l2object->type;
    format.fmt.pix_mp.pixelformat = pixelformat;
    format.fmt.pix_mp.width = width;
    format.fmt.pix_mp.height = height;
    format.fmt.pix_mp.field = field;
    format.fmt.pix_mp.num_planes = n_v4l_planes;

    /* try to ask our preferred stride but it's not a failure if not
     * accepted */
    for (i = 0; i < n_v4l_planes; i++) {
      gint stride = GST_VIDEO_INFO_PLANE_STRIDE (&info, i);

      if (GST_VIDEO_FORMAT_INFO_IS_TILED (info.finfo))
        stride = GST_VIDEO_TILE_X_TILES (stride) <<
            GST_VIDEO_FORMAT_INFO_TILE_WS (info.finfo);

      format.fmt.pix_mp.plane_fmt[i].bytesperline = stride;
    }

    if (GST_VIDEO_INFO_FORMAT (&info) == GST_VIDEO_FORMAT_ENCODED)
      format.fmt.pix_mp.plane_fmt[0].sizeimage = ENCODED_BUFFER_SIZE;
  } else {
    gint stride = GST_VIDEO_INFO_PLANE_STRIDE (&info, 0);

    format.type = v4l2object->type;

    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = pixelformat;
    format.fmt.pix.field = field;

    if (GST_VIDEO_FORMAT_INFO_IS_TILED (info.finfo))
      stride = GST_VIDEO_TILE_X_TILES (stride) <<
          GST_VIDEO_FORMAT_INFO_TILE_WS (info.finfo);

    /* try to ask our preferred stride */
    format.fmt.pix.bytesperline = stride;

    if (GST_VIDEO_INFO_FORMAT (&info) == GST_VIDEO_FORMAT_ENCODED)
      format.fmt.pix.sizeimage = ENCODED_BUFFER_SIZE;
  }

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Desired format is %dx%d, format "
      "%" GST_FOURCC_FORMAT ", nb planes %d", format.fmt.pix.width,
      format.fmt.pix_mp.height,
      GST_FOURCC_ARGS (format.fmt.pix.pixelformat),
      is_mplane ? format.fmt.pix_mp.num_planes : 1);

#ifndef GST_DISABLE_GST_DEBUG
  if (is_mplane) {
    for (i = 0; i < format.fmt.pix_mp.num_planes; i++)
      GST_DEBUG_OBJECT (v4l2object->dbg_obj, "  stride %d",
          format.fmt.pix_mp.plane_fmt[i].bytesperline);
  } else {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "  stride %d",
        format.fmt.pix.bytesperline);
  }
#endif

  if (is_mplane) {
    format.fmt.pix_mp.colorspace = colorspace;
    format.fmt.pix_mp.quantization = range;
    format.fmt.pix_mp.ycbcr_enc = matrix;
    format.fmt.pix_mp.xfer_func = transfer;
  } else {
    format.fmt.pix.priv = V4L2_PIX_FMT_PRIV_MAGIC;
    format.fmt.pix.colorspace = colorspace;
    format.fmt.pix.quantization = range;
    format.fmt.pix.ycbcr_enc = matrix;
    format.fmt.pix.xfer_func = transfer;
  }

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Desired colorspace is %d:%d:%d:%d",
      colorspace, range, matrix, transfer);

  if (try_only) {
    if (v4l2object->ioctl (fd, VIDIOC_TRY_FMT, &format) < 0)
      goto try_fmt_failed;
  } else {
    if (v4l2object->ioctl (fd, VIDIOC_S_FMT, &format) < 0)
      goto set_fmt_failed;
  }

  if (is_mplane) {
    colorspace = format.fmt.pix_mp.colorspace;
    range = format.fmt.pix_mp.quantization;
    matrix = format.fmt.pix_mp.ycbcr_enc;
    transfer = format.fmt.pix_mp.xfer_func;
  } else {
    colorspace = format.fmt.pix.colorspace;
    range = format.fmt.pix.quantization;
    matrix = format.fmt.pix.ycbcr_enc;
    transfer = format.fmt.pix.xfer_func;
  }

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Got format of %dx%d, format "
      "%" GST_FOURCC_FORMAT ", nb planes %d, colorspace %d:%d:%d:%d",
      format.fmt.pix.width, format.fmt.pix_mp.height,
      GST_FOURCC_ARGS (format.fmt.pix.pixelformat),
      is_mplane ? format.fmt.pix_mp.num_planes : 1,
      colorspace, range, matrix, transfer);

#ifndef GST_DISABLE_GST_DEBUG
  if (is_mplane) {
    for (i = 0; i < format.fmt.pix_mp.num_planes; i++)
      GST_DEBUG_OBJECT (v4l2object->dbg_obj, "  stride %d, sizeimage %d",
          format.fmt.pix_mp.plane_fmt[i].bytesperline,
          format.fmt.pix_mp.plane_fmt[i].sizeimage);
  } else {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "  stride %d, sizeimage %d",
        format.fmt.pix.bytesperline, format.fmt.pix.sizeimage);
  }
#endif

  if (format.fmt.pix.pixelformat != pixelformat)
    goto invalid_pixelformat;

  /* Only negotiate size with raw data.
   * For some codecs the dimensions are *not* in the bitstream, IIRC VC1
   * in ASF mode for example, there is also not reason for a driver to
   * change the size. */
  if (info.finfo->format != GST_VIDEO_FORMAT_ENCODED) {
    /* We can crop larger images */
    if (format.fmt.pix.width < width || format.fmt.pix.height < height)
      goto invalid_dimensions;

    /* Note, this will be adjusted if upstream has non-centered cropping. */
    align.padding_top = 0;
    align.padding_bottom = format.fmt.pix.height - height;
    align.padding_left = 0;
    align.padding_right = format.fmt.pix.width - width;
  }

  if (is_mplane && format.fmt.pix_mp.num_planes != n_v4l_planes)
    goto invalid_planes;

  /* used to check colorimetry and interlace mode fields presence */
  s = gst_caps_get_structure (caps, 0);

  if (!gst_v4l2_object_get_interlace_mode (format.fmt.pix.field,
          &info.interlace_mode))
    goto invalid_field;
  if (gst_structure_has_field (s, "interlace-mode")) {
    if (format.fmt.pix.field != field)
      goto invalid_field;
  }

  if (gst_v4l2_object_get_colorspace (&format, &info.colorimetry)) {
    if (gst_structure_has_field (s, "colorimetry")) {
      if (!gst_v4l2_video_colorimetry_matches (&info.colorimetry,
              gst_structure_get_string (s, "colorimetry")))
        goto invalid_colorimetry;
    }
  } else {
    /* The driver (or libv4l2) is miss-behaving, just ignore colorimetry from
     * the TRY_FMT */
    disable_colorimetry = TRUE;
    if (gst_structure_has_field (s, "colorimetry"))
      gst_structure_remove_field (s, "colorimetry");
  }

  /* In case we have skipped the try_fmt probes, we'll need to set the
   * colorimetry and interlace-mode back into the caps. */
  if (v4l2object->skip_try_fmt_probes) {
    if (!disable_colorimetry && !gst_structure_has_field (s, "colorimetry")) {
      gchar *str = gst_video_colorimetry_to_string (&info.colorimetry);
      gst_structure_set (s, "colorimetry", G_TYPE_STRING, str, NULL);
      g_free (str);
    }

    if (!gst_structure_has_field (s, "interlace-mode"))
      gst_structure_set (s, "interlace-mode", G_TYPE_STRING,
          gst_video_interlace_mode_to_string (info.interlace_mode), NULL);
  }

  if (try_only)                 /* good enough for trying only */
    return TRUE;

  if (GST_VIDEO_INFO_HAS_ALPHA (&info)) {
    struct v4l2_control ctl = { 0, };
    ctl.id = V4L2_CID_ALPHA_COMPONENT;
    ctl.value = 0xff;

    if (v4l2object->ioctl (fd, VIDIOC_S_CTRL, &ctl) < 0)
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Failed to set alpha component value");
  }

  /* Is there a reason we require the caller to always specify a framerate? */
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Desired framerate: %u/%u", fps_n,
      fps_d);

  memset (&streamparm, 0x00, sizeof (struct v4l2_streamparm));
  streamparm.type = v4l2object->type;

  if (v4l2object->ioctl (fd, VIDIOC_G_PARM, &streamparm) < 0)
    goto get_parm_failed;

  if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE
      || v4l2object->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
    GST_VIDEO_INFO_FPS_N (&info) =
        streamparm.parm.capture.timeperframe.denominator;
    GST_VIDEO_INFO_FPS_D (&info) =
        streamparm.parm.capture.timeperframe.numerator;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Got capture framerate: %u/%u",
        streamparm.parm.capture.timeperframe.denominator,
        streamparm.parm.capture.timeperframe.numerator);

    /* We used to skip frame rate setup if the camera was already setup
     * with the requested frame rate. This breaks some cameras though,
     * causing them to not output data (several models of Thinkpad cameras
     * have this problem at least).
     * So, don't skip. */
    GST_LOG_OBJECT (v4l2object->dbg_obj, "Setting capture framerate to %u/%u",
        fps_n, fps_d);
    /* We want to change the frame rate, so check whether we can. Some cheap USB
     * cameras don't have the capability */
    if ((streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) == 0) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj,
          "Not setting capture framerate (not supported)");
      goto done;
    }

    /* Note: V4L2 wants the frame interval, we have the frame rate */
    streamparm.parm.capture.timeperframe.numerator = fps_d;
    streamparm.parm.capture.timeperframe.denominator = fps_n;

    /* some cheap USB cam's won't accept any change */
    if (v4l2object->ioctl (fd, VIDIOC_S_PARM, &streamparm) < 0)
      goto set_parm_failed;

    if (streamparm.parm.capture.timeperframe.numerator > 0 &&
        streamparm.parm.capture.timeperframe.denominator > 0) {
      /* get new values */
      fps_d = streamparm.parm.capture.timeperframe.numerator;
      fps_n = streamparm.parm.capture.timeperframe.denominator;

      GST_INFO_OBJECT (v4l2object->dbg_obj, "Set capture framerate to %u/%u",
          fps_n, fps_d);
    } else {
      /* fix v4l2 capture driver to provide framerate values */
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Reuse caps framerate %u/%u - fix v4l2 capture driver", fps_n, fps_d);
    }

    GST_VIDEO_INFO_FPS_N (&info) = fps_n;
    GST_VIDEO_INFO_FPS_D (&info) = fps_d;
  } else if (v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT
      || v4l2object->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
    GST_VIDEO_INFO_FPS_N (&info) =
        streamparm.parm.output.timeperframe.denominator;
    GST_VIDEO_INFO_FPS_D (&info) =
        streamparm.parm.output.timeperframe.numerator;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Got output framerate: %u/%u",
        streamparm.parm.output.timeperframe.denominator,
        streamparm.parm.output.timeperframe.numerator);

    GST_LOG_OBJECT (v4l2object->dbg_obj, "Setting output framerate to %u/%u",
        fps_n, fps_d);
    if ((streamparm.parm.output.capability & V4L2_CAP_TIMEPERFRAME) == 0) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj,
          "Not setting output framerate (not supported)");
      goto done;
    }

    /* Note: V4L2 wants the frame interval, we have the frame rate */
    streamparm.parm.output.timeperframe.numerator = fps_d;
    streamparm.parm.output.timeperframe.denominator = fps_n;

    if (v4l2object->ioctl (fd, VIDIOC_S_PARM, &streamparm) < 0)
      goto set_parm_failed;

    if (streamparm.parm.output.timeperframe.numerator > 0 &&
        streamparm.parm.output.timeperframe.denominator > 0) {
      /* get new values */
      fps_d = streamparm.parm.output.timeperframe.numerator;
      fps_n = streamparm.parm.output.timeperframe.denominator;

      GST_INFO_OBJECT (v4l2object->dbg_obj, "Set output framerate to %u/%u",
          fps_n, fps_d);
    } else {
      /* fix v4l2 output driver to provide framerate values */
      GST_WARNING_OBJECT (v4l2object->dbg_obj,
          "Reuse caps framerate %u/%u - fix v4l2 output driver", fps_n, fps_d);
    }

    GST_VIDEO_INFO_FPS_N (&info) = fps_n;
    GST_VIDEO_INFO_FPS_D (&info) = fps_d;
  }

done:
  /* add boolean return, so we can fail on drivers bugs */
  gst_v4l2_object_save_format (v4l2object, fmtdesc, &format, &info, &align);

  /* now configure the pool */
  if (!gst_v4l2_object_setup_pool (v4l2object, caps))
    goto pool_failed;

  return TRUE;

  /* ERRORS */
invalid_caps:
  {
    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "can't parse caps %" GST_PTR_FORMAT,
        caps);
    return FALSE;
  }
try_fmt_failed:
  {
    if (errno == EINVAL) {
      GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
          (_("Device '%s' has no supported format"), v4l2object->videodev),
          ("Call to TRY_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    } else {
      GST_V4L2_ERROR (error, RESOURCE, FAILED,
          (_("Device '%s' failed during initialization"),
              v4l2object->videodev),
          ("Call to TRY_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    }
    return FALSE;
  }
set_fmt_failed:
  {
    if (errno == EBUSY) {
      GST_V4L2_ERROR (error, RESOURCE, BUSY,
          (_("Device '%s' is busy"), v4l2object->videodev),
          ("Call to S_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    } else if (errno == EINVAL) {
      GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
          (_("Device '%s' has no supported format"), v4l2object->videodev),
          ("Call to S_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    } else {
      GST_V4L2_ERROR (error, RESOURCE, FAILED,
          (_("Device '%s' failed during initialization"),
              v4l2object->videodev),
          ("Call to S_FMT failed for %" GST_FOURCC_FORMAT " @ %dx%d: %s",
              GST_FOURCC_ARGS (pixelformat), width, height,
              g_strerror (errno)));
    }
    return FALSE;
  }
invalid_dimensions:
  {
    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        (_("Device '%s' cannot capture at %dx%d"),
            v4l2object->videodev, width, height),
        ("Tried to capture at %dx%d, but device returned size %dx%d",
            width, height, format.fmt.pix.width, format.fmt.pix.height));
    return FALSE;
  }
invalid_pixelformat:
  {
    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        (_("Device '%s' cannot capture in the specified format"),
            v4l2object->videodev),
        ("Tried to capture in %" GST_FOURCC_FORMAT
            ", but device returned format" " %" GST_FOURCC_FORMAT,
            GST_FOURCC_ARGS (pixelformat),
            GST_FOURCC_ARGS (format.fmt.pix.pixelformat)));
    return FALSE;
  }
invalid_planes:
  {
    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        (_("Device '%s' does support non-contiguous planes"),
            v4l2object->videodev),
        ("Device wants %d planes", format.fmt.pix_mp.num_planes));
    return FALSE;
  }
invalid_field:
  {
    enum v4l2_field wanted_field;

    if (is_mplane)
      wanted_field = format.fmt.pix_mp.field;
    else
      wanted_field = format.fmt.pix.field;

    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        (_("Device '%s' does not support %s interlacing"),
            v4l2object->videodev,
            field == V4L2_FIELD_NONE ? "progressive" : "interleaved"),
        ("Device wants %s interlacing",
            wanted_field == V4L2_FIELD_NONE ? "progressive" : "interleaved"));
    return FALSE;
  }
invalid_colorimetry:
  {
    gchar *wanted_colorimetry;

    wanted_colorimetry = gst_video_colorimetry_to_string (&info.colorimetry);

    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        (_("Device '%s' does not support %s colorimetry"),
            v4l2object->videodev, gst_structure_get_string (s, "colorimetry")),
        ("Device wants %s colorimetry", wanted_colorimetry));

    g_free (wanted_colorimetry);
    return FALSE;
  }
get_parm_failed:
  {
    /* it's possible that this call is not supported */
    if (errno != EINVAL && errno != ENOTTY) {
      GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
          (_("Could not get parameters on device '%s'"),
              v4l2object->videodev), GST_ERROR_SYSTEM);
    }
    goto done;
  }
set_parm_failed:
  {
    GST_V4L2_ERROR (error, RESOURCE, SETTINGS,
        (_("Video device did not accept new frame rate setting.")),
        GST_ERROR_SYSTEM);
    goto done;
  }
pool_failed:
  {
    /* setup_pool already send the error */
    return FALSE;
  }
}

gboolean
gst_v4l2_object_set_format (GstV4l2Object * v4l2object, GstCaps * caps,
    GstV4l2Error * error)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Setting format to %" GST_PTR_FORMAT,
      caps);
  return gst_v4l2_object_set_format_full (v4l2object, caps, FALSE, error);
}

gboolean
gst_v4l2_object_try_format (GstV4l2Object * v4l2object, GstCaps * caps,
    GstV4l2Error * error)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Trying format %" GST_PTR_FORMAT,
      caps);
  return gst_v4l2_object_set_format_full (v4l2object, caps, TRUE, error);
}

/**
 * gst_v4l2_object_acquire_format:
 * @v4l2object: the object
 * @info: a GstVideoInfo to be filled
 *
 * Acquire the driver chosen format. This is useful in decoder or encoder elements where
 * the output format is chosen by the HW.
 *
 * Returns: %TRUE on success, %FALSE on failure.
 */
gboolean
gst_v4l2_object_acquire_format (GstV4l2Object * v4l2object, GstVideoInfo * info)
{
  struct v4l2_fmtdesc *fmtdesc;
  struct v4l2_format fmt;
  struct v4l2_crop crop;
  struct v4l2_selection sel;
  struct v4l2_rect *r = NULL;
  GstVideoFormat format;
  guint width, height;
  GstVideoAlignment align;

  gst_video_info_init (info);
  gst_video_alignment_reset (&align);

  memset (&fmt, 0x00, sizeof (struct v4l2_format));
  fmt.type = v4l2object->type;
  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_FMT, &fmt) < 0)
    goto get_fmt_failed;

  fmtdesc = gst_v4l2_object_get_format_from_fourcc (v4l2object,
      fmt.fmt.pix.pixelformat);
  if (fmtdesc == NULL)
    goto unsupported_format;

  /* No need to care about mplane, the four first params are the same */
  format = gst_v4l2_object_v4l2fourcc_to_video_format (fmt.fmt.pix.pixelformat);

  /* fails if we do no translate the fmt.pix.pixelformat to GstVideoFormat */
  if (format == GST_VIDEO_FORMAT_UNKNOWN)
    goto unsupported_format;

  if (fmt.fmt.pix.width == 0 || fmt.fmt.pix.height == 0)
    goto invalid_dimensions;

  width = fmt.fmt.pix.width;
  height = fmt.fmt.pix.height;

  /* Use the default compose rectangle */
  memset (&sel, 0, sizeof (struct v4l2_selection));
  sel.type = v4l2object->type;
  sel.target = V4L2_SEL_TGT_COMPOSE_DEFAULT;
  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_SELECTION, &sel) >= 0) {
    r = &sel.r;
  } else {
    /* For ancient kernels, fall back to G_CROP */
    memset (&crop, 0, sizeof (struct v4l2_crop));
    crop.type = v4l2object->type;
    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_CROP, &crop) >= 0)
      r = &crop.c;
  }
  if (r) {
    align.padding_left = r->left;
    align.padding_top = r->top;
    align.padding_right = width - r->width - r->left;
    align.padding_bottom = height - r->height - r->top;
    width = r->width;
    height = r->height;
  }

  gst_video_info_set_format (info, format, width, height);

  switch (fmt.fmt.pix.field) {
    case V4L2_FIELD_ANY:
    case V4L2_FIELD_NONE:
      info->interlace_mode = GST_VIDEO_INTERLACE_MODE_PROGRESSIVE;
      break;
    case V4L2_FIELD_INTERLACED:
    case V4L2_FIELD_INTERLACED_TB:
    case V4L2_FIELD_INTERLACED_BT:
      info->interlace_mode = GST_VIDEO_INTERLACE_MODE_INTERLEAVED;
      break;
    default:
      goto unsupported_field;
  }

  gst_v4l2_object_get_colorspace (&fmt, &info->colorimetry);

  gst_v4l2_object_save_format (v4l2object, fmtdesc, &fmt, info, &align);

  /* Shall we setup the pool ? */

  return TRUE;

get_fmt_failed:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Video device did not provide output format.")), GST_ERROR_SYSTEM);
    return FALSE;
  }
invalid_dimensions:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Video device returned invalid dimensions.")),
        ("Expected non 0 dimensions, got %dx%d", fmt.fmt.pix.width,
            fmt.fmt.pix.height));
    return FALSE;
  }
unsupported_field:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,
        (_("Video device uses an unsupported interlacing method.")),
        ("V4L2 field type %d not supported", fmt.fmt.pix.field));
    return FALSE;
  }
unsupported_format:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,
        (_("Video device uses an unsupported pixel format.")),
        ("V4L2 format %" GST_FOURCC_FORMAT " not supported",
            GST_FOURCC_ARGS (fmt.fmt.pix.pixelformat)));
    return FALSE;
  }
}

gboolean
gst_v4l2_object_set_crop (GstV4l2Object * obj)
{
  struct v4l2_selection sel = { 0 };
  struct v4l2_crop crop = { 0 };

  sel.type = obj->type;
  sel.target = V4L2_SEL_TGT_CROP;
  sel.flags = 0;
  sel.r.left = obj->align.padding_left;
  sel.r.top = obj->align.padding_top;
  sel.r.width = obj->info.width;
  sel.r.height = obj->info.height;

  crop.type = obj->type;
  crop.c = sel.r;

  if (obj->align.padding_left + obj->align.padding_top +
      obj->align.padding_right + obj->align.padding_bottom == 0) {
    GST_DEBUG_OBJECT (obj->dbg_obj, "no cropping needed");
    return TRUE;
  }

  GST_DEBUG_OBJECT (obj->dbg_obj,
      "Desired cropping left %u, top %u, size %ux%u", crop.c.left, crop.c.top,
      crop.c.width, crop.c.height);

  if (obj->ioctl (obj->video_fd, VIDIOC_S_SELECTION, &sel) < 0) {
    if (errno != ENOTTY) {
      GST_WARNING_OBJECT (obj->dbg_obj,
          "Failed to set crop rectangle with VIDIOC_S_SELECTION: %s",
          g_strerror (errno));
      return FALSE;
    } else {
      if (obj->ioctl (obj->video_fd, VIDIOC_S_CROP, &crop) < 0) {
        GST_WARNING_OBJECT (obj->dbg_obj, "VIDIOC_S_CROP failed");
        return FALSE;
      }

      if (obj->ioctl (obj->video_fd, VIDIOC_G_CROP, &crop) < 0) {
        GST_WARNING_OBJECT (obj->dbg_obj, "VIDIOC_G_CROP failed");
        return FALSE;
      }

      sel.r = crop.c;
    }
  }

  GST_DEBUG_OBJECT (obj->dbg_obj,
      "Got cropping left %u, top %u, size %ux%u", crop.c.left, crop.c.top,
      crop.c.width, crop.c.height);

  return TRUE;
}

gboolean
gst_v4l2_object_caps_equal (GstV4l2Object * v4l2object, GstCaps * caps)
{
  GstStructure *config;
  GstCaps *oldcaps;
  gboolean ret;

  if (!v4l2object->pool)
    return FALSE;

  config = gst_buffer_pool_get_config (v4l2object->pool);
  gst_buffer_pool_config_get_params (config, &oldcaps, NULL, NULL, NULL);

  ret = oldcaps && gst_caps_is_equal (caps, oldcaps);

  gst_structure_free (config);

  return ret;
}

gboolean
gst_v4l2_object_caps_is_subset (GstV4l2Object * v4l2object, GstCaps * caps)
{
  GstStructure *config;
  GstCaps *oldcaps;
  gboolean ret;

  if (!v4l2object->pool)
    return FALSE;

  config = gst_buffer_pool_get_config (v4l2object->pool);
  gst_buffer_pool_config_get_params (config, &oldcaps, NULL, NULL, NULL);

  ret = oldcaps && gst_caps_is_subset (oldcaps, caps);

  gst_structure_free (config);

  return ret;
}

GstCaps *
gst_v4l2_object_get_current_caps (GstV4l2Object * v4l2object)
{
  GstStructure *config;
  GstCaps *oldcaps;

  if (!v4l2object->pool)
    return NULL;

  config = gst_buffer_pool_get_config (v4l2object->pool);
  gst_buffer_pool_config_get_params (config, &oldcaps, NULL, NULL, NULL);

  if (oldcaps)
    gst_caps_ref (oldcaps);

  gst_structure_free (config);

  return oldcaps;
}

gboolean
gst_v4l2_object_unlock (GstV4l2Object * v4l2object)
{
  gboolean ret = TRUE;

  GST_LOG_OBJECT (v4l2object->dbg_obj, "start flushing");

  if (v4l2object->pool && gst_buffer_pool_is_active (v4l2object->pool))
    gst_buffer_pool_set_flushing (v4l2object->pool, TRUE);

  return ret;
}

gboolean
gst_v4l2_object_unlock_stop (GstV4l2Object * v4l2object)
{
  gboolean ret = TRUE;

  GST_LOG_OBJECT (v4l2object->dbg_obj, "stop flushing");

  if (v4l2object->pool && gst_buffer_pool_is_active (v4l2object->pool))
    gst_buffer_pool_set_flushing (v4l2object->pool, FALSE);

  return ret;
}

gboolean
gst_v4l2_object_stop (GstV4l2Object * v4l2object)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "stopping");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    goto done;
  if (!GST_V4L2_IS_ACTIVE (v4l2object))
    goto done;

  if (v4l2object->pool) {
    if (!gst_v4l2_buffer_pool_orphan (&v4l2object->pool)) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj, "deactivating pool");
      gst_buffer_pool_set_active (v4l2object->pool, FALSE);
      gst_object_unref (v4l2object->pool);
    }
    v4l2object->pool = NULL;
  }

  GST_V4L2_SET_INACTIVE (v4l2object);

done:
  return TRUE;
}

GstCaps *
gst_v4l2_object_probe_caps (GstV4l2Object * v4l2object, GstCaps * filter)
{
  GstCaps *ret;
  GSList *walk;
  GSList *formats;

  formats = gst_v4l2_object_get_format_list (v4l2object);

  ret = gst_caps_new_empty ();

  if (v4l2object->keep_aspect && !v4l2object->par) {
    struct v4l2_cropcap cropcap;

    memset (&cropcap, 0, sizeof (cropcap));

    cropcap.type = v4l2object->type;
    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_CROPCAP, &cropcap) < 0) {
      if (errno != ENOTTY)
        GST_WARNING_OBJECT (v4l2object->dbg_obj,
            "Failed to probe pixel aspect ratio with VIDIOC_CROPCAP: %s",
            g_strerror (errno));
    } else if (cropcap.pixelaspect.numerator && cropcap.pixelaspect.denominator) {
      v4l2object->par = g_new0 (GValue, 1);
      g_value_init (v4l2object->par, GST_TYPE_FRACTION);
      gst_value_set_fraction (v4l2object->par, cropcap.pixelaspect.numerator,
          cropcap.pixelaspect.denominator);
    }
  }

  for (walk = formats; walk; walk = walk->next) {
    struct v4l2_fmtdesc *format;
    GstStructure *template;
    GstCaps *tmp;

    format = (struct v4l2_fmtdesc *) walk->data;

    template = gst_v4l2_object_v4l2fourcc_to_bare_struct (format->pixelformat);

    if (!template) {
      GST_DEBUG_OBJECT (v4l2object->dbg_obj,
          "unknown format %" GST_FOURCC_FORMAT,
          GST_FOURCC_ARGS (format->pixelformat));
      continue;
    }

    /* If we have a filter, check if we need to probe this format or not */
    if (filter) {
      GstCaps *format_caps = gst_caps_new_empty ();

      gst_caps_append_structure (format_caps, gst_structure_copy (template));

      if (!gst_caps_can_intersect (format_caps, filter)) {
        gst_caps_unref (format_caps);
        gst_structure_free (template);
        continue;
      }

      gst_caps_unref (format_caps);
    }

    tmp = gst_v4l2_object_probe_caps_for_format (v4l2object,
        format->pixelformat, template);
    if (tmp)
      gst_caps_append (ret, tmp);

    gst_structure_free (template);
  }

  if (filter) {
    GstCaps *tmp;

    tmp = ret;
    ret = gst_caps_intersect_full (filter, ret, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (tmp);
  }

  GST_INFO_OBJECT (v4l2object->dbg_obj, "probed caps: %" GST_PTR_FORMAT, ret);

  return ret;
}

GstCaps *
gst_v4l2_object_get_caps (GstV4l2Object * v4l2object, GstCaps * filter)
{
  GstCaps *ret;

  if (v4l2object->probed_caps == NULL)
    v4l2object->probed_caps = gst_v4l2_object_probe_caps (v4l2object, NULL);

  if (filter) {
    ret = gst_caps_intersect_full (filter, v4l2object->probed_caps,
        GST_CAPS_INTERSECT_FIRST);
  } else {
    ret = gst_caps_ref (v4l2object->probed_caps);
  }

  return ret;
}

static gboolean
gst_v4l2_object_match_buffer_layout (GstV4l2Object * obj, guint n_planes,
    gsize offset[GST_VIDEO_MAX_PLANES], gint stride[GST_VIDEO_MAX_PLANES],
    gsize buffer_size, guint padded_height)
{
  guint p;
  gboolean need_fmt_update = FALSE;

  if (n_planes != GST_VIDEO_INFO_N_PLANES (&obj->info)) {
    GST_WARNING_OBJECT (obj->dbg_obj,
        "Cannot match buffers with different number planes");
    return FALSE;
  }

  for (p = 0; p < n_planes; p++) {
    if (stride[p] < obj->info.stride[p]) {
      GST_DEBUG_OBJECT (obj->dbg_obj,
          "Not matching as remote stride %i is smaller than %i on plane %u",
          stride[p], obj->info.stride[p], p);
      return FALSE;
    } else if (stride[p] > obj->info.stride[p]) {
      GST_LOG_OBJECT (obj->dbg_obj,
          "remote stride %i is higher than %i on plane %u",
          stride[p], obj->info.stride[p], p);
      need_fmt_update = TRUE;
    }

    if (offset[p] < obj->info.offset[p]) {
      GST_DEBUG_OBJECT (obj->dbg_obj,
          "Not matching as offset %" G_GSIZE_FORMAT
          " is smaller than %" G_GSIZE_FORMAT " on plane %u",
          offset[p], obj->info.offset[p], p);
      return FALSE;
    } else if (offset[p] > obj->info.offset[p]) {
      GST_LOG_OBJECT (obj->dbg_obj,
          "Remote offset %" G_GSIZE_FORMAT
          " is higher than %" G_GSIZE_FORMAT " on plane %u",
          offset[p], obj->info.offset[p], p);
      need_fmt_update = TRUE;
    }
  }

  if (need_fmt_update) {
    struct v4l2_format format;
    gint wanted_stride[GST_VIDEO_MAX_PLANES] = { 0, };

    format = obj->format;

    if (padded_height) {
      GST_DEBUG_OBJECT (obj->dbg_obj, "Padded height %u", padded_height);

      obj->align.padding_bottom =
          padded_height - GST_VIDEO_INFO_HEIGHT (&obj->info);
    } else {
      GST_WARNING_OBJECT (obj->dbg_obj,
          "Failed to compute padded height; keep the default one");
      padded_height = format.fmt.pix_mp.height;
    }

    /* update the current format with the stride we want to import from */
    if (V4L2_TYPE_IS_MULTIPLANAR (obj->type)) {
      guint i;

      GST_DEBUG_OBJECT (obj->dbg_obj, "Wanted strides:");

      for (i = 0; i < obj->n_v4l2_planes; i++) {
        gint plane_stride = stride[i];

        if (GST_VIDEO_FORMAT_INFO_IS_TILED (obj->info.finfo))
          plane_stride = GST_VIDEO_TILE_X_TILES (plane_stride) <<
              GST_VIDEO_FORMAT_INFO_TILE_WS (obj->info.finfo);

        format.fmt.pix_mp.plane_fmt[i].bytesperline = plane_stride;
        format.fmt.pix_mp.height = padded_height;
        wanted_stride[i] = plane_stride;
        GST_DEBUG_OBJECT (obj->dbg_obj, "    [%u] %i", i, wanted_stride[i]);
      }
    } else {
      gint plane_stride = stride[0];

      GST_DEBUG_OBJECT (obj->dbg_obj, "Wanted stride: %i", plane_stride);

      if (GST_VIDEO_FORMAT_INFO_IS_TILED (obj->info.finfo))
        plane_stride = GST_VIDEO_TILE_X_TILES (plane_stride) <<
            GST_VIDEO_FORMAT_INFO_TILE_WS (obj->info.finfo);

      format.fmt.pix.bytesperline = plane_stride;
      format.fmt.pix.height = padded_height;
      wanted_stride[0] = plane_stride;
    }

    if (obj->ioctl (obj->video_fd, VIDIOC_S_FMT, &format) < 0) {
      GST_WARNING_OBJECT (obj->dbg_obj,
          "Something went wrong trying to update current format: %s",
          g_strerror (errno));
      return FALSE;
    }

    gst_v4l2_object_save_format (obj, obj->fmtdesc, &format, &obj->info,
        &obj->align);

    if (V4L2_TYPE_IS_MULTIPLANAR (obj->type)) {
      guint i;

      for (i = 0; i < obj->n_v4l2_planes; i++) {
        if (format.fmt.pix_mp.plane_fmt[i].bytesperline != wanted_stride[i]) {
          GST_DEBUG_OBJECT (obj->dbg_obj,
              "[%i] Driver did not accept the new stride (wants %i, got %i)",
              i, wanted_stride[i], format.fmt.pix_mp.plane_fmt[i].bytesperline);
          return FALSE;
        }
      }

      if (format.fmt.pix_mp.height != padded_height) {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "Driver did not accept the padded height (wants %i, got %i)",
            padded_height, format.fmt.pix_mp.height);
      }
    } else {
      if (format.fmt.pix.bytesperline != wanted_stride[0]) {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "Driver did not accept the new stride (wants %i, got %i)",
            wanted_stride[0], format.fmt.pix.bytesperline);
        return FALSE;
      }

      if (format.fmt.pix.height != padded_height) {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "Driver did not accept the padded height (wants %i, got %i)",
            padded_height, format.fmt.pix.height);
      }
    }
  }

  if (obj->align.padding_bottom) {
    /* Crop because of vertical padding */
    GST_DEBUG_OBJECT (obj->dbg_obj, "crop because of bottom padding of %d",
        obj->align.padding_bottom);
    gst_v4l2_object_set_crop (obj);
  }

  return TRUE;
}

static gboolean
validate_video_meta_struct (GstV4l2Object * obj, const GstStructure * s)
{
  guint i;

  for (i = 0; i < gst_structure_n_fields (s); i++) {
    const gchar *name = gst_structure_nth_field_name (s, i);

    if (!g_str_equal (name, "padding-top")
        && !g_str_equal (name, "padding-bottom")
        && !g_str_equal (name, "padding-left")
        && !g_str_equal (name, "padding-right")) {
      GST_WARNING_OBJECT (obj->dbg_obj, "Unknown video meta field: '%s'", name);
      return FALSE;
    }
  }

  return TRUE;
}

static gboolean
gst_v4l2_object_match_buffer_layout_from_struct (GstV4l2Object * obj,
    const GstStructure * s, GstCaps * caps, guint buffer_size)
{
  GstVideoInfo info;
  GstVideoAlignment align;
  gsize plane_size[GST_VIDEO_MAX_PLANES];

  if (!validate_video_meta_struct (obj, s))
    return FALSE;

  if (!gst_video_info_from_caps (&info, caps)) {
    GST_WARNING_OBJECT (obj->dbg_obj, "Failed to create video info");
    return FALSE;
  }

  gst_video_alignment_reset (&align);

  gst_structure_get_uint (s, "padding-top", &align.padding_top);
  gst_structure_get_uint (s, "padding-bottom", &align.padding_bottom);
  gst_structure_get_uint (s, "padding-left", &align.padding_left);
  gst_structure_get_uint (s, "padding-right", &align.padding_right);

  if (align.padding_top || align.padding_bottom || align.padding_left ||
      align.padding_right) {
    GST_DEBUG_OBJECT (obj->dbg_obj,
        "Upstream requested padding (top: %d bottom: %d left: %d right: %d)",
        align.padding_top, align.padding_bottom, align.padding_left,
        align.padding_right);
  }

  if (!gst_video_info_align_full (&info, &align, plane_size)) {
    GST_WARNING_OBJECT (obj->dbg_obj, "Failed to align video info");
    return FALSE;
  }

  if (GST_VIDEO_INFO_SIZE (&info) != buffer_size) {
    GST_WARNING_OBJECT (obj->dbg_obj,
        "Requested buffer size (%d) doesn't match video info size (%"
        G_GSIZE_FORMAT ")", buffer_size, GST_VIDEO_INFO_SIZE (&info));
    return FALSE;
  }

  GST_DEBUG_OBJECT (obj->dbg_obj,
      "try matching buffer layout requested by downstream");

  gst_v4l2_object_match_buffer_layout (obj, GST_VIDEO_INFO_N_PLANES (&info),
      info.offset, info.stride, buffer_size,
      GST_VIDEO_INFO_PLANE_HEIGHT (&info, 0, plane_size));

  return TRUE;
}

gboolean
gst_v4l2_object_decide_allocation (GstV4l2Object * obj, GstQuery * query)
{
  GstCaps *caps;
  GstBufferPool *pool = NULL, *other_pool = NULL;
  GstStructure *config;
  guint size, min, max, own_min = 0;
  gboolean update;
  gboolean has_video_meta;
  gboolean can_share_own_pool, pushing_from_our_pool = FALSE;
  GstAllocator *allocator = NULL;
  GstAllocationParams params = { 0 };
  guint video_idx;

  GST_DEBUG_OBJECT (obj->dbg_obj, "decide allocation");

  g_return_val_if_fail (obj->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
      obj->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, FALSE);

  gst_query_parse_allocation (query, &caps, NULL);

  if (obj->pool == NULL) {
    if (!gst_v4l2_object_setup_pool (obj, caps))
      goto pool_failed;
  }

  if (gst_query_get_n_allocation_params (query) > 0)
    gst_query_parse_nth_allocation_param (query, 0, &allocator, &params);

  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
    update = TRUE;
  } else {
    pool = NULL;
    min = max = 0;
    size = 0;
    update = FALSE;
  }

  GST_DEBUG_OBJECT (obj->dbg_obj, "allocation: size:%u min:%u max:%u pool:%"
      GST_PTR_FORMAT, size, min, max, pool);

  has_video_meta =
      gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE,
      &video_idx);

  if (has_video_meta) {
    const GstStructure *params;
    gst_query_parse_nth_allocation_meta (query, video_idx, &params);

    if (params)
      gst_v4l2_object_match_buffer_layout_from_struct (obj, params, caps, size);
  }

  can_share_own_pool = (has_video_meta || !obj->need_video_meta);

  gst_v4l2_get_driver_min_buffers (obj);
  /* We can't share our own pool, if it exceed V4L2 capacity */
  if (min + obj->min_buffers + 1 > VIDEO_MAX_FRAME)
    can_share_own_pool = FALSE;

  /* select a pool */
  switch (obj->mode) {
    case GST_V4L2_IO_RW:
      if (pool) {
        /* in READ/WRITE mode, prefer a downstream pool because our own pool
         * doesn't help much, we have to write to it as well */
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "read/write mode: using downstream pool");
        /* use the bigest size, when we use our own pool we can't really do any
         * other size than what the hardware gives us but for downstream pools
         * we can try */
        size = MAX (size, obj->info.size);
      } else if (can_share_own_pool) {
        /* no downstream pool, use our own then */
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "read/write mode: no downstream pool, using our own");
        pool = gst_object_ref (obj->pool);
        size = obj->info.size;
        pushing_from_our_pool = TRUE;
      }
      break;

    case GST_V4L2_IO_USERPTR:
    case GST_V4L2_IO_DMABUF_IMPORT:
      /* in importing mode, prefer our own pool, and pass the other pool to
       * our own, so it can serve itself */
      if (pool == NULL)
        goto no_downstream_pool;
      gst_v4l2_buffer_pool_set_other_pool (GST_V4L2_BUFFER_POOL (obj->pool),
          pool);
      other_pool = pool;
      gst_object_unref (pool);
      pool = gst_object_ref (obj->pool);
      size = obj->info.size;
      break;

    case GST_V4L2_IO_MMAP:
    case GST_V4L2_IO_DMABUF:
      /* in streaming mode, prefer our own pool */
      /* Check if we can use it ... */
      if (can_share_own_pool) {
        if (pool)
          gst_object_unref (pool);
        pool = gst_object_ref (obj->pool);
        size = obj->info.size;
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "streaming mode: using our own pool %" GST_PTR_FORMAT, pool);
        pushing_from_our_pool = TRUE;
      } else if (pool) {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "streaming mode: copying to downstream pool %" GST_PTR_FORMAT,
            pool);
      } else {
        GST_DEBUG_OBJECT (obj->dbg_obj,
            "streaming mode: no usable pool, copying to generic pool");
        size = MAX (size, obj->info.size);
      }
      break;
    case GST_V4L2_IO_AUTO:
    default:
      GST_WARNING_OBJECT (obj->dbg_obj, "unhandled mode");
      break;
  }

  if (size == 0)
    goto no_size;

  /* If pushing from our own pool, configure it with queried minimum,
   * otherwise use the minimum required */
  if (pushing_from_our_pool) {
    /* When pushing from our own pool, we need what downstream one, to be able
     * to fill the pipeline, the minimum required to decoder according to the
     * driver and 2 more, so we don't endup up with everything downstream or
     * held by the decoder. We account 2 buffers for v4l2 so when one is being
     * pushed downstream the other one can already be queued for the next
     * frame. */
    own_min = min + obj->min_buffers + 2;

    /* If no allocation parameters where provided, allow for a little more
     * buffers and enable copy threshold */
    if (!update) {
      own_min += 2;
      gst_v4l2_buffer_pool_copy_at_threshold (GST_V4L2_BUFFER_POOL (pool),
          TRUE);
    } else {
      gst_v4l2_buffer_pool_copy_at_threshold (GST_V4L2_BUFFER_POOL (pool),
          FALSE);
    }

  } else {
    /* In this case we'll have to configure two buffer pool. For our buffer
     * pool, we'll need what the driver one, and one more, so we can dequeu */
    own_min = obj->min_buffers + 1;
    own_min = MAX (own_min, GST_V4L2_MIN_BUFFERS);

    /* for the downstream pool, we keep what downstream wants, though ensure
     * at least a minimum if downstream didn't suggest anything (we are
     * expecting the base class to create a default one for the context) */
    min = MAX (min, GST_V4L2_MIN_BUFFERS);

    /* To import we need the other pool to hold at least own_min */
    if (obj->pool == pool)
      min += own_min;
  }

  /* Request a bigger max, if one was suggested but it's too small */
  if (max != 0)
    max = MAX (min, max);

  /* First step, configure our own pool */
  config = gst_buffer_pool_get_config (obj->pool);

  if (obj->need_video_meta || has_video_meta) {
    GST_DEBUG_OBJECT (obj->dbg_obj, "activate Video Meta");
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }

  gst_buffer_pool_config_set_allocator (config, allocator, &params);
  gst_buffer_pool_config_set_params (config, caps, size, own_min, 0);

  GST_DEBUG_OBJECT (obj->dbg_obj, "setting own pool config to %"
      GST_PTR_FORMAT, config);

  /* Our pool often need to adjust the value */
  if (!gst_buffer_pool_set_config (obj->pool, config)) {
    config = gst_buffer_pool_get_config (obj->pool);

    GST_DEBUG_OBJECT (obj->dbg_obj, "own pool config changed to %"
        GST_PTR_FORMAT, config);

    /* our pool will adjust the maximum buffer, which we are fine with */
    if (!gst_buffer_pool_set_config (obj->pool, config))
      goto config_failed;
  }

  /* Now configure the other pool if different */
  if (obj->pool != pool)
    other_pool = pool;

  if (other_pool) {
    config = gst_buffer_pool_get_config (other_pool);
    gst_buffer_pool_config_set_allocator (config, allocator, &params);
    gst_buffer_pool_config_set_params (config, caps, size, min, max);

    GST_DEBUG_OBJECT (obj->dbg_obj, "setting other pool config to %"
        GST_PTR_FORMAT, config);

    /* if downstream supports video metadata, add this to the pool config */
    if (has_video_meta) {
      GST_DEBUG_OBJECT (obj->dbg_obj, "activate Video Meta");
      gst_buffer_pool_config_add_option (config,
          GST_BUFFER_POOL_OPTION_VIDEO_META);
    }

    if (!gst_buffer_pool_set_config (other_pool, config)) {
      config = gst_buffer_pool_get_config (other_pool);

      if (!gst_buffer_pool_config_validate_params (config, caps, size, min,
              max)) {
        gst_structure_free (config);
        goto config_failed;
      }

      if (!gst_buffer_pool_set_config (other_pool, config))
        goto config_failed;
    }
  }

  if (pool) {
    /* For simplicity, simply read back the active configuration, so our base
     * class get the right information */
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_params (config, NULL, &size, &min, &max);
    gst_structure_free (config);
  }

  if (update)
    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
  else
    gst_query_add_allocation_pool (query, pool, size, min, max);

  if (allocator)
    gst_object_unref (allocator);

  if (pool)
    gst_object_unref (pool);

  return TRUE;

pool_failed:
  {
    /* setup_pool already send the error */
    goto cleanup;
  }
config_failed:
  {
    GST_ELEMENT_ERROR (obj->element, RESOURCE, SETTINGS,
        (_("Failed to configure internal buffer pool.")), (NULL));
    goto cleanup;
  }
no_size:
  {
    GST_ELEMENT_ERROR (obj->element, RESOURCE, SETTINGS,
        (_("Video device did not suggest any buffer size.")), (NULL));
    goto cleanup;
  }
cleanup:
  {
    if (allocator)
      gst_object_unref (allocator);

    if (pool)
      gst_object_unref (pool);
    return FALSE;
  }
no_downstream_pool:
  {
    GST_ELEMENT_ERROR (obj->element, RESOURCE, SETTINGS,
        (_("No downstream pool to import from.")),
        ("When importing DMABUF or USERPTR, we need a pool to import from"));
    return FALSE;
  }
}

gboolean
gst_v4l2_object_propose_allocation (GstV4l2Object * obj, GstQuery * query)
{
  GstBufferPool *pool;
  /* we need at least 2 buffers to operate */
  guint size, min, max;
  GstCaps *caps;
  gboolean need_pool;

  /* Set defaults allocation parameters */
  size = obj->info.size;
  min = GST_V4L2_MIN_BUFFERS;
  max = VIDEO_MAX_FRAME;

  gst_query_parse_allocation (query, &caps, &need_pool);

  if (caps == NULL)
    goto no_caps;

  switch (obj->mode) {
    case GST_V4L2_IO_MMAP:
    case GST_V4L2_IO_DMABUF:
      if ((pool = obj->pool))
        gst_object_ref (pool);
      break;
    default:
      pool = NULL;
      break;
  }

  if (pool != NULL) {
    GstCaps *pcaps;
    GstStructure *config;

    /* we had a pool, check caps */
    config = gst_buffer_pool_get_config (pool);
    gst_buffer_pool_config_get_params (config, &pcaps, NULL, NULL, NULL);

    GST_DEBUG_OBJECT (obj->dbg_obj,
        "we had a pool with caps %" GST_PTR_FORMAT, pcaps);
    if (!gst_caps_is_equal (caps, pcaps)) {
      gst_structure_free (config);
      gst_object_unref (pool);
      goto different_caps;
    }
    gst_structure_free (config);
  }
  gst_v4l2_get_driver_min_buffers (obj);

  min = MAX (obj->min_buffers, GST_V4L2_MIN_BUFFERS);

  gst_query_add_allocation_pool (query, pool, size, min, max);

  /* we also support various metadata */
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  if (pool)
    gst_object_unref (pool);

  return TRUE;

  /* ERRORS */
no_caps:
  {
    GST_DEBUG_OBJECT (obj->dbg_obj, "no caps specified");
    return FALSE;
  }
different_caps:
  {
    /* different caps, we can't use this pool */
    GST_DEBUG_OBJECT (obj->dbg_obj, "pool has different caps");
    return FALSE;
  }
}

gboolean
gst_v4l2_object_try_import (GstV4l2Object * obj, GstBuffer * buffer)
{
  GstVideoMeta *vmeta;
  guint n_mem = gst_buffer_n_memory (buffer);

  /* only import if requested */
  switch (obj->mode) {
    case GST_V4L2_IO_USERPTR:
    case GST_V4L2_IO_DMABUF_IMPORT:
      break;
    default:
      GST_DEBUG_OBJECT (obj->dbg_obj,
          "The io-mode does not enable importation");
      return FALSE;
  }

  vmeta = gst_buffer_get_video_meta (buffer);
  if (!vmeta && obj->need_video_meta) {
    GST_DEBUG_OBJECT (obj->dbg_obj, "Downstream buffer uses standard "
        "stride/offset while the driver does not.");
    return FALSE;
  }

  /* we need matching strides/offsets and size */
  if (vmeta) {
    guint plane_height[GST_VIDEO_MAX_PLANES] = { 0, };

    gst_video_meta_get_plane_height (vmeta, plane_height);

    if (!gst_v4l2_object_match_buffer_layout (obj, vmeta->n_planes,
            vmeta->offset, vmeta->stride, gst_buffer_get_size (buffer),
            plane_height[0]))
      return FALSE;
  }

  /* we can always import single memory buffer, but otherwise we need the same
   * amount of memory object. */
  if (n_mem != 1 && n_mem != obj->n_v4l2_planes) {
    GST_DEBUG_OBJECT (obj->dbg_obj, "Can only import %i memory, "
        "buffers contains %u memory", obj->n_v4l2_planes, n_mem);
    return FALSE;
  }

  /* For DMABuf importation we need DMABuf of course */
  if (obj->mode == GST_V4L2_IO_DMABUF_IMPORT) {
    guint i;

    for (i = 0; i < n_mem; i++) {
      GstMemory *mem = gst_buffer_peek_memory (buffer, i);

      if (!gst_is_dmabuf_memory (mem)) {
        GST_DEBUG_OBJECT (obj->dbg_obj, "Cannot import non-DMABuf memory.");
        return FALSE;
      }
    }
  }

  /* for the remaining, only the kernel driver can tell */
  return TRUE;
}
