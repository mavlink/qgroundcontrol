#include "NTRIPSourceTableController.h"

#include <QtCore/QPointer>

#include "GPSQtRuntimeScheduler.h"
#include "NTRIPHttpResponse.h"
#include "NTRIPSourceTable.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIP.NTRIPSourceTableController")

NTRIPSourceTableController::NTRIPSourceTableController(QObject* parent, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _model(new NTRIPSourceTableModel(this))
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
{
    qCDebug(NTRIPSourceTableControllerLog) << this;
}

NTRIPSourceTableController::~NTRIPSourceTableController()
{
    qCDebug(NTRIPSourceTableControllerLog) << this;
    _abortReply();
}

QAbstractListModel* NTRIPSourceTableController::mountpointModel() const
{
    return _model;
}

void NTRIPSourceTableController::fetch(const NTRIPTransportConfig& config, const QGeoCoordinate& sortCoord)
{
    if (!_scheduler) {
        _abortReply();
        _onFetchError(tr("GPS scheduler unavailable"));
        return;
    }
    const QString cacheKey = config.casterIdentity();

    // Debounce repeat clicks for the same caster, but let a request for a
    // different caster supersede an in-flight one (_abortReply below cancels it).
    if (_fetchStatus == FetchStatus::InProgress && cacheKey == _lastFetchKey) {
        _sortCoord = sortCoord;
        return;
    }

    if (_fetchedAtMs >= 0 && cacheKey == _lastFetchKey) {
        const qint64 age = _scheduler->nowMs() - _fetchedAtMs;
        if (age >= 0 && age < kCacheTtlMs) {
            qCDebug(NTRIPSourceTableControllerLog) << "Source table cache hit, age:" << age << "ms";
            const QPointer<NTRIPSourceTableController> guard(this);
            const auto generation = _generation;
            _sortCoord = sortCoord;
            _model->updateDistances(sortCoord);
            if (!guard || generation != _generation) {
                return;
            }
            _fetchStatus = FetchStatus::Success;
            emit fetchStatusChanged();
            return;
        }
    }

    // Same validation the streaming transport runs (host/port/control chars,
    // RFC 7617 username) so the fetch can't reach a caster the stream rejects.
    if (const QString invalid = config.validationError(); !invalid.isEmpty()) {
        _abortReply();
        _onFetchError(invalid);
        return;
    }

    _abortReply();

    const auto generation = _generation;
    const QPointer<NTRIPSourceTableController> guard(this);
    _sortCoord = sortCoord;
    _lastFetchKey = cacheKey;
    _fetchedAtMs = -1;
    _fetchStatus = FetchStatus::InProgress;
    const bool errorChanged = !_fetchError.isEmpty();
    _fetchError.clear();
    if (errorChanged) {
        emit fetchErrorChanged();
        if (!guard || generation != _generation) {
            return;
        }
    }
    emit fetchStatusChanged();
    if (!guard || generation != _generation) {
        return;
    }

    _body.clear();
    _reply = new NTRIPHttpResponse(config, NTRIPHttpResponse::Mode::SourceTable, this, _scheduler);
    const auto reply = _reply;
    connect(reply, &NTRIPHttpResponse::plaintextCredentialsWarning, this,
            &NTRIPSourceTableController::plaintextCredentialsWarning);
    connect(reply, &NTRIPHttpResponse::bodyReceived, this, [this, reply, generation](const QByteArray& bytes, qint64) {
        if (generation == _generation && reply == _reply) {
            _body.append(bytes);
        }
    });
    connect(reply, &NTRIPHttpResponse::failed, this, [this, reply, generation](const NTRIPFailure& failure) {
        if (generation == _generation && reply == _reply) {
            _abortReply();
            _onFetchError(failure.detail);
        }
    });
    connect(reply, &NTRIPHttpResponse::completed, this, [this, reply, generation]() {
        if (generation == _generation && reply == _reply) {
            _onReplyFinished();
        }
    });
    reply->start();
}

void NTRIPSourceTableController::_onReplyFinished()
{
    if (!_reply) {
        return;
    }
    const QString body = QString::fromUtf8(_body);
    _abortReply();
    if (!body.contains(QStringLiteral("ENDSOURCETABLE"))) {
        _onFetchError(tr("Response does not contain a valid source table"));
        return;
    }
    _onSourceTableReceived(body);
}

void NTRIPSourceTableController::_onSourceTableReceived(const QString& table)
{
    const QPointer<NTRIPSourceTableController> guard(this);
    const auto generation = _generation;
    _model->parseSourceTable(table);
    if (!guard || generation != _generation) {
        return;
    }
    _fetchedAtMs = _scheduler ? _scheduler->nowMs() : -1;

    _model->updateDistances(_sortCoord);
    if (!guard || generation != _generation) {
        return;
    }

    _fetchStatus = FetchStatus::Success;
    emit fetchStatusChanged();
    if (guard && generation == _generation) {
        emit mountpointModelChanged();
    }
}

void NTRIPSourceTableController::_onFetchError(const QString& error)
{
    const QPointer<NTRIPSourceTableController> guard(this);
    const auto generation = _generation;
    _fetchedAtMs = -1;
    _fetchError = error;
    _fetchStatus = FetchStatus::Error;
    _model->clear();
    if (!guard || generation != _generation) {
        return;
    }
    emit fetchErrorChanged();
    if (guard && generation == _generation) {
        emit fetchStatusChanged();
    }
}

void NTRIPSourceTableController::_abortReply()
{
    ++_generation;
    const QPointer<NTRIPHttpResponse> reply = _reply;
    _reply = nullptr;
    _body.clear();
    if (reply) {
        disconnect(reply, nullptr, this, nullptr);
        reply->stop();
        if (reply) {
            reply->deleteLater();
        }
    }
}

void NTRIPSourceTableController::injectSourceTableForTest(const QString& table)
{
    _abortReply();
    _onSourceTableReceived(table);
}

void NTRIPSourceTableController::injectFetchErrorForTest(const QString& error)
{
    _abortReply();
    _onFetchError(error);
}

void NTRIPSourceTableController::selectMountpoint(const QString& mountpoint)
{
    emit mountpointSelected(mountpoint);
}
