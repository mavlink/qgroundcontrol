#pragma once

#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtPositioning/QGeoCoordinate>

class Fact;
class FactGroup;
class NTRIPTransport;

class NTRIPGgaProvider : public QObject
{
    Q_OBJECT

public:
    enum class PositionSource {
        Auto        = 0,
        VehicleGPS  = 1,
        VehicleEKF  = 2,
        RTKBase     = 3,
        GCSPosition = 4
    };
    Q_ENUM(PositionSource)

    explicit NTRIPGgaProvider(QObject* parent = nullptr);

    void start(NTRIPTransport* transport);
    void stop();
    void setPositionAcquired();

    QString currentSource() const { return _source; }

    static QByteArray makeGGA(const QGeoCoordinate& coord, double altitude_msl);

signals:
    void sourceChanged(const QString& source);

private:
    void _sendGGA();

    QPair<QGeoCoordinate, QString> _getBestPosition() const;
    QPair<QGeoCoordinate, QString> _getVehicleGPSPosition() const;
    QPair<QGeoCoordinate, QString> _getVehicleEKFPosition() const;
    QPair<QGeoCoordinate, QString> _getRTKBasePosition() const;
    QPair<QGeoCoordinate, QString> _getGCSPosition() const;

    QPointer<NTRIPTransport> _transport;
    QTimer _timer;
    QString _source;
};
