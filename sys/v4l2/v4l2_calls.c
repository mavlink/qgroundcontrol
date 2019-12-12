/* GStreamer
 *
 * Copyright (C) 2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2006 Edgard Lima <edgard.lima@gmail.com>
 *
 * v4l2_calls.c - generic V4L2 calls handling
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __sun
/* Needed on older Solaris Nevada builds (72 at least) */
#include <stropts.h>
#include <sys/ioccom.h>
#endif
#include "gstv4l2object.h"
#include "gstv4l2tuner.h"
#include "gstv4l2colorbalance.h"

#include "gstv4l2src.h"
#include "gstv4l2sink.h"
#include "gstv4l2videodec.h"

#include "gst/gst-i18n-plugin.h"

GST_DEBUG_CATEGORY_EXTERN (v4l2_debug);
#define GST_CAT_DEFAULT v4l2_debug

/******************************************************
 * gst_v4l2_get_capabilities():
 *   get the device's capturing capabilities
 * return value: TRUE on success, FALSE on error
 ******************************************************/
static gboolean
gst_v4l2_get_capabilities (GstV4l2Object * v4l2object)
{
  GstElement *e;

  e = v4l2object->element;

  GST_DEBUG_OBJECT (e, "getting capabilities");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_QUERYCAP,
          &v4l2object->vcap) < 0)
    goto cap_failed;

  if (v4l2object->vcap.capabilities & V4L2_CAP_DEVICE_CAPS)
    v4l2object->device_caps = v4l2object->vcap.device_caps;
  else
    v4l2object->device_caps = v4l2object->vcap.capabilities;

  GST_LOG_OBJECT (e, "driver:      '%s'", v4l2object->vcap.driver);
  GST_LOG_OBJECT (e, "card:        '%s'", v4l2object->vcap.card);
  GST_LOG_OBJECT (e, "bus_info:    '%s'", v4l2object->vcap.bus_info);
  GST_LOG_OBJECT (e, "version:     %08x", v4l2object->vcap.version);
  GST_LOG_OBJECT (e, "capabilities: %08x", v4l2object->device_caps);

  return TRUE;

  /* ERRORS */
