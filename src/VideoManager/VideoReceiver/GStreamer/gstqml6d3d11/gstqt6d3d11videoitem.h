/* GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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
#include <QQuickItem>
#include <QQuickWindow>
#include <mutex>
#include "gstqsg6d3d11node.h"

class GstQt6D3D11VideoItem;

class GstQt6D3D11VideoItemProxy : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  GstQt6D3D11VideoItemProxy (GstQt6D3D11VideoItem * videoItem)
    : item (videoItem)
  {
  }

  void InvalidateRef (void);

  void SetSink (GstElement * sink);
  void SetCaps (GstCaps * caps);
  void SetBuffer (GstBuffer * buffer);
  void SetForceAspectRatio (bool force);

  gint64 AdapterLuid (void);
  GstQt6D3D11VideoItem * Item (void);

private:
  GstQt6D3D11VideoItem *item;
  std::mutex lock;
};

class GstQt6D3D11VideoItem : public QQuickItem
{
  friend class GstQt6D3D11VideoItemProxy;

  Q_OBJECT
  QML_ELEMENT

public:
  GstQt6D3D11VideoItem ();
  ~GstQt6D3D11VideoItem ();

  QSharedPointer<GstQt6D3D11VideoItemProxy> Proxy (void);

protected:
  QSGNode * updatePaintNode (QSGNode * oldNode, UpdatePaintNodeData * data);
  void hoverEnterEvent (QHoverEvent * event);
  void hoverLeaveEvent (QHoverEvent * event);
  void hoverMoveEvent (QHoverEvent * event);
  void touchEvent (QTouchEvent * event);
  void mousePressEvent (QMouseEvent * event);
  void mouseReleaseEvent (QMouseEvent * event);
  void wheelEvent (QWheelEvent * event);

private:
  void setSink (GstElement * sink);
  void setCaps (GstCaps * caps);
  void setBuffer (GstBuffer * buffer);
  void setForceAspectRatio (bool force);
  gint64 AdapterLuid ();
  void fitStreamToAllocatedSize (GstVideoRectangle * result);
  QPointF mapPointToStreamSize (QPointF point);
  void sendMouseEvent (QMouseEvent * event, gboolean is_press);
  void clearResource ();
  void clearFrameResource ();
  bool setupShader ();

private slots:
  void handleWindowChanged (QQuickWindow * window);
  void onBeforeSynchronizing (void);
  void onSceneGraphInvalidated (void);
  void onBeforeRendering (void);

private:
  QSharedPointer<GstQt6D3D11VideoItemProxy> proxy_;
  QQuickWindow *window_ = nullptr;
  GstCaps *caps_ = nullptr;
  GstBuffer *buffer_ = nullptr;
  bool force_aspect_ratio_ = true;
  gint64 luid_ = 0;
  GstVideoInfo info_;
  int display_width_ = 0;
  int display_height_ = 0;
  GWeakRef sink_;
  std::mutex lock_;
  bool mouse_hovering_ = false;
  bool update_par_ = false;
  GstD3D11Device *qt_device_ = nullptr;
  GstQSG6D3D11Node *node_ = nullptr;
};

void gst_qt6_d3d11_video_item_init_once (void);
