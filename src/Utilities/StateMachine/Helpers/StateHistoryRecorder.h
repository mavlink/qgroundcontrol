#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>

class QAbstractState;
class QGCStateMachine;

/// Records state machine transitions for debugging and analysis.
///
/// Maintains a circular buffer of state transitions with timestamps,
/// transition reasons, and optional metadata.
///
/// Usage:
/// @code
/// QGCStateMachine machine("MyMachine", nullptr);
/// StateHistoryRecorder recorder(&machine);
/// recorder.setEnabled(true);
///
/// // ... run state machine ...
///
/// qDebug() << recorder.dumpHistory();
/// @endcode
class StateHistoryRecorder : public QObject
{
    Q_OBJECT

public:
    /// Reason for a state transition
    enum TransitionReason {
        Entered,        ///< State was entered
        Exited,         ///< State was exited
        Timeout,        ///< Transition due to timeout
        Error,          ///< Transition due to error
        Signal,         ///< Transition triggered by signal
        Event,          ///< Transition triggered by event
        Unknown         ///< Unknown reason
    };
    Q_ENUM(TransitionReason)

    /// A recorded state transition entry
    struct HistoryEntry {
        QDateTime timestamp;
        QString stateName;
        TransitionReason reason;
        QString details;

        QJsonObject toJson() const;
        QString toString() const;
    };

    explicit StateHistoryRecorder(QGCStateMachine* machine, int maxEntries = 1000);
    ~StateHistoryRecorder() override = default;

    /// Enable or disable recording
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }

    /// Set the maximum number of entries to keep (circular buffer)
    void setMaxEntries(int max);
    int maxEntries() const { return _maxEntries; }

    /// Clear all recorded history
    void clear();

    /// Get the number of recorded entries
    int count() const { return _history.size(); }

    /// Get all history entries (oldest first)
    QList<HistoryEntry> history() const { return _history; }

    /// Get the last N entries
    QList<HistoryEntry> lastEntries(int n) const;

    /// Get entries for a specific state
    QList<HistoryEntry> entriesForState(const QString& stateName) const;

    /// Get a human-readable dump of the history
    QString dumpHistory() const;

    /// Export history as JSON array
    QJsonArray toJson() const;

    /// Log history to debug output
    void logHistory() const;

    /// Manually add an entry (for custom transition types)
    void addEntry(const QString& stateName, TransitionReason reason,
                  const QString& details = QString());

private slots:
    void _onStateEntered();
    void _onStateExited();

private:
    void _addEntry(const HistoryEntry& entry);
    void _connectToState(QAbstractState* state);

    QGCStateMachine* _machine = nullptr;
    bool _enabled = false;
    int _maxEntries = 1000;
    QList<HistoryEntry> _history;
    QList<QMetaObject::Connection> _connections;
};
