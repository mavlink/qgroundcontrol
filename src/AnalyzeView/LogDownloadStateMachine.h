#pragma once

#include "QGCStateMachine.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(LogDownloadStateMachineLog)

class LogDownloadController;
class QGCLogEntry;
class Vehicle;

/// State machine for log list requests and log file downloads.
///
/// Manages two workflows:
///
/// 1. Log List Request:
/// ```
///     ┌────────┐
///     │  Idle  │
///     └───┬────┘
///         │ (refresh)
///         ▼
///  ┌──────────────┐
///  │RequestingList│◄──────┐
///  └──────┬───────┘       │
///         │               │ (retry)
///    (entries received)   │
///         ▼               │
///  ┌──────────────┐       │
///  │FindingMissing│───────┘
///  └──────┬───────┘
///         │ (all received / max retries)
///         ▼
///  ┌──────────────┐
///  │ ListComplete │
///  └──────────────┘
/// ```
///
/// 2. Log Data Download:
/// ```
///     ┌────────┐
///     │  Idle  │
///     └───┬────┘
///         │ (download)
///         ▼
///  ┌──────────────┐
///  │  Preparing   │
///  └──────┬───────┘
///         │
///         ▼
///  ┌──────────────┐
///  │ Downloading  │◄──────┐
///  └──────┬───────┘       │
///         │               │ (gaps found)
///    (data received)      │
///         ▼               │
///  ┌──────────────┐       │
///  │ FindingGaps  │───────┘
///  └──────┬───────┘
///         │ (chunk complete)
///         ▼
///  ┌──────────────┐
///  │  NextChunk   │──► Downloading
///  └──────┬───────┘
///         │ (log complete)
///         ▼
///  ┌──────────────┐
///  │   NextLog    │──► Preparing
///  └──────┬───────┘
///         │ (all done)
///         ▼
///  ┌──────────────┐
///  │ AllComplete  │
///  └──────────────┘
/// ```
class LogDownloadStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit LogDownloadStateMachine(LogDownloadController* controller, QObject* parent = nullptr);
    ~LogDownloadStateMachine() override;

    /// Start requesting log list from vehicle
    void startListRequest();

    /// Start downloading selected logs
    void startDownload(const QString& downloadPath);

    /// Cancel current operation
    void cancel();

    /// Handle log entry received from vehicle
    void handleLogEntry(uint32_t time_utc, uint32_t size, uint16_t id, uint16_t num_logs);

    /// Handle log data received from vehicle
    void handleLogData(uint32_t offset, uint16_t id, uint8_t count, const uint8_t* data);

    /// @return true if requesting log list
    bool isRequestingList() const { return _requestingList; }

    /// @return true if downloading logs
    bool isDownloading() const { return _downloading; }

signals:
    void requestingListChanged();
    void downloadingChanged();
    void listComplete();
    void downloadComplete();
    void operationFailed(const QString& error);

private slots:
    void _processTimeout();

private:
    void _buildListStateMachine();
    void _buildDownloadStateMachine();
    void _setupTransitions();

    void _requestLogList(uint32_t start, uint32_t end);
    void _requestLogData(uint16_t id, uint32_t offset, uint32_t count);
    void _requestLogEnd();

    void _findMissingEntries();
    void _findMissingData();
    bool _entriesComplete() const;
    bool _chunkComplete() const;
    bool _logComplete() const;

    void _prepareNextLog();
    void _advanceChunk();
    void _finishCurrentLog();

    void _setRequestingList(bool requesting);
    void _setDownloading(bool downloading);

    LogDownloadController* _controller;
    Vehicle* _vehicle = nullptr;

    // List request states
    QGCState* _listIdleState = nullptr;
    QGCState* _requestingListState = nullptr;
    QGCState* _findingMissingEntriesState = nullptr;
    QGCFinalState* _listCompleteState = nullptr;
    QGCFinalState* _listFailedState = nullptr;

    // Download states
    QGCState* _downloadIdleState = nullptr;
    QGCState* _preparingState = nullptr;
    QGCState* _downloadingState = nullptr;
    QGCState* _findingGapsState = nullptr;
    QGCState* _nextChunkState = nullptr;
    QGCState* _nextLogState = nullptr;
    QGCFinalState* _downloadCompleteState = nullptr;
    QGCFinalState* _downloadFailedState = nullptr;
    QGCFinalState* _downloadCancelledState = nullptr;

    // State tracking
    bool _requestingList = false;
    bool _downloading = false;
    int _retries = 0;
    int _apmOffset = 0;
    QString _downloadPath;

    static constexpr int _maxRetries = 3;
    static constexpr int _listTimeoutMs = 5000;
    static constexpr int _dataTimeoutMs = 500;
};
