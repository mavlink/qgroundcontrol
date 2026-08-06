#include "QGCQVideoSinkController.h"

#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtGui/QWindow>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQuick/QQuickWindow>
#include <memory>

#include "GstScoped.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(QGCQVideoSinkControllerLog, "Video.GStreamer.QGCQVideoSinkController")

QGCQVideoSinkController::QGCQVideoSinkController(GstElement* element, QObject* parent)
    : QObject(parent), _element(element ? GST_ELEMENT(gst_object_ref(element)) : nullptr)
{
    _emitTimer.setInterval(1000);
    _emitTimer.setSingleShot(false);
    QObject::connect(&_emitTimer, &QTimer::timeout, this, &QGCQVideoSinkController::_onEmitTimer);
    _emitTimer.start();
}

QGCQVideoSinkController::~QGCQVideoSinkController()
{
    _releaseElementBinding();
    if (_element) {
        gst_object_unref(_element);
        _element = nullptr;
    }
    _emitTimer.stop();
}

QList<QGCQVideoSinkController*> QGCQVideoSinkController::controllersOf(const QObject* receiver)
{
    return receiver ? receiver->findChildren<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly)
                    : QList<QGCQVideoSinkController*>{};
}

void QGCQVideoSinkController::syncActiveToWindowVisibility(QObject* receiver, QQuickVideoOutput* videoOutput)
{
    if (!receiver || !videoOutput)
        return;

    auto applyVisibility = [receiver](QWindow* win) {
        const QWindow::Visibility v = win ? win->visibility() : QWindow::Hidden;
        const bool active = win && (v != QWindow::Hidden && v != QWindow::Minimized);
        for (auto* c : controllersOf(receiver))
            c->setActive(active);
    };
    // Track the previous connection so windowChanged drops it before wiring the new window,
    // else an old hidden window keeps gating the live receiver.
    auto prevConn = std::make_shared<QMetaObject::Connection>();
    auto wireWindow = [applyVisibility, prevConn, receiver](QQuickWindow* qw) {
        if (*prevConn) {
            QObject::disconnect(*prevConn);
            *prevConn = QMetaObject::Connection{};
        }
        if (!qw) {
            applyVisibility(nullptr);
            return;
        }
        applyVisibility(qw);
        *prevConn = QObject::connect(qw, &QWindow::visibilityChanged, receiver,
                                     [applyVisibility, qw](QWindow::Visibility) { applyVisibility(qw); });
    };
    wireWindow(videoOutput->window());
    QObject::connect(videoOutput, &QQuickVideoOutput::windowChanged, receiver, wireWindow);
}

const GstElement* QGCQVideoSinkController::element() const noexcept
{
    return _element;
}

void QGCQVideoSinkController::updateNegotiation(const QString& format, const QSize& resolution)
{
    if (thread() != QThread::currentThread()) {
        qCCritical(QGCQVideoSinkControllerLog) << "called from wrong thread";
        return;
    }
    if (_bindingReleased)
        return;
    bool changed = false;
    {
        QMutexLocker locker(&_stateMutex);
        if (format != _negotiatedFormat || resolution != _negotiatedResolution) {
            _negotiatedFormat = format;
            _negotiatedResolution = resolution;
            changed = true;
        }
    }
    if (changed) {
        qCDebug(QGCQVideoSinkControllerLog).noquote()
            << "Negotiation update: format=" << format << "size=" << resolution;
        emit negotiationChanged();
    }
}

void QGCQVideoSinkController::refreshLatency()
{
    if (thread() != QThread::currentThread()) {
        qCCritical(QGCQVideoSinkControllerLog) << "called from wrong thread";
        return;
    }
    if (!_element || _bindingReleased)
        return;
    // Pipeline-level latency was recalculated upstream (e.g. RTSP jitter-buffer reconfigure);
    // re-query the sink so GstBaseSink re-primes its cached latency for the new depth.
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_latency());
    if (!gst_element_query(_element, query.get())) {
        qCDebug(QGCQVideoSinkControllerLog) << "Latency query not handled by sink element";
    }
}

void QGCQVideoSinkController::setActive(bool active)
{
    if (thread() != QThread::currentThread()) {
        qCCritical(QGCQVideoSinkControllerLog) << "called from wrong thread";
        return;
    }
    if (!_element || _bindingReleased)
        return;
    g_object_set(_element, "active", active ? TRUE : FALSE, nullptr);
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
        g_object_set(_element, "active", FALSE, "qvideosink", static_cast<gpointer>(nullptr), nullptr);
        _bindingReleased = true;
        return;
    }
    g_object_set(_element, "qvideosink", static_cast<gpointer>(raw), nullptr);
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
    _emitTimer.stop();
}

quint64 QGCQVideoSinkController::frameCount() const noexcept
{
    if (!_element || _bindingReleased)
        return 0;
    guint64 delivered = 0;
    g_object_get(_element, "frames-delivered", &delivered, nullptr);
    return static_cast<quint64>(delivered);
}

QString QGCQVideoSinkController::negotiatedFormat() const
{
    QMutexLocker locker(&_stateMutex);
    return _negotiatedFormat;
}

QSize QGCQVideoSinkController::negotiatedResolution() const
{
    QMutexLocker locker(&_stateMutex);
    return _negotiatedResolution;
}

void QGCQVideoSinkController::_releaseElementBinding() noexcept
{
    if (!_element || _bindingReleased)
        return;

    if (_sinkDestroyedConnection) {
        QObject::disconnect(_sinkDestroyedConnection);
        _sinkDestroyedConnection = {};
    }
    g_object_set(_element, "active", FALSE, "qvideosink", static_cast<gpointer>(nullptr), nullptr);
    _bindingReleased = true;
}

void QGCQVideoSinkController::_onEmitTimer()
{
    if (!_element || _bindingReleased)
        return;
    guint64 delivered = 0;
    g_object_get(_element, "frames-delivered", &delivered, nullptr);
    if (delivered != _lastEmittedFrameTotal) {
        _lastEmittedFrameTotal = delivered;
        emit frameCountsChanged();
    }
}
