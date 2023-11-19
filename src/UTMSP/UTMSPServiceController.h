/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "qelapsedtimer.h"
#include "qgeocoordinate.h"
#include <QGCMAVLink.h>
#include <QObject>

#include "UTMSPAuthorization.h"
#include "UTMSPFlightPlanManager.h"
#include "UTMSPNetworkRemoteIDManager.h"

class UTMSPServiceController : public QObject, public UTMSPFlightPlanManager, public UTMSPNetworkRemoteIDManager
{
    Q_OBJECT

public:
    UTMSPServiceController(QObject *parent = nullptr);
    ~UTMSPServiceController();

    Q_PROPERTY(bool                 responseFlag            READ responseFlag       NOTIFY responseFlagChanged)
    Q_PROPERTY(QString              responseFlightID        READ responseFlightID   NOTIFY responseFlightIDChanged)
    Q_PROPERTY(QString              responseJson            READ responseJson       NOTIFY responseJsonChanged)
    Q_PROPERTY(bool                 activationFlag          READ activationFlag     NOTIFY activationFlagChanged)

    std::string flightPlanAuthorization();
    bool networkRemoteID(const mavlink_message_t& message,
                         const std::string& serialNumber,
                         const std::string& operatorID,
                         const std::string& flightID);

    bool                responseFlag    () const {return _responseFlag;};
    QString             responseFlightID() const {return _responseFlightID;};
    QString             responseJson    () const {return _responseJson;};
    bool                activationFlag  () const {return _activationFlag;};

signals:
    void responseFlagChanged                     (void);
    void responseFlightIDChanged                 (void);
    void responseJsonChanged                     (void);
    void activationFlagChanged                   (void);
    void stopTelemetryFlagChanged                (bool value);

public slots:
    void updatePolygonBoundary (const QList<QGeoCoordinate> &boundary)                   {_boundaryPolygon = boundary;};
    void updateStartDateTime   (const QString &startDateTime)                            {_startDateTime = startDateTime.toStdString();};
    void updateEndDateTime     (const QString &endDateTime)                              {_endDateTime = endDateTime.toStdString();};
    void updateMinAltitude     (const int &minAltitude)                                  {_minAltitude = minAltitude;};
    void updateMaxAltitude     (const int &maxAltitude)                                  {_minAltitude = maxAltitude;};
    void updateLastCoordinates (const double &lastLatitude, const double &lastLongitude) {_lastLatitude = lastLatitude;_lastLongitude = lastLongitude;};

private:
    UTMSPAuthorization                    _utmspAuth;
    UTMSPFlightPlanManager::FlightState   _currentState;
    float                                 _lastHdop = 1.f;
    QElapsedTimer                         _timerLastSent;
    double                                _vehicleLatitude;
    double                                _vehicleLongitude;
    double                                _vehicleAltitude;
    double                                _vehicleHeading;
    double                                _vehicleVelocityX;
    double                                _vehicleVelocityY;
    double                                _vehicleVelocityZ;
    double                                _vehicleRelativeAltitude;
    json                                  _coordinateList;
    bool                                  _streamingFlag;

    static QList<QGeoCoordinate> _boundaryPolygon;
    static std::string           _startDateTime;
    static std::string           _endDateTime;
    static int                   _minAltitude;
    static int                   _maxAltitude;
    static bool                  _responseFlag;
    static QString               _responseFlightID;
    static QString               _responseJson;
    static bool                  _uploadFlag;
    static bool                  _activationFlag;
    static std::string           _clientID;
    static std::string           _clientPassword;
    static std::string           _blenderToken;
    static double                _lastLatitude;
    static double                _lastLongitude;
    static double                _lastAltitude;
};
