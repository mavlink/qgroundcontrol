/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
