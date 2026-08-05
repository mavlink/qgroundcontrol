#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <gst/gst.h>

class QQuickVideoOutput;
class QVideoSink;

/// GUI-thread companion for the GstQgcQVideoSink element: mirrors negotiation/telemetry into
/// Q_PROPERTYs for QML and owns the 1 Hz timer polling `frames-delivered`. Driven by
/// GstVideoReceiver's bus pump. Ownership: parent owns the controller; the controller owns one
/// GstElement ref so deferred QObject teardown can still clear the element binding.
class QGCQVideoSinkController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(quint64 frameCount READ frameCount NOTIFY frameCountsChanged)
    Q_PROPERTY(QString negotiatedFormat READ negotiatedFormat NOTIFY negotiationChanged)
    Q_PROPERTY(QSize negotiatedResolution READ negotiatedResolution NOTIFY negotiationChanged)

public:
    /// @p element is the GstQgcQVideoSink to control. Controller takes a ref so QObject teardown
    /// can safely clear the element binding even if the parent bin is released first.
    QGCQVideoSinkController(GstElement* element, QObject* parent = nullptr);
    ~QGCQVideoSinkController() override;

    /// A receiver's owning controllers — direct children only, never a deep QObject-tree walk.
    static QList<QGCQVideoSinkController*> controllersOf(const QObject* receiver);

    /// Sync every controller owned by @p receiver to @p videoOutput's window visibility (drop frames
    /// while hidden/minimized), re-wiring across windowChanged. Wiring is parented to @p receiver.
    static void syncActiveToWindowVisibility(QObject* receiver, QQuickVideoOutput* videoOutput);

    // setActive(false) drops frames at the element; setVideoSink swaps the destination under
    // GST_OBJECT_LOCK. QPointer so a caller-thread destruction race is caught here, not in GObject.
    void setActive(bool active);
    void setVideoSink(QPointer<QVideoSink> sink);

    /// Stop the poll timer synchronously ahead of deleteLater so a deferred destruction can't keep
    /// binding the element while a replacement controller is installed on it. Idempotent.
    void prepareForRelease();

    const GstElement* element() const noexcept;
    void updateNegotiation(const QString& format, const QSize& resolution);

    quint64 frameCount() const noexcept;
    QString negotiatedFormat() const;
    QSize negotiatedResolution() const;

public slots:
    /// Re-prime sink-side latency after a pipeline latency recalculation (e.g. RTSP
    /// jitter-buffer reconfigure). Re-queries the element latency and pushes it back.
    void refreshLatency();

signals:
    void frameCountsChanged();
    void negotiationChanged();

private:
    void _releaseElementBinding() noexcept;
    void _onEmitTimer();

    GstElement* _element = nullptr;  // owned ref
    QMetaObject::Connection _sinkDestroyedConnection;
    bool _bindingReleased = false;
    QTimer _emitTimer;

    mutable QMutex _stateMutex;
    QString _negotiatedFormat;
    QSize _negotiatedResolution;
    quint64 _lastEmittedFrameTotal = 0;
};
