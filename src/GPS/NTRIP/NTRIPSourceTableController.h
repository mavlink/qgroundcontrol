#pragma once

#include "NTRIPTransportConfig.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableControllerLog)

class NTRIPSourceTableFetcher;
class NTRIPSourceTableModel;
class QmlObjectListModel;

class NTRIPSourceTableController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(FetchStatus fetchStatus READ fetchStatus NOTIFY fetchStatusChanged)
    Q_PROPERTY(QString fetchError READ fetchError NOTIFY fetchErrorChanged)
    Q_PROPERTY(QmlObjectListModel* mountpointModel READ mountpointModel NOTIFY mountpointModelChanged)

public:
    enum class FetchStatus { Idle, InProgress, Success, Error };
    Q_ENUM(FetchStatus)

    static constexpr int kCacheTtlMs = 60000;

    explicit NTRIPSourceTableController(QObject *parent = nullptr);

    FetchStatus fetchStatus() const { return _fetchStatus; }
    QString fetchError() const { return _fetchError; }
    QmlObjectListModel *mountpointModel() const;

    void fetch(const NTRIPTransportConfig &config);
    void fetch(const QString &host, int port, const QString &username,
               const QString &password, bool useTls);
    Q_INVOKABLE void selectMountpoint(const QString &mountpoint);

signals:
    void fetchStatusChanged();
    void fetchErrorChanged();
    void mountpointModelChanged();

private:
    NTRIPSourceTableModel *_model = nullptr;
    NTRIPSourceTableFetcher *_fetcher = nullptr;
    FetchStatus _fetchStatus = FetchStatus::Idle;
    QString _fetchError;
    qint64 _fetchedAtMs = 0;

    // Cache key for the most recent fetch — compared via operator== to decide
    // if a cached source table is still reusable. Previously this was four
    // flat fields that duplicated NTRIPTransportConfig's identity;
    // storing the config directly keeps one definition of "same caster".
    NTRIPTransportConfig _lastFetchConfig;
};
