#include "VideoDiagnostics.h"

VideoDiagnostics::VideoDiagnostics(QObject* parent)
    : QObject(parent)
{
}

VideoDiagnostics::~VideoDiagnostics() = default;

void VideoDiagnostics::clearError()
{
    if (_lastError.isEmpty() && _lastErrorCategory == VideoReceiver::ErrorCategory::Transient)
        return;

    _lastError.clear();
    _lastErrorCategory = VideoReceiver::ErrorCategory::Transient;
    emit lastErrorChanged(_lastError);
}

void VideoDiagnostics::setError(VideoReceiver::ErrorCategory category, const QString& message)
{
    const QString formatted = _formatError(category, message);
    if (_lastError == formatted && _lastErrorCategory == category)
        return;

    _lastError = formatted;
    _lastErrorCategory = category;
    emit lastErrorChanged(_lastError);
}

void VideoDiagnostics::setRestartAttempts(int attempts)
{
    if (_restartAttempts == attempts)
        return;

    _restartAttempts = attempts;
    emit restartAttemptsChanged(_restartAttempts);
}

QString VideoDiagnostics::_formatError(VideoReceiver::ErrorCategory category, const QString& message)
{
    const char* tag = nullptr;
    switch (category) {
        case VideoReceiver::ErrorCategory::Transient:
            tag = "[transient] ";
            break;
        case VideoReceiver::ErrorCategory::Fatal:
            tag = "[fatal] ";
            break;
        case VideoReceiver::ErrorCategory::MissingPlugin:
            tag = "[codec] ";
            break;
    }

    return QLatin1String(tag) + message;
}
