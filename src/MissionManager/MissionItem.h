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
#include "MissionCommands.h"

Q_DECLARE_LOGGING_CATEGORY(MissionItemLog)

class MissionItem : public QObject
{
    Q_OBJECT
    
public:
    MissionItem(QObject* parent = NULL);

    MissionItem(int             sequenceNumber,
                MAV_CMD         command,
                MAV_FRAME       frame,
                double          param1,
                double          param2,
                double          param3,
                double          param4,
                double          param5,
                double          param6,
                double          param7,
                bool            autoContinue,
                bool            isCurrentItem,
                QObject*        parent = NULL);

    MissionItem(const MissionItem& other, QObject* parent = NULL);

    ~MissionItem();

    const MissionItem& operator=(const MissionItem& other);
    
    Q_PROPERTY(double           altDifference           READ altDifference          WRITE setAltDifference      NOTIFY altDifferenceChanged)        ///< Change in altitude from previous waypoint
    Q_PROPERTY(double           altPercent              READ altPercent             WRITE setAltPercent         NOTIFY altPercentChanged)           ///< Percent of total altitude change in mission altitude
    Q_PROPERTY(double           azimuth                 READ azimuth                WRITE setAzimuth            NOTIFY azimuthChanged)              ///< Azimuth to previous waypoint
    Q_PROPERTY(QString          category                READ category                                           NOTIFY commandChanged)
    Q_PROPERTY(MavlinkQmlSingleton::Qml_MAV_CMD command READ command                WRITE setCommand            NOTIFY commandChanged)
    Q_PROPERTY(QString          commandDescription      READ commandDescription                                 NOTIFY commandChanged)
    Q_PROPERTY(QString          commandName             READ commandName                                        NOTIFY commandChanged)
    Q_PROPERTY(QGeoCoordinate   coordinate              READ coordinate             WRITE setCoordinate         NOTIFY coordinateChanged)
    Q_PROPERTY(bool             dirty                   READ dirty                  WRITE setDirty              NOTIFY dirtyChanged)
    Q_PROPERTY(double           distance                READ distance               WRITE setDistance           NOTIFY distanceChanged)             ///< Distance to previous waypoint
    Q_PROPERTY(bool             friendlyEditAllowed     READ friendlyEditAllowed                                NOTIFY friendlyEditAllowedChanged)
    Q_PROPERTY(bool             homePosition            READ homePosition                                       CONSTANT)                           ///< true: This item is being used as a home position indicator
    Q_PROPERTY(bool             homePositionValid       READ homePositionValid      WRITE setHomePositionValid  NOTIFY homePositionValidChanged)    ///< true: Home position should be shown
    Q_PROPERTY(bool             isCurrentItem           READ isCurrentItem          WRITE setIsCurrentItem      NOTIFY isCurrentItemChanged)
    Q_PROPERTY(bool             rawEdit                 READ rawEdit                WRITE setRawEdit            NOTIFY rawEditChanged)              ///< true: raw item editing with all params
    Q_PROPERTY(bool             relativeAltitude        READ relativeAltitude                                   NOTIFY frameChanged)
    Q_PROPERTY(int              sequenceNumber          READ sequenceNumber         WRITE setSequenceNumber     NOTIFY sequenceNumberChanged)
    Q_PROPERTY(bool             standaloneCoordinate    READ standaloneCoordinate                               NOTIFY commandChanged)
    Q_PROPERTY(bool             specifiesCoordinate     READ specifiesCoordinate                                NOTIFY commandChanged)

    // These properties are used to display the editing ui
    Q_PROPERTY(QmlObjectListModel*  checkboxFacts   READ checkboxFacts  NOTIFY uiModelChanged)
    Q_PROPERTY(QmlObjectListModel*  comboboxFacts   READ comboboxFacts  NOTIFY uiModelChanged)
    Q_PROPERTY(QmlObjectListModel*  textFieldFacts  READ textFieldFacts NOTIFY uiModelChanged)

    /// List of child mission items. Child mission item are subsequent mision items which do not specify a coordinate. They
    /// are shown next to the part item in the ui.
    Q_PROPERTY(QmlObjectListModel*  childItems      READ childItems     CONSTANT)

    // Property accesors
    
    double          altDifference       (void) const    { return _altDifference; }
    double          altPercent          (void) const    { return _altPercent; }
    double          azimuth             (void) const    { return _azimuth; }
    QString         category            (void) const;
    MavlinkQmlSingleton::Qml_MAV_CMD command(void) const { return (MavlinkQmlSingleton::Qml_MAV_CMD)_commandFact.cookedValue().toInt(); };
    QString         commandDescription  (void) const;
    QString         commandName         (void) const;
    QGeoCoordinate  coordinate          (void) const;
    bool            dirty               (void) const    { return _dirty; }
    double          distance            (void) const    { return _distance; }
    bool            friendlyEditAllowed (void) const;
    bool            homePosition        (void) const    { return _homePositionSpecialCase; }
    bool            homePositionValid   (void) const    { return _homePositionValid; }
    bool            isCurrentItem       (void) const    { return _isCurrentItem; }
    bool            rawEdit             (void) const;
    int             sequenceNumber      (void) const    { return _sequenceNumber; }
    bool            standaloneCoordinate(void) const;
    bool            specifiesCoordinate (void) const;


    QmlObjectListModel* textFieldFacts  (void);
    QmlObjectListModel* checkboxFacts   (void);
    QmlObjectListModel* comboboxFacts   (void);
    QmlObjectListModel* childItems      (void) { return &_childItems; }

    void setRawEdit(bool rawEdit);
    void setDirty(bool dirty);
    void setSequenceNumber(int sequenceNumber);
    
