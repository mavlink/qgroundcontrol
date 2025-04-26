/* GStreamer
 * Copyright (C) 2022 Matthew Waters <matthew@centricular.com>
 * Copyright (C) 2023 Seungha Yang <seungha@centricular.com>
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

#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/d3d11/gstd3d11.h>
#include <QtQuick/QQuickItem>
#include <QtQuick/QSGTexture>
#include <QtQuick/QSGTextureProvider>
#include <QtQuick/QSGSimpleTextureNode>

class GstQSG6D3D11Node : public QSGTextureProvider, public QSGSimpleTextureNode
{
  Q_OBJECT

public:
  GstQSG6D3D11Node (QQuickItem * item, GstD3D11Device * device);
  ~GstQSG6D3D11Node ();

  QSGTexture *texture() const override;

  void SetCaps (GstCaps * caps);
  void SetBuffer (GstBuffer * buffer);

private:
  void resize (guint width, guint height);
  void mapTexture ();
  void unmapTexture ();

private:
  QQuickWindow *window_;
  GstD3D11Device *qt_device_;
  GstD3D11Device *device_ = nullptr;
  GstBuffer *buffer_ = nullptr;
  GstCaps *caps_ = nullptr;
  GstBuffer *sharable_ = nullptr;
  GstBuffer *backbuffer_ = nullptr;
  GstVideoFrame frame_;
  GstVideoInfo info_;
  GstVideoInfo render_info_;
  GstBufferPool *pool_ = nullptr;
  GstD3D11Converter *conv_ = nullptr;
};