cap_failed:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, SETTINGS,
        (_("Error getting capabilities for device '%s': "
                "It isn't a v4l2 driver. Check if it is a v4l1 driver."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

/******************************************************
 * The video4linux command line tool v4l2-ctrl
 * normalises the names of the controls received from
 * the kernel like:
 *
 *     "Exposure (absolute)" -> "exposure_absolute"
 *
 * We follow their lead here.  @name is modified
 * in-place.
 ******************************************************/
static void
gst_v4l2_normalise_control_name (gchar * name)
{
  int i, j;
  for (i = 0, j = 0; name[j]; ++j) {
    if (g_ascii_isalnum (name[j])) {
      if (i > 0 && !g_ascii_isalnum (name[j - 1]))
        name[i++] = '_';
      name[i++] = g_ascii_tolower (name[j]);
    }
  }
  name[i++] = '\0';
}

/******************************************************
 * gst_v4l2_empty_lists() and gst_v4l2_fill_lists():
 *   fill/empty the lists of enumerations
 * return value: TRUE on success, FALSE on error
 ******************************************************/
static gboolean
gst_v4l2_fill_lists (GstV4l2Object * v4l2object)
{
  gint n, next;
  struct v4l2_queryctrl control = { 0, };

  GstElement *e;

  e = v4l2object->element;

  GST_DEBUG_OBJECT (e, "getting enumerations");
  GST_V4L2_CHECK_OPEN (v4l2object);

  GST_DEBUG_OBJECT (e, "  channels");
  /* and now, the channels */
  for (n = 0;; n++) {
    struct v4l2_input input;
    GstV4l2TunerChannel *v4l2channel;
    GstTunerChannel *channel;

    memset (&input, 0, sizeof (input));

    input.index = n;
    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_ENUMINPUT, &input) < 0) {
      if (errno == EINVAL || errno == ENOTTY)
        break;                  /* end of enumeration */
      else {
        GST_ELEMENT_ERROR (e, RESOURCE, SETTINGS,
            (_("Failed to query attributes of input %d in device %s"),
                n, v4l2object->videodev),
            ("Failed to get %d in input enumeration for %s. (%d - %s)",
                n, v4l2object->videodev, errno, strerror (errno)));
        return FALSE;
      }
    }

    GST_LOG_OBJECT (e, "   index:     %d", input.index);
    GST_LOG_OBJECT (e, "   name:      '%s'", input.name);
    GST_LOG_OBJECT (e, "   type:      %08x", input.type);
    GST_LOG_OBJECT (e, "   audioset:  %08x", input.audioset);
    GST_LOG_OBJECT (e, "   std:       %016" G_GINT64_MODIFIER "x",
        (guint64) input.std);
    GST_LOG_OBJECT (e, "   status:    %08x", input.status);

    v4l2channel = g_object_new (GST_TYPE_V4L2_TUNER_CHANNEL, NULL);
    channel = GST_TUNER_CHANNEL (v4l2channel);
    channel->label = g_strdup ((const gchar *) input.name);
    channel->flags = GST_TUNER_CHANNEL_INPUT;
    v4l2channel->index = n;

    if (input.type == V4L2_INPUT_TYPE_TUNER) {
      struct v4l2_tuner vtun;

      v4l2channel->tuner = input.tuner;
      channel->flags |= GST_TUNER_CHANNEL_FREQUENCY;

      vtun.index = input.tuner;
      if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_TUNER, &vtun) < 0) {
        GST_ELEMENT_ERROR (e, RESOURCE, SETTINGS,
            (_("Failed to get setting of tuner %d on device '%s'."),
                input.tuner, v4l2object->videodev), GST_ERROR_SYSTEM);
        g_object_unref (G_OBJECT (channel));
        return FALSE;
      }

      channel->freq_multiplicator =
          62.5 * ((vtun.capability & V4L2_TUNER_CAP_LOW) ? 1 : 1000);
      channel->min_frequency = vtun.rangelow * channel->freq_multiplicator;
      channel->max_frequency = vtun.rangehigh * channel->freq_multiplicator;
      channel->min_signal = 0;
      channel->max_signal = 0xffff;
    }
    if (input.audioset) {
      /* we take the first. We don't care for
       * the others for now */
      while (!(input.audioset & (1 << v4l2channel->audio)))
        v4l2channel->audio++;
      channel->flags |= GST_TUNER_CHANNEL_AUDIO;
    }

    v4l2object->channels =
        g_list_prepend (v4l2object->channels, (gpointer) channel);
  }
  v4l2object->channels = g_list_reverse (v4l2object->channels);

  GST_DEBUG_OBJECT (e, "  norms");
  /* norms... */
  for (n = 0;; n++) {
    struct v4l2_standard standard = { 0, };
    GstV4l2TunerNorm *v4l2norm;

    GstTunerNorm *norm;

    /* fill in defaults */
    standard.frameperiod.numerator = 1;
    standard.frameperiod.denominator = 0;
    standard.index = n;

    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_ENUMSTD, &standard) < 0) {
      if (errno == EINVAL || errno == ENOTTY)
        break;                  /* end of enumeration */
#ifdef ENODATA
      else if (errno == ENODATA)
        break;                  /* end of enumeration, as of Linux 3.7-rc1 */
#endif
      else {
        GST_ELEMENT_ERROR (e, RESOURCE, SETTINGS,
            (_("Failed to query norm on device '%s'."),
                v4l2object->videodev),
            ("Failed to get attributes for norm %d on divide '%s'. (%d - %s)",
                n, v4l2object->videodev, errno, strerror (errno)));
        return FALSE;
      }
    }

    GST_DEBUG_OBJECT (e, "    '%s', fps: %d / %d",
        standard.name, standard.frameperiod.denominator,
        standard.frameperiod.numerator);

    v4l2norm = g_object_new (GST_TYPE_V4L2_TUNER_NORM, NULL);
    norm = GST_TUNER_NORM (v4l2norm);
    norm->label = g_strdup ((const gchar *) standard.name);
    gst_value_set_fraction (&norm->framerate,
        standard.frameperiod.denominator, standard.frameperiod.numerator);
    v4l2norm->index = standard.id;

    GST_DEBUG_OBJECT (v4l2object->dbg_obj, "index=%08x, label=%s",
        (unsigned int) v4l2norm->index, norm->label);

    v4l2object->norms = g_list_prepend (v4l2object->norms, (gpointer) norm);
  }
  v4l2object->norms = g_list_reverse (v4l2object->norms);

  GST_DEBUG_OBJECT (e, "  controls+menus");

  /* and lastly, controls+menus (if appropriate) */
  next = V4L2_CTRL_FLAG_NEXT_CTRL;
  n = 0;
  control.id = next;

  while (TRUE) {
    GstV4l2ColorBalanceChannel *v4l2channel;
    GstColorBalanceChannel *channel;

    if (!next)
      n++;

  retry:
    /* when we reached the last official CID, continue with private CIDs */
    if (n == V4L2_CID_LASTP1) {
      GST_DEBUG_OBJECT (e, "checking private CIDs");
      n = V4L2_CID_PRIVATE_BASE;
    }
    GST_DEBUG_OBJECT (e, "checking control %08x", n);

    control.id = n | next;
    if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_QUERYCTRL,
            &control) < 0) {
      if (next) {
        if (n > 0) {
          GST_DEBUG_OBJECT (e, "controls finished");
          break;
        } else {
          GST_DEBUG_OBJECT (e, "V4L2_CTRL_FLAG_NEXT_CTRL not supported.");
          next = 0;
          n = V4L2_CID_BASE;
          goto retry;
        }
      }
      if (errno == EINVAL || errno == ENOTTY || errno == EIO || errno == ENOENT) {
        if (n < V4L2_CID_PRIVATE_BASE) {
          GST_DEBUG_OBJECT (e, "skipping control %08x", n);
          /* continue so that we also check private controls */
          n = V4L2_CID_PRIVATE_BASE - 1;
          continue;
        } else {
          GST_DEBUG_OBJECT (e, "controls finished");
          break;
        }
      } else {
        GST_WARNING_OBJECT (e, "Failed querying control %d on device '%s'. "
            "(%d - %s)", n, v4l2object->videodev, errno, strerror (errno));
        continue;
      }
    }
    /* bogus driver might mess with id in unexpected ways (e.g. set to 0), so
     * make sure to simply try all if V4L2_CTRL_FLAG_NEXT_CTRL not supported */
    if (next)
      n = control.id;
    if (control.flags & V4L2_CTRL_FLAG_DISABLED) {
      GST_DEBUG_OBJECT (e, "skipping disabled control");
      continue;
    }

    if (control.type == V4L2_CTRL_TYPE_CTRL_CLASS) {
      GST_DEBUG_OBJECT (e, "starting control class '%s'", control.name);
      continue;
    }

    switch (control.type) {
      case V4L2_CTRL_TYPE_INTEGER:
      case V4L2_CTRL_TYPE_BOOLEAN:
      case V4L2_CTRL_TYPE_MENU:
      case V4L2_CTRL_TYPE_INTEGER_MENU:
      case V4L2_CTRL_TYPE_BITMASK:
      case V4L2_CTRL_TYPE_BUTTON:{
        control.name[31] = '\0';
        gst_v4l2_normalise_control_name ((gchar *) control.name);
        g_datalist_id_set_data (&v4l2object->controls,
            g_quark_from_string ((const gchar *) control.name),
            GINT_TO_POINTER (n));
        break;
      }
      default:
        GST_DEBUG_OBJECT (e,
            "Control type for '%s' not supported for extra controls.",
            control.name);
        break;
    }

    switch (n) {
      case V4L2_CID_BRIGHTNESS:
      case V4L2_CID_CONTRAST:
      case V4L2_CID_SATURATION:
      case V4L2_CID_HUE:
      case V4L2_CID_BLACK_LEVEL:
      case V4L2_CID_AUTO_WHITE_BALANCE:
      case V4L2_CID_DO_WHITE_BALANCE:
      case V4L2_CID_RED_BALANCE:
      case V4L2_CID_BLUE_BALANCE:
      case V4L2_CID_GAMMA:
      case V4L2_CID_EXPOSURE:
      case V4L2_CID_AUTOGAIN:
      case V4L2_CID_GAIN:
      case V4L2_CID_SHARPNESS:
        /* we only handle these for now (why?) */
        break;
      case V4L2_CID_HFLIP:
      case V4L2_CID_VFLIP:
      case V4L2_CID_PAN_RESET:
      case V4L2_CID_TILT_RESET:
        /* not handled here, handled by VideoOrientation interface */
        control.id++;
        break;
      case V4L2_CID_AUDIO_VOLUME:
      case V4L2_CID_AUDIO_BALANCE:
      case V4L2_CID_AUDIO_BASS:
      case V4L2_CID_AUDIO_TREBLE:
      case V4L2_CID_AUDIO_MUTE:
      case V4L2_CID_AUDIO_LOUDNESS:
        /* FIXME: We should implement GstMixer interface instead */
        /* but let's not be pedantic and make element more useful for now */
        break;
      case V4L2_CID_ALPHA_COMPONENT:
        v4l2object->has_alpha_component = TRUE;
        break;
      default:
        GST_DEBUG_OBJECT (e,
            "ControlID %s (%x) unhandled, FIXME", control.name, n);
        control.id++;
        break;
    }
    if (n != control.id)
      continue;

    GST_DEBUG_OBJECT (e, "Adding ControlID %s (%x)", control.name, n);
    v4l2channel = g_object_new (GST_TYPE_V4L2_COLOR_BALANCE_CHANNEL, NULL);
    channel = GST_COLOR_BALANCE_CHANNEL (v4l2channel);
    channel->label = g_strdup ((const gchar *) control.name);
    v4l2channel->id = n;

