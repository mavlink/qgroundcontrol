#include "QGCQVideoSinkController.h"

#include <utility>

#include <QtCore/QThread>
#include <QtGui/QWindow>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include "QGCLoggingCategory.h"
#include "gstqgc/GstQgcSinkBin.h"
#include "gstqgc/gstqgcqvideosink.h"

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
    GstQgc::setSinkActive(_element.get(), active);
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
        _releaseElementBinding();
        return;
    }
    // Unbinding turned the element off: bound again, it goes on as setActive() last asked.
    GstQgc::setSinkTarget(_element.get(), raw);
    GstQgc::setSinkActive(_element.get(), _active);
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
    return _element ? GstQgc::sinkPictureSize(_element.get()) : QSize();
}

void QGCQVideoSinkController::_releaseElementBinding()
{
    if (!_element || _bindingReleased)
        return;

    if (_sinkDestroyedConnection) {
        QObject::disconnect(_sinkDestroyedConnection);
        _sinkDestroyedConnection = {};
    }
    GstQgc::setSinkActive(_element.get(), false);
    GstQgc::setSinkTarget(_element.get(), nullptr);
    _bindingReleased = true;
    _clearVideoSinkFrame();
}

void QGCQVideoSinkController::_clearVideoSinkFrame()
{
    releaseFrame();
    _videoSink.clear();
}

namespace GStreamer {

std::shared_ptr<const GstQgc::SinkBin> createVideoSink(const QByteArray& conversionElement,
                                                       bool disablePixelAspectRatio, [[maybe_unused]] bool gpuZeroCopy)
{
    GstQgc::SinkBinConfig config{.conversionElement = conversionElement, .disablePar = disablePixelAspectRatio};
#if defined(QGC_HAS_ANY_GPU_PATH)
    config.gpuZeroCopy = gpuZeroCopy;
#endif
    GstQgc::SinkBin sinkBin = GstQgc::makeSinkBin(config);
    if (!sinkBin) {
        // completeInit() found the sink's elements at startup: the registry changed underfoot, or the chain doesn't
        // link here.
        qCCritical(QGCQVideoSinkControllerLog) << "the video sink bin could not be built";
        return nullptr;
    }
    return std::make_shared<const GstQgc::SinkBin>(std::move(sinkBin));
}

QGCQVideoSinkController* setupQVideoSinkElement(GstElement* qvideosink, QVideoSink* videoSink,
                                                QObject* controllerParent)
{
    if (!qvideosink || !videoSink || !controllerParent) {
        // controllerParent owns the controller's QObject lifetime — else it leaks (caller has no handle).
        qCWarning(QGCQVideoSinkControllerLog) << "null qvideosink, videoSink, or controllerParent";
        return nullptr;
    }

    auto* controller = new QGCQVideoSinkController(qvideosink, controllerParent);

    // Route the initial install through the controller so its destroyed-sink QPointer guard
    // covers setup, not just later swaps.
    controller->setVideoSink(QPointer<QVideoSink>(videoSink));

    return controller;
}

}  // namespace GStreamer
