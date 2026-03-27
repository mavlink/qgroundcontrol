#pragma once

#include "GPSEvent.h"
#include "GPSEventModel.h"
#include "GPSRTKFactGroup.h"

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>

Q_DECLARE_LOGGING_CATEGORY(GPSManagerLog)

class FactGroup;
class GPSRtk;
class QGeoPositionInfoSource;
class QGeoSatelliteInfoSource;
class QmlObjectListModel;
class RTCMMavlink;
class SatelliteModel;

class GPSManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_MOC_INCLUDE("RTCMMavlink.h")
    Q_PROPERTY(int deviceCount READ deviceCount NOTIFY deviceCountChanged)
    Q_PROPERTY(QmlObjectListModel* devices READ devices CONSTANT)
    Q_PROPERTY(RTCMMavlink* rtcmMavlink READ rtcmMavlink CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    static constexpr int kMaxEventLogSize = 50;

    // Constructor is public because Q_APPLICATION_STATIC requires a publicly constructible type.
    // Do not construct directly — use GPSManager::instance().
    explicit GPSManager(QObject *parent = nullptr);
    ~GPSManager() override;

    static GPSManager *instance();

    Q_INVOKABLE void connectGPS(const QString &device, const QString &gpsType);
    Q_INVOKABLE void disconnectGPS(const QString &device);
    Q_INVOKABLE void disconnectAll();

    bool connected() const;
    bool isDeviceRegistered(const QString &device) const;
    int deviceCount() const;
    QmlObjectListModel *devices() const { return _devices; }
    QString lastError() const { return _lastError; }

    GPSRtk *gpsRtk(const QString &device = {}) const;

    FactGroup *gpsRtkFactGroup();
    SatelliteModel *rtkSatelliteModel() const;
    QGeoPositionInfoSource *gpsPositionSource() const;
    QGeoSatelliteInfoSource *gpsSatelliteInfoSource() const;
    RTCMMavlink *rtcmMavlink() const { return _rtcmMavlink; }
    GPSEventModel *eventModel() { return &_eventModel; }

    // Public: called by PositionManager and NTRIPManager event-logger callback in addition to GPS internals.
    void logEvent(const GPSEvent &event);

signals:
    void eventLogged(const GPSEvent &event);
    void deviceCountChanged();
    void lastErrorChanged();
    void deviceConnected(const QString &device);
    void deviceDisconnected(const QString &device);
    void positionSourceChanged();

private:
    GPSRtk *_primaryDevice() const;

    QmlObjectListModel *_devices = nullptr;
    // QPointer rather than raw pointer: disconnectGPS calls deleteLater() for async safety,
    // so QPointer guards against use-after-free if the object is destroyed before map removal.
    QHash<QString, QPointer<GPSRtk>> _deviceMap;
    QString _mostRecentDevice;

    RTCMMavlink *_rtcmMavlink = nullptr;
    GPSEventModel _eventModel{this};
    QString _lastError;
    // Parented via brace-init so QML/JavaScript ownership tracking sees a proper parent chain
    // and does not attempt to reclaim this stack-member FactGroup.
    GPSRTKFactGroup _defaultFactGroup{this};
};
