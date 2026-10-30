#include "QGCQVideoSinkController.h"

#include <QtCore/QThread>
#include <QtGui/QWindow>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCQVideoSinkControllerLog, "Video.GStreamer.QGCQVideoSinkController")

QGCQVideoSinkController::QGCQVideoSinkController(GstElement* element, QObject* parent)
    : QObject(parent)
    , _element(element ? GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(element))) : nullptr)
{
}

QGCQVideoSinkController::~QGCQVideoSinkController()
{
    _releaseElementBinding();
}

QList<QGCQVideoSinkController*> QGCQVideoSinkController::controllersOf(const QObject* receiver)
{
    return receiver ? receiver->findChildren<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly)
                    : QList<QGCQVideoSinkController*>{};
}

void QGCQVideoSinkController::followWindowVisibility(QQuickItem* videoOutput)
{
    (void) QObject::disconnect(_visibilityConnection);
    _videoOutput = videoOutput;
    // QQuickVideoOutput never emits windowChanged (its itemChange() skips the base), so the window is taken now; an
    // output that moves to another window is followed again from there.
    if (QQuickWindow* window = videoOutput ? videoOutput->window() : nullptr) {
        _visibilityConnection = QObject::connect(window, &QWindow::visibilityChanged, this,
                                                 &QGCQVideoSinkController::_applyWindowVisibility);
    }
    _applyWindowVisibility();
}

void QGCQVideoSinkController::_applyWindowVisibility()
{
    const QWindow* const window = _videoOutput ? _videoOutput->window() : nullptr;
    const QWindow::Visibility visibility = window ? window->visibility() : QWindow::Hidden;
    setActive((visibility != QWindow::Hidden) && (visibility != QWindow::Minimized));
}

void QGCQVideoSinkController::setActive(bool active)
{
    if (thread() != QThread::currentThread()) {
        qCCritical(QGCQVideoSinkControllerLog) << "called from wrong thread";
        return;
    }
    if (!_element || _bindingReleased)
        return;
    g_object_set(_element.get(), "active", active ? TRUE : FALSE, nullptr);
}

void QGCQVideoSinkController::setVideoSink(QPointer<QVideoSink> sink)
{
    if (thread() != QThread::currentThread()) {
        qCCritical(QGCQVideoSinkControllerLog) << "called from wrong thread";
        return;
    }
    if (!_element)
        return;
    if (_sinkDestroyedConnection) {
        QObject::disconnect(_sinkDestroyedConnection);
        _sinkDestroyedConnection = {};
    }
    QVideoSink* raw = sink.data();
    if (!raw) {
        // Caller's QVideoSink was destroyed between the call site and here — clear the
        // element's snapshot and gate show_frame until a replacement sink is installed.
        g_object_set(_element.get(), "active", FALSE, "qvideosink", static_cast<gpointer>(nullptr), nullptr);
        _bindingReleased = true;
        _clearVideoSinkFrame();
        return;
    }
    g_object_set(_element.get(), "qvideosink", static_cast<gpointer>(raw), nullptr);
    if (_videoSink != raw) {
        _clearVideoSinkFrame();
    }
    _videoSink = raw;
    _sinkDestroyedConnection = QObject::connect(raw, &QObject::destroyed, this, [this]() {
        setVideoSink(QPointer<QVideoSink>());
    });
    _bindingReleased = false;
}

void QGCQVideoSinkController::prepareForRelease()
{
    if (thread() != QThread::currentThread()) {
        qCCritical(QGCQVideoSinkControllerLog) << "called from wrong thread";
        return;
    }
    _releaseElementBinding();
}

void QGCQVideoSinkController::releaseFrame()
{
    if (thread() != QThread::currentThread()) {
        qCCritical(QGCQVideoSinkControllerLog) << "called from wrong thread";
        return;
    }
    if (_videoSink) {
        _videoSink->setVideoFrame(QVideoFrame());
    }
}

void QGCQVideoSinkController::_releaseElementBinding() noexcept
{
    if (!_element || _bindingReleased)
        return;

    if (_sinkDestroyedConnection) {
        QObject::disconnect(_sinkDestroyedConnection);
        _sinkDestroyedConnection = {};
    }
    g_object_set(_element.get(), "active", FALSE, "qvideosink", static_cast<gpointer>(nullptr), nullptr);
    _bindingReleased = true;
    _clearVideoSinkFrame();
}

void QGCQVideoSinkController::_clearVideoSinkFrame()
{
    releaseFrame();
    _videoSink.clear();
}
