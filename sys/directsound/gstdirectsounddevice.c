/* GStreamer
 * Copyright (C) 2018 Sebastian Dröge <sebastian@centricular.com>
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

#include "gstdirectsounddevice.h"

#include <windows.h>
#include <dsound.h>
#include <mmsystem.h>
#include <stdio.h>

#ifdef GST_DIRECTSOUND_SRC_DEVICE_PROVIDER
#include "gstdirectsoundsrc.h"
#else
#include "gstdirectsoundsink.h"
#endif

G_DEFINE_TYPE (GstDirectSoundDeviceProvider, gst_directsound_device_provider,
    GST_TYPE_DEVICE_PROVIDER);

static GList *gst_directsound_device_provider_probe (GstDeviceProvider *
    provider);

static void
gst_directsound_device_provider_class_init (GstDirectSoundDeviceProviderClass *
    klass)
{
  GstDeviceProviderClass *dm_class = GST_DEVICE_PROVIDER_CLASS (klass);

  gst_device_provider_class_set_static_metadata (dm_class,
#ifdef GST_DIRECTSOUND_SRC_DEVICE_PROVIDER
      "DirectSound Source Device Provider", "Source/Audio",
      "List DirectSound source devices",
#else
      "DirectSound Sink Device Provider", "Sink/Audio",
      "List DirectSound sink devices",
#endif
      "Sebastian Dröge <sebastian@centricular.com>");

  dm_class->probe = gst_directsound_device_provider_probe;
}

static void
gst_directsound_device_provider_init (GstDirectSoundDeviceProvider * provider)
{
}

static gchar *
guid_to_string (LPGUID guid)
{
  gunichar2 *wstr = NULL;
  gchar *str = NULL;

  if (StringFromCLSID (guid, &wstr) == S_OK) {
    str = g_utf16_to_utf8 (wstr, -1, NULL, NULL, NULL);
    CoTaskMemFree (wstr);
  }

  return str;
}

typedef struct
{
  GstDirectSoundDeviceProvider *self;
  GList **devices;
} ProbeData;

static BOOL CALLBACK
gst_directsound_enum_callback (GUID * pGUID, TCHAR * strDesc,
    TCHAR * strDrvName, VOID * pContext)
{
  ProbeData *probe_data = (ProbeData *) (pContext);
  gchar *driver, *description, *guid_str;
  GstStructure *props;
  GstDevice *device;
#ifdef GST_DIRECTSOUND_SRC_DEVICE_PROVIDER
  static GstStaticCaps caps = GST_STATIC_CAPS (GST_DIRECTSOUND_SRC_CAPS);
#else
  static GstStaticCaps caps = GST_STATIC_CAPS (GST_DIRECTSOUND_SINK_CAPS);
#endif

  description = g_locale_to_utf8 (strDesc, -1, NULL, NULL, NULL);
  if (!description) {
    GST_ERROR_OBJECT (probe_data->self,
        "Failed to convert description from locale encoding to UTF8");
    return TRUE;
  }

  driver = g_locale_to_utf8 (strDrvName, -1, NULL, NULL, NULL);
  if (!driver) {
    GST_ERROR_OBJECT (probe_data->self,
        "Failed to convert driver name from locale encoding to UTF8");
    return TRUE;
  }

  /* NULL for the primary sound card */
  guid_str = pGUID ? guid_to_string (pGUID) : NULL;

  GST_INFO_OBJECT (probe_data->self, "sound device name: %s, %s (GUID %s)",
      description, driver, GST_STR_NULL (guid_str));

  props = gst_structure_new ("directsound-proplist",
      "device.api", G_TYPE_STRING, "directsound",
      "device.guid", G_TYPE_STRING, GST_STR_NULL (guid_str),
      "directsound.device.driver", G_TYPE_STRING, driver,
      "directsound.device.description", G_TYPE_STRING, description, NULL);

