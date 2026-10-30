#pragma once

#include <gst/gst.h>

#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQuick/QQuickItem>

#include "GstScoped.h"

class QVideoSink;

/// GUI-thread companion for the GstQgcQVideoSink element: binds it to the QVideoSink it delivers to, and lets it
/// deliver only while that output's window is visible. Ownership: parent owns the controller; the controller owns one
/// GstElement ref so deferred QObject teardown can still clear the element binding.
/// Every member function runs on the controller's (GUI) thread.
class QGCQVideoSinkController : public QObject
{
    Q_OBJECT

public:
    /// @p element is the GstQgcQVideoSink to control. Controller takes a ref so QObject teardown
    /// can safely clear the element binding even if the parent bin is released first.
    QGCQVideoSinkController(GstElement* element, QObject* parent = nullptr);
    ~QGCQVideoSinkController() override;

    /// A receiver's owning controllers — direct children only, never a deep QObject-tree walk.
    static QList<QGCQVideoSinkController*> controllersOf(const QObject* receiver);

    // setActive(false) drops frames at the element; setVideoSink swaps the destination under
    // GST_OBJECT_LOCK. QPointer so a caller-thread destruction race is caught here, not in GObject.
    // Releasing a binding (swap, null, prepareForRelease, destruction) also clears the released QVideoSink's
    // current frame so it stops pinning a GstSample/decoder buffer.
    void setActive(bool active);
    void setVideoSink(QPointer<QVideoSink> sink);

    /// Delivers only while @p videoOutput's current window is visible (not hidden or minimized); call again when the
    /// item moves to another window. nullptr stops delivering.
    void followWindowVisibility(QQuickItem* videoOutput);

    /// Release the element binding synchronously ahead of deleteLater, so a deferred destruction can't touch the
    /// element while a replacement controller is installed on it. Idempotent.
    void prepareForRelease();

    /// Clear the bound QVideoSink's current frame, and the GstSample/decoder buffer it pins, but keep the binding for
    /// the next stream.
    void releaseFrame();

private:
    void _releaseElementBinding() noexcept;
    void _clearVideoSinkFrame();
    void _applyWindowVisibility();

    GStreamer::GstElementPtr _element;
    QPointer<QVideoSink> _videoSink;
    QMetaObject::Connection _sinkDestroyedConnection;
    bool _bindingReleased = false;

    QPointer<QQuickItem> _videoOutput;
    QMetaObject::Connection _visibilityConnection;
};
