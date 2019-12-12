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

#ifndef __GST_DIRECTSOUND_DEVICE_H__
#define __GST_DIRECTSOUND_DEVICE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _GstDirectSoundDeviceProvider GstDirectSoundDeviceProvider;
typedef struct _GstDirectSoundDeviceProviderClass GstDirectSoundDeviceProviderClass;

#define GST_TYPE_DIRECTSOUND_DEVICE_PROVIDER                 (gst_directsound_device_provider_get_type())
#define GST_IS_DIRECTSOUND_DEVICE_PROVIDER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_DIRECTSOUND_DEVICE_PROVIDER))
#define GST_IS_DIRECTSOUND_DEVICE_PROVIDER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_DIRECTSOUND_DEVICE_PROVIDER))
#define GST_DIRECTSOUND_DEVICE_PROVIDER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_DIRECTSOUND_DEVICE_PROVIDER, GstDirectSoundDeviceProviderClass))
#define GST_DIRECTSOUND_DEVICE_PROVIDER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_DIRECTSOUND_DEVICE_PROVIDER, GstDirectSoundDeviceProvider))
#define GST_DIRECTSOUND_DEVICE_PROVIDER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_DEVICE_PROVIDER, GstDirectSoundDeviceProviderClass))
#define GST_DIRECTSOUND_DEVICE_PROVIDER_CAST(obj)            ((GstDirectSoundDeviceProvider *)(obj))

struct _GstDirectSoundDeviceProvider {
  GstDeviceProvider parent;
};

struct _GstDirectSoundDeviceProviderClass {
  GstDeviceProviderClass parent_class;
};

GType gst_directsound_device_provider_get_type (void);


typedef struct _GstDirectSoundDevice GstDirectSoundDevice;
typedef struct _GstDirectSoundDeviceClass GstDirectSoundDeviceClass;

#define GST_TYPE_DIRECTSOUND_DEVICE                 (gst_directsound_device_get_type())
#define GST_IS_DIRECTSOUND_DEVICE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_DIRECTSOUND_DEVICE))
#define GST_IS_DIRECTSOUND_DEVICE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_DIRECTSOUND_DEVICE))
#define GST_DIRECTSOUND_DEVICE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_DIRECTSOUND_DEVICE, GstDirectSoundDeviceClass))
#define GST_DIRECTSOUND_DEVICE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_DIRECTSOUND_DEVICE, GstDirectSoundDevice))
#define GST_DIRECTSOUND_DEVICE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_DEVICE, GstDirectSoundDeviceClass))
#define GST_DIRECTSOUND_DEVICE_CAST(obj)            ((GstDirectSoundDevice *)(obj))

struct _GstDirectSoundDevice {
  GstDevice parent;

  gchar *guid;
};

struct _GstDirectSoundDeviceClass {
  GstDeviceClass parent_class;
};

GType gst_directsound_device_get_type (void);

G_END_DECLS

#endif /* __GST_DIRECTSOUND_DEVICE_H__ */
