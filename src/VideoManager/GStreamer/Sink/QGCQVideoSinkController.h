#pragma once

#include <gst/gst.h>
#include <memory>

#include <QtCore/QByteArray>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtQuick/QQuickItem>

#include "GstScoped.h"

class QVideoSink;

namespace GstQgc {
struct SinkBin;
}

/// GUI-thread companion for the GstQgcQVideoSink element: binds it to the QVideoSink it delivers to, and lets it
/// deliver only while that output's window is visible. Ownership: parent owns the controller; the controller owns one
/// GstElement ref so deferred QObject teardown can still clear the element binding.
/// Every member function runs on the controller's (GUI) thread.
class QGCQVideoSinkController : public QObject
{
    Q_OBJECT

public:
    /// @p element is the GstQgcQVideoSink to control.
    QGCQVideoSinkController(GstElement* element, QObject* parent = nullptr);
    ~QGCQVideoSinkController() override;

    /// setActive(false) drops frames at the element. Without a QVideoSink the element is inactive whatever was asked
    /// for; binding one again goes on with what setActive() asked for last (the window's visibility while following
    /// one), which is active to begin with.
    void setActive(bool active);

    /// Swaps the destination (GstQgc::setSinkTarget()); a QPointer so a caller-thread destruction race is caught here,
    /// not in GObject. Releasing a binding (swap, null, prepareForRelease(), destruction) also clears the released
    /// QVideoSink's current frame so it stops pinning a GstSample/decoder buffer.
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

    /// GstQgc::sinkPictureSize() of the element; invalid without one.
    QSize pictureSize() const;

private:
    void _releaseElementBinding();
    void _clearVideoSinkFrame();
    void _applyWindowVisibility();

    GStreamer::GstElementPtr _element;
    QPointer<QVideoSink> _videoSink;
    QMetaObject::Connection _sinkDestroyedConnection;
    bool _bindingReleased = false;
    /// What setActive() asked for last, kept while no QVideoSink is bound; the element's default.
    bool _active = true;

    QPointer<QQuickItem> _videoOutput;
    QMetaObject::Connection _visibilityConnection;
};

namespace GStreamer {

/// A new sink bin (GstQgc::makeSinkBin()) with the settings of a VideoReceiver::DisplayConfig, which are fixed once it
/// is built, or nullptr. Shared: the receiver and its worker's pipeline each hold it.
std::shared_ptr<const GstQgc::SinkBin> createVideoSink(const QByteArray& conversionElement,
                                                       bool disablePixelAspectRatio, bool gpuZeroCopy);

/// Binds @p qvideosink, the qgcqvideosink of a sink bin, to @p videoSink through a new QGCQVideoSinkController, a child
/// of @p controllerParent. Returns the controller, or nullptr on failure.
QGCQVideoSinkController* setupQVideoSinkElement(GstElement* qvideosink, QVideoSink* videoSink,
                                                QObject* controllerParent);

}  // namespace GStreamer
