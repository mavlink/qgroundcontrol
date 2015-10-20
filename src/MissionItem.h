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
#include "QmlObjectListModel.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(MissionItemLog)

class MissionItem : public QObject
{
    Q_OBJECT
    
public:
    MissionItem(QObject         *parent = 0,
                int             sequenceNumber = 0,
                QGeoCoordinate  coordiante = QGeoCoordinate(),
                int             action = MAV_CMD_NAV_WAYPOINT,
                double          param1 = 0.0,
                double          param2 = defaultAcceptanceRadius,
                double          param3 = defaultLoiterOrbitRadius,
                double          param4 = defaultHeading,
                bool            autocontinue = true,
                bool            isCurrentItem = false,
                int             frame = MAV_FRAME_GLOBAL_RELATIVE_ALT);

    MissionItem(const MissionItem& other, QObject* parent = NULL);
    ~MissionItem();

    const MissionItem& operator=(const MissionItem& other);
    
    /// Returns true if the item has been modified since the last time dirty was false
    Q_PROPERTY(bool                 dirty               READ dirty                  WRITE setDirty          NOTIFY dirtyChanged)
    
    Q_PROPERTY(int                  sequenceNumber      READ sequenceNumber         WRITE setSequenceNumber NOTIFY sequenceNumberChanged)
    Q_PROPERTY(bool                 isCurrentItem       READ isCurrentItem          WRITE setIsCurrentItem  NOTIFY isCurrentItemChanged)

    Q_PROPERTY(bool                 specifiesCoordinate READ specifiesCoordinate                            NOTIFY commandChanged)
    Q_PROPERTY(QGeoCoordinate       coordinate          READ coordinate             WRITE setCoordinate     NOTIFY coordinateChanged)

    Q_PROPERTY(bool                 specifiesHeading    READ specifiesHeading                               NOTIFY commandChanged)
    Q_PROPERTY(double               heading             READ headingDegrees         WRITE setHeadingDegrees NOTIFY headingDegreesChanged)

    Q_PROPERTY(QStringList          commandNames        READ commandNames                                   CONSTANT)
    Q_PROPERTY(QString              commandName         READ commandName                                    NOTIFY commandChanged)
    Q_PROPERTY(QString              commandDescription  READ commandDescription                             NOTIFY commandChanged)
    Q_PROPERTY(QStringList          valueLabels         READ valueLabels                                    NOTIFY commandChanged)
    Q_PROPERTY(QStringList          valueStrings        READ valueStrings                                   NOTIFY valueStringsChanged)
    Q_PROPERTY(int                  commandByIndex      READ commandByIndex         WRITE setCommandByIndex NOTIFY commandChanged)
    Q_PROPERTY(QmlObjectListModel*  textFieldFacts      READ textFieldFacts                                 NOTIFY commandChanged)
    Q_PROPERTY(QmlObjectListModel*  checkboxFacts       READ checkboxFacts                                  NOTIFY commandChanged)
    Q_PROPERTY(MavlinkQmlSingleton::Qml_MAV_CMD command READ command                WRITE setCommand        NOTIFY commandChanged)
    Q_PROPERTY(QmlObjectListModel*  childItems          READ childItems                                     CONSTANT)

    /// true: this item is being used as a home position indicator
    Q_PROPERTY(bool                 homePosition        MEMBER _homePositionSpecialCase                     CONSTANT)
    
    /// true: home position should be shown
    Q_PROPERTY(bool                 homePositionValid   READ homePositionValid      WRITE setHomePositionValid NOTIFY homePositionValidChanged)

    // Property accesors
    
    int sequenceNumber(void) const { return _sequenceNumber; }
    void setSequenceNumber(int sequenceNumber);
    
    bool isCurrentItem(void) const { return _isCurrentItem; }
    void setIsCurrentItem(bool isCurrentItem);
    
    bool specifiesCoordinate(void) const;
    QGeoCoordinate coordinate(void) const;
    void setCoordinate(const QGeoCoordinate& coordinate);
    
    bool specifiesHeading(void) const;
    double headingDegrees(void) const;
    void setHeadingDegrees(double headingDegrees);

    // This is public for unit testing
    double _yawRadians(void) const;

    QStringList commandNames(void);
    QString commandName(void);
    QString commandDescription(void);

    int commandByIndex(void);
    void setCommandByIndex(int index);
    
    MavlinkQmlSingleton::Qml_MAV_CMD command(void) { return (MavlinkQmlSingleton::Qml_MAV_CMD)_command; };
    void setCommand(MavlinkQmlSingleton::Qml_MAV_CMD command) { setAction(command); }
    
    QStringList valueLabels(void);
    QStringList valueStrings(void);
    
    QmlObjectListModel* textFieldFacts(void);
    QmlObjectListModel* checkboxFacts(void);
    
    bool dirty(void) { return _dirty; }
    void setDirty(bool dirty);
    
    QmlObjectListModel* childItems(void) { return &_childItems; }