#if 0
    /* FIXME: it will be need just when handling private controls
     *(currently none of base controls are of this type) */
    if (control.type == V4L2_CTRL_TYPE_MENU) {
      struct v4l2_querymenu menu, *mptr;

      int i;

      menu.id = n;
      for (i = 0;; i++) {
        menu.index = i;
        if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_QUERYMENU,
                &menu) < 0) {
          if (errno == EINVAL)
            break;              /* end of enumeration */
          else {
            GST_ELEMENT_ERROR (e, RESOURCE, SETTINGS,
                (_("Failed getting controls attributes on device '%s'."),
                    v4l2object->videodev),
                ("Failed to get %d in menu enumeration for %s. (%d - %s)",
                    n, v4l2object->videodev, errno, strerror (errno)));
            return FALSE;
          }
        }
        mptr = g_malloc (sizeof (menu));
        memcpy (mptr, &menu, sizeof (menu));
        menus = g_list_append (menus, mptr);
      }
    }
    v4l2object->menus = g_list_append (v4l2object->menus, menus);
#endif

    switch (control.type) {
      case V4L2_CTRL_TYPE_INTEGER:
        channel->min_value = control.minimum;
        channel->max_value = control.maximum;
        break;
      case V4L2_CTRL_TYPE_BOOLEAN:
        channel->min_value = FALSE;
        channel->max_value = TRUE;
        break;
      default:
        /* FIXME we should find out how to handle V4L2_CTRL_TYPE_BUTTON.
           BUTTON controls like V4L2_CID_DO_WHITE_BALANCE can just be set (1) or
           unset (0), but can't be queried */
        GST_DEBUG_OBJECT (e,
            "Control with non supported type %s (%x), type=%d",
            control.name, n, control.type);
        channel->min_value = channel->max_value = 0;
        break;
    }

    v4l2object->colors =
        g_list_prepend (v4l2object->colors, (gpointer) channel);
  }
  v4l2object->colors = g_list_reverse (v4l2object->colors);

  GST_DEBUG_OBJECT (e, "done");
  return TRUE;
}


