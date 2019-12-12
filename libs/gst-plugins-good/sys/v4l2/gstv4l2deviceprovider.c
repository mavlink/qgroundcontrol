/* GStreamer
 * Copyright (C) 2012 Olivier Crete <olivier.crete@collabora.com>
 *
 * gstv4l2deviceprovider.c: V4l2 device probing and monitoring
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstv4l2deviceprovider.h"

#include <string.h>
#include <sys/stat.h>

#include <gst/gst.h>

#include "gstv4l2object.h"
#include "v4l2-utils.h"

#ifdef HAVE_GUDEV
#include <gudev/gudev.h>
#endif

static GstV4l2Device *gst_v4l2_device_new (const gchar * device_path,
    const gchar * device_name, GstCaps * caps, GstV4l2DeviceType type,
    GstStructure * props);


G_DEFINE_TYPE (GstV4l2DeviceProvider, gst_v4l2_device_provider,
    GST_TYPE_DEVICE_PROVIDER);

static void gst_v4l2_device_provider_finalize (GObject * object);
static GList *gst_v4l2_device_provider_probe (GstDeviceProvider * provider);

#ifdef HAVE_GUDEV
static gboolean gst_v4l2_device_provider_start (GstDeviceProvider * provider);
static void gst_v4l2_device_provider_stop (GstDeviceProvider * provider);
#endif


static void
gst_v4l2_device_provider_class_init (GstV4l2DeviceProviderClass * klass)
{
  GstDeviceProviderClass *dm_class = GST_DEVICE_PROVIDER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  dm_class->probe = gst_v4l2_device_provider_probe;

#ifdef HAVE_GUDEV
  dm_class->start = gst_v4l2_device_provider_start;
  dm_class->stop = gst_v4l2_device_provider_stop;
#endif

  gobject_class->finalize = gst_v4l2_device_provider_finalize;

  gst_device_provider_class_set_static_metadata (dm_class,
      "Video (video4linux2) Device Provider", "Source/Sink/Video",
      "List and monitor video4linux2 source and sink devices",
      "Olivier Crete <olivier.crete@collabora.com>");
}

static void
gst_v4l2_device_provider_init (GstV4l2DeviceProvider * provider)
{
#ifdef HAVE_GUDEV
  g_cond_init (&provider->started_cond);
#endif
}

static void
gst_v4l2_device_provider_finalize (GObject * object)
{
#ifdef HAVE_GUDEV
  GstV4l2DeviceProvider *provider = GST_V4L2_DEVICE_PROVIDER (object);

  g_cond_clear (&provider->started_cond);
#endif

  G_OBJECT_CLASS (gst_v4l2_device_provider_parent_class)->finalize (object);
}

static GstV4l2Device *
gst_v4l2_device_provider_probe_device (GstV4l2DeviceProvider * provider,
    const gchar * device_path, const gchar * device_name, GstStructure * props)
{
  GstV4l2Object *v4l2obj = NULL;
  GstCaps *caps;
  GstV4l2Device *device = NULL;
  struct stat st;
  GstV4l2DeviceType type = GST_V4L2_DEVICE_TYPE_INVALID;

  g_return_val_if_fail (props != NULL, NULL);

  if (stat (device_path, &st) == -1)
    goto destroy;

  if (!S_ISCHR (st.st_mode))
    goto destroy;

  v4l2obj = gst_v4l2_object_new (NULL, GST_OBJECT (provider),
      V4L2_BUF_TYPE_VIDEO_CAPTURE, device_path, NULL, NULL, NULL);

  if (!gst_v4l2_open (v4l2obj))
    goto destroy;

  gst_structure_set (props, "device.api", G_TYPE_STRING, "v4l2", NULL);
  gst_structure_set (props, "device.path", G_TYPE_STRING, device_path, NULL);

  gst_structure_set (props, "v4l2.device.driver", G_TYPE_STRING,
      v4l2obj->vcap.driver, NULL);
  gst_structure_set (props, "v4l2.device.card", G_TYPE_STRING,
      v4l2obj->vcap.card, NULL);
  gst_structure_set (props, "v4l2.device.bus_info", G_TYPE_STRING,
      v4l2obj->vcap.bus_info, NULL);
  gst_structure_set (props, "v4l2.device.version", G_TYPE_UINT,
      v4l2obj->vcap.version, NULL);
  gst_structure_set (props, "v4l2.device.capabilities", G_TYPE_UINT,
      v4l2obj->vcap.capabilities, NULL);
  gst_structure_set (props, "v4l2.device.device_caps", G_TYPE_UINT,
      v4l2obj->vcap.device_caps, NULL);

  if (v4l2obj->device_caps &
      (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
    /* We ignore touch sensing devices; those are't really video */
    if (v4l2obj->device_caps & V4L2_CAP_TOUCH)
      goto close;

    type = GST_V4L2_DEVICE_TYPE_SOURCE;
    v4l2obj->skip_try_fmt_probes = TRUE;
  }

  if (v4l2obj->device_caps &
      (V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_OUTPUT_MPLANE)) {
    /* We ignore M2M devices that are both capture and output for now
     * The provider is not for them */
    if (type != GST_V4L2_DEVICE_TYPE_INVALID)
      goto close;

    type = GST_V4L2_DEVICE_TYPE_SINK;

    /* We have opened as a capture as we didn't know, now that know,
     * let's fixed it */
    if (v4l2obj->device_caps & V4L2_CAP_VIDEO_OUTPUT_MPLANE)
      v4l2obj->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    else
      v4l2obj->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  }

  if (type == GST_V4L2_DEVICE_TYPE_INVALID)
    goto close;

  caps = gst_v4l2_object_get_caps (v4l2obj, NULL);

  if (caps == NULL)
    goto close;
  if (gst_caps_is_empty (caps)) {
    gst_caps_unref (caps);
    goto close;
  }

  device = gst_v4l2_device_new (device_path,
      device_name ? device_name : (gchar *) v4l2obj->vcap.card, caps, type,
      props);
  gst_caps_unref (caps);

