#pragma once

#include "QGCStateMachine.h"
#include "QGCMAVLink.h"

#include <QtCore/QList>
#include <QtCore/QTimer>

class PlanManager;
class MissionItem;

/// State machine for MAVLink mission protocol transactions.
///
/// Handles three types of transactions:
/// - Read: Load mission items from vehicle
/// - Write: Send mission items to vehicle
/// - RemoveAll: Clear all mission items from vehicle
///
/// State Diagram:
///
/// ```
///                    ┌─────────┐
///                    │  Idle   │◄─────────────────────────────────┐
///                    └────┬────┘                                  │
///          ┌──────────────┼──────────────┐                        │
///          ▼              ▼              ▼                        │
///   ┌──────────────┐ ┌──────────┐ ┌─────────────┐                 │
///   │ RequestList  │ │SendCount │ │ SendClearAll│                 │
///   └──────┬───────┘ └────┬─────┘ └──────┬──────┘                 │
///          │              │              │                        │
///     (count_rcvd)   (request_rcvd) (ack_rcvd)                    │
///          ▼              ▼              │                        │
///   ┌──────────────┐ ┌──────────┐        │                        │
///   │ RequestItems │ │ SendItem │        │                        │
///   └──────┬───────┘ └────┬─────┘        │                        │
///          │              │              │                        │
///     (all_items)    (ack_rcvd)          │                        │
///          │              │              │                        │
///          ▼              ▼              ▼                        │
///   ┌──────────────┐ ┌──────────┐ ┌─────────────┐                 │
///   │  SendAck     │ │WriteDone │ │ ClearDone   │                 │
///   └──────┬───────┘ └────┬─────┘ └──────┬──────┘                 │
///          │              │              │                        │
///          └──────────────┴──────────────┴────────────────────────┘
/// ```
class PlanManagerStateMachine : public QGCStateMachine
{
    Q_OBJECT

public:
    explicit PlanManagerStateMachine(PlanManager* planManager, QObject* parent = nullptr);
    ~PlanManagerStateMachine() override;

    /// Start reading mission items from vehicle
    void startRead();

    /// Start writing mission items to vehicle
    /// @param missionItems Items to write (ownership transferred)
    void startWrite(const QList<MissionItem*>& missionItems);

    /// Start removing all mission items from vehicle
    void startRemoveAll();

    /// Cancel current transaction
    void cancel();

    /// @return true if a transaction is in progress
    bool inProgress() const;

    /// Handle incoming MAVLink message
    void handleMessage(const mavlink_message_t& message);

    /// Get items to write and transfer ownership to caller
    /// After calling this, the state machine no longer owns the items
    QList<MissionItem*> takeWriteMissionItems();

    /// Get items read from vehicle and transfer ownership to caller
    /// After calling this, the state machine no longer owns the items
    QList<MissionItem*> takeMissionItems();

    /// Get current retry count
    int retryCount() const { return _retryCount; }

    /// Get last mission request sequence number
    int lastMissionRequest() const { return _lastMissionRequest; }

signals:
    void transactionComplete(bool success);
    void readComplete(bool success);
    void writeComplete(bool success);
    void removeAllComplete(bool success);
    void progressChanged(double pct);
    void errorOccurred(int errorCode, const QString& errorMsg);

private slots:
    void _ackTimeout();

private:
    void _buildStateMachine();
    void _setupTransitions();

    // Message handlers
    void _handleMissionCount(const mavlink_message_t& message);
    void _handleMissionItem(const mavlink_message_t& message);
    void _handleMissionRequest(const mavlink_message_t& message);
    void _handleMissionAck(const mavlink_message_t& message);

    // Protocol actions
    void _sendRequestList();
    void _sendMissionCount();
    void _sendMissionItem(int seq);
    void _sendMissionRequest(int seq);
    void _sendMissionAck();
    void _sendClearAll();

    // State management
    void _startAckTimeout(int timeoutMs);
    void _stopAckTimeout();
    void _finishTransaction(bool success);
    void _retryCurrentAction();

    // Data management
    void _clearMissionItems();
    void _clearWriteMissionItems();

    PlanManager* _planManager;

    // Timer for ack timeouts
    QTimer* _ackTimeoutTimer = nullptr;

    // Retry tracking
    int _retryCount = 0;
    static constexpr int _maxRetryCount = 5;
    static constexpr int _ackTimeoutMilliseconds = 1500;
    static constexpr int _retryTimeoutMilliseconds = 250;

    // Read transaction state
    QList<int> _itemIndicesToRead;
    int _missionItemCountToRead = 0;

    // Write transaction state
    QList<int> _itemIndicesToWrite;
    int _lastMissionRequest = -1;

    // Mission items
    QList<MissionItem*> _missionItems;       // Items read from vehicle
    QList<MissionItem*> _writeMissionItems;  // Items being written to vehicle

    // States
    QGCState* _idleState = nullptr;

    // Read states
    QGCState* _requestListState = nullptr;
    QGCState* _waitingForCountState = nullptr;
    QGCState* _requestItemsState = nullptr;
    QGCState* _waitingForItemState = nullptr;
    QGCState* _sendReadAckState = nullptr;

    // Write states
    QGCState* _sendCountState = nullptr;
    QGCState* _waitingForRequestState = nullptr;
    QGCState* _sendItemState = nullptr;
    QGCState* _waitingForWriteAckState = nullptr;

    // Remove all states
    QGCState* _sendClearAllState = nullptr;
    QGCState* _waitingForClearAckState = nullptr;

    // Final states
    QGCFinalState* _readCompleteState = nullptr;
    QGCFinalState* _writeCompleteState = nullptr;
    QGCFinalState* _clearCompleteState = nullptr;
    QGCFinalState* _errorState = nullptr;

    // Current transaction type for completion handling
    enum class TransactionType {
        None,
        Read,
        Write,
        RemoveAll
    };
    TransactionType _currentTransaction = TransactionType::None;
};
