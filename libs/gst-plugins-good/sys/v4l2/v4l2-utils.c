/*
 * Copyright (C) 2014 Collabora Ltd.
 *     Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
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
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "v4l2-utils.h"

/**************************/
/* Common device iterator */
/**************************/

#ifdef HAVE_GUDEV
#include <gudev/gudev.h>

struct _GstV4l2GUdevIterator
{
  GstV4l2Iterator parent;
  GList *devices;
  GUdevDevice *device;
  GUdevClient *client;
};

GstV4l2Iterator *
gst_v4l2_iterator_new (void)
{
  static const gchar *subsystems[] = { "video4linux", NULL };
  struct _GstV4l2GUdevIterator *it;

  it = g_slice_new0 (struct _GstV4l2GUdevIterator);

  it->client = g_udev_client_new (subsystems);
  it->devices = g_udev_client_query_by_subsystem (it->client, "video4linux");

  return (GstV4l2Iterator *) it;
}

gboolean
gst_v4l2_iterator_next (GstV4l2Iterator * _it)
{
  struct _GstV4l2GUdevIterator *it = (struct _GstV4l2GUdevIterator *) _it;
  const gchar *device_name;

  if (it->device)
    g_object_unref (it->device);

  it->device = NULL;
  it->parent.device_path = NULL;
  it->parent.device_name = NULL;

  if (it->devices == NULL)
    return FALSE;

  it->device = it->devices->data;
  it->devices = g_list_delete_link (it->devices, it->devices);

  device_name = g_udev_device_get_property (it->device, "ID_V4L_PRODUCT");
  if (!device_name)
    device_name = g_udev_device_get_property (it->device, "ID_MODEL_ENC");
  if (!device_name)
    device_name = g_udev_device_get_property (it->device, "ID_MODEL");

  it->parent.device_path = g_udev_device_get_device_file (it->device);
  it->parent.device_name = device_name;
  it->parent.sys_path = g_udev_device_get_sysfs_path (it->device);

  return TRUE;
}

void
gst_v4l2_iterator_free (GstV4l2Iterator * _it)
{
  struct _GstV4l2GUdevIterator *it = (struct _GstV4l2GUdevIterator *) _it;
  g_list_free_full (it->devices, g_object_unref);
  gst_object_unref (it->client);
  g_slice_free (struct _GstV4l2GUdevIterator, it);
}

#else /* No GUDEV */

struct _GstV4l2FsIterator
{
  GstV4l2Iterator parent;
  gint base_idx;
  gint video_idx;
  gchar *device;
};

GstV4l2Iterator *
gst_v4l2_iterator_new (void)
{
  struct _GstV4l2FsIterator *it;

  it = g_slice_new0 (struct _GstV4l2FsIterator);
  it->base_idx = 0;
  it->video_idx = -1;
  it->device = NULL;

  return (GstV4l2Iterator *) it;
}

gboolean
gst_v4l2_iterator_next (GstV4l2Iterator * _it)
{
  struct _GstV4l2FsIterator *it = (struct _GstV4l2FsIterator *) _it;
  static const gchar *dev_base[] = { "/dev/video", "/dev/v4l2/video", NULL };
  gchar *device = NULL;

  g_free ((gchar *) it->parent.device_path);
  it->parent.device_path = NULL;

  while (device == NULL) {
    it->video_idx++;

    if (it->video_idx >= 64) {
      it->video_idx = 0;
      it->base_idx++;
    }

    if (dev_base[it->base_idx] == NULL) {
      it->video_idx = 0;
      break;
    }

    device = g_strdup_printf ("%s%d", dev_base[it->base_idx], it->video_idx);

    if (g_file_test (device, G_FILE_TEST_EXISTS)) {
      it->parent.device_path = device;
      break;
    }

    g_free (device);
    device = NULL;
  }

  return it->parent.device_path != NULL;
}

void
gst_v4l2_iterator_free (GstV4l2Iterator * _it)
{
  struct _GstV4l2FsIterator *it = (struct _GstV4l2FsIterator *) _it;
  g_free ((gchar *) it->parent.device_path);
  g_slice_free (struct _GstV4l2FsIterator, it);
}

#endif

void
gst_v4l2_clear_error (GstV4l2Error * v4l2err)
{
  if (v4l2err) {
    g_clear_error (&v4l2err->error);
    g_free (v4l2err->dbg_message);
    v4l2err->dbg_message = NULL;
  }
}

void
gst_v4l2_error (gpointer element, GstV4l2Error * v4l2err)
{
  GError *error;

  if (!v4l2err || !v4l2err->error)
    return;

  error = v4l2err->error;

  if (error->message)
    GST_WARNING_OBJECT (element, "error: %s", error->message);

  if (v4l2err->dbg_message)
    GST_WARNING_OBJECT (element, "error: %s", v4l2err->dbg_message);

  gst_element_message_full (GST_ELEMENT (element), GST_MESSAGE_ERROR,
      error->domain, error->code, error->message, v4l2err->dbg_message,
      v4l2err->file, v4l2err->func, v4l2err->line);

  error->message = NULL;
  v4l2err->dbg_message = NULL;

  gst_v4l2_clear_error (v4l2err);
}