static void
gst_v4l2_empty_lists (GstV4l2Object * v4l2object)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "deleting enumerations");

  g_list_foreach (v4l2object->channels, (GFunc) g_object_unref, NULL);
  g_list_free (v4l2object->channels);
  v4l2object->channels = NULL;

  g_list_foreach (v4l2object->norms, (GFunc) g_object_unref, NULL);
  g_list_free (v4l2object->norms);
  v4l2object->norms = NULL;

  g_list_foreach (v4l2object->colors, (GFunc) g_object_unref, NULL);
  g_list_free (v4l2object->colors);
  v4l2object->colors = NULL;

  g_datalist_clear (&v4l2object->controls);
}

static void
gst_v4l2_adjust_buf_type (GstV4l2Object * v4l2object)
{
  /* when calling gst_v4l2_object_new the user decides the initial type
   * so adjust it if multi-planar is supported
   * the driver should make it exclusive. So the driver should
   * not support both MPLANE and non-PLANE.
   * Because even when using MPLANE it still possibles to use it
   * in a contiguous manner. In this case the first v4l2 plane
   * contains all the gst planes.
   */
  switch (v4l2object->type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
      if (v4l2object->device_caps &
          (V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_VIDEO_M2M_MPLANE)) {
        GST_DEBUG ("adjust type to multi-planar output");
        v4l2object->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
      }
      break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
      if (v4l2object->device_caps &
          (V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_M2M_MPLANE)) {
        GST_DEBUG ("adjust type to multi-planar capture");
        v4l2object->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
      }
      break;
    default:
      break;
  }
}