    void setIsCurrentItem(bool isCurrentItem);
    
    void setCoordinate(const QGeoCoordinate& coordinate);
    
    void setCommandByIndex(int index);

    void setCommand(MavlinkQmlSingleton::Qml_MAV_CMD command);

    void setHomePositionValid(bool homePositionValid);
    void setHomePositionSpecialCase(bool homePositionSpecialCase) { _homePositionSpecialCase = homePositionSpecialCase; }

    void setAltDifference   (double altDifference);
    void setAltPercent      (double altPercent);
    void setAzimuth         (double azimuth);
    void setDistance        (double distance);

    // C++ only methods

    MAV_FRAME   frame       (void)  const { return (MAV_FRAME)_frameFact.rawValue().toInt(); }
    bool        autoContinue(void)  const { return _autoContinueFact.rawValue().toBool(); }
    double      param1      (void)  const { return _param1Fact.rawValue().toDouble(); }
    double      param2      (void)  const { return _param2Fact.rawValue().toDouble(); }
    double      param3      (void)  const { return _param3Fact.rawValue().toDouble(); }
    double      param4      (void)  const { return _param4Fact.rawValue().toDouble(); }
    double      param5      (void)  const { return _param5Fact.rawValue().toDouble(); }
    double      param6      (void)  const { return _param6Fact.rawValue().toDouble(); }
    double      param7      (void)  const { return _param7Fact.rawValue().toDouble(); }

    void setCommand     (MAV_CMD command);
    void setFrame       (MAV_FRAME frame);
    void setAutoContinue(bool autoContinue);
    void setParam1      (double param1);
    void setParam2      (double param2);
    void setParam3      (double param3);
    void setParam4      (double param4);
    void setParam5      (double param5);
    void setParam6      (double param6);
    void setParam7      (double param7);

    // C++ only methods
    
    void save(QTextStream &saveStream);
    bool load(QTextStream &loadStream);

    bool relativeAltitude(void) { return frame() == MAV_FRAME_GLOBAL_RELATIVE_ALT; }

    static const double defaultAltitude;

public slots:
    void setDefaultsForCommand(void);

signals:
    void altDifferenceChanged       (double altDifference);
    void altPercentChanged          (double altPercent);
    void azimuthChanged             (double azimuth);
    void commandChanged             (MavlinkQmlSingleton::Qml_MAV_CMD command);
    void coordinateChanged          (const QGeoCoordinate& coordinate);
    void dirtyChanged               (bool dirty);
    void distanceChanged            (double distance);
    void frameChanged               (int frame);
    void friendlyEditAllowedChanged (bool friendlyEditAllowed);
    void headingDegreesChanged      (double heading);
    void homePositionValidChanged   (bool homePostionValid);
    void isCurrentItemChanged       (bool isCurrentItem);
    void rawEditChanged             (bool rawEdit);
    void sequenceNumberChanged      (int sequenceNumber);
    void uiModelChanged             (void);
    
private slots:
    void _setDirtyFromSignal(void);
    void _sendCommandChanged(void);
    void _sendCoordinateChanged(void);
    void _sendFrameChanged(void);
    void _sendFriendlyEditAllowedChanged(void);
    void _sendUiModelChanged(void);
    void _syncAltitudeRelativeToHomeToFrame(const QVariant& value);
    void _syncFrameToAltitudeRelativeToHome(void);

private:
    void _clearParamMetaData(void);
    void _connectSignals(void);
    void _setupMetaData(void);

private:
    bool        _rawEdit;
    bool        _dirty;
    int         _sequenceNumber;
    bool        _isCurrentItem;
    double      _altDifference;             ///< Difference in altitude from previous waypoint
    double      _altPercent;                ///< Percent of total altitude change in mission
    double      _azimuth;                   ///< Azimuth to previous waypoint
    double      _distance;                  ///< Distance to previous waypoint
    bool        _homePositionSpecialCase;   ///< true: This item is being used as a ui home position indicator
    bool        _homePositionValid;         ///< true: Home psition should be displayed

    Fact    _altitudeRelativeToHomeFact;
    Fact    _autoContinueFact;
    Fact    _commandFact;
    Fact    _frameFact;
    Fact    _param1Fact;
    Fact    _param2Fact;
    Fact    _param3Fact;
    Fact    _param4Fact;
    Fact    _param5Fact;
    Fact    _param6Fact;
    Fact    _param7Fact;
    Fact    _supportedCommandFact;
    
    static FactMetaData*    _altitudeMetaData;
    static FactMetaData*    _commandMetaData;
    static FactMetaData*    _defaultParamMetaData;
    static FactMetaData*    _frameMetaData;
    static FactMetaData*    _latitudeMetaData;
    static FactMetaData*    _longitudeMetaData;

    FactMetaData    _param1MetaData;
    FactMetaData    _param2MetaData;
    FactMetaData    _param3MetaData;
    FactMetaData    _param4MetaData;
    FactMetaData    _param5MetaData;
    FactMetaData    _param6MetaData;
    FactMetaData    _param7MetaData;

    /// This is used to reference any subsequent mission items which do not specify a coordinate.
    QmlObjectListModel  _childItems;

    bool _syncingAltitudeRelativeToHomeAndFrame;    ///< true: already in a sync signal, prevents signal loop
    bool _syncingHeadingDegreesAndParam4;           ///< true: already in a sync signal, prevents signal loop

    const QMap<MAV_CMD, MavCmdInfo*>& _mavCmdInfoMap;
};

QDebug operator<<(QDebug dbg, const MissionItem& missionItem);
QDebug operator<<(QDebug dbg, const MissionItem* missionItem);

#endif
