/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/*
 * GStreamer
 * Copyright (C) 2022 Matthew Waters <matthew@centricular.com>
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
#include <gst/gl/gl.h>

#include "gstqt6gl.h"
#include <QtQuick/QQuickItem>
#include <QtQuick/QSGTexture>
#include <QtQuick/QSGTextureProvider>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtGui/QOpenGLFunctions>

class GstQSG6OpenGLNode : public QSGTextureProvider, public QSGSimpleTextureNode, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  GstQSG6OpenGLNode(QQuickItem *item);
  ~GstQSG6OpenGLNode();

  QSGTexture *texture() const override;

  void setCaps(GstCaps *caps);
  void setBuffer(GstBuffer *buffer);
  GstBuffer *getBuffer();

  void updateQSGTexture();

private:
  QQuickWindow *window_;
  GstBuffer * buffer_;
  gboolean buffer_was_bound;
  GstBuffer * sync_buffer_;
  GstMemory * mem_;
  QSGTexture *dummy_tex_;
  GstVideoInfo v_info;
  GstVideoFrame v_frame;
};
