#pragma once

#include "GPSEvent.h"
#include "GPSEventModel.h"

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringView>

Q_DECLARE_LOGGING_CATEGORY(GPSManagerLog)

class FactGroup;
class GPSPositionSource;
class GPSRtk;
class QGeoPositionInfoSource;
class QmlObjectListModel;
class RTCMMavlink;
class RTCMRouter;
class RTKSatelliteModel;

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

    explicit GPSManager(QObject *parent = nullptr);
    ~GPSManager() override;

    static GPSManager *instance();

    Q_INVOKABLE void connectGPS(const QString &device, const QString &gpsType);
    Q_INVOKABLE void disconnectGPS(const QString &device);
    Q_INVOKABLE void disconnectAll();

    bool connected() const;
    bool isDeviceConnected(const QString &device) const;
    int deviceCount() const;
    QmlObjectListModel *devices() { return _devices; }
    QString lastError() const { return _lastError; }

    GPSRtk *gpsRtk(const QString &device = {}) const;

    FactGroup *gpsRtkFactGroup();
    RTKSatelliteModel *rtkSatelliteModel();
    QGeoPositionInfoSource *gpsPositionSource();
    RTCMRouter *rtcmRouter() { return _rtcmRouter; }
    RTCMMavlink *rtcmMavlink() { return _rtcmMavlink; }
    GPSEventModel *eventModel() { return &_eventModel; }

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
    QHash<QString, GPSRtk*> _deviceMap;

    RTCMMavlink *_rtcmMavlink = nullptr;
    RTCMRouter *_rtcmRouter = nullptr;
    GPSEventModel _eventModel{this};
    QString _lastError;
};