close:

  gst_v4l2_close (v4l2obj);

destroy:

  if (v4l2obj)
    gst_v4l2_object_destroy (v4l2obj);

  if (props)
    gst_structure_free (props);

  return device;
}


static GList *
gst_v4l2_device_provider_probe (GstDeviceProvider * provider)
{
  GstV4l2DeviceProvider *self = GST_V4L2_DEVICE_PROVIDER (provider);
  GstV4l2Iterator *it;
  GList *devices = NULL;

  it = gst_v4l2_iterator_new ();

  while (gst_v4l2_iterator_next (it)) {
    GstStructure *props;
    GstV4l2Device *device;

    props = gst_structure_new ("v4l2-proplist", "device.path", G_TYPE_STRING,
        it->device_path, "udev-probed", G_TYPE_BOOLEAN, FALSE, NULL);
    device = gst_v4l2_device_provider_probe_device (self, it->device_path, NULL,
        props);

    if (device) {
      gst_object_ref_sink (device);
      devices = g_list_prepend (devices, device);
    }
  }

  gst_v4l2_iterator_free (it);

  return devices;
}

#ifdef HAVE_GUDEV

static GstDevice *
gst_v4l2_device_provider_device_from_udev (GstV4l2DeviceProvider * provider,
    GUdevDevice * udev_device)
{
  GstV4l2Device *gstdev;
  const gchar *device_path = g_udev_device_get_device_file (udev_device);
  const gchar *device_name, *str;
  GstStructure *props;

  props = gst_structure_new ("v4l2deviceprovider", "udev-probed",
      G_TYPE_BOOLEAN, TRUE, NULL);

  str = g_udev_device_get_property (udev_device, "ID_PATH");
  if (!(str && *str)) {
    str = g_udev_device_get_sysfs_path (udev_device);
  }
  if (str && *str)
    gst_structure_set (props, "device.bus_path", G_TYPE_STRING, str, NULL);

  if ((str = g_udev_device_get_sysfs_path (udev_device)) && *str)
    gst_structure_set (props, "sysfs.path", G_TYPE_STRING, str, NULL);

  if ((str = g_udev_device_get_property (udev_device, "ID_ID")) && *str)
    gst_structure_set (props, "udev.id", G_TYPE_STRING, str, NULL);

  if ((str = g_udev_device_get_property (udev_device, "ID_BUS")) && *str)
    gst_structure_set (props, "device.bus", G_TYPE_STRING, str, NULL);

  if ((str = g_udev_device_get_property (udev_device, "SUBSYSTEM")) && *str)
    gst_structure_set (props, "device.subsystem", G_TYPE_STRING, str, NULL);

  if ((str = g_udev_device_get_property (udev_device, "ID_VENDOR_ID")) && *str)
    gst_structure_set (props, "device.vendor.id", G_TYPE_STRING, str, NULL);

  str = g_udev_device_get_property (udev_device, "ID_VENDOR_FROM_DATABASE");
  if (!(str && *str)) {
    str = g_udev_device_get_property (udev_device, "ID_VENDOR_ENC");
    if (!(str && *str)) {
      str = g_udev_device_get_property (udev_device, "ID_VENDOR");
    }
  }
  if (str && *str)
    gst_structure_set (props, "device.vendor.name", G_TYPE_STRING, str, NULL);

  if ((str = g_udev_device_get_property (udev_device, "ID_MODEL_ID")) && *str)
    gst_structure_set (props, "device.product.id", G_TYPE_STRING, str, NULL);

  device_name = g_udev_device_get_property (udev_device, "ID_V4L_PRODUCT");
  if (!(device_name && *device_name)) {
    device_name =
        g_udev_device_get_property (udev_device, "ID_MODEL_FROM_DATABASE");
    if (!(device_name && *device_name)) {
      device_name = g_udev_device_get_property (udev_device, "ID_MODEL_ENC");
      if (!(device_name && *device_name)) {
        device_name = g_udev_device_get_property (udev_device, "ID_MODEL");
      }
    }
  }
  if (device_name && *device_name)
    gst_structure_set (props, "device.product.name", G_TYPE_STRING, device_name,
        NULL);

  if ((str = g_udev_device_get_property (udev_device, "ID_SERIAL")) && *str)
    gst_structure_set (props, "device.serial", G_TYPE_STRING, str, NULL);

  if ((str = g_udev_device_get_property (udev_device, "ID_V4L_CAPABILITIES"))
      && *str)
    gst_structure_set (props, "device.capabilities", G_TYPE_STRING, str, NULL);

  gstdev = gst_v4l2_device_provider_probe_device (provider, device_path,
      device_name, props);

  if (gstdev)
    gstdev->syspath = g_strdup (g_udev_device_get_sysfs_path (udev_device));

  return GST_DEVICE (gstdev);
}

