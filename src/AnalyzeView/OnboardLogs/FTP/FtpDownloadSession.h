#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <chrono>
#include <cstdint>
#include <optional>

#include "OnboardLogEntry.h"
#include "OnboardLogEntryQueue.h"
#include "OnboardLogSession.h"

/// Queue, cancellation, and progress state for one MAVLink FTP download batch.
class FtpDownloadSession final : public OnboardLogSessionBase
{
    Q_DISABLE_COPY_MOVE(FtpDownloadSession)

public:
    struct Cancellation
    {
        QPointer<OnboardLogEntry> currentEntry;
        bool wasActive = false;
        bool cancelRemoteDownload = false;
    };

    struct ProgressUpdate
    {
        uint64_t bytes = 0;
        qreal rate = 0.;
        qreal progress = 0.;
    };

    FtpDownloadSession() = default;

    [[nodiscard]] quint64 begin(const QList<QPointer<OnboardLogEntry>>& entries, const QString& path);
    Cancellation cancel();
    bool finishCancellation();
    void finish();
    void clear();

    QPointer<OnboardLogEntry> takeNext();
    void completeCurrent();

    void setInFlight(bool inFlight) { _inFlight = inFlight; }

    bool inFlight() const { return _inFlight; }

    std::optional<ProgressUpdate> updateProgress(qreal progress, std::chrono::milliseconds minimumInterval);

    void requestRefresh() { _refreshRequested = true; }

    bool isCurrent(quint64 generation, const OnboardLogEntry* entry = nullptr) const;

    QPointer<OnboardLogEntry> currentEntry() const { return _currentEntry; }

    uint64_t currentEntrySize() const { return _currentEntrySize; }

    qsizetype pendingCount() const { return _entries.size(); }

    const QString& path() const { return _path; }

private:
    void _resetProgress();

    OnboardLogEntryQueue _entries;
    QPointer<OnboardLogEntry> _currentEntry;
    uint64_t _currentEntrySize = 0;
    QString _path;
    QElapsedTimer _elapsed;
    uint64_t _bytesAtLastUpdate = 0;
    qreal _rateAverage = 0.;
    bool _rateInitialized = false;
    bool _inFlight = false;
    bool _refreshRequested = false;
};
