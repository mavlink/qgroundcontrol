#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "VideoReceiver.h"

/// Per-stream diagnostics model.
///
/// Keeps user-visible error state and restart counters out of VideoStream.
class VideoDiagnostics : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(VideoReceiver::ErrorCategory lastErrorCategory READ lastErrorCategory NOTIFY lastErrorChanged)
    Q_PROPERTY(int restartAttempts READ restartAttempts NOTIFY restartAttemptsChanged)

public:
    explicit VideoDiagnostics(QObject* parent = nullptr);
    ~VideoDiagnostics() override;

    [[nodiscard]] QString lastError() const { return _lastError; }
    [[nodiscard]] VideoReceiver::ErrorCategory lastErrorCategory() const { return _lastErrorCategory; }
    [[nodiscard]] int restartAttempts() const { return _restartAttempts; }

    void clearError();
    void setError(VideoReceiver::ErrorCategory category, const QString& message);
    void setRestartAttempts(int attempts);

signals:
    void lastErrorChanged(const QString& lastError);
    void restartAttemptsChanged(int attempts);

private:
    static QString _formatError(VideoReceiver::ErrorCategory category, const QString& message);

    QString _lastError;
    VideoReceiver::ErrorCategory _lastErrorCategory = VideoReceiver::ErrorCategory::Transient;
    int _restartAttempts = 0;
};