static void
uevent_cb (GUdevClient * client, const gchar * action, GUdevDevice * device,
    GstV4l2DeviceProvider * self)
{
  GstDeviceProvider *provider = GST_DEVICE_PROVIDER (self);

  /* Not V4L2, ignoring */
  if (g_udev_device_get_property_as_int (device, "ID_V4L_VERSION") != 2)
    return;

  if (!strcmp (action, "add")) {
    GstDevice *gstdev = NULL;

    gstdev = gst_v4l2_device_provider_device_from_udev (self, device);

    if (gstdev)
      gst_device_provider_device_add (provider, gstdev);
  } else if (!strcmp (action, "remove")) {
    GstV4l2Device *gstdev = NULL;
    GList *item;

    GST_OBJECT_LOCK (self);
    for (item = provider->devices; item; item = item->next) {
      gstdev = item->data;

      if (!strcmp (gstdev->syspath, g_udev_device_get_sysfs_path (device))) {
        gst_object_ref (gstdev);
        break;
      }

      gstdev = NULL;
    }
    GST_OBJECT_UNLOCK (provider);

    if (gstdev) {
      gst_device_provider_device_remove (provider, GST_DEVICE (gstdev));
      g_object_unref (gstdev);
    }
  } else {
    GST_WARNING ("Unhandled action %s", action);
  }
}

static gpointer
provider_thread (gpointer data)
{
  GstV4l2DeviceProvider *provider = data;
  GMainContext *context = NULL;
  GMainLoop *loop = NULL;
  GUdevClient *client;
  GList *devices;
  static const gchar *subsystems[] = { "video4linux", NULL };

  GST_OBJECT_LOCK (provider);
  if (provider->context)
    context = g_main_context_ref (provider->context);
  if (provider->loop)
    loop = g_main_loop_ref (provider->loop);

  if (context == NULL || loop == NULL) {
    provider->started = TRUE;
    g_cond_broadcast (&provider->started_cond);
    GST_OBJECT_UNLOCK (provider);
    return NULL;
  }
  GST_OBJECT_UNLOCK (provider);

  g_main_context_push_thread_default (context);

  client = g_udev_client_new (subsystems);

  g_signal_connect (client, "uevent", G_CALLBACK (uevent_cb), provider);

  devices = g_udev_client_query_by_subsystem (client, "video4linux");

  while (devices) {
    GUdevDevice *udev_device = devices->data;
    GstDevice *gstdev;

    devices = g_list_remove (devices, udev_device);

    if (g_udev_device_get_property_as_int (udev_device, "ID_V4L_VERSION") == 2) {
      gstdev =
          gst_v4l2_device_provider_device_from_udev (provider, udev_device);
      if (gstdev)
        gst_device_provider_device_add (GST_DEVICE_PROVIDER (provider), gstdev);
    }

    g_object_unref (udev_device);
  }

  GST_OBJECT_LOCK (provider);
  provider->started = TRUE;
  g_cond_broadcast (&provider->started_cond);
  GST_OBJECT_UNLOCK (provider);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_object_unref (client);
  g_main_context_unref (context);

  gst_object_unref (provider);

  return NULL;
}

