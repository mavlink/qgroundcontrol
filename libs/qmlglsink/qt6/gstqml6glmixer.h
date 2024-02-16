/*
 * GStreamer
 * Copyright (C) 2020 Matthew Waters <matthew@centricular.com>
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

#ifndef __GST_QML6_GL_MIXER_H__
#define __GST_QML6_GL_MIXER_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/gl/gl.h>
#include "qt6glrenderer.h"
#include "qt6glitem.h"

G_BEGIN_DECLS

#define GST_TYPE_QML6_GL_MIXER_PAD (gst_qml6_gl_mixer_pad_get_type())
G_DECLARE_FINAL_TYPE(GstQml6GLMixerPad, gst_qml6_gl_mixer_pad, GST, QML6_GL_MIXER_PAD, GstGLMixerPad);

#define GST_TYPE_QML6_GL_MIXER (gst_qml6_gl_mixer_get_type())
G_DECLARE_FINAL_TYPE(GstQml6GLMixer, gst_qml6_gl_mixer, GST, QML6_GL_MIXER, GstGLMixer);

G_END_DECLS

#endif /* __GST_QML6_GL_MIXER_H__ */
