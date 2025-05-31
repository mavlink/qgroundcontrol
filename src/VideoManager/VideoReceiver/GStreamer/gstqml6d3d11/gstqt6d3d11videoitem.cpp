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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/d3d11/gstd3d11.h>
#include <gst/d3d11/gstd3d11-private.h>
#include "gstqt6d3d11videoitem.h"
#include "gstqsg6d3d11node.h"

GST_DEBUG_CATEGORY_EXTERN (gst_qt6_d3d11_debug);
#define GST_CAT_DEFAULT gst_qt6_d3d11_debug

void
GstQt6D3D11VideoItemProxy::InvalidateRef (void)
{
  std::lock_guard < std::mutex > lk (lock);
  item = nullptr;
}

void
GstQt6D3D11VideoItemProxy::SetSink (GstElement * sink)
{
  std::lock_guard < std::mutex > lk (lock);
  if (!item)
    return;

  item->setSink (sink);
}

void
GstQt6D3D11VideoItemProxy::SetCaps (GstCaps * caps)
{
  std::lock_guard < std::mutex > lk (lock);
  if (!item)
    return;

  item->setCaps (caps);
}

void
GstQt6D3D11VideoItemProxy::SetBuffer (GstBuffer * buffer)
{
  std::lock_guard < std::mutex > lk (lock);
  if (!item)
    return;

  item->setBuffer (buffer);
}

void
GstQt6D3D11VideoItemProxy::SetForceAspectRatio (bool force)
{
  std::lock_guard < std::mutex > lk (lock);
  if (!item)
    return item->setForceAspectRatio (force);
}

gint64
GstQt6D3D11VideoItemProxy::AdapterLuid ()
{
  std::lock_guard < std::mutex > lk (lock);
  if (!item)
    return 0;

  return item->AdapterLuid ();
}

GstQt6D3D11VideoItem *
GstQt6D3D11VideoItemProxy::Item (void)
{
  std::lock_guard < std::mutex > lk (lock);

  return item;
}

GstQt6D3D11VideoItem::GstQt6D3D11VideoItem ()
{
  g_weak_ref_init (&sink_, nullptr);

  connect (this, &QQuickItem::windowChanged, this,
      &GstQt6D3D11VideoItem::handleWindowChanged);

  proxy_ = QSharedPointer < GstQt6D3D11VideoItemProxy >
      (new GstQt6D3D11VideoItemProxy (this));

  setFlag (ItemHasContents, true);
  setAcceptedMouseButtons (Qt::AllButtons);
  setAcceptHoverEvents (true);
  setAcceptTouchEvents (true);

  gst_video_info_init (&info_);

  GST_DEBUG ("%p New Qt6 Video Item", this);
}

GstQt6D3D11VideoItem::~GstQt6D3D11VideoItem ()
{
  GST_DEBUG ("%p Destroying Qt6 Video Item", this);

  proxy_->InvalidateRef ();
  proxy_.clear ();

  gst_clear_buffer (&buffer_);
  gst_clear_caps (&caps_);

  g_weak_ref_clear (&sink_);
}

QSharedPointer < GstQt6D3D11VideoItemProxy > GstQt6D3D11VideoItem::Proxy (void)
{
  return proxy_;
}

void
GstQt6D3D11VideoItem::setSink (GstElement * sink)
{
  std::lock_guard < std::mutex > lk (lock_);
  g_weak_ref_set (&sink_, sink);
}

void
GstQt6D3D11VideoItem::setCaps (GstCaps * caps)
{
  std::lock_guard < std::mutex > lk (lock_);
  guint num, den;

  gst_caps_replace (&caps_, caps);
  gst_video_info_from_caps (&info_, caps);

  gst_video_calculate_display_ratio (&num, &den, info_.width, info_.height,
      info_.par_n, info_.par_d, 1, 1);

  display_width_ = info_.width;
  display_height_ = info_.height;

  if (display_width_ % num == 0) {
    display_height_ = gst_util_uint64_scale_int (display_width_, den, num);
  } else {
    display_width_ = gst_util_uint64_scale_int (display_height_, num, den);
  }

  update_par_ = true;
}

void
GstQt6D3D11VideoItem::setBuffer (GstBuffer * buffer)
{
  std::lock_guard < std::mutex > lk (lock_);

  gst_buffer_replace (&buffer_, buffer);

  if (this->window_)
    QMetaObject::invokeMethod (this->window_, "update", Qt::QueuedConnection);
}

void
GstQt6D3D11VideoItem::setForceAspectRatio (bool force)
{
  std::lock_guard < std::mutex > lk (lock_);

  if (force != force_aspect_ratio_)
    update_par_ = true;

  force_aspect_ratio_ = force;
}

