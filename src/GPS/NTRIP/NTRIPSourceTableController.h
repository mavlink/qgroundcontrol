#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "NTRIPConfiguration.h"

class NTRIPSourceTableModel;
class NTRIPSourceTableSortModel;
class RuntimeScheduler;

/// Fetches caster source tables over the same HTTP request builder and decoder as the correction
/// stream, so HTTP/1.x and NTRIP v1 "SOURCETABLE 200 OK" responses share one path.
class NTRIPSourceTableController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(FetchStatus fetchStatus READ fetchStatus NOTIFY fetchStatusChanged FINAL)
    Q_PROPERTY(QString fetchError READ fetchError NOTIFY fetchErrorChanged FINAL)
    Q_PROPERTY(QString securityWarning READ securityWarning NOTIFY securityWarningChanged FINAL)
    Q_PROPERTY(QAbstractItemModel* mountpointModel READ mountpointModel CONSTANT FINAL)

public:
    enum class FetchStatus
    {
        Idle,
        InProgress,
        Success,
        Error
    };
    Q_ENUM(FetchStatus)

    static constexpr std::chrono::milliseconds CACHE_TTL{60000};
    static constexpr std::chrono::milliseconds FETCH_TIMEOUT{10000};
    static constexpr qint64 MAX_SOURCE_TABLE_BYTES = 8 * 1024 * 1024;

    explicit NTRIPSourceTableController(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~NTRIPSourceTableController() override;

    FetchStatus fetchStatus() const { return _fetchStatus; }

    QString fetchError() const { return _fetchError; }

    /// Set while the latest fetch sends caster credentials without TLS.
    QString securityWarning() const { return _lastFetchConfig.credentialsInClearWarning(); }

    QAbstractItemModel* mountpointModel() const;

    void fetch(const NTRIPConnectionConfig& config, const QGeoCoordinate& sortCoord = {});
    void cancel();

signals:
    void fetchStatusChanged();
    void fetchErrorChanged();
    void securityWarningChanged();
    /// A self-signed caster certificate was trusted on first use; the owner persists the pin.
    void certificatePinned(const QString& pin);

private:
    friend class NTRIPSourceTableControllerTest;

    void _onSourceTableReceived(const QString& table);
    void _onFetchError(const QString& error);
    /// Commits the fetch outcome, then emits the change signals.
    void _setFetchState(FetchStatus status, const QString& error);
    void _abortFetch();
    void _startFetch();
    void _armFetchTimeout();
    void _readReply(const QByteArray& bytes);
    void _finishFetch(const QString& error = {});

    NTRIPSourceTableModel* _model = nullptr;
    NTRIPSourceTableSortModel* _sortedModel = nullptr;
    struct FetchAttempt;
    /// The fetch in progress; null otherwise.
    std::unique_ptr<FetchAttempt> _attempt;
    RuntimeScheduler* const _scheduler;
    QGeoCoordinate _sortCoord;
    FetchStatus _fetchStatus = FetchStatus::Idle;
    QString _fetchError;
    std::optional<quint64> _cacheStoredAtUs;

    NTRIPConnectionConfig _lastFetchConfig;  ///< Mountpoint is excluded from source-table identity.
};
