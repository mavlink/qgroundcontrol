#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

#include "MAVLinkEnums.h"
#include "PlanManager.h"

class Vehicle;

/// A rally point as exchanged with the vehicle
struct RallyPointData
{
    QGeoCoordinate coordinate;
    MAV_FRAME frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;  ///< Frame of coordinate.altitude()
};

/// \brief This is the base class for firmware specific rally point managers. A rally point manager is responsible
/// for communicating with the vehicle to set/get rally points.

class RallyPointManager : public PlanManager
{
    Q_OBJECT

public:
    RallyPointManager(Vehicle* vehicle);
    ~RallyPointManager();

    bool supported(void) const;
    void sendToVehicle(const QList<RallyPointData>& rgPoints);
    void removeAll(void);

    QString editorQml(void) const { return QStringLiteral("qrc:/FirmwarePlugin/RallyPointEditor.qml"); }

    QList<RallyPointData> points(void) const { return _rgPoints; }

    /// Error codes returned in error signal
    typedef enum {
        InternalError,
        TooFewPoints,           ///< Too few points for valid geofence
        TooManyPoints,          ///< Too many points for valid geofence
        InvalidCircleRadius,
    } ErrorCode_t;

signals:
    void loadComplete       (void);
    void inProgressChanged  (bool inProgress);
    void error              (int errorCode, const QString& errorMsg);
    void removeAllComplete  (bool error);
    void sendComplete       (bool error);

private slots:
    void _sendComplete              (bool error);
    void _planManagerLoadComplete   (bool removeAllRequested);

protected:
    void _sendError(ErrorCode_t errorCode, const QString& errorMsg);

    QList<RallyPointData> _rgPoints;
    QList<RallyPointData> _rgSendPoints;
};
