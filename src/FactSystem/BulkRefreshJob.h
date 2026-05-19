#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include <functional>

class ParameterManager;

/// Fires a batch of PARAM_REQUEST_READs with exponential backoff retry.
/// Owned by ParameterManager (as QObject parent); self-deletes on completion.
class BulkRefreshJob : public QObject
{
public:
    BulkRefreshJob(ParameterManager *mgr, int componentId, const QStringList &resolvedNames,
                   bool notifyFailure, std::function<void(const QString &)> requestFn,
                   QObject *parent);

    static constexpr int kMaxRetryRounds   = 3;    ///< Number of batch-level retry rounds before giving up
    static constexpr int kRetryBaseDelayMs = 1000;  ///< Base retry interval; doubled each round

private:
    void _sendPendingRequests();
    void _onParamSuccess(int componentId, const QString &paramName, int paramIndex);
    void _onParamFailure(int componentId, const QString &paramName, int paramIndex);
    void _checkRoundComplete();

    ParameterManager *_mgr;
    int _componentId;
    bool _notifyFailure;
    std::function<void(const QString&)> _requestFn;
    QSet<QString> _pending;
    QSet<QString> _failed;
    int _round = 0;
    const int _retryBaseDelayMs;  ///< 50 ms in unit tests, kRetryBaseDelayMs otherwise
    QTimer _retryTimer;
};