gint64
GstQt6D3D11VideoItem::AdapterLuid (void)
{
  std::lock_guard < std::mutex > lk (lock_);

  if (!qt_device_)
    return 0;

  return luid_;
}

void
GstQt6D3D11VideoItem::handleWindowChanged (QQuickWindow * window)
{
  if (window) {
    connect (window, &QQuickWindow::beforeSynchronizing, this,
        &GstQt6D3D11VideoItem::onBeforeSynchronizing, Qt::DirectConnection);
    connect (window, &QQuickWindow::sceneGraphInvalidated, this,
        &GstQt6D3D11VideoItem::onSceneGraphInvalidated, Qt::DirectConnection);
  } else {
    gst_clear_object (&qt_device_);
    window_ = nullptr;
  }
  node_ = nullptr;
}

QSGNode *
GstQt6D3D11VideoItem::updatePaintNode (QSGNode * old_node,
    UpdatePaintNodeData * data)
{
  if (!qt_device_)
    return old_node;

  GstQSG6D3D11Node *new_node = static_cast < GstQSG6D3D11Node * >(old_node);
  GstVideoRectangle src, dst, result;

  std::lock_guard < std::mutex > lk (lock_);

  GST_TRACE ("%p updatePaintNode", this);

  if (!new_node) {
    bool is_smooth = this->smooth ();
    new_node = new GstQSG6D3D11Node (this, qt_device_);
    new_node->setFiltering (is_smooth ? QSGTexture::Filtering::Linear :
        QSGTexture::Filtering::Nearest);
  }

  if (force_aspect_ratio_ && caps_) {
    src.w = display_width_;
    src.h = display_height_;

    dst.x = boundingRect ().x ();
    dst.y = boundingRect ().y ();
    dst.w = boundingRect ().width ();
    dst.h = boundingRect ().height ();

    gst_video_sink_center_rect (src, dst, &result, TRUE);
  } else {
    result.x = boundingRect ().x ();
    result.y = boundingRect ().y ();
    result.w = boundingRect ().width ();
    result.h = boundingRect ().height ();
  }

  new_node->setRect (QRectF (result.x, result.y, result.w, result.h));
  node_ = new_node;
  if (caps_ || !force_aspect_ratio_)
    update_par_ = false;
  else
    update_par_ = true;

  return new_node;
}

void
GstQt6D3D11VideoItem::onBeforeSynchronizing (void)
{
  QSGRendererInterface *iface;
  QQuickWindow *window;
  ID3D11Device *device;
  std::unique_lock < std::mutex > lk (lock_);

  if (window_)
    return;

  GST_DEBUG ("%p Scene graph initialized", this);

  window = this->window ();

  if (!window) {
    GST_WARNING ("Initialized scene graph has no associated window");
    return;
  }

  iface = window->rendererInterface ();
  if (!iface) {
    GST_WARNING ("Couldn't get renderer interface");
    return;
  }

  if (iface->graphicsApi () != QSGRendererInterface::Direct3D11) {
    GST_WARNING ("Not a d3d11 api");
    return;
  }

  device = reinterpret_cast < ID3D11Device * >(iface->getResource (window,
          QSGRendererInterface::DeviceResource));
  if (!device) {
    GST_WARNING ("Couldn't get d3d11 device");
    return;
  }

  qt_device_ = gst_d3d11_device_new_wrapped (device);
  g_object_get (qt_device_, "adapter-luid", &luid_, nullptr);
  window_ = window;

  lk.unlock ();

  connect (window, &QQuickWindow::beforeRendering, this,
      &GstQt6D3D11VideoItem::onBeforeRendering, Qt::DirectConnection);
}

void
GstQt6D3D11VideoItem::onSceneGraphInvalidated (void)
{
  std::unique_lock < std::mutex > lk (lock_);

  GST_DEBUG ("%p Scene graph invalidated", this);

  gst_clear_object (&qt_device_);
  window_ = nullptr;
  node_ = nullptr;
}

void
GstQt6D3D11VideoItem::onBeforeRendering (void)
{
  std::lock_guard < std::mutex > lk (lock_);

  if (update_par_ && caps_ && node_) {
    GstVideoRectangle src, dst, result;

    GST_DEBUG ("Updating node rect");

    if (force_aspect_ratio_ && caps_) {
      src.w = display_width_;
      src.h = display_height_;

      dst.x = boundingRect ().x ();
      dst.y = boundingRect ().y ();
      dst.w = boundingRect ().width ();
      dst.h = boundingRect ().height ();

      gst_video_sink_center_rect (src, dst, &result, TRUE);
    } else {
      result.x = boundingRect ().x ();
      result.y = boundingRect ().y ();
      result.w = boundingRect ().width ();
      result.h = boundingRect ().height ();
    }

    node_->setRect (QRectF (result.x, result.y, result.w, result.h));
    update_par_ = false;
  }

  if (node_) {
    node_->SetCaps (caps_);
    node_->SetBuffer (buffer_);
  }

  return;
}