/******************************************************
 * gst_v4l2_open():
 *   open the video device (v4l2object->videodev)
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_open (GstV4l2Object * v4l2object)
{
  struct stat st;
  int libv4l2_fd = -1;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Trying to open device %s",
      v4l2object->videodev);

  GST_V4L2_CHECK_NOT_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  /* be sure we have a device */
  if (!v4l2object->videodev)
    v4l2object->videodev = g_strdup ("/dev/video");

  /* check if it is a device */
  if (stat (v4l2object->videodev, &st) == -1)
    goto stat_failed;

  if (!S_ISCHR (st.st_mode))
    goto no_device;

  /* open the device */
  v4l2object->video_fd =
      open (v4l2object->videodev, O_RDWR /* | O_NONBLOCK */ );

  if (!GST_V4L2_IS_OPEN (v4l2object))
    goto not_open;

#ifdef HAVE_LIBV4L2
  if (v4l2object->fd_open)
    libv4l2_fd = v4l2object->fd_open (v4l2object->video_fd,
        V4L2_ENABLE_ENUM_FMT_EMULATION);
#endif

  /* Note the v4l2_xxx functions are designed so that if they get passed an
     unknown fd, the will behave exactly as their regular xxx counterparts, so
     if v4l2_fd_open fails, we continue as normal (missing the libv4l2 custom
     cam format to normal formats conversion). Chances are big we will still
     fail then though, as normally v4l2_fd_open only fails if the device is not
     a v4l2 device. */
  if (libv4l2_fd != -1)
    v4l2object->video_fd = libv4l2_fd;

  /* get capabilities, error will be posted */
  if (!gst_v4l2_get_capabilities (v4l2object))
    goto error;

  /* do we need to be a capture device? */
  if (GST_IS_V4L2SRC (v4l2object->element) &&
      !(v4l2object->device_caps & (V4L2_CAP_VIDEO_CAPTURE |
              V4L2_CAP_VIDEO_CAPTURE_MPLANE)))
    goto not_capture;

  if (GST_IS_V4L2SINK (v4l2object->element) &&
      !(v4l2object->device_caps & (V4L2_CAP_VIDEO_OUTPUT |
              V4L2_CAP_VIDEO_OUTPUT_MPLANE)))
    goto not_output;

  if (GST_IS_V4L2_VIDEO_DEC (v4l2object->element) &&
      !GST_V4L2_IS_M2M (v4l2object->device_caps))
    goto not_m2m;

  gst_v4l2_adjust_buf_type (v4l2object);

  /* create enumerations, posts errors. */
  if (!gst_v4l2_fill_lists (v4l2object))
    goto error;

  GST_INFO_OBJECT (v4l2object->dbg_obj,
      "Opened device '%s' (%s) successfully",
      v4l2object->vcap.card, v4l2object->videodev);

  if (v4l2object->extra_controls)
    gst_v4l2_set_controls (v4l2object, v4l2object->extra_controls);

  /* UVC devices are never interlaced, and doing VIDIOC_TRY_FMT on them
   * causes expensive and slow USB IO, so don't probe them for interlaced
   */
  if (!strcmp ((char *) v4l2object->vcap.driver, "uvcusb") ||
      !strcmp ((char *) v4l2object->vcap.driver, "uvcvideo")) {
    v4l2object->never_interlaced = TRUE;
  }

  return TRUE;

  /* ERRORS */
