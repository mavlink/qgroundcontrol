#include "BulkRefreshJob.h"
#include "ParameterManager.h"

#include "AppMessages.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(ParameterManagerLog)

BulkRefreshJob::BulkRefreshJob(ParameterManager *mgr, int componentId, const QStringList &resolvedNames,
                               bool notifyFailure, std::function<void(const QString &)> requestFn,
                               QObject *parent)
    : QObject(parent)
    , _mgr(mgr)
    , _componentId(componentId)
    , _notifyFailure(notifyFailure)
    , _requestFn(std::move(requestFn))
    , _pending(QSet<QString>(resolvedNames.cbegin(), resolvedNames.cend()))
    , _retryBaseDelayMs(QGC::runningUnitTests() ? 50 : kRetryBaseDelayMs)
{
    _retryTimer.setSingleShot(true);
    connect(&_retryTimer, &QTimer::timeout, this, &BulkRefreshJob::_sendPendingRequests);
    connect(_mgr, &ParameterManager::_paramRequestReadSuccess, this, &BulkRefreshJob::_onParamSuccess);
    connect(_mgr, &ParameterManager::_paramRequestReadFailure, this, &BulkRefreshJob::_onParamFailure);
    _sendPendingRequests();
}

void BulkRefreshJob::_sendPendingRequests()
{
    qCDebug(ParameterManagerLog) << "BulkRefreshJob: round" << _round
                                 << "\u2014 firing" << _pending.count() << "requests";
    for (const QString &name : std::as_const(_pending)) {
        _requestFn(name);
    }
}

void BulkRefreshJob::_onParamSuccess(int componentId, const QString &paramName, int /*paramIndex*/)
{
    if (componentId != _componentId) {
        return;
    }
    const bool wasPending = _pending.remove(paramName);
    const bool wasFailed  = _failed.remove(paramName);
    if (!wasPending && !wasFailed) {
        return;
    }
    _checkRoundComplete();
}

void BulkRefreshJob::_onParamFailure(int componentId, const QString &paramName, int /*paramIndex*/)
{
    if (componentId != _componentId || !_pending.remove(paramName)) {
        return;
    }
    _failed.insert(paramName);
    _checkRoundComplete();
}

void BulkRefreshJob::_checkRoundComplete()
{
    if (!_pending.isEmpty()) {
        return;
    }

    _retryTimer.stop();

    if (_failed.isEmpty()) {
        qCDebug(ParameterManagerLog) << "BulkRefreshJob: complete after round" << _round
                                     << "\u2014 all params refreshed successfully";
        deleteLater();
        return;
    }

    if (_round < kMaxRetryRounds) {
        const int delayMs = _retryBaseDelayMs << _round;
        qCDebug(ParameterManagerLog) << "BulkRefreshJob: round" << _round
                                     << "finished with" << _failed.count()
                                     << "failures \u2014 retrying in" << delayMs << "ms";
        _pending = std::move(_failed);
        _failed.clear();
        ++_round;
        _retryTimer.start(delayMs);
    } else {
        qCDebug(ParameterManagerLog) << "BulkRefreshJob: complete after round" << _round
                                     << "\u2014" << _failed.count() << "params still failed";
        if (_notifyFailure) {
            const QStringList failedList(_failed.cbegin(), _failed.cend());
            QGC::showAppMessage(_mgr->tr("Parameter refresh failed for: %1").arg(failedList.join(QStringLiteral(", "))),
                                _mgr->tr("Parameter Bulk Refresh"));
        }
        deleteLater();
    }
}
