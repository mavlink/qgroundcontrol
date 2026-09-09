#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtQmlIntegration/QtQmlIntegration>

#include <memory>

#include "GPSRecordingBuffer.h"

/// Main-thread controls for an explicit, bounded capture. Receiver workers hold only buffer()/stream tokens.
class GPSRecordingController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by GPSManager")
    Q_PROPERTY(bool recording READ recording NOTIFY stateChanged)
    Q_PROPERTY(bool hasRecording READ hasRecording NOTIFY stateChanged)
    Q_PROPERTY(int eventCount READ eventCount NOTIFY stateChanged)
    Q_PROPERTY(qint64 bytesRecorded READ bytesRecorded NOTIFY stateChanged)
    Q_PROPERTY(bool limitReached READ limitReached NOTIFY stateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY stateChanged)
    Q_PROPERTY(QString lastExportPath READ lastExportPath NOTIFY stateChanged)

public:
    explicit GPSRecordingController(QObject* parent = nullptr, std::shared_ptr<GPSRecordingBuffer> buffer = {});
    ~GPSRecordingController() override;
    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool exportRecording(const QUrl& destination);

    std::shared_ptr<GPSRecordingBuffer> buffer() const { return _buffer; }

    bool recording() const { return _buffer->status().recording; }

    bool hasRecording() const { return _buffer->status().eventCount > 0; }

    int eventCount() const { return static_cast<int>(_buffer->status().eventCount); }

    qint64 bytesRecorded() const { return _buffer->status().bytesRecorded; }

    bool limitReached() const { return _buffer->status().limitReached; }

    QString errorString() const { return _errorString; }

    QString lastExportPath() const { return _lastExportPath; }

signals:
    void stateChanged();

private:
    void _refresh();
    bool _fail(const QString& error);
    std::shared_ptr<GPSRecordingBuffer> _buffer;
    QTimer _statusTimer;
    GPSRecordingBuffer::Status _lastStatus;
    QString _errorString;
    QString _lastExportPath;
};