stat_failed:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, NOT_FOUND,
        (_("Cannot identify device '%s'."), v4l2object->videodev),
        GST_ERROR_SYSTEM);
    goto error;
  }
no_device:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, NOT_FOUND,
        (_("This isn't a device '%s'."), v4l2object->videodev),
        GST_ERROR_SYSTEM);
    goto error;
  }
not_open:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, OPEN_READ_WRITE,
        (_("Could not open device '%s' for reading and writing."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    goto error;
  }
not_capture:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, NOT_FOUND,
        (_("Device '%s' is not a capture device."),
            v4l2object->videodev),
        ("Capabilities: 0x%x", v4l2object->device_caps));
    goto error;
  }
not_output:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, NOT_FOUND,
        (_("Device '%s' is not a output device."),
            v4l2object->videodev),
        ("Capabilities: 0x%x", v4l2object->device_caps));
    goto error;
  }
not_m2m:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, NOT_FOUND,
        (_("Device '%s' is not a M2M device."),
            v4l2object->videodev),
        ("Capabilities: 0x%x", v4l2object->device_caps));
    goto error;
  }
error:
  {
    if (GST_V4L2_IS_OPEN (v4l2object)) {
      /* close device */
      v4l2object->close (v4l2object->video_fd);
      v4l2object->video_fd = -1;
    }
    /* empty lists */
    gst_v4l2_empty_lists (v4l2object);

    return FALSE;
  }
}

gboolean
gst_v4l2_dup (GstV4l2Object * v4l2object, GstV4l2Object * other)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Trying to dup device %s",
      other->videodev);

  GST_V4L2_CHECK_OPEN (other);
  GST_V4L2_CHECK_NOT_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (other);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  v4l2object->vcap = other->vcap;
  v4l2object->device_caps = other->device_caps;
  gst_v4l2_adjust_buf_type (v4l2object);

  v4l2object->video_fd = v4l2object->dup (other->video_fd);
  if (!GST_V4L2_IS_OPEN (v4l2object))
    goto not_open;

  g_free (v4l2object->videodev);
  v4l2object->videodev = g_strdup (other->videodev);

  GST_INFO_OBJECT (v4l2object->dbg_obj,
      "Cloned device '%s' (%s) successfully",
      v4l2object->vcap.card, v4l2object->videodev);

  v4l2object->never_interlaced = other->never_interlaced;
  v4l2object->no_initial_format = other->no_initial_format;

  return TRUE;

not_open:
  {
    GST_ELEMENT_ERROR (v4l2object->element, RESOURCE, OPEN_READ_WRITE,
        (_("Could not dup device '%s' for reading and writing."),
            v4l2object->videodev), GST_ERROR_SYSTEM);

    return FALSE;
  }
}


/******************************************************
 * gst_v4l2_close():
 *   close the video device (v4l2object->video_fd)
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_close (GstV4l2Object * v4l2object)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "Trying to close %s",
      v4l2object->videodev);

  GST_V4L2_CHECK_OPEN (v4l2object);
  GST_V4L2_CHECK_NOT_ACTIVE (v4l2object);

  /* close device */
  v4l2object->close (v4l2object->video_fd);
  v4l2object->video_fd = -1;

  /* empty lists */
  gst_v4l2_empty_lists (v4l2object);

  return TRUE;
}


/******************************************************
 * gst_v4l2_get_norm()
 *   Get the norm of the current device
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_get_norm (GstV4l2Object * v4l2object, v4l2_std_id * norm)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "getting norm");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_STD, norm) < 0)
    goto std_failed;

  return TRUE;

  /* ERRORS */
