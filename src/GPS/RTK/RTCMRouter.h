#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>

Q_DECLARE_LOGGING_CATEGORY(RTCMRouterLog)

class RTCMMavlink;
class GPSRtk;

class RTCMRouter : public QObject
{
    Q_OBJECT

public:
    explicit RTCMRouter(RTCMMavlink *mavlink, QObject *parent = nullptr);

    void setGpsRtk(GPSRtk *gpsRtk);
    void addGpsRtk(GPSRtk *gpsRtk);
    void removeGpsRtk(GPSRtk *gpsRtk);

public slots:
    void routeToVehicles(const QByteArray &data);
    void routeToBaseStation(const QByteArray &data);
    void routeAll(const QByteArray &data);

private:
    RTCMMavlink *_mavlink = nullptr;
    QList<QPointer<GPSRtk>> _gpsDevices;
};
