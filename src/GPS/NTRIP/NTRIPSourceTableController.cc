#include "NTRIPSourceTableController.h"

#include <chrono>
#include <utility>

#include <QtCore/QDateTime>

#include "MonotonicClock.h"
#include "NTRIPHttpCodec.h"
#include "NTRIPHttpSession.h"
#include "NTRIPSourceTable.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIP.NTRIPSourceTableController")

struct NTRIPSourceTableController::FetchAttempt
{
    FetchAttempt(RuntimeScheduler* scheduler, QObject* context)
        : timeout(scheduler, context)
    {}

    QPointer<NTRIPHttpSession> session;
    ScheduledTask timeout;
    NTRIPHttpDecoder decoder{NTRIPHttpDecoder::Purpose::SourceTable};
    QByteArray request;
    QByteArray body;
};

NTRIPSourceTableController::NTRIPSourceTableController(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _model(new NTRIPSourceTableModel(this))
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
{}

NTRIPSourceTableController::~NTRIPSourceTableController()
{
    _notifications.close();
    // Detach the session before child cleanup so its callbacks cannot reach a destroyed controller.
    _abortFetch();
}

QAbstractItemModel* NTRIPSourceTableController::mountpointModel() const
{
    return _model;
}

void NTRIPSourceTableController::fetch(const NTRIPConnectionConfig& config, const QGeoCoordinate& sortCoord)
{
    if (_deferModelMutation([this, config, sortCoord]() { fetch(config, sortCoord); })) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    auto casterConfig = config;
    casterConfig.mountpoint.clear();
    const bool sameCaster = casterConfig == _lastFetchConfig;
    const QString invalid = config.validationError();

    if (invalid.isEmpty() && _activeSession() && _fetchStatus == FetchStatus::InProgress && sameCaster) {
        _sortCoord = sortCoord;
        return;
    }

    const auto fetch = _fetchRevision.advance(this);
    _abortFetch();
    if (!fetch.isCurrent()) {
        return;
    }
    if (!invalid.isEmpty()) {
        _completeFetch(fetch, {}, invalid);
        return;
    }

    if (_model->count() > 0 && _cacheStoredAtUs && sameCaster) {
        const auto nowUs = _scheduler->nowUs();
        if (MonotonicClock::remaining(*_cacheStoredAtUs, nowUs, std::chrono::milliseconds(kCacheTtlMs)) >
            std::chrono::microseconds::zero()) {
            const qint64 age = MonotonicClock::ageMilliseconds(*_cacheStoredAtUs, nowUs);
            qCDebug(NTRIPSourceTableControllerLog) << "Source table cache hit, age:" << age << "ms";
            _sortCoord = sortCoord;
            _model->updateDistances(_sortCoord);
            if (!fetch.isCurrent()) {
                return;
            }
            _fetchStatus = FetchStatus::Success;
            _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
            return;
        }
    }

    _cacheStoredAtUs.reset();
    _sortCoord = sortCoord;
    _lastFetchConfig = casterConfig;
    _fetchStatus = FetchStatus::InProgress;
    _fetchError.clear();

    const auto request = NTRIPHttpRequest::build(config, NTRIPHttpRequest::Purpose::SourceTable);
    if (!request.error.isEmpty()) {
        _completeFetch(fetch, {}, request.error);
        return;
    }
    if (request.credentialsInClear) {
        qCWarning(NTRIPSourceTableControllerLog) << "Sending source-table credentials without TLS";
    }
    _setSecurityWarning(request.credentialsInClear ? tr("Credentials are being sent without TLS encryption.")
                                                   : QString());
    _startFetch(fetch, request.bytes);
    if (fetch.isCurrent() && _activeSession()) {
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchErrorChanged);
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
    }
}

void NTRIPSourceTableController::_startFetch(const GPSRevision::Token& fetch, const QByteArray& request)
{
    _attempt = std::make_unique<FetchAttempt>(_scheduler, this);
    _attempt->request = request;
    auto* session = new NTRIPHttpSession(this);
    _attempt->session = session;
    const QPointer<NTRIPHttpSession> guard(session);
    const auto current = [this, guard, fetch]() { return fetch.isCurrent() && guard && _activeSession() == guard; };
    connect(session, &NTRIPHttpSession::established, this, [this, current]() {
        if (current() && !_activeSession()->write(_attempt->request) && current()) {
            _finishFetch(tr("Failed to send source table request"));
        }
    });
    connect(session, &NTRIPHttpSession::bytesReceived, this, [this, current](const QByteArray& bytes) {
        if (current()) {
            _readReply(bytes);
        }
    });
    connect(session, &NTRIPHttpSession::failed, this, [this, current](NTRIPError, const QString& message) {
        if (current()) {
            _finishFetch(message);
        }
    });
    connect(session, &NTRIPHttpSession::closed, this, [this, current]() {
        if (current()) {
            _finishFetch(tr("Response does not contain a valid source table"));
        }
    });
    connect(session, &QObject::destroyed, this, [this, fetch, attempt = _attempt.get()]() {
        if (fetch.isCurrent() && _attempt.get() == attempt) {
            _completeFetch(fetch, {}, tr("Source table connection was destroyed before completion"));
        }
    });
    _attempt->timeout.schedule(std::chrono::milliseconds(kFetchTimeoutMs), [this, current]() {
        if (current()) {
            _finishFetch(tr("Source table request timed out"));
        }
    });
    session->open(_lastFetchConfig);
}

