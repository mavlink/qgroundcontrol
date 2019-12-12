/* GStreamer
 * Copyright (C) 2016 Hyunjun Ko <zzoon@igalia.com>
 *
 * gstosxaudiodeviceeprovider.h: OSX audio probing and monitoring
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


#ifndef __GST_OSX_AUDIO_DEIVCE_PROVIDER_H__
#define __GST_OSX_AUDIO_DEIVCE_PROVIDER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstosxcoreaudiocommon.h"

G_BEGIN_DECLS
typedef struct _GstOsxAudioDeviceProvider GstOsxAudioDeviceProvider;
typedef struct _GstOsxAudioDeviceProviderClass GstOsxAudioDeviceProviderClass;

#define GST_TYPE_OSX_AUDIO_DEVICE_PROVIDER                 (gst_osx_audio_device_provider_get_type())
#define GST_IS_OSX_AUDIO_DEVICE_PROVIDER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_OSX_AUDIO_DEVICE_PROVIDER))
#define GST_IS_OSX_AUDIO_DEVICE_PROVIDER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_OSX_AUDIO_DEVICE_PROVIDER))
#define GST_OSX_AUDIO_DEVICE_PROVIDER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_OSX_AUDIO_DEVICE_PROVIDER, GstOsxAudioDeviceProviderClass))
#define GST_OSX_AUDIO_DEVICE_PROVIDER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_OSX_AUDIO_DEVICE_PROVIDER, GstOsxAudioDeviceProvider))
#define GST_OSX_AUDIO_DEVICE_PROVIDER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_DEVICE_PROVIDER, GstOsxAudioDeviceProviderClass))
#define GST_OSX_AUDIO_DEVICE_PROVIDER_CAST(obj)            ((GstOsxAudioDeviceProvider *)(obj))

struct _GstOsxAudioDeviceProvider
{
  GstDeviceProvider parent;
};

struct _GstOsxAudioDeviceProviderClass
{
  GstDeviceProviderClass parent_class;
};

GType gst_osx_audio_device_provider_get_type (void);

typedef struct _GstOsxAudioDevice GstOsxAudioDevice;
typedef struct _GstOsxAudioDeviceClass GstOsxAudioDeviceClass;

#define GST_TYPE_OSX_AUDIO_DEVICE                 (gst_osx_audio_device_get_type())
#define GST_IS_OSX_AUDIO_DEVICE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_OSX_AUDIO_DEVICE))
#define GST_IS_OSX_AUDIO_DEVICE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_OSX_AUDIO_DEVICE))
#define GST_OSX_AUDIO_DEVICE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_OSX_AUDIO_DEVICE, GstOsxAudioClass))
#define GST_OSX_AUDIO_DEVICE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_OSX_AUDIO_DEVICE, GstOsxAudioDevice))
#define GST_OSX_AUDIO_DEVICE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_DEVICE, GstOsxAudioDeviceClass))
#define GST_OSX_AUDIO_DEVICE_CAST(obj)            ((GstOsxAudioDevice *)(obj))

typedef enum
{
  GST_OSX_AUDIO_DEVICE_TYPE_INVALID = 0,
  GST_OSX_AUDIO_DEVICE_TYPE_SOURCE,
  GST_OSX_AUDIO_DEVICE_TYPE_SINK
} GstOsxAudioDeviceType;

struct _GstOsxAudioDevice
{
  GstDevice parent;

  const gchar *element;
  AudioDeviceID device_id;
};

struct _GstOsxAudioDeviceClass
{
  GstDeviceClass parent_class;
};

GType gst_osx_audio_device_get_type (void);

G_END_DECLS
#endif /* __GST_OSX_AUDIO_DEIVCE_PROVIDER_H__ */
