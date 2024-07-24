/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SimulatedPosition.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QDateTime>
#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(SimulatedPositionLog, "qgc.positionmanager.simulatedposition")

SimulatedPosition::SimulatedPosition(QObject* parent)
    : QGeoPositionInfoSource(parent)
    , m_updateTimer(new QTimer(this))
{
    // qCDebug(SimulatedPositionLog) << Q_FUNC_INFO << this;

    m_lastPosition.setTimestamp(QDateTime::currentDateTime());
    m_lastPosition.setCoordinate(QGeoCoordinate(47.3977420, 8.5455941, 488.));
    m_lastPosition.setAttribute(QGeoPositionInfo::Attribute::Direction, s_heading);
    m_lastPosition.setAttribute(QGeoPositionInfo::Attribute::GroundSpeed, s_horizontalVelocityMetersPerSec);
    m_lastPosition.setAttribute(QGeoPositionInfo::Attribute::VerticalSpeed, s_verticalVelocityMetersPerSec);

    (void) connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &SimulatedPosition::_vehicleAdded);

    m_updateTimer->setSingleShot(false);
    (void) connect(m_updateTimer, &QTimer::timeout, this, &SimulatedPosition::_updatePosition);
}

SimulatedPosition::~SimulatedPosition()
{
    // qCDebug(SimulatedPositionLog) << Q_FUNC_INFO << this;
}

void SimulatedPosition::startUpdates()
{
    m_updateTimer->start(qMax(updateInterval(), minimumUpdateInterval()));
}

void SimulatedPosition::stopUpdates()
{
    m_updateTimer->stop();
}

void SimulatedPosition::requestUpdate(int /*timeout*/)
{
    emit errorOccurred(QGeoPositionInfoSource::UpdateTimeoutError);
}

void SimulatedPosition::_updatePosition()
{
    const int intervalMsecs = m_updateTimer->interval();

    const QGeoCoordinate coord = m_lastPosition.coordinate();
    const qreal horizontalDistance = s_horizontalVelocityMetersPerSec * (1000. / static_cast<qreal>(intervalMsecs));
    const qreal verticalDistance = s_verticalVelocityMetersPerSec * (1000. / static_cast<qreal>(intervalMsecs));

    m_lastPosition.setCoordinate(coord.atDistanceAndAzimuth(horizontalDistance, s_heading, verticalDistance));
    emit positionUpdated(m_lastPosition);
}

void SimulatedPosition::_vehicleAdded(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    if (vehicle->homePosition().isValid()) {
        m_lastPosition.setCoordinate(vehicle->homePosition());
    } else {
        m_homePositionChangedConnection = connect(vehicle, &Vehicle::homePositionChanged, this, &SimulatedPosition::_vehicleHomePositionChanged);
    }
}

void SimulatedPosition::_vehicleHomePositionChanged(QGeoCoordinate homePosition)
{
    if (homePosition.isValid()) {
        m_lastPosition.setCoordinate(homePosition);
        if (m_homePositionChangedConnection) {
            (void) disconnect(m_homePositionChangedConnection);
            m_homePositionChangedConnection = QMetaObject::Connection();
        }
    }
}