    bool homePositionValid(void) { return _homePositionValid; }
    void setHomePositionValid(bool homePositionValid);
    
    // C++ only methods
    
    /// Returns true if this item can be edited in the ui
    bool canEdit(void);

    double latitude(void)  const { return _latitudeFact->value().toDouble(); }
    double longitude(void) const { return _longitudeFact->value().toDouble(); }
    double altitude(void)  const { return _altitudeFact->value().toDouble(); }
    
    void setLatitude(double latitude);
    void setLongitude(double longitude);
    void setAltitude(double altitude);
    
    double x(void) const { return latitude(); }
    double y(void) const { return longitude(); }
    double z(void) const { return altitude(); }
    
    void setX(double x);
    void setY(double y);
    void setZ(double z);
    
    bool autoContinue() const {
        return _autocontinue;
    }
    double loiterOrbitRadius() const {
        return _loiterOrbitRadiusFact->value().toDouble();
    }
    double acceptanceRadius() const {
        return param2();
    }
    double holdTime() const {
        return param1();
    }
    double param1() const {
        return _param1Fact->value().toDouble();
    }
    double param2() const {
        return _param2Fact->value().toDouble();
    }
    double param3() const {
        return loiterOrbitRadius();
    }
    double param4() const {
        return _yawRadians();
    }
    double param5() const {
        return latitude();
    }
    double param6() const {
        return longitude();
    }
    double param7() const {
        return altitude();
    }
    // MAV_FRAME
    int frame() const;
    
    // MAV_CMD
    int command() const {
        return _command;
    }
    /** @brief Returns true if x, y, z contain reasonable navigation data */
    bool isNavigationType();

    /** @brief Get the time this waypoint was reached */
    quint64 reachedTime() const { return _reachedTime; }

    void save(QTextStream &saveStream);
    bool load(QTextStream &loadStream);
    
    void setHomePositionSpecialCase(bool homePositionSpecialCase) { _homePositionSpecialCase = homePositionSpecialCase; }

    static const double defaultPitch;
    static const double defaultHeading;
    static const double defaultAltitude;
    static const double defaultAcceptanceRadius;
    static const double defaultLoiterOrbitRadius;
    static const double defaultLoiterTurns;

signals:
    void sequenceNumberChanged(int sequenceNumber);
    void isCurrentItemChanged(bool isCurrentItem);
    void coordinateChanged(const QGeoCoordinate& coordinate);
    void headingDegreesChanged(double heading);
    void dirtyChanged(bool dirty);
    void homePositionValidChanged(bool homePostionValid);

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
    void setLoiterOrbitRadius (double radius);
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
    /** @brief Set waypoint as reached */
    void setReached     () { _reachedTime = QGC::groundTimeMilliseconds(); }
    /** @brief Wether this waypoint has been reached yet */
    bool isReached      () { return (_reachedTime > 0); }

    void setChanged() {
        emit changed(this);
    }
    
private slots:
    void _factValueChanged(QVariant value);
    void _coordinateFactChanged(QVariant value);
    void _headingDegreesFactChanged(QVariant value);

private:
    QString _oneDecimalString(double value);
    void _connectSignals(void);
    void _setYawRadians(double yawRadians);

private:
    typedef struct {
        MAV_CMD     command;
        const char* name;
    } MavCmd2Name_t;
    
    int                                 _sequenceNumber;
    int                                 _frame;
    MavlinkQmlSingleton::Qml_MAV_CMD    _command;
    bool                                _autocontinue;
    bool                                _isCurrentItem;
    quint64                             _reachedTime;
    
    Fact*           _latitudeFact;
    Fact*           _longitudeFact;
    Fact*           _altitudeFact;
    Fact*           _headingDegreesFact;
    Fact*           _loiterOrbitRadiusFact;
    Fact*           _param1Fact;
    Fact*           _param2Fact;
    Fact*           _altitudeRelativeToHomeFact;
    
    FactMetaData*   _pitchMetaData;
    FactMetaData*   _acceptanceRadiusMetaData;
    FactMetaData*   _holdTimeMetaData;
    FactMetaData*   _loiterTurnsMetaData;
    FactMetaData*   _loiterSecondsMetaData;
    FactMetaData*   _delaySecondsMetaData;
    FactMetaData*   _jumpSequenceMetaData;
    FactMetaData*   _jumpRepeatMetaData;
    
    bool _dirty;
    
    bool _homePositionSpecialCase;  ///< true: this item is being used as a ui home position indicator
    bool _homePositionValid;        ///< true: home psition should be displayed
    
    /// This is used to reference any subsequent mission items which do not specify a coordinate.
    QmlObjectListModel  _childItems;
    
    static const int            _cMavCmd2Name = 9;
    static const MavCmd2Name_t  _rgMavCmd2Name[_cMavCmd2Name];
};

QDebug operator<<(QDebug dbg, const MissionItem& missionItem);
QDebug operator<<(QDebug dbg, const MissionItem* missionItem);

#endif
