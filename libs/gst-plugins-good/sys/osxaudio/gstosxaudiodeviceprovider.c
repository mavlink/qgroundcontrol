/* GStreamer
 * Copyright (C) 2016 Hyunjun Ko <zzoon@igalia.com>
 *
 * gstosxaudiodeviceprovider.c: OSX audio device probing and monitoring
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

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include "gstosxaudiosrc.h"
#include "gstosxaudiosink.h"
#include "gstosxaudiodeviceprovider.h"

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_OSX_AUDIO_SRC_CAPS)
    );

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_OSX_AUDIO_SINK_CAPS)
    );

static GstOsxAudioDevice *gst_osx_audio_device_new (AudioDeviceID device_id,
    const gchar * device_name, GstOsxAudioDeviceType type,
    GstCoreAudio * core_audio);

G_DEFINE_TYPE (GstOsxAudioDeviceProvider, gst_osx_audio_device_provider,
    GST_TYPE_DEVICE_PROVIDER);

static GList *gst_osx_audio_device_provider_probe (GstDeviceProvider *
    provider);

static void
gst_osx_audio_device_provider_class_init (GstOsxAudioDeviceProviderClass *
    klass)
{
  GstDeviceProviderClass *dm_class = GST_DEVICE_PROVIDER_CLASS (klass);

  dm_class->probe = gst_osx_audio_device_provider_probe;

  gst_device_provider_class_set_static_metadata (dm_class,
      "OSX Audio Device Provider", "Source/Sink/Audio",
      "List and monitor OSX audio source and sink devices",
      "Hyunjun Ko <zzoon@igalia.com>");
}

static void
gst_osx_audio_device_provider_init (GstOsxAudioDeviceProvider * provider)
{
}

static GstOsxAudioDevice *
gst_osx_audio_device_provider_probe_device (GstOsxAudioDeviceProvider *
    provider, AudioDeviceID device_id, const gchar * device_name,
    GstOsxAudioDeviceType type)
{
  GstOsxAudioDevice *device = NULL;
  GstCoreAudio *core_audio;

  core_audio = gst_core_audio_new (NULL);
  core_audio->is_src = type == GST_OSX_AUDIO_DEVICE_TYPE_SOURCE ? TRUE : FALSE;
  core_audio->device_id = device_id;

  if (!gst_core_audio_open (core_audio)) {
    GST_ERROR ("CoreAudio device could not be opened");
    goto done;
  }

  device = gst_osx_audio_device_new (device_id, device_name, type, core_audio);

  gst_core_audio_close (core_audio);

done:
  g_object_unref (core_audio);

  return device;
}

static inline gchar *
_audio_device_get_name (AudioDeviceID device_id, gboolean output)
{
  OSStatus status = noErr;
  UInt32 propertySize = 0;
  gchar *device_name = NULL;
  AudioObjectPropertyScope prop_scope;

  AudioObjectPropertyAddress deviceNameAddress = {
    kAudioDevicePropertyDeviceName,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster
  };

  prop_scope = output ? kAudioDevicePropertyScopeOutput :
      kAudioDevicePropertyScopeInput;

  deviceNameAddress.mScope = prop_scope;

  /* Get the length of the device name */
  status = AudioObjectGetPropertyDataSize (device_id,
      &deviceNameAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    goto beach;
  }

  /* Get the name of the device */
  device_name = (gchar *) g_malloc (propertySize);
  status = AudioObjectGetPropertyData (device_id,
      &deviceNameAddress, 0, NULL, &propertySize, device_name);
  if (status != noErr) {
    g_free (device_name);
    device_name = NULL;
  }

beach:
  return device_name;
}

