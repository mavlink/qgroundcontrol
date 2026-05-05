#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>
#include <cstdint>

#include "OnboardLogTransport.h"

class FtpTransport;
class LogProtocolTransport;
class OnboardLogSortFilterModel;
class QAbstractItemModel;
class OnboardLogModel;
class Vehicle;

/// QML facade picking LOG-protocol vs MAVLink-FTP transport per active vehicle.
class OnboardLogController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_MOC_INCLUDE("Vehicle.h")
    Q_MOC_INCLUDE("QAbstractItemModel")
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT FINAL)
    Q_PROPERTY(bool downloadingLogs READ downloadingLogs NOTIFY downloadingLogsChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(TransportKind transportKind READ transportKind NOTIFY transportKindChanged FINAL)
    Q_PROPERTY(qreal batchProgress READ batchProgress NOTIFY batchProgressChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(bool allLogsSelected READ allLogsSelected NOTIFY selectionChanged FINAL)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged FINAL)

#ifdef QGC_UNITTEST_BUILD
    friend class OnboardLogDownloadTest;
#endif

public:
    enum class TransportKind : uint8_t
    {
        LogProtocol,
        MavlinkFtp,
    };
    Q_ENUM(TransportKind)

    explicit OnboardLogController(QObject* parent = nullptr);
    ~OnboardLogController() override;

    QAbstractItemModel* model() const;
    bool downloadingLogs() const;

    bool busy() const { return _active && _active->busy(); }

    TransportKind transportKind() const;
    qreal batchProgress() const;
    QString errorMessage() const;
    bool allLogsSelected() const;

    bool sortAscending() const { return _sortAscending; }

    Q_INVOKABLE void ensureLoaded();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void download(const QString& path = QString());
    Q_INVOKABLE bool eraseAllForVehicle(Vehicle* expectedVehicle);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void selectAll(bool select);
    Q_INVOKABLE int selectedCount() const;
    Q_INVOKABLE void toggleSortByDate();

    void setSortAscending(bool ascending);

signals:
    void downloadingLogsChanged();
    void busyChanged();
    void transportKindChanged();
    void batchProgressChanged();
    void errorMessageChanged();
    void selectionChanged();
    void sortAscendingChanged();

private slots:
    void _setActiveVehicle(Vehicle* vehicle);
    void _reevaluateTransport();
    void _activeRequestingListChanged();
    void _activeListingFinished(OnboardLogTransport::ListingResult result);

private:
    enum class LoadState : uint8_t
    {
        NotLoaded,
        Loading,
        Loaded,
        Failed,
    };

    static bool _shouldAutoSelectFtp(bool firmwareAllowsFtp, bool capabilitiesKnown, uint64_t capabilityBits);
    void _invalidateLoadState();
    void _setActiveTransport(OnboardLogTransport* transport);
    OnboardLogModel* sourceModel() const;

    LogProtocolTransport* _logTransport = nullptr;
    FtpTransport* _ftpTransport = nullptr;
    OnboardLogSortFilterModel* _sortModel = nullptr;
    OnboardLogTransport* _active = nullptr;
    QPointer<Vehicle> _vehicle;
    QPointer<Vehicle> _pendingVehicle;
    QPointer<Vehicle> _loadVehicle;
    QPointer<OnboardLogTransport> _loadTransport;
    LoadState _loadState = LoadState::NotLoaded;
    bool _changingVehicle = false;
    bool _vehicleChangePending = false;
    bool _sortAscending = false;
};