static gboolean
gst_v4l2_device_provider_start (GstDeviceProvider * provider)
{
  GstV4l2DeviceProvider *self = GST_V4L2_DEVICE_PROVIDER (provider);

  GST_OBJECT_LOCK (self);
  g_assert (self->context == NULL);

  self->context = g_main_context_new ();
  self->loop = g_main_loop_new (self->context, FALSE);

  self->thread = g_thread_new ("v4l2-device-provider", provider_thread,
      g_object_ref (self));

  while (self->started == FALSE)
    g_cond_wait (&self->started_cond, GST_OBJECT_GET_LOCK (self));

  GST_OBJECT_UNLOCK (self);

  return TRUE;
}

static void
gst_v4l2_device_provider_stop (GstDeviceProvider * provider)
{
  GstV4l2DeviceProvider *self = GST_V4L2_DEVICE_PROVIDER (provider);
  GMainContext *context;
  GMainLoop *loop;
  GSource *idle_stop_source;

  GST_OBJECT_LOCK (self);
  context = self->context;
  loop = self->loop;
  self->context = NULL;
  self->loop = NULL;
  GST_OBJECT_UNLOCK (self);

  if (!context || !loop)
    return;

  idle_stop_source = g_idle_source_new ();
  g_source_set_callback (idle_stop_source, (GSourceFunc) g_main_loop_quit, loop,
      (GDestroyNotify) g_main_loop_unref);
  g_source_attach (idle_stop_source, context);
  g_source_unref (idle_stop_source);
  g_main_context_unref (context);

  g_thread_join (self->thread);
  self->thread = NULL;
  self->started = FALSE;
}

#endif

enum
{
  PROP_DEVICE_PATH = 1,
};

G_DEFINE_TYPE (GstV4l2Device, gst_v4l2_device, GST_TYPE_DEVICE);

static void gst_v4l2_device_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_v4l2_device_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_v4l2_device_finalize (GObject * object);
static GstElement *gst_v4l2_device_create_element (GstDevice * device,
    const gchar * name);

static void
gst_v4l2_device_class_init (GstV4l2DeviceClass * klass)
{
  GstDeviceClass *dev_class = GST_DEVICE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  dev_class->create_element = gst_v4l2_device_create_element;

  object_class->get_property = gst_v4l2_device_get_property;
  object_class->set_property = gst_v4l2_device_set_property;
  object_class->finalize = gst_v4l2_device_finalize;

  g_object_class_install_property (object_class, PROP_DEVICE_PATH,
      g_param_spec_string ("device-path", "Device Path",
          "The Path of the device node", "",
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gst_v4l2_device_init (GstV4l2Device * device)
{
}

static void
gst_v4l2_device_finalize (GObject * object)
{
  GstV4l2Device *device = GST_V4L2_DEVICE (object);

  g_free (device->device_path);
  g_free (device->syspath);

  G_OBJECT_CLASS (gst_v4l2_device_parent_class)->finalize (object);
}

static GstElement *
gst_v4l2_device_create_element (GstDevice * device, const gchar * name)
{
  GstV4l2Device *v4l2_dev = GST_V4L2_DEVICE (device);
  GstElement *elem;

  elem = gst_element_factory_make (v4l2_dev->element, name);
  g_object_set (elem, "device", v4l2_dev->device_path, NULL);

  return elem;
}

static GstV4l2Device *
gst_v4l2_device_new (const gchar * device_path, const gchar * device_name,
    GstCaps * caps, GstV4l2DeviceType type, GstStructure * props)
{
  GstV4l2Device *gstdev;
  const gchar *element = NULL;
  const gchar *klass = NULL;

  g_return_val_if_fail (device_path, NULL);
  g_return_val_if_fail (device_name, NULL);
  g_return_val_if_fail (caps, NULL);

  switch (type) {
    case GST_V4L2_DEVICE_TYPE_SOURCE:
      element = "v4l2src";
      klass = "Video/Source";
      break;
    case GST_V4L2_DEVICE_TYPE_SINK:
      element = "v4l2sink";
      klass = "Video/Sink";
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  gstdev = g_object_new (GST_TYPE_V4L2_DEVICE, "device-path", device_path,
      "display-name", device_name, "caps", caps, "device-class", klass,
      "properties", props, NULL);

  gstdev->element = element;


  return gstdev;
}


static void
gst_v4l2_device_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstV4l2Device *device;

  device = GST_V4L2_DEVICE_CAST (object);

  switch (prop_id) {
    case PROP_DEVICE_PATH:
      g_value_set_string (value, device->device_path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_v4l2_device_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstV4l2Device *device;

  device = GST_V4L2_DEVICE_CAST (object);

  switch (prop_id) {
    case PROP_DEVICE_PATH:
      device->device_path = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