static inline gboolean
_audio_device_has_output (AudioDeviceID device_id)
{
  OSStatus status = noErr;
  UInt32 propertySize;

  AudioObjectPropertyAddress streamsAddress = {
    kAudioDevicePropertyStreams,
    kAudioDevicePropertyScopeOutput,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyDataSize (device_id,
      &streamsAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    return FALSE;
  }
  if (propertySize == 0) {
    return FALSE;
  }

  return TRUE;
}

static inline AudioDeviceID *
_audio_system_get_devices (gint * ndevices)
{
  OSStatus status = noErr;
  UInt32 propertySize = 0;
  AudioDeviceID *devices = NULL;

  AudioObjectPropertyAddress audioDevicesAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  status = AudioObjectGetPropertyDataSize (kAudioObjectSystemObject,
      &audioDevicesAddress, 0, NULL, &propertySize);
  if (status != noErr) {
    GST_WARNING ("failed getting number of devices: %d", (int) status);
    return NULL;
  }

  *ndevices = propertySize / sizeof (AudioDeviceID);

  devices = (AudioDeviceID *) g_malloc (propertySize);
  if (devices) {
    status = AudioObjectGetPropertyData (kAudioObjectSystemObject,
        &audioDevicesAddress, 0, NULL, &propertySize, devices);
    if (status != noErr) {
      GST_WARNING ("failed getting the list of devices: %d", (int) status);
      g_free (devices);
      *ndevices = 0;
      return NULL;
    }
  }

  return devices;
}

static GList *
gst_osx_audio_device_provider_probe (GstDeviceProvider * provider)
{
  GstOsxAudioDeviceProvider *self = GST_OSX_AUDIO_DEVICE_PROVIDER (provider);
  GList *devices = NULL;
  GstOsxAudioDevice *device = NULL;
  AudioDeviceID *osx_devices = NULL;
  gint i, ndevices = 0;

  osx_devices = _audio_system_get_devices (&ndevices);

  if (ndevices < 1) {
    GST_WARNING ("no audio output devices found");
    goto done;
  }

  GST_INFO ("found %d audio device(s)", ndevices);

  for (i = 0; i < ndevices; i++) {
    gchar *device_name;
    GstOsxAudioDeviceType type = GST_OSX_AUDIO_DEVICE_TYPE_INVALID;

    if ((device_name = _audio_device_get_name (osx_devices[i], FALSE))) {
      if (!_audio_device_has_output (osx_devices[i])) {
        GST_DEBUG ("Input Device ID: %u Name: %s",
            (unsigned) osx_devices[i], device_name);
        type = GST_OSX_AUDIO_DEVICE_TYPE_SOURCE;

      } else {
        GST_DEBUG ("Output Device ID: %u Name: %s",
            (unsigned) osx_devices[i], device_name);
        type = GST_OSX_AUDIO_DEVICE_TYPE_SINK;
      }

      device =
          gst_osx_audio_device_provider_probe_device (self, osx_devices[i],
          device_name, type);
      if (device) {
        gst_object_ref_sink (device);
        devices = g_list_prepend (devices, device);
      }

      g_free (device_name);
    }
  }

done:
  g_free (osx_devices);

  return devices;
}

enum
{
  PROP_DEVICE_ID = 1,
};

G_DEFINE_TYPE (GstOsxAudioDevice, gst_osx_audio_device, GST_TYPE_DEVICE);

static void gst_osx_audio_device_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_osx_audio_device_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static GstElement *gst_osx_audio_device_create_element (GstDevice * device,
    const gchar * name);

static void
gst_osx_audio_device_class_init (GstOsxAudioDeviceClass * klass)
{
  GstDeviceClass *dev_class = GST_DEVICE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  dev_class->create_element = gst_osx_audio_device_create_element;

  object_class->get_property = gst_osx_audio_device_get_property;
  object_class->set_property = gst_osx_audio_device_set_property;

  g_object_class_install_property (object_class, PROP_DEVICE_ID,
      g_param_spec_int ("device-id", "Device ID", "Device ID of input device",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_osx_audio_device_init (GstOsxAudioDevice * device)
{
}

static GstElement *
gst_osx_audio_device_create_element (GstDevice * device, const gchar * name)
{
  GstOsxAudioDevice *osxdev = GST_OSX_AUDIO_DEVICE (device);
  GstElement *elem;

  elem = gst_element_factory_make (osxdev->element, name);
  g_object_set (elem, "device", osxdev->device_id, NULL);

  return elem;
}

static GstOsxAudioDevice *
gst_osx_audio_device_new (AudioDeviceID device_id, const gchar * device_name,
    GstOsxAudioDeviceType type, GstCoreAudio * core_audio)
{
  GstOsxAudioDevice *gstdev;
  const gchar *element_name = NULL;
  const gchar *klass = NULL;
  GstCaps *template_caps, *caps;

  g_return_val_if_fail (device_id > 0, NULL);
  g_return_val_if_fail (device_name, NULL);

  switch (type) {
    case GST_OSX_AUDIO_DEVICE_TYPE_SOURCE:
      element_name = "osxaudiosrc";
      klass = "Audio/Source";

      template_caps = gst_static_pad_template_get_caps (&src_factory);
      caps = gst_core_audio_probe_caps (core_audio, template_caps);
      gst_caps_unref (template_caps);

      break;
    case GST_OSX_AUDIO_DEVICE_TYPE_SINK:
      element_name = "osxaudiosink";
      klass = "Audio/Sink";

      template_caps = gst_static_pad_template_get_caps (&sink_factory);
      caps = gst_core_audio_probe_caps (core_audio, template_caps);
      gst_caps_unref (template_caps);

      break;
    default:
      g_assert_not_reached ();
      break;
  }

  gstdev = g_object_new (GST_TYPE_OSX_AUDIO_DEVICE, "device-id",
      device_id, "display-name", device_name, "caps", caps, "device-class",
      klass, NULL);

  gstdev->element = element_name;


  return gstdev;
}

static void
gst_osx_audio_device_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstOsxAudioDevice *device;

  device = GST_OSX_AUDIO_DEVICE_CAST (object);

  switch (prop_id) {
    case PROP_DEVICE_ID:
      g_value_set_int (value, device->device_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_osx_audio_device_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstOsxAudioDevice *device;

  device = GST_OSX_AUDIO_DEVICE_CAST (object);

  switch (prop_id) {
    case PROP_DEVICE_ID:
      device->device_id = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
