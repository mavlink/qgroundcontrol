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
{}

QGCQVideoSinkController::~QGCQVideoSinkController()
{
    _releaseElementBinding();
}

void QGCQVideoSinkController::followWindowVisibility(QQuickItem* videoOutput)
{
    (void) QObject::disconnect(_visibilityConnection);
    _videoOutput = videoOutput;
    // Qt defect, seen in Qt 6.12.0: QQuickVideoOutput::itemChange() never calls QQuickItem::itemChange()
    // (qtmultimedia/src/multimediaquick/qquickvideooutput.cpp), so the output emits no windowChanged. The window is
    // taken now, and VideoStreamOutput.qml registers an output again after it moved to another window. Once the minimum
    // Qt has the fix, connect to QQuickItem::windowChanged here and drop that re-registration.
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
    _active = active;
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
    // Unbinding turned the element off: bound again, it goes on as setActive() last asked.
    g_object_set(_element.get(), "qvideosink", static_cast<gpointer>(raw), "active", _active ? TRUE : FALSE, nullptr);
    if (_videoSink != raw) {
        _clearVideoSinkFrame();
    }
    _videoSink = raw;
    _sinkDestroyedConnection =
        QObject::connect(raw, &QObject::destroyed, this, [this]() { setVideoSink(QPointer<QVideoSink>()); });
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

QSize QGCQVideoSinkController::pictureSize() const
{
    if (!_element) {
        return {};
    }
    gint width = 0;
    gint height = 0;
    g_object_get(_element.get(), "picture-width", &width, "picture-height", &height, nullptr);
    return ((width > 0) && (height > 0)) ? QSize(width, height) : QSize();
}

void QGCQVideoSinkController::_releaseElementBinding()
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