void
GstQt6D3D11VideoItem::hoverEnterEvent (QHoverEvent * event)
{
  mouse_hovering_ = true;
}

void
GstQt6D3D11VideoItem::hoverLeaveEvent (QHoverEvent * event)
{
  mouse_hovering_ = false;
}

void
GstQt6D3D11VideoItem::fitStreamToAllocatedSize (GstVideoRectangle * result)
{
  if (force_aspect_ratio_) {
    GstVideoRectangle src, dst;

    src.x = 0;
    src.y = 0;
    src.w = display_width_;
    src.h = display_height_;

    dst.x = 0;
    dst.y = 0;
    dst.w = width ();
    dst.h = height ();

    gst_video_sink_center_rect (src, dst, result, TRUE);
  } else {
    result->x = 0;
    result->y = 0;
    result->w = width ();
    result->h = height ();
  }
}

QPointF
GstQt6D3D11VideoItem::mapPointToStreamSize (QPointF pos)
{
  gdouble stream_width, stream_height;
  GstVideoRectangle result;
  double stream_x, stream_y;
  double x, y;

  fitStreamToAllocatedSize (&result);

  stream_width = (gdouble) GST_VIDEO_INFO_WIDTH (&info_);
  stream_height = (gdouble) GST_VIDEO_INFO_HEIGHT (&info_);
  x = pos.x ();
  y = pos.y ();

  /* from display coordinates to stream coordinates */
  if (result.w > 0)
    stream_x = (x - result.x) / result.w * stream_width;
  else
    stream_x = 0.;

  /* clip to stream size */
  stream_x = CLAMP (stream_x, 0., stream_width);

  /* same for y-axis */
  if (result.h > 0)
    stream_y = (y - result.y) / result.h * stream_height;
  else
    stream_y = 0.;

  stream_y = CLAMP (stream_y, 0., stream_height);

  return QPointF (stream_x, stream_y);
}

static GstNavigationModifierType
translateModifiers (Qt::KeyboardModifiers modifiers)
{
  return (GstNavigationModifierType) (
      ((modifiers & Qt::KeyboardModifier::ShiftModifier) ?
          GST_NAVIGATION_MODIFIER_SHIFT_MASK : 0) | ((modifiers &
              Qt::KeyboardModifier::ControlModifier) ?
          GST_NAVIGATION_MODIFIER_CONTROL_MASK : 0) | ((modifiers &
              Qt::
              KeyboardModifier::AltModifier) ? GST_NAVIGATION_MODIFIER_MOD1_MASK
          : 0) | ((modifiers & Qt::
              KeyboardModifier::MetaModifier) ?
          GST_NAVIGATION_MODIFIER_META_MASK : 0));
}

static GstNavigationModifierType
translateMouseButtons (Qt::MouseButtons buttons)
{
  return (GstNavigationModifierType) (
      ((buttons & Qt::LeftButton) ? GST_NAVIGATION_MODIFIER_BUTTON1_MASK : 0) |
      ((buttons & Qt::RightButton) ? GST_NAVIGATION_MODIFIER_BUTTON2_MASK : 0) |
      ((buttons & Qt::MiddleButton) ? GST_NAVIGATION_MODIFIER_BUTTON3_MASK : 0)
      | ((buttons & Qt::BackButton) ? GST_NAVIGATION_MODIFIER_BUTTON4_MASK : 0)
      | ((buttons & Qt::ForwardButton) ? GST_NAVIGATION_MODIFIER_BUTTON5_MASK :
          0));
}

void
GstQt6D3D11VideoItem::hoverMoveEvent (QHoverEvent * event)
{
  if (!mouse_hovering_)
    return;

  if (event->position () == event->oldPos ())
    return;

  std::lock_guard < std::mutex > lk (lock_);
  if (GST_VIDEO_INFO_FORMAT (&info_) == GST_VIDEO_FORMAT_UNKNOWN)
    return;

  GstElement *sink = (GstElement *) g_weak_ref_get (&sink_);
  if (!sink)
    return;

  auto pos = mapPointToStreamSize (event->position ());
  gst_navigation_send_event_simple (GST_NAVIGATION (sink),
      gst_navigation_event_new_mouse_move (pos.x (), pos.y (),
          translateModifiers (event->modifiers ())));

  gst_object_unref (sink);
}