std_failed:
  {
    GST_DEBUG ("Failed to get the current norm for device %s",
        v4l2object->videodev);
    return FALSE;
  }
}


/******************************************************
 * gst_v4l2_set_norm()
 *   Set the norm of the current device
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_set_norm (GstV4l2Object * v4l2object, v4l2_std_id norm)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "trying to set norm to "
      "%" G_GINT64_MODIFIER "x", (guint64) norm);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_S_STD, &norm) < 0)
    goto std_failed;

  return TRUE;

  /* ERRORS */
std_failed:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to set norm for device '%s'."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

/******************************************************
 * gst_v4l2_get_frequency():
 *   get the current frequency
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_get_frequency (GstV4l2Object * v4l2object,
    gint tunernum, gulong * frequency)
{
  struct v4l2_frequency freq = { 0, };

  GstTunerChannel *channel;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "getting current tuner frequency");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  channel = gst_tuner_get_channel (GST_TUNER (v4l2object->element));

  freq.tuner = tunernum;
  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_FREQUENCY, &freq) < 0)
    goto freq_failed;

  *frequency = freq.frequency * channel->freq_multiplicator;

  return TRUE;

  /* ERRORS */
freq_failed:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to get current tuner frequency for device '%s'."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}


/******************************************************
 * gst_v4l2_set_frequency():
 *   set frequency
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_set_frequency (GstV4l2Object * v4l2object,
    gint tunernum, gulong frequency)
{
  struct v4l2_frequency freq = { 0, };

  GstTunerChannel *channel;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj,
      "setting current tuner frequency to %lu", frequency);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  channel = gst_tuner_get_channel (GST_TUNER (v4l2object->element));

  freq.tuner = tunernum;
  /* fill in type - ignore error */
  (void) v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_FREQUENCY, &freq);
  freq.frequency = frequency / channel->freq_multiplicator;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_S_FREQUENCY, &freq) < 0)
    goto freq_failed;

  return TRUE;

  /* ERRORS */
freq_failed:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to set current tuner frequency for device '%s' to %lu Hz."),
            v4l2object->videodev, frequency), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

/******************************************************
 * gst_v4l2_signal_strength():
 *   get the strength of the signal on the current input
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_signal_strength (GstV4l2Object * v4l2object,
    gint tunernum, gulong * signal_strength)
{
  struct v4l2_tuner tuner = { 0, };

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "trying to get signal strength");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  tuner.index = tunernum;
  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_TUNER, &tuner) < 0)
    goto tuner_failed;

  *signal_strength = tuner.signal;

  return TRUE;

  /* ERRORS */
tuner_failed:
  {
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to get signal strength for device '%s'."),
            v4l2object->videodev), GST_ERROR_SYSTEM);
    return FALSE;
  }
}

/******************************************************
 * gst_v4l2_get_attribute():
 *   try to get the value of one specific attribute
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_get_attribute (GstV4l2Object * v4l2object,
    int attribute_num, int *value)
{
  struct v4l2_control control = { 0, };

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "getting value of attribute %d",
      attribute_num);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  control.id = attribute_num;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_CTRL, &control) < 0)
    goto ctrl_failed;

  *value = control.value;

  return TRUE;

  /* ERRORS */
ctrl_failed:
  {
    GST_WARNING_OBJECT (v4l2object,
        _("Failed to get value for control %d on device '%s'."),
        attribute_num, v4l2object->videodev);
    return FALSE;
  }
}


/******************************************************
 * gst_v4l2_set_attribute():
 *   try to set the value of one specific attribute
 * return value: TRUE on success, FALSE on error
 ******************************************************/
gboolean
gst_v4l2_set_attribute (GstV4l2Object * v4l2object,
    int attribute_num, const int value)
{
  struct v4l2_control control = { 0, };

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "setting value of attribute %d to %d",
      attribute_num, value);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  control.id = attribute_num;
  control.value = value;
  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_S_CTRL, &control) < 0)
    goto ctrl_failed;

  return TRUE;

  /* ERRORS */
