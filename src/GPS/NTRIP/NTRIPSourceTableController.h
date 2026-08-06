#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "NTRIPTransportConfig.h"

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableControllerLog)

class NTRIPSourceTableModel;
class NTRIPSourceTableControllerTest;
class QNetworkAccessManager;
class QNetworkReply;

class NTRIPSourceTableController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QtCore/QAbstractListModel")
    Q_PROPERTY(FetchStatus fetchStatus READ fetchStatus NOTIFY fetchStatusChanged)
    Q_PROPERTY(QString fetchError READ fetchError NOTIFY fetchErrorChanged)
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

    QAbstractListModel* mountpointModel() const;

    void fetch(const NTRIPTransportConfig& config, const QGeoCoordinate& sortCoord = {});
    Q_INVOKABLE void selectMountpoint(const QString& mountpoint);

signals:
    void fetchStatusChanged();
    void fetchErrorChanged();
    void mountpointModelChanged();
    /// Emitted when the user picks a mountpoint. The manager/QML layer persists
    /// it to NTRIPSettings — this controller does not write settings directly.
    void mountpointSelected(const QString& mountpoint);

private:
    friend class NTRIPSourceTableControllerTest;

    /// Test seam: drive the reply-processing paths without a live network reply.
    void injectSourceTableForTest(const QString& table);
    void injectFetchErrorForTest(const QString& error);

    void _onReplyFinished();
    void _onSourceTableReceived(const QString& table);
    void _onFetchError(const QString& error);
    void _abortReply();

    NTRIPSourceTableModel* _model = nullptr;
    QNetworkAccessManager* _networkManager = nullptr;
    QNetworkReply* _reply = nullptr;
    bool _replyTooLarge = false;
    QGeoCoordinate _sortCoord;
    FetchStatus _fetchStatus = FetchStatus::Idle;
    QString _fetchError;
    qint64 _fetchedAtMs = 0;

    // Cache key for the most recent fetch — NTRIPTransportConfig::casterIdentity()
    // so it stays in lockstep with the config's own notion of "same caster".
    QString _lastFetchKey;
};
