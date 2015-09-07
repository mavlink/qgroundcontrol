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
    MissionItem(QObject         *parent = 0,
                int             sequenceNumber = 0,
                QGeoCoordinate  coordiante = QGeoCoordinate(),
                double          param1 = 0.0,
                double          param2 = 0.0,
                double          param3 = 0.0,
                double          param4 = 0.0,
                bool            autocontinue = true,
                bool            isCurrentItem = false,
                int             frame = MAV_FRAME_GLOBAL,
                int             action = MAV_CMD_NAV_WAYPOINT);

    MissionItem(const MissionItem& other);
    ~MissionItem();

    const MissionItem& operator=(const MissionItem& other);
    
    Q_PROPERTY(int              sequenceNumber      READ sequenceNumber         WRITE setSequenceNumber NOTIFY sequenceNumberChanged)
    Q_PROPERTY(bool             isCurrentItem       READ isCurrentItem          WRITE setIsCurrentItem  NOTIFY isCurrentItemChanged)
    Q_PROPERTY(bool             specifiesCoordinate READ specifiesCoordinate                            NOTIFY specifiesCoordinateChanged)
    Q_PROPERTY(QGeoCoordinate   coordinate          READ coordinate             WRITE setCoordinate     NOTIFY coordinateChanged)
    Q_PROPERTY(double           yaw                 READ yaw                    WRITE setYaw            NOTIFY yawChanged)
    Q_PROPERTY(QStringList      commandNames        READ commandNames                                   CONSTANT)
    Q_PROPERTY(QString          commandName         READ commandName                                    NOTIFY commandChanged)
    Q_PROPERTY(QStringList      valueLabels         READ valueLabels                                    NOTIFY commandChanged)
    Q_PROPERTY(QStringList      valueStrings        READ valueStrings                                   NOTIFY valueStringsChanged)
    Q_PROPERTY(int              commandByIndex      READ commandByIndex         WRITE setCommandByIndex NOTIFY commandChanged)
    Q_PROPERTY(MavlinkQmlSingleton::Qml_MAV_CMD command READ command            WRITE setCommand        NOTIFY commandChanged)
    
    // Property accesors
    
    int sequenceNumber(void) const { return _sequenceNumber; }
    void setSequenceNumber(int sequenceNumber);
    
    bool isCurrentItem(void) const { return _isCurrentItem; }
    void setIsCurrentItem(bool isCurrentItem);
    
    bool specifiesCoordinate(void) const;
    
    QGeoCoordinate coordinate(void) const { return _coordinate; }
    void setCoordinate(const QGeoCoordinate& coordinate);
    
    QStringList commandNames(void);
    QString commandName(void);

    int commandByIndex(void);
    void setCommandByIndex(int index);
    
    MavlinkQmlSingleton::Qml_MAV_CMD command(void) { return (MavlinkQmlSingleton::Qml_MAV_CMD)getAction(); };
    void setCommand(MavlinkQmlSingleton::Qml_MAV_CMD command) { setAction(command); }
    
    QStringList valueLabels(void);
    QStringList valueStrings(void);
    
    // C++ only methods

    double latitude(void)  const { return _coordinate.latitude(); }
    double longitude(void) const { return _coordinate.longitude(); }
    double altitude(void)  const { return _coordinate.altitude(); }
    
    void setLatitude(double latitude);
    void setLongitude(double longitude);
    void setAltitude(double altitude);
    
    double x(void) const { return latitude(); }
    double y(void) const { return longitude(); }
    double z(void) const { return altitude(); }
    
    void setX(double x);
    void setY(double y);
    void setZ(double z);
    
    double yaw(void) const { return _yaw; }
    void setYaw(double yaw);
    
    bool getAutoContinue() const {
        return _autocontinue;
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
        return yaw();
    }
    double getParam5() const {
        return latitude();
    }
    double getParam6() const {
        return longitude();
    }
    double getParam7() const {
        return altitude();
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
    /** @brief Returns true if x, y, z contain reasonable navigation data */
    bool isNavigationType();

    /** @brief Get the time this waypoint was reached */
    quint64 getReachedTime() const { return _reachedTime; }

    void save(QTextStream &saveStream);
    bool load(QTextStream &loadStream);
    
    
signals:
    void sequenceNumberChanged(int sequenceNumber);
    void specifiesCoordinateChanged(bool specifiesCoordinate);
    void isCurrentItemChanged(bool isCurrentItem);
    void coordinateChanged(const QGeoCoordinate& coordinate);
    void yawChanged(double yaw);

    /** @brief Announces a change to the waypoint data */
    void changed(MissionItem* wp);

    
    void commandNameChanged(QString type);
    void commandChanged(MavlinkQmlSingleton::Qml_MAV_CMD command);
    void valueLabelsChanged(QStringList valueLabels);
    void valueStringsChanged(QStringList valueStrings);
    
public:
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

private:
    QString _oneDecimalString(double value);

private:
    typedef struct {
        MAV_CMD     command;
        const char* name;
    } MavCmd2Name_t;
    
    int             _sequenceNumber;
    QGeoCoordinate  _coordinate;
    double          _yaw;
    int             _frame;
    int             _action;
    bool            _autocontinue;
    bool            _isCurrentItem;
    double          _orbit;
    double          _param1;
    double          _param2;
    int             _turns;
    quint64         _reachedTime;
    
    static const int            _cMavCmd2Name = 9;
    static const MavCmd2Name_t  _rgMavCmd2Name[_cMavCmd2Name];
};

#endif
