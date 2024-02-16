/*
 * GStreamer
 * Copyright (C) 2023 Matthew Waters <matthew@centricular.com>
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

#ifndef __GST_QSG6_MATERIAL_H__
#define __GST_QSG6_MATERIAL_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/gl/gl.h>

#include "gstqt6gl.h"
#include <QtQuick/QSGMaterial>
#include <QtQuick/QSGMaterialShader>
#include <QtGui/QOpenGLFunctions>
#include <QtQuick/QSGTexture>

class QRhi;
class QRhiResourceUpdateBatch;
class GstQSGMaterialShader;

class GstQSGMaterial : public QSGMaterial
{
protected:
    GstQSGMaterial();
    ~GstQSGMaterial();
public:
    static GstQSGMaterial *new_for_format (GstVideoFormat format);

    void setCaps (GstCaps * caps);
    gboolean setBuffer (GstBuffer * buffer);
    GstBuffer * getBuffer (bool * was_bound);
    bool compatibleWith(GstVideoInfo *v_info);

    void setFiltering(QSGTexture::Filtering);

    QSGTexture * bind(GstQSGMaterialShader *, QRhi *, QRhiResourceUpdateBatch *, guint binding, GstVideoFormat);

    /* QSGMaterial */
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;

    struct {
        int input_swizzle[4];
        QMatrix4x4 color_matrix;
        bool dirty;
    } uniforms;

private:
    GstBuffer * buffer_;
    bool buffer_was_bound;
    GWeakRef qt_context_ref_;
    GstBuffer * sync_buffer_;
    GstMemory * mem_;
    GstVideoInfo v_info;
    GstVideoFrame v_frame;
    QSGTexture::Filtering m_filtering;
};

#endif /* __GST_QSG6_MATERIAL_H__ */
