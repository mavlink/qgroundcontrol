/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PositionManager.h"

QGCPositionManager::QGCPositionManager(QGCApplication* app) :
    QGCTool(app),
    _updateInterval(0),
    _currentSource(nullptr)
{
    _defaultSource = QGeoPositionInfoSource::createDefaultSource(this);
    _simulatedSource = new SimulatedPosition();

    // Enable this to get a simulated target on desktop

    // if (_defaultSource == nullptr) {
    //     _defaultSource = _simulatedSource;
    // }

    setPositionSource(QGCPositionSource::GPS);
}

QGCPositionManager::~QGCPositionManager()
{
    delete(_simulatedSource);
}

void QGCPositionManager::positionUpdated(const QGeoPositionInfo &update)
{

    QGeoCoordinate position(update.coordinate().latitude(), update.coordinate().longitude());

    emit lastPositionUpdated(update.isValid(), QVariant::fromValue(position));
    emit positionInfoUpdated(update);
}

int QGCPositionManager::updateInterval() const
{
    return _updateInterval;
}

void QGCPositionManager::setPositionSource(QGCPositionManager::QGCPositionSource source)
{
    if (_currentSource != nullptr) {
        _currentSource->stopUpdates();
        disconnect(_currentSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));
    }

    switch(source) {
    case QGCPositionManager::Log:
        break;
    case QGCPositionManager::Simulated:
        _currentSource = _simulatedSource;
        break;
    case QGCPositionManager::GPS:
    default:        
        _currentSource = _defaultSource;
        break;
    }

    if (_currentSource != nullptr) {
        _updateInterval = _currentSource->minimumUpdateInterval();
        _currentSource->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
        _currentSource->setUpdateInterval(_updateInterval);
        _currentSource->startUpdates();

        connect(_currentSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)));
    }
}

