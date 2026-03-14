#include "NTRIPSourceTableController.h"
#include "NTRIPSettings.h"
#include "NTRIPSourceTable.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"

#include <QtCore/QDateTime>

QGC_LOGGING_CATEGORY(NTRIPSourceTableControllerLog, "GPS.NTRIPSourceTableController")

NTRIPSourceTableController::NTRIPSourceTableController(QObject *parent)
    : QObject(parent)
{
}

QmlObjectListModel *NTRIPSourceTableController::mountpointModel() const
{
    return _model ? _model->mountpoints() : nullptr;
}

void NTRIPSourceTableController::fetch(const NTRIPTransportConfig &config)
{
    fetch(config.host, config.port, config.username, config.password, config.useTls);
}

void NTRIPSourceTableController::fetch(const QString &host, int port,
                                        const QString &username, const QString &password,
                                        bool useTls)
{
    if (_fetchStatus == FetchStatus::InProgress) {
        return;
    }

    if (_model && _model->count() > 0 && _fetchedAtMs > 0) {
        const qint64 age = QDateTime::currentMSecsSinceEpoch() - _fetchedAtMs;
        if (age < kCacheTtlMs) {
            qCDebug(NTRIPSourceTableControllerLog) << "Source table cache hit, age:" << age << "ms";
            _fetchStatus = FetchStatus::Success;
            emit fetchStatusChanged();
            return;
        }
    }

    if (host.isEmpty()) {
        _fetchError = tr("Host address is empty");
        _fetchStatus = FetchStatus::Error;
        emit fetchErrorChanged();
        emit fetchStatusChanged();
        return;
    }

    if (!_model) {
        _model = new NTRIPSourceTableModel(this);
        emit mountpointModelChanged();
    }

    if (_fetcher) {
        _fetcher->deleteLater();
        _fetcher = nullptr;
    }

    _fetchStatus = FetchStatus::InProgress;
    _fetchError.clear();
    emit fetchStatusChanged();

    _fetcher = new NTRIPSourceTableFetcher(host, port, username, password, useTls, this);

    connect(_fetcher, &NTRIPSourceTableFetcher::sourceTableReceived, this, [this](const QString &table) {
        _model->parseSourceTable(table);
        _fetchedAtMs = QDateTime::currentMSecsSinceEpoch();

        MultiVehicleManager *mvm = MultiVehicleManager::instance();
        if (mvm && mvm->activeVehicle()) {
            QGeoCoordinate coord = mvm->activeVehicle()->coordinate();
            if (coord.isValid()) {
                _model->updateDistances(coord);
                _model->sortByDistance();
            }
        }

        _fetchStatus = FetchStatus::Success;
        emit fetchStatusChanged();
        emit mountpointModelChanged();
    });

    connect(_fetcher, &NTRIPSourceTableFetcher::error, this, [this](const QString &err) {
        _fetchError = err;
        _fetchStatus = FetchStatus::Error;
        emit fetchErrorChanged();
        emit fetchStatusChanged();
    });

    connect(_fetcher, &NTRIPSourceTableFetcher::finished, this, [this]() {
        _fetcher->deleteLater();
        _fetcher = nullptr;
    });

    _fetcher->fetch();
}

void NTRIPSourceTableController::selectMountpoint(const QString &mountpoint)
{
    NTRIPSettings *settings = SettingsManager::instance()->ntripSettings();
    if (settings && settings->ntripMountpoint()) {
        settings->ntripMountpoint()->setRawValue(mountpoint);
    }
}
