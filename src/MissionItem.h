/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef MissionItem_H
#define MissionItem_H

#include <QObject>
#include <QString>
#include <QtQml>
#include <QTextStream>
#include <QGeoCoordinate>

#include "QGCMAVLink.h"
#include "QGC.h"
#include "MavlinkQmlSingleton.h"

class MissionItem : public QObject
{
    Q_OBJECT
public:
    MissionItem(
        QObject *parent = 0,
        quint16 id = 0,
        double  x = 0.0,
        double  y = 0.0,
        double  z = 0.0,
        double  param1 = 0.0,
        double  param2 = 0.0,
        double  param3 = 0.0,
        double  param4 = 0.0,
        bool    autocontinue = true,
        bool    current = false,
        int     frame = MAV_FRAME_GLOBAL,
        int     action = MAV_CMD_NAV_WAYPOINT,
        const QString& description=QString(""));

    MissionItem(const MissionItem& other);
    ~MissionItem();

    const MissionItem& operator=(const MissionItem& other);

    Q_PROPERTY(bool hasCoordinate READ hasCoordinate NOTIFY hasCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate NOTIFY coordinateChanged)
    
    Q_PROPERTY(QString commandName READ commandName NOTIFY commandNameChanged)
    Q_PROPERTY(MavlinkQmlSingleton::Qml_MAV_CMD command READ command WRITE setCommand NOTIFY commandChanged)
    
    Q_PROPERTY(double  longitude READ longitude  NOTIFY longitudeChanged)
    Q_PROPERTY(double  latitude  READ latitude   NOTIFY latitudeChanged)
    Q_PROPERTY(double  altitude  READ altitude   NOTIFY altitudeChanged)
    Q_PROPERTY(quint16 id        READ id         CONSTANT)
    
    Q_PROPERTY(QStringList valueLabels READ valueLabels NOTIFY commandChanged)
    Q_PROPERTY(QStringList valueStrings READ valueStrings NOTIFY valueStringsChanged)

    double  latitude()  { return _x; }
    double  longitude() { return _y; }
    double  altitude()  { return _z; }
    quint16 id()        { return _id; }

    quint16 getId() const {
        return _id;
    }
    double getX() const {
        return _x;
    }
    double getY() const {
        return _y;
    }
    double getZ() const {
        return _z;
    }
    double getLatitude() const {
        return _x;
    }
    double getLongitude() const {
        return _y;
    }
    double getAltitude() const {
        return _z;
    }
    double getYaw() const {
        return _yaw;
    }
    bool getAutoContinue() const {
        return _autocontinue;
    }
    bool getCurrent() const {
        return _current;
    }
    double getLoiterOrbit() const {
        return _orbit;
    }
    double getAcceptanceRadius() const {
        return _param2;
    }
    double getHoldTime() const {
        return _param1;
    }
    double getParam1() const {
        return _param1;
    }
    double getParam2() const {
        return _param2;
    }
    double getParam3() const {
        return _orbit;
    }
    double getParam4() const {
        return _yaw;
    }
    double getParam5() const {
        return _x;
    }
    double getParam6() const {
        return _y;
    }
    double getParam7() const {
        return _z;
    }
    int getTurns() const {
        return _param1;
    }
    // MAV_FRAME
    int getFrame() const {
        return _frame;
    }
    // MAV_CMD
    int getAction() const {
        return _action;
    }
    const QString& getName() const {
        return _name;
    }
    const QString& getDescription() const {
        return _description;
    }

    /** @brief Returns true if x, y, z contain reasonable navigation data */
    bool isNavigationType();

    /** @brief Get the time this waypoint was reached */
    quint64 getReachedTime() const { return _reachedTime; }

    void save(QTextStream &saveStream);
    bool load(QTextStream &loadStream);
    
    bool hasCoordinate(void);
    QGeoCoordinate coordinate(void);
    
    QString commandName(void);

    MavlinkQmlSingleton::Qml_MAV_CMD command(void) { return (MavlinkQmlSingleton::Qml_MAV_CMD)getAction(); };
    void setCommand(MavlinkQmlSingleton::Qml_MAV_CMD command) { setAction(command); }
    
    QStringList valueLabels(void);
    QStringList valueStrings(void);
    
protected:
    quint16 _id;
    double  _x;
    double  _y;
    double  _z;
    double  _yaw;
    int     _frame;
    int     _action;
    bool    _autocontinue;
    bool    _current;
    double  _orbit;
    double  _param1;
    double  _param2;
    int     _turns;
    QString _name;
    QString _description;
    quint64 _reachedTime;

public:
    void setId          (quint16 _id);
    void setX           (double _x);
    void setY           (double _y);
    void setZ           (double _z);
    void setLatitude    (double lat);
    void setLongitude   (double lon);
    void setAltitude    (double alt);
    /** @brief Yaw angle in COMPASS DEGREES: 0-360 */
    void setYaw         (int _yaw);
    /** @brief Yaw angle in COMPASS DEGREES: 0-360 */
    void setYaw         (double _yaw);
    /** @brief Set the waypoint action */
    void setAction      (int _action);
    void setFrame       (int _frame);
    void setAutocontinue(bool autoContinue);
    void setCurrent     (bool _current);
    void setLoiterOrbit (double _orbit);
    void setParam1      (double _param1);
    void setParam2      (double _param2);
    void setParam3      (double param3);
    void setParam4      (double param4);
    void setParam5      (double param5);
    void setParam6      (double param6);
    void setParam7      (double param7);
    void setAcceptanceRadius(double radius);
    void setHoldTime    (int holdTime);
    void setHoldTime    (double holdTime);
    /** @brief Number of turns for loiter waypoints */
    void setTurns       (int _turns);
    /** @brief Set waypoint as reached */
    void setReached     () { _reachedTime = QGC::groundTimeMilliseconds(); }
    /** @brief Wether this waypoint has been reached yet */
    bool isReached      () { return (_reachedTime > 0); }

    void setChanged() {
        emit changed(this);
    }

signals:
    /** @brief Announces a change to the waypoint data */
    void changed(MissionItem* wp);

    void latitudeChanged    ();
    void longitudeChanged   ();
    void altitudeChanged    ();
    void hasCoordinateChanged(bool hasCoordinate);
    void coordinateChanged(void);
    void commandNameChanged(QString type);
    void commandChanged(MavlinkQmlSingleton::Qml_MAV_CMD command);
    void valueLabelsChanged(QStringList valueLabels);
    void valueStringsChanged(QStringList valueStrings);
    
private:
    QString _oneDecimalString(double value);
};

QML_DECLARE_TYPE(MissionItem)

#endif