void NTRIPSourceTableController::_readReply(const QByteArray& bytes)
{
    const auto result = _attempt->decoder.feed(bytes, QDateTime::currentDateTimeUtc());
    if (result.failure) {
        _finishFetch(result.failure->detail);
        return;
    }
    if (_attempt->body.size() + result.body.size() >= kMaxSourceTableBytes) {
        _finishFetch(tr("Source table too large (exceeds %1 MB)").arg(kMaxSourceTableBytes / (1024 * 1024)));
        return;
    }
    _attempt->body += result.body;
    if (ntripSourceTableComplete(_attempt->body)) {
        _finishFetch();
    } else if (result.complete) {
        _finishFetch(tr("Response does not contain a valid source table"));
    }
}

void NTRIPSourceTableController::_finishFetch(const QString& error)
{
    if (!_activeSession()) {
        return;
    }
    _completeFetch(_fetchRevision.current(this), QString::fromUtf8(_attempt->body),
                   error.isEmpty() ? std::nullopt : std::optional(error));
}

void NTRIPSourceTableController::_setSecurityWarning(const QString& warning)
{
    if (_securityWarning == warning) {
        return;
    }
    _securityWarning = warning;
    _notifications.emitSignal(this, &NTRIPSourceTableController::securityWarningChanged);
}

void NTRIPSourceTableController::_completeFetch(const GPSRevision::Token& fetch, QString table,
                                                std::optional<QString> error)
{
    if (!fetch.isCurrent()) {
        return;
    }
    const GPSNotificationQueue::Scope publish(_notifications);
    _abortFetch();
    if (!fetch.isCurrent()) {
        return;
    }
    if (error) {
        _onFetchError(*error);
    } else {
        _onSourceTableReceived(table);
    }
}

void NTRIPSourceTableController::_onSourceTableReceived(const QString& table)
{
    if (_deferModelMutation([this, table]() { _onSourceTableReceived(table); })) {
        return;
    }
    const auto fetch = _fetchRevision.current(this);
    _model->parseSourceTable(table, _sortCoord);
    if (!fetch.isCurrent()) {
        return;
    }
    _cacheStoredAtUs = _scheduler->nowUs();
    _fetchStatus = FetchStatus::Success;
    _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
    _notifications.emitSignal(this, &NTRIPSourceTableController::mountpointModelChanged);
}

void NTRIPSourceTableController::_onFetchError(const QString& error)
{
    if (_deferModelMutation([this, error]() { _onFetchError(error); })) {
        return;
    }
    const auto fetch = _fetchRevision.current(this);
    _cacheStoredAtUs.reset();
    _fetchError = error;
    _fetchStatus = FetchStatus::Error;
    _model->clear();
    if (!fetch.isCurrent()) {
        return;
    }
    _notifications.emitSignal(this, &NTRIPSourceTableController::fetchErrorChanged);
    _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
}

bool NTRIPSourceTableController::_deferModelMutation(std::function<void()> action)
{
    if (!_model->_mutating) {
        return false;
    }
    // A reset observer can replace this fetch. Retire its publication now, but
    // defer the replacement (including status signals) until the model is stable.
    QMetaObject::invokeMethod(
        this,
        [fetch = _fetchRevision.advance(this), action = std::move(action)]() {
            if (fetch.isCurrent()) {
                action();
            }
        },
        Qt::QueuedConnection);
    return true;
}

void NTRIPSourceTableController::_abortFetch()
{
    // Detach every attempt resource before abort callbacks can start another fetch or delete us.
    const auto attempt = std::move(_attempt);
    if (!attempt) {
        return;
    }
    attempt->timeout.cancel();
    if (const auto session = attempt->session) {
        session->retire();
    }
}

QPointer<NTRIPHttpSession> NTRIPSourceTableController::_activeSession() const
{
    return _attempt ? _attempt->session : QPointer<NTRIPHttpSession>();
}

QByteArray NTRIPSourceTableController::_activeRequest() const
{
    return _attempt ? _attempt->request : QByteArray();
}

void NTRIPSourceTableController::cancel()
{
    const auto fetch = _fetchRevision.advance(this);
    _abortFetch();
    if (fetch.isCurrent() && _fetchStatus == FetchStatus::InProgress) {
        _fetchStatus = FetchStatus::Idle;
        _notifications.emitSignal(this, &NTRIPSourceTableController::fetchStatusChanged);
    }
}

void NTRIPSourceTableController::injectSourceTableForTest(const QString& table)
{
    _completeFetch(_fetchRevision.advance(this), table);
}

void NTRIPSourceTableController::injectFetchErrorForTest(const QString& error)
{
    _completeFetch(_fetchRevision.advance(this), {}, error);
}

void NTRIPSourceTableController::selectMountpoint(const QString& mountpoint)
{
    emit mountpointSelected(mountpoint);
}
