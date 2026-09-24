#pragma once

#include <functional>
#include <memory>
#include <optional>

#include <QtCore/QAbstractListModel>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtPositioning/QGeoCoordinate>

#include "GPSNotificationQueue.h"
#include "NTRIPConfiguration.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableControllerLog)

class NTRIPSourceTableModel;
class NTRIPSourceTableControllerTest;
class QTcpSocket;

/// Fetches caster source tables over the same HTTP request builder and decoder as the correction
/// stream, so HTTP/1.x and NTRIP v1 "SOURCETABLE 200 OK" responses share one path.
class NTRIPSourceTableController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(FetchStatus fetchStatus READ fetchStatus NOTIFY fetchStatusChanged)
    Q_PROPERTY(QString fetchError READ fetchError NOTIFY fetchErrorChanged)
    Q_PROPERTY(QString securityWarning READ securityWarning NOTIFY securityWarningChanged)
    Q_PROPERTY(QAbstractListModel* mountpointModel READ mountpointModel NOTIFY mountpointModelChanged)

public:
    enum class FetchStatus
    {
        Idle,
        InProgress,
        Success,
        Error
    };
    Q_ENUM(FetchStatus)

    static constexpr int kCacheTtlMs = 60000;
    static constexpr int kFetchTimeoutMs = 10000;
    static constexpr qint64 kMaxSourceTableBytes = 8 * 1024 * 1024;

    explicit NTRIPSourceTableController(QObject* parent = nullptr);
    ~NTRIPSourceTableController() override;

    FetchStatus fetchStatus() const { return _fetchStatus; }

    QString fetchError() const { return _fetchError; }

    /// Set while the latest fetch sends caster credentials without TLS.
    QString securityWarning() const { return _securityWarning; }

    QAbstractListModel* mountpointModel() const;

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
    friend class NTRIPReentrancyTest;

    /// Test seam: drive the reply-processing paths without a live network reply.
    void injectSourceTableForTest(const QString& table);
    void injectFetchErrorForTest(const QString& error);

    void _onSourceTableReceived(const QString& table);
    void _onFetchError(const QString& error);
    void _completeFetch(quint64 revision, QString table, std::optional<QString> error = std::nullopt);
    void _abortFetch();
    QPointer<QTcpSocket> _activeSocket() const;
    QByteArray _activeRequest() const;
    void _startFetch(quint64 revision, const QByteArray& request);
    void _readReply(quint64 revision);
    void _finishFetch(const QString& error = {});
    void _setSecurityWarning(const QString& warning);
    bool _deferModelMutation(std::function<void()> action);

    NTRIPSourceTableModel* _model = nullptr;
    struct FetchAttempt;
    std::unique_ptr<FetchAttempt> _attempt;
    QGeoCoordinate _sortCoord;
    FetchStatus _fetchStatus = FetchStatus::Idle;
    QString _fetchError;
    QString _securityWarning;
    QElapsedTimer _cacheAge;
    quint64 _fetchRevision = 0;

    NTRIPConnectionConfig _lastFetchConfig;  ///< Mountpoint is excluded from source-table identity.
    GPSNotificationQueue _notifications{this};
};