void
GstQt6D3D11VideoItem::touchEvent (QTouchEvent * event)
{
  std::lock_guard < std::mutex > lk (lock_);
  if (GST_VIDEO_INFO_FORMAT (&info_) == GST_VIDEO_FORMAT_UNKNOWN)
    return;

  GstElement *sink = (GstElement *) g_weak_ref_get (&sink_);
  if (!sink)
    return;

  if (event->type () == QEvent::TouchCancel) {
    gst_navigation_send_event_simple (GST_NAVIGATION (sink),
        gst_navigation_event_new_touch_cancel (translateModifiers
            (event->modifiers ())));
  } else {
    const QList < QTouchEvent::TouchPoint > points = event->points ();
    gboolean sent_event = FALSE;

    for (int i = 0; i < points.count (); i++) {
      GstEvent *nav_event;
      QPointF pos = mapPointToStreamSize (points[i].position ());

      switch (points[i].state ()) {
        case QEventPoint::Pressed:
          nav_event =
              gst_navigation_event_new_touch_down ((guint) points[i].id (),
              pos.x (), pos.y (), (gdouble) points[i].pressure (),
              translateModifiers (event->modifiers ()));
          break;
        case QEventPoint::Updated:
          nav_event =
              gst_navigation_event_new_touch_motion ((guint) points[i].id (),
              pos.x (), pos.y (), (gdouble) points[i].pressure (),
              translateModifiers (event->modifiers ()));
          break;
        case QEventPoint::Released:
          nav_event =
              gst_navigation_event_new_touch_up ((guint) points[i].id (),
              pos.x (), pos.y (), translateModifiers (event->modifiers ()));
          break;
          /* Don't send an event if the point did not change */
        default:
          nav_event = nullptr;
          break;
      }

      if (nav_event) {
        gst_navigation_send_event_simple (GST_NAVIGATION (sink), nav_event);
        sent_event = TRUE;
      }
    }

    /* Group simultaneous touch events with a frame event */
    if (sent_event) {
      gst_navigation_send_event_simple (GST_NAVIGATION (sink),
          gst_navigation_event_new_touch_frame (translateModifiers
              (event->modifiers ())));
    }
  }

  gst_object_unref (sink);
}

void
GstQt6D3D11VideoItem::sendMouseEvent (QMouseEvent * event, gboolean is_press)
{
  std::lock_guard < std::mutex > lk (lock_);
  if (GST_VIDEO_INFO_FORMAT (&info_) == GST_VIDEO_FORMAT_UNKNOWN)
    return;

  GstElement *sink = (GstElement *) g_weak_ref_get (&sink_);
  if (!sink)
    return;

  gint button = 0;
  switch (event->button ()) {
    case Qt::LeftButton:
      button = 1;
      break;
    case Qt::RightButton:
      button = 2;
      break;
    case Qt::MiddleButton:
      button = 3;
    default:
      break;
  }

  auto pos = mapPointToStreamSize (event->pos ());
  GstEvent *navi;
  GstNavigationModifierType type = (GstNavigationModifierType)
      (translateModifiers (event->modifiers ()) |
      translateMouseButtons (event->buttons ()));
  if (is_press) {
    navi = gst_navigation_event_new_mouse_button_press (button,
        pos.x (), pos.y (), type);
  } else {
    navi = gst_navigation_event_new_mouse_button_release (button,
        pos.x (), pos.y (), type);
  }

  gst_navigation_send_event_simple (GST_NAVIGATION (sink), navi);
  gst_object_unref (sink);
}

void
GstQt6D3D11VideoItem::mousePressEvent (QMouseEvent * event)
{
  sendMouseEvent (event, TRUE);
}

void
GstQt6D3D11VideoItem::mouseReleaseEvent (QMouseEvent * event)
{
  sendMouseEvent (event, FALSE);
}

void
GstQt6D3D11VideoItem::wheelEvent (QWheelEvent * event)
{
  std::lock_guard < std::mutex > lk (lock_);
  if (GST_VIDEO_INFO_FORMAT (&info_) == GST_VIDEO_FORMAT_UNKNOWN)
    return;

  GstElement *sink = (GstElement *) g_weak_ref_get (&sink_);
  if (!sink)
    return;

  auto pos = mapPointToStreamSize (event->position ());
  auto delta = event->angleDelta ();
  GstNavigationModifierType type = (GstNavigationModifierType)
      (translateModifiers (event->modifiers ()) |
      translateMouseButtons (event->buttons ()));

  gst_navigation_send_event_simple (GST_NAVIGATION (sink),
      gst_navigation_event_new_mouse_scroll (pos.x (), pos.y (),
          delta.x (), delta.y (), type));
  gst_object_unref (sink);
}

void
gst_qt6_d3d11_video_item_init_once (void)
{
  GST_D3D11_CALL_ONCE_BEGIN {
    qmlRegisterType < GstQt6D3D11VideoItem >
        ("org.freedesktop.gstreamer.Qt6D3D11VideoItem",
        1, 0, "GstD3D11Qt6VideoItem");
  } GST_D3D11_CALL_ONCE_END;
}
