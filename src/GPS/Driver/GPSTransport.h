#pragma once

#include <QtCore/QObject>

/// Abstract serial/USB transport for the synchronous GPS driver library.
///
/// Threading contract:
///   * All virtual methods (open/close/read/write/waitFor*/bytesAvailable/setBaudRate)
///     are blocking and MUST be called from the worker thread that owns the GPSProvider.
///     Calling them from the GUI thread will stall the UI.
///   * After moveToThread(), any caller outside the worker thread must use
///     QMetaObject::invokeMethod with a QueuedConnection or equivalent marshaling.
///   * Implementations should only touch their underlying IO handle on the owning
///     thread. Construction and parenting may happen on any thread prior to moveToThread().
///
/// Rationale for QObject base: subclasses live on a worker thread (moveToThread)
/// and need the Qt object model for parent-based lifetime (transport is parented to
/// GPSProvider and torn down when the worker thread exits via deleteLater).
class GPSTransport : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;
    ~GPSTransport() override = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual qint64 read(char *data, qint64 maxSize) = 0;
    virtual qint64 write(const char *data, qint64 size) = 0;
    virtual bool waitForReadyRead(int msecs) = 0;
    virtual bool waitForBytesWritten(int msecs) = 0;
    virtual qint64 bytesAvailable() const = 0;
    virtual bool setBaudRate(qint32 baudRate) = 0;
};
