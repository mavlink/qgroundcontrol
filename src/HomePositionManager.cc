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

#include "UAS.h"
#include "UASInterface.h"
#include "HomePositionManager.h"
#include "QGC.h"
#include "QGCMessageBox.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"

#define PI 3.1415926535897932384626433832795
#define MEAN_EARTH_DIAMETER	12756274.0
#define UMR	0.017453292519943295769236907684886

IMPLEMENT_QGC_SINGLETON(HomePositionManager, HomePositionManager)

const char* HomePositionManager::_settingsGroup =   "HomePositionManager";
const char* HomePositionManager::_latitudeKey =     "Latitude";
const char* HomePositionManager::_longitudeKey =    "Longitude";
const char* HomePositionManager::_altitudeKey =     "Altitude";

HomePositionManager::HomePositionManager(QObject* parent)
    : QObject(parent)
    , homeLat(47.3769)
    , homeLon(8.549444)
    , homeAlt(470.0)
{
    _loadSettings();
}

HomePositionManager::~HomePositionManager()
{

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
    
    foreach(QString name, settings.childGroups()) {
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
        _homePositions.append(new HomePosition("ETH Campus", QGeoCoordinate(47.3769, 8.549444, 470.0)));
    }
    
    // Deprecated settings for old editor

    settings.beginGroup("QGC_UASMANAGER");
    bool changed =  setHomePosition(settings.value("HOMELAT", homeLat).toDouble(),
                                    settings.value("HOMELON", homeLon).toDouble(),
                                    settings.value("HOMEALT", homeAlt).toDouble());

    // Make sure to fire the change - this will
    // make sure widgets get the signal once
    if (!changed)
    {
        emit homePositionChanged(homeLat, homeLon, homeAlt);
    }

    settings.endGroup();
}

bool HomePositionManager::setHomePosition(double lat, double lon, double alt)
{
    // Checking for NaN and infitiny
    // and checking for borders
    bool changed = false;
    if (!isnan(lat) && !isnan(lon) && !isnan(alt)
        && !isinf(lat) && !isinf(lon) && !isinf(alt)
        && lat <= 90.0 && lat >= -90.0 && lon <= 180.0 && lon >= -180.0)
        {

        if (fabs(homeLat - lat) > 1e-7) changed = true;
        if (fabs(homeLon - lon) > 1e-7) changed = true;
        if (fabs(homeAlt - alt) > 0.5f) changed = true;

        if (changed)
        {
            homeLat = lat;
            homeLon = lon;
            homeAlt = alt;

            emit homePositionChanged(homeLat, homeLon, homeAlt);
        }
    }
    return changed;
}

bool HomePositionManager::setHomePositionAndNotify(double lat, double lon, double alt)
{
    // Checking for NaN and infitiny
    // and checking for borders
    bool changed = setHomePosition(lat, lon, alt);

    if (changed) {
        MultiVehicleManager::instance()->setHomePositionForAllVehicles(homeLat, homeLon, homeAlt);
    }

	return changed;
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

HomePosition::HomePosition(const QString& name, const QGeoCoordinate& coordinate, QObject* parent)
    : QObject(parent)
    , _coordinate(coordinate)
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
    HomePositionManager::instance()->_storeSettings();
    emit nameChanged(name);
}

QGeoCoordinate HomePosition::coordinate(void)
{
    return _coordinate;
}

void HomePosition::setCoordinate(const QGeoCoordinate& coordinate)
{
    _coordinate = coordinate;
    HomePositionManager::instance()->_storeSettings();
    emit coordinateChanged(coordinate);
}
