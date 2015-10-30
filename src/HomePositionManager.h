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

#ifndef HomePositionManager_H
#define HomePositionManager_H

#include "QmlObjectListModel.h"
#include "QGCToolbox.h"

#include <QGeoCoordinate>

class HomePositionManager;

class HomePosition : public QObject
{
    Q_OBJECT
    
public:
    HomePosition(const QString& name, const QGeoCoordinate& coordinate, HomePositionManager* homePositionManager, QObject* parent = NULL);
    ~HomePosition();
    
    Q_PROPERTY(QString          name        READ name           WRITE setName       NOTIFY nameChanged)
    Q_PROPERTY(QGeoCoordinate   coordinate  READ coordinate     WRITE setCoordinate NOTIFY coordinateChanged)
    
    // Property accessors
    
    QString name(void);
    void setName(const QString& name);
    
    QGeoCoordinate coordinate(void);
    void setCoordinate(const QGeoCoordinate& coordinate);

signals:
    void nameChanged(const QString& name);
    void coordinateChanged(const QGeoCoordinate& coordinate);
    
private:
    QGeoCoordinate          _coordinate;
    HomePositionManager*    _homePositionManager;
};

class HomePositionManager : public QGCTool
{
    Q_OBJECT
    
public:
    HomePositionManager(QGCApplication* app);

    Q_PROPERTY(QmlObjectListModel* homePositions READ homePositions CONSTANT)
    
    /// If name is not already a home position a new one will be added, otherwise the existing
    /// home position will be updated
    Q_INVOKABLE void updateHomePosition(const QString& name, const QGeoCoordinate& coordinate);
    
    Q_INVOKABLE void deleteHomePosition(const QString& name);
    
    // Property accesors
    
    QmlObjectListModel* homePositions(void) { return &_homePositions; }
    
    // Should only be called by HomePosition
    void _storeSettings(void);
    
    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

private:
    void _loadSettings(void);
    
    QmlObjectListModel      _homePositions;
    
    static const char* _settingsGroup;
    static const char* _latitudeKey;
    static const char* _longitudeKey;
    static const char* _altitudeKey;
    
// Everything below is deprecated and will be removed once old Map code is removed
public:
    
    // Deprecated methods
    
    /** @brief Get home position latitude */
    double getHomeLatitude() const {
        return homeLat;
    }
    /** @brief Get home position longitude */
    double getHomeLongitude() const {
        return homeLon;
    }
    /** @brief Get home position altitude */
    double getHomeAltitude() const {
        return homeAlt;
    }

protected:
    double homeLat;
    double homeLon;
    double homeAlt;
};

#endif
