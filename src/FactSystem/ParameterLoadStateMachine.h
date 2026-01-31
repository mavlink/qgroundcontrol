#pragma once

#include "QGCStateMachine.h"
#include "MAVLinkLib.h"

#include <QtCore/QMap>
#include <QtCore/QTimer>

class ParameterManager;

/// State machine for the initial parameter load workflow.
///
/// This manages the overall parameter loading process, including:
/// - FTP download path (ArduPilot)
/// - Conventional MAVLink PARAM_REQUEST_LIST
/// - Batch retry for missing parameters
/// - Default component wait logic
///
/// State Diagram:
///
/// ```
///                    ┌─────────┐
///                    │  Idle   │
///                    └────┬────┘
///          ┌──────────────┼──────────────┐
///          │         (start)             │
///          │              │              │
///     (ArduPilot)         │     (high-latency/log)
///          │              │              │
///          ▼              ▼              ▼
///   ┌──────────────┐ ┌──────────────┐   │
///   │FTPDownloading│ │SendRequestLst│   │
///   └──────┬───────┘ └──────┬───────┘   │
///          │                │           │
///     (ftp_complete)   (param_rcvd)     │
///     (ftp_failed)          │           │
///          │                ▼           │
///          │         ┌──────────────┐   │
///          └────────►│ReceivingPrms│   │
///                    └──────┬───────┘   │
///                           │           │
///                      (timeout)        │
///                           ▼           │
///                    ┌──────────────┐   │
///                    │ BatchRetrying│◄─┐│
///                    └──────┬───────┘  ││
///                           │          ││
///                    (still_missing)───┘│
///                    (all_received)     │
///                           │           │
///                           ▼           │
///                    ┌──────────────┐   │
///                    │WaitDefComp   │   │
///                    └──────┬───────┘   │
///                           │           │
///                           ▼           │
///                    ┌──────────────┐   │
///                    │  Complete    │◄──┘
///                    └──────────────┘
/// ```
class ParameterLoadStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit ParameterLoadStateMachine(ParameterManager* paramManager, QObject* parent = nullptr);
    ~ParameterLoadStateMachine() override;

    /// Start parameter loading
    /// @param componentId Component to request (MAV_COMP_ID_ALL for all)
    void startLoad(uint8_t componentId = MAV_COMP_ID_ALL);

    /// Cancel the current load operation
    void cancel();

    /// Called by ParameterManager when PARAM_VALUE is received
    void handleParamValue(int componentId, const QString& parameterName,
                          int parameterCount, int parameterIndex,
                          MAV_PARAM_TYPE mavParamType, const QVariant& parameterValue);

    /// Called when FTP download completes
    void handleFTPComplete(const QString& fileName, const QString& errorMsg);

    /// Called for FTP download progress
    void handleFTPProgress(float progress);

    /// @return true if parameter loading is in progress
    bool isLoading() const;

    /// @return Current load progress [0.0, 1.0]
    double loadProgress() const { return _loadProgress; }

    /// @return true if initial load has completed (success or failure)
    bool isInitialLoadComplete() const { return _initialLoadComplete; }

    /// @return true if index batch queue is active
    bool isIndexBatchQueueActive() const { return _indexBatchQueueActive; }

    /// @return true if waiting for default component params
    bool isWaitingForDefaultComponent() const { return _waitingForDefaultComponent; }

    /// Set timeout values for testing
    void setTestTimeouts(int initialRequestTimeoutMs, int waitingParamTimeoutMs);

signals:
    /// Emitted when parameter load completes
    /// @param success true if load was successful
    /// @param missingParameters true if some parameters could not be loaded
    void loadComplete(bool success, bool missingParameters);

    /// Emitted when load progress changes
    void progressChanged(double progress);

private slots:
    void _waitingParamTimeout();
    void _initialRequestTimeout();

private:
    void _buildStateMachine();

    // State entry actions
    void _onIdleEntry();
    void _onFTPDownloadingEntry();
    void _onSendingRequestListEntry();
    void _onReceivingParamsEntry();
    void _onBatchRetryingEntry();
    void _onWaitingDefaultComponentEntry();
    void _onCompleteEntry();

    // Helper methods
    bool _fillIndexBatchQueue(bool waitingParamTimeout);
    void _updateProgress();
    bool _allParamsReceived() const;
    bool _hasDefaultComponentParams() const;
    void _sendParamRequestList(uint8_t componentId);
    void _checkInitialLoadComplete();
    void _setLoadProgress(double progress);

    ParameterManager* _paramManager = nullptr;
    uint8_t _targetComponentId = MAV_COMP_ID_ALL;
    bool _useFTP = false;

    // State tracking
    bool _initialLoadComplete = false;
    bool _indexBatchQueueActive = false;
    bool _waitingForDefaultComponent = false;
    bool _readParamIndexProgressActive = false;
    double _loadProgress = 0.0;

    // Retry tracking
    int _initialRequestRetryCount = 0;
    static constexpr int _maxInitialRequestListRetry = 4;
    static constexpr int _maxInitialLoadRetrySingleParam = 5;
    bool _disableAllRetries = false;

    // Batch queue for index-based re-requests
    static constexpr int _maxBatchSize = 10;
    QList<int> _indexBatchQueue;

    // Track previous waiting param count for cache writes
    int _prevWaitingReadParamIndexCount = 0;

    // Timers
    QTimer* _initialRequestTimeoutTimer = nullptr;
    QTimer* _waitingParamTimeoutTimer = nullptr;
    int _initialRequestTimeoutMs = 5000;
    int _waitingParamTimeoutMs = 3000;

    // States
    QGCState* _idleState = nullptr;
    QGCState* _ftpDownloadingState = nullptr;
    QGCState* _sendingRequestListState = nullptr;
    QGCState* _receivingParamsState = nullptr;
    QGCState* _batchRetryingState = nullptr;
    QGCState* _waitingDefaultComponentState = nullptr;
    QGCFinalState* _completeState = nullptr;
};
