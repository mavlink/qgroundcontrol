/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/
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

