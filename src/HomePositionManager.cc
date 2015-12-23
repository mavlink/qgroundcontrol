/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#include <QList>
#include <QApplication>
#include <QTimer>
#include <QSettings>
#include <QtQml>

#include "UAS.h"
#include "UASInterface.h"
#include "HomePositionManager.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"

#define PI 3.1415926535897932384626433832795
#define MEAN_EARTH_DIAMETER	12756274.0
#define UMR	0.017453292519943295769236907684886

const char* HomePositionManager::_settingsGroup =   "HomePositionManager";
const char* HomePositionManager::_latitudeKey =     "Latitude";
const char* HomePositionManager::_longitudeKey =    "Longitude";
const char* HomePositionManager::_altitudeKey =     "Altitude";

HomePositionManager::HomePositionManager(QGCApplication* app)
    : QGCTool(app)
    , homeLat(47.3769)
    , homeLon(8.549444)
    , homeAlt(470.0)
{
    qmlRegisterUncreatableType<HomePositionManager> ("QGroundControl", 1, 0, "HomePositionManager", "Reference only");
}

void HomePositionManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);


    _loadSettings();
}

void HomePositionManager::_storeSettings(void)
{
    QSettings settings;
    
    settings.remove(_settingsGroup);
    settings.beginGroup(_settingsGroup);
    
    for (int i=0; i<_homePositions.count(); i++) {
        HomePosition* homePos = qobject_cast<HomePosition*>(_homePositions[i]);
        
        qDebug() << "Saving" << homePos->name();
        
        settings.beginGroup(homePos->name());
        settings.setValue(_latitudeKey, homePos->coordinate().latitude());
        settings.setValue(_longitudeKey, homePos->coordinate().longitude());
        settings.setValue(_altitudeKey, homePos->coordinate().altitude());
        settings.endGroup();
    }
    
    settings.endGroup();
    
    // Deprecated settings for old editor
    settings.beginGroup("QGC_UASMANAGER");
    settings.setValue("HOMELAT", homeLat);
    settings.setValue("HOMELON", homeLon);
    settings.setValue("HOMEALT", homeAlt);
    settings.endGroup();
}

void HomePositionManager::_loadSettings(void)
{
    QSettings settings;
    
    _homePositions.clear();
    
    settings.beginGroup(_settingsGroup);
    
    foreach(const QString &name, settings.childGroups()) {
        QGeoCoordinate coordinate;
        
        qDebug() << "Load setting" << name;
        
        settings.beginGroup(name);
        coordinate.setLatitude(settings.value(_latitudeKey).toDouble());
        coordinate.setLongitude(settings.value(_longitudeKey).toDouble());
        coordinate.setAltitude(settings.value(_altitudeKey).toDouble());
        settings.endGroup();
        
        _homePositions.append(new HomePosition(name, coordinate, this));
    }
    
    settings.endGroup();
    
    if (_homePositions.count() == 0) {
        _homePositions.append(new HomePosition("ETH Campus", QGeoCoordinate(47.3769, 8.549444, 470.0), this));
    }    
}

void HomePositionManager::updateHomePosition(const QString& name, const QGeoCoordinate& coordinate)
{
    HomePosition * homePos = NULL;
    
    for (int i=0; i<_homePositions.count(); i++) {
        homePos = qobject_cast<HomePosition*>(_homePositions[i]);
        if (homePos->name() == name) {
            break;
        }
        homePos = NULL;
    }
    
    if (homePos == NULL) {
        HomePosition* homePos = new HomePosition(name, coordinate, this);
        _homePositions.append(homePos);
    } else {
        homePos->setName(name);
        homePos->setCoordinate(coordinate);
    }
    
    _storeSettings();
}

void HomePositionManager::deleteHomePosition(const QString& name)
{
    // Don't allow delete of last position
    if (_homePositions.count() == 1) {
        return;
    }
    
    qDebug() << "Attempting delete" << name;
    
    for (int i=0; i<_homePositions.count(); i++) {
        if (qobject_cast<HomePosition*>(_homePositions[i])->name() == name) {
            qDebug() << "Deleting" << name;
            _homePositions.removeAt(i);
            break;
        }
    }
    
    _storeSettings();
}

HomePosition::HomePosition(const QString& name, const QGeoCoordinate& coordinate, HomePositionManager* homePositionManager, QObject* parent)
    : QObject(parent)
    , _coordinate(coordinate)
    , _homePositionManager(homePositionManager)
{
    setObjectName(name);
}

HomePosition::~HomePosition()
{
    
}

QString HomePosition::name(void)
{
    return objectName();
}

void HomePosition::setName(const QString& name)
{
    setObjectName(name);
    _homePositionManager->_storeSettings();
    emit nameChanged(name);
}

QGeoCoordinate HomePosition::coordinate(void)
{
    return _coordinate;
}

void HomePosition::setCoordinate(const QGeoCoordinate& coordinate)
{
    _coordinate = coordinate;
    _homePositionManager->_storeSettings();
    emit coordinateChanged(coordinate);
}