ctrl_failed:
  {
    GST_WARNING_OBJECT (v4l2object,
        _("Failed to set value %d for control %d on device '%s'."),
        value, attribute_num, v4l2object->videodev);
    return FALSE;
  }
}

static gboolean
set_control (GQuark field_id, const GValue * value, gpointer user_data)
{
  GstV4l2Object *v4l2object = user_data;
  GQuark normalised_field_id;
  gpointer *d;

  /* 32 bytes is the maximum size for a control name according to v4l2 */
  gchar name[32];

  /* Backwards compatibility: in the past GStreamer would normalise strings in
     a subtly different way to v4l2-ctl.  e.g. the kernel's "Focus (absolute)"
     would become "focus__absolute_" whereas now it becomes "focus_absolute".
     Please remove the following in GStreamer 1.5 for 1.6 */
  strncpy (name, g_quark_to_string (field_id), sizeof (name));
  name[31] = '\0';
  gst_v4l2_normalise_control_name (name);
  normalised_field_id = g_quark_from_string (name);
  if (normalised_field_id != field_id)
    g_warning ("In GStreamer 1.4 the way V4L2 control names were normalised "
        "changed.  Instead of setting \"%s\" please use \"%s\".  The former is "
        "deprecated and will be removed in a future version of GStreamer",
        g_quark_to_string (field_id), name);
  field_id = normalised_field_id;

  d = g_datalist_id_get_data (&v4l2object->controls, field_id);
  if (!d) {
    GST_WARNING_OBJECT (v4l2object,
        "Control '%s' does not exist or has an unsupported type.",
        g_quark_to_string (field_id));
    return TRUE;
  }
  if (!G_VALUE_HOLDS (value, G_TYPE_INT)) {
    GST_WARNING_OBJECT (v4l2object,
        "'int' value expected for control '%s'.", g_quark_to_string (field_id));
    return TRUE;
  }
  gst_v4l2_set_attribute (v4l2object, GPOINTER_TO_INT (d),
      g_value_get_int (value));
  return TRUE;
}

gboolean
gst_v4l2_set_controls (GstV4l2Object * v4l2object, GstStructure * controls)
{
  return gst_structure_foreach (controls, set_control, v4l2object);
}

gboolean
gst_v4l2_get_input (GstV4l2Object * v4l2object, gint * input)
{
  gint n;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "trying to get input");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_INPUT, &n) < 0)
    goto input_failed;

  *input = n;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "input: %d", n);

  return TRUE;

  /* ERRORS */
input_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to get current input on device '%s'. May be it is a radio device"), v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}

gboolean
gst_v4l2_set_input (GstV4l2Object * v4l2object, gint input)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "trying to set input to %d", input);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_S_INPUT, &input) < 0)
    goto input_failed;

  return TRUE;

  /* ERRORS */
input_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to set input %d on device %s."),
            input, v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}

gboolean
gst_v4l2_get_output (GstV4l2Object * v4l2object, gint * output)
{
  gint n;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "trying to get output");

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_G_OUTPUT, &n) < 0)
    goto output_failed;

  *output = n;

  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "output: %d", n);

  return TRUE;

  /* ERRORS */
output_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to get current output on device '%s'. May be it is a radio device"), v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}

gboolean
gst_v4l2_set_output (GstV4l2Object * v4l2object, gint output)
{
  GST_DEBUG_OBJECT (v4l2object->dbg_obj, "trying to set output to %d", output);

  if (!GST_V4L2_IS_OPEN (v4l2object))
    return FALSE;

  if (v4l2object->ioctl (v4l2object->video_fd, VIDIOC_S_OUTPUT, &output) < 0)
    goto output_failed;

  return TRUE;

  /* ERRORS */
output_failed:
  if (v4l2object->device_caps & V4L2_CAP_TUNER) {
    /* only give a warning message if driver actually claims to have tuner
     * support
     */
    GST_ELEMENT_WARNING (v4l2object->element, RESOURCE, SETTINGS,
        (_("Failed to set output %d on device %s."),
            output, v4l2object->videodev), GST_ERROR_SYSTEM);
  }
  return FALSE;
}
