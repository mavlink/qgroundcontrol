#include "NTRIPSourceTableController.h"

#include <chrono>
#include <utility>

#include <QtCore/QPointer>

#include "MonotonicClock.h"
#include "NTRIPHttpSession.h"
#include "NTRIPSourceTable.h"
#include "QGCLoggingCategory.h"
#include "RuntimeScheduler.h"
#include "ScheduledTask.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIPSourceTableController")

struct NTRIPSourceTableController::FetchAttempt
{
    FetchAttempt(RuntimeScheduler* scheduler, QObject* context)
        : timeout(scheduler, context)
    {}

    QPointer<NTRIPHttpSession> session;
    ScheduledTask timeout;
    QByteArray body;
};

NTRIPSourceTableController::NTRIPSourceTableController(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _model(new NTRIPSourceTableModel(this))
    , _sortedModel(new NTRIPSourceTableSortModel(_model, this))
    , _scheduler(RuntimeScheduler::orDefault(scheduler, this))
{}

NTRIPSourceTableController::~NTRIPSourceTableController()
{
    // Detach the session before child cleanup so its callbacks cannot reach a destroyed controller.
    _abortFetch();
}

QAbstractItemModel* NTRIPSourceTableController::mountpointModel() const
{
    return _sortedModel;
}

void NTRIPSourceTableController::fetch(const NTRIPConnectionConfig& config, const QGeoCoordinate& sortCoord)
{
    auto casterConfig = config;
    casterConfig.mountpoint.clear();
    const bool sameCaster = casterConfig == _lastFetchConfig;
    const QString error = config.validationError();

    if (error.isEmpty() && _attempt && sameCaster) {
        _sortCoord = sortCoord;
        return;
    }

    _abortFetch();
    if (!error.isEmpty()) {
        _onFetchError(error);
        return;
    }

    if (_model->count() > 0 && _cacheStoredAtUs && sameCaster) {
        const auto nowUs = _scheduler->nowUs();
        if (MonotonicClock::fresh(*_cacheStoredAtUs, nowUs, CACHE_TTL)) {
            qCDebug(NTRIPSourceTableControllerLog) << "Source table cache hit, age:"
                                                   << std::chrono::duration_cast<std::chrono::milliseconds>(
                                                          *MonotonicClock::age(*_cacheStoredAtUs, nowUs));
            _sortCoord = sortCoord;
            _model->updateDistances(_sortCoord);
            _setFetchState(FetchStatus::Success, QString());
            return;
        }
    }

    _cacheStoredAtUs.reset();
    _sortCoord = sortCoord;
    const QString previousWarning = securityWarning();
    _lastFetchConfig = casterConfig;

    if (casterConfig.sendsCredentialsInClear()) {
        qCWarning(NTRIPSourceTableControllerLog) << "Sending source-table credentials without TLS";
    }
    _startFetch();
    if (securityWarning() != previousWarning) {
        emit securityWarningChanged();
    }
    // The session reports failures to open asynchronously or, without a TLS backend, already from _startFetch().
    if (_attempt) {
        _setFetchState(FetchStatus::InProgress, QString());
    }
}

void NTRIPSourceTableController::_startFetch()
{
    _attempt = std::make_unique<FetchAttempt>(_scheduler, this);
    auto* session = new NTRIPHttpSession(this, _scheduler, NTRIPHttpPurpose::SourceTable);
    _attempt->session = session;
    // _abortFetch() retires the session, which disconnects it, so these run only for the live attempt.
    connect(session, &NTRIPHttpSession::certificatePinned, this, &NTRIPSourceTableController::certificatePinned);
    connect(session, &NTRIPHttpSession::responseStarted, this, &NTRIPSourceTableController::_armFetchTimeout);
    connect(session, &NTRIPHttpSession::bodyReceived, this, &NTRIPSourceTableController::_readReply);
    connect(session, &NTRIPHttpSession::failed, this,
            [this](const NTRIPFailure& failure) { _finishFetch(failure.detail); });
    connect(session, &NTRIPHttpSession::finished, this,
            [this]() { _finishFetch(tr("Response does not contain a valid source table")); });
    _armFetchTimeout();
    if (const QString openError = session->open(_lastFetchConfig); !openError.isEmpty()) {
        _finishFetch(openError);
    }
}

void NTRIPSourceTableController::_armFetchTimeout()
{
    // An inactivity timeout: a large table on a slow link keeps the fetch alive while bytes arrive.
    _attempt->timeout.schedule(FETCH_TIMEOUT, [this]() {
        // A refusal the caster is still explaining ends on its own deadline.
        if (!_attempt->session || !_attempt->session->awaitingErrorBody()) {
            _finishFetch(tr("Source table request timed out"));
        }
    });
}

void NTRIPSourceTableController::_readReply(const QByteArray& bytes)
{
    _armFetchTimeout();
    if (_attempt->body.size() + bytes.size() >= MAX_SOURCE_TABLE_BYTES) {
        _finishFetch(tr("Source table too large (exceeds %1 MB)").arg(MAX_SOURCE_TABLE_BYTES / (1024 * 1024)));
        return;
    }
    const qsizetype checkedSize = _attempt->body.size();
    _attempt->body += bytes;
    // A complete table ends the fetch before any framing error in the bytes that follow it.
    if (ntripSourceTableComplete(_attempt->body, checkedSize)) {
        _finishFetch();
    }
}

void NTRIPSourceTableController::_finishFetch(const QString& error)
{
    if (!_attempt) {
        return;
    }
    const QByteArray body = _attempt->body;
    _abortFetch();
    if (error.isEmpty()) {
        _onSourceTableReceived(QString::fromUtf8(body));
    } else {
        _onFetchError(error);
    }
}

void NTRIPSourceTableController::_onSourceTableReceived(const QString& table)
{
    _model->parseSourceTable(table, _sortCoord);
    _cacheStoredAtUs = _scheduler->nowUs();
    _setFetchState(FetchStatus::Success, QString());
}

void NTRIPSourceTableController::_onFetchError(const QString& error)
{
    _cacheStoredAtUs.reset();
    _model->clear();
    _setFetchState(FetchStatus::Error, error);
}

void NTRIPSourceTableController::_setFetchState(FetchStatus status, const QString& error)
{
    const bool statusChanged = std::exchange(_fetchStatus, status) != status;
    const bool errorChanged = std::exchange(_fetchError, error) != error;
    if (errorChanged) {
        emit fetchErrorChanged();
    }
    if (statusChanged) {
        emit fetchStatusChanged();
    }
}

void NTRIPSourceTableController::_abortFetch()
{
    if (!_attempt) {
        return;
    }
    // retire() disconnects the session before aborting it, so the abort cannot call back into this controller.
    if (_attempt->session) {
        _attempt->session->retire();
    }
    _attempt.reset();
}

void NTRIPSourceTableController::cancel()
{
    _abortFetch();
    if (_fetchStatus == FetchStatus::InProgress) {
        _setFetchState(FetchStatus::Idle, QString());
    }
}