#ifdef GST_DIRECTSOUND_SRC_DEVICE_PROVIDER
  device = g_object_new (GST_TYPE_DIRECTSOUND_DEVICE, "device-guid", guid_str,
      "display-name", description, "caps", gst_static_caps_get (&caps),
      "device-class", "Audio/Source", "properties", props, NULL);
#else
  device = g_object_new (GST_TYPE_DIRECTSOUND_DEVICE, "device-guid", guid_str,
      "display-name", description, "caps", gst_static_caps_get (&caps),
      "device-class", "Audio/Sink", "properties", props, NULL);
#endif

  *probe_data->devices = g_list_prepend (*probe_data->devices, device);

  g_free (description);
  g_free (driver);
  g_free (guid_str);

  gst_structure_free (props);

  return TRUE;
}

static GList *
gst_directsound_device_provider_probe (GstDeviceProvider * provider)
{
  GstDirectSoundDeviceProvider *self =
      GST_DIRECTSOUND_DEVICE_PROVIDER (provider);
  GList *devices = NULL;
  ProbeData probe_data = { self, &devices };
  HRESULT hRes;

#ifdef GST_DIRECTSOUND_SRC_DEVICE_PROVIDER
  hRes = DirectSoundCaptureEnumerate ((LPDSENUMCALLBACK)
      gst_directsound_enum_callback, (VOID *) & probe_data);
#else
  hRes = DirectSoundEnumerate ((LPDSENUMCALLBACK)
      gst_directsound_enum_callback, (VOID *) & probe_data);
#endif

  if (FAILED (hRes))
    GST_ERROR_OBJECT (self, "Failed to enumerate devices");

  return devices;
}

enum
{
  PROP_DEVICE_GUID = 1,
};

G_DEFINE_TYPE (GstDirectSoundDevice, gst_directsound_device, GST_TYPE_DEVICE);

static void gst_directsound_device_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_directsound_device_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_directsound_device_finalize (GObject * object);
static GstElement *gst_directsound_device_create_element (GstDevice * device,
    const gchar * name);

static void
gst_directsound_device_class_init (GstDirectSoundDeviceClass * klass)
{
  GstDeviceClass *dev_class = GST_DEVICE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  dev_class->create_element = gst_directsound_device_create_element;

  object_class->get_property = gst_directsound_device_get_property;
  object_class->set_property = gst_directsound_device_set_property;
  object_class->finalize = gst_directsound_device_finalize;

  g_object_class_install_property (object_class, PROP_DEVICE_GUID,
      g_param_spec_string ("device-guid", "Device GUID",
          "Device GUID", NULL,
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gst_directsound_device_init (GstDirectSoundDevice * device)
{
}

static void
gst_directsound_device_finalize (GObject * object)
{
  GstDirectSoundDevice *device = GST_DIRECTSOUND_DEVICE (object);

  g_free (device->guid);

  G_OBJECT_CLASS (gst_directsound_device_parent_class)->finalize (object);
}

static GstElement *
gst_directsound_device_create_element (GstDevice * device, const gchar * name)
{
  GstDirectSoundDevice *directsound_dev = GST_DIRECTSOUND_DEVICE (device);
  GstElement *elem;

#ifdef GST_DIRECTSOUND_SRC_DEVICE_PROVIDER
  elem = gst_element_factory_make ("directsoundsrc", name);
#else
  elem = gst_element_factory_make ("directsoundsink", name);
#endif

  g_object_set (elem, "device", directsound_dev->guid, NULL);

  return elem;
}

static void
gst_directsound_device_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDirectSoundDevice *device = GST_DIRECTSOUND_DEVICE_CAST (object);

  switch (prop_id) {
    case PROP_DEVICE_GUID:
      g_value_set_string (value, device->guid);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_directsound_device_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDirectSoundDevice *device = GST_DIRECTSOUND_DEVICE_CAST (object);

  switch (prop_id) {
    case PROP_DEVICE_GUID:
      device->guid = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
