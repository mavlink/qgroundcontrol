#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>

#include "NTRIPConfiguration.h"
#include "NotificationQueue.h"
#include "OperationRevision.h"
#include "ScheduledTask.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableControllerLog)

class NTRIPHttpSession;
class NTRIPSourceTableModel;
class NTRIPSourceTableSortModel;
class NTRIPSourceTableControllerTest;
class RuntimeScheduler;

/// Fetches caster source tables over the same HTTP request builder and decoder as the correction
/// stream, so HTTP/1.x and NTRIP v1 "SOURCETABLE 200 OK" responses share one path.
class NTRIPSourceTableController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FetchStatus fetchStatus READ fetchStatus NOTIFY fetchStatusChanged FINAL)
    Q_PROPERTY(QString fetchError READ fetchError NOTIFY fetchErrorChanged FINAL)
    Q_PROPERTY(QString securityWarning READ securityWarning NOTIFY securityWarningChanged FINAL)
    Q_PROPERTY(QAbstractItemModel* mountpointModel READ mountpointModel NOTIFY mountpointModelChanged FINAL)

public:
    enum class FetchStatus
    {
        Idle,
        InProgress,
        Success,
        Error
    };
    Q_ENUM(FetchStatus)

    static constexpr std::chrono::milliseconds kCacheTtl{60000};
    static constexpr std::chrono::milliseconds kFetchTimeout{10000};
    static constexpr qint64 kMaxSourceTableBytes = 8 * 1024 * 1024;

    explicit NTRIPSourceTableController(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NTRIPSourceTableController() override;

    FetchStatus fetchStatus() const { return _fetchStatus; }

    QString fetchError() const { return _fetchError; }

    /// Set while the latest fetch sends caster credentials without TLS.
    QString securityWarning() const { return _securityWarning; }

    QAbstractItemModel* mountpointModel() const;

    void fetch(const NTRIPConnectionConfig& config, const QGeoCoordinate& sortCoord = {});
    void cancel();
    Q_INVOKABLE void selectMountpoint(const QString& mountpoint);

signals:
    void fetchStatusChanged();
    void fetchErrorChanged();
    void securityWarningChanged();
    void mountpointModelChanged();
    /// Emitted when the user picks a mountpoint. The manager/QML layer persists
    /// it to NTRIPSettings — this controller does not write settings directly.
    void mountpointSelected(const QString& mountpoint);

private:
    friend class NTRIPSourceTableControllerTest;

    /// Test seam: drive the reply-processing paths without a live network reply.
    void injectSourceTableForTest(const QString& table);
    void injectFetchErrorForTest(const QString& error);

    void _onSourceTableReceived(const QString& table);
    void _onFetchError(const QString& error);
    void _completeFetch(const OperationRevision::Token& fetch, QString table,
                        std::optional<QString> error = std::nullopt);
    void _abortFetch();
    QPointer<NTRIPHttpSession> _activeSession() const;
    QByteArray _activeRequest() const;
    void _startFetch(const OperationRevision::Token& fetch, const QByteArray& request);
    void _readReply(const QByteArray& bytes);
    void _finishFetch(const QString& error = {});
    void _setSecurityWarning(const QString& warning);

    NTRIPSourceTableModel* _model = nullptr;
    NTRIPSourceTableSortModel* _sortedModel = nullptr;
    struct FetchAttempt;
    std::unique_ptr<FetchAttempt> _attempt;
    RuntimeScheduler* const _scheduler;
    QGeoCoordinate _sortCoord;
    FetchStatus _fetchStatus = FetchStatus::Idle;
    QString _fetchError;
    QString _securityWarning;
    std::optional<quint64> _cacheStoredAtUs;
    OperationRevision _fetchRevision;

    NTRIPConnectionConfig _lastFetchConfig;  ///< Mountpoint is excluded from source-table identity.
    NotificationQueue _notifications{this};
};
