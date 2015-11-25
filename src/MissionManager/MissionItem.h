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
#include <QJsonObject>
#include <QJsonValue>

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
    
    Q_PROPERTY(MavlinkQmlSingleton::Qml_MAV_CMD command READ command            WRITE setCommand            NOTIFY commandChanged)
    Q_PROPERTY(QString          commandDescription  READ commandDescription                                 NOTIFY commandChanged)
    Q_PROPERTY(QString          commandName         READ commandName                                        NOTIFY commandChanged)
    Q_PROPERTY(QGeoCoordinate   coordinate          READ coordinate             WRITE setCoordinate         NOTIFY coordinateChanged)
    Q_PROPERTY(bool             dirty               READ dirty                  WRITE setDirty              NOTIFY dirtyChanged)
    Q_PROPERTY(double           distance            READ distance               WRITE setDistance           NOTIFY distanceChanged)             ///< Distance to previous waypoint
    Q_PROPERTY(bool             friendlyEditAllowed READ friendlyEditAllowed                                NOTIFY friendlyEditAllowedChanged)
    Q_PROPERTY(bool             homePosition        READ homePosition                                       CONSTANT)                           ///< true: This item is being used as a home position indicator
    Q_PROPERTY(bool             homePositionValid   READ homePositionValid      WRITE setHomePositionValid  NOTIFY homePositionValidChanged)    ///< true: Home position should be shown
    Q_PROPERTY(bool             isCurrentItem       READ isCurrentItem          WRITE setIsCurrentItem      NOTIFY isCurrentItemChanged)
    Q_PROPERTY(bool             rawEdit             READ rawEdit                WRITE setRawEdit            NOTIFY rawEditChanged)              ///< true: raw item editing with all params
    Q_PROPERTY(int              sequenceNumber      READ sequenceNumber         WRITE setSequenceNumber     NOTIFY sequenceNumberChanged)
    Q_PROPERTY(bool             specifiesCoordinate READ specifiesCoordinate                                NOTIFY commandChanged)
    Q_PROPERTY(Fact*            supportedCommand    READ supportedCommand                                   NOTIFY commandChanged)

    // These properties are used to display the editing ui
    Q_PROPERTY(QmlObjectListModel*  checkboxFacts   READ checkboxFacts  NOTIFY uiModelChanged)
    Q_PROPERTY(QmlObjectListModel*  comboboxFacts   READ comboboxFacts  NOTIFY uiModelChanged)
    Q_PROPERTY(QmlObjectListModel*  textFieldFacts  READ textFieldFacts NOTIFY uiModelChanged)

    /// List of child mission items. Child mission item are subsequent mision items which do not specify a coordinate. They
    /// are shown next to the part item in the ui.
    Q_PROPERTY(QmlObjectListModel*  childItems      READ childItems     CONSTANT)

    // Property accesors
    
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
    bool            specifiesCoordinate (void) const;
    Fact*           supportedCommand    (void)          { return &_supportedCommandFact; }


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

    void setDistance(double distance);

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

    static const double defaultTakeoffPitch;
    static const double defaultHeading;
    static const double defaultAltitude;
    static const double defaultAcceptanceRadius;
    static const double defaultLoiterOrbitRadius;
    static const double defaultLoiterTurns;

public slots:
    void setDefaultsForCommand(void);

signals:
    void commandChanged             (MavlinkQmlSingleton::Qml_MAV_CMD command);
    void coordinateChanged          (const QGeoCoordinate& coordinate);
    void dirtyChanged               (bool dirty);
    void distanceChanged            (float distance);
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
    void _syncCommandToSupportedCommand(const QVariant& value);
    void _syncFrameToAltitudeRelativeToHome(void);
    void _syncSupportedCommandToCommand(const QVariant& value);

private:
    void _clearParamMetaData(void);
    void _connectSignals(void);
    bool _loadMavCmdInfoJson(void);
    void _setupMetaData(void);
    bool _validateKeyTypes(QJsonObject& jsonObject, const QStringList& keys, const QList<QJsonValue::Type>& types);

    static QVariant _degreesToRadians(const QVariant& degrees);
    static QVariant _radiansToDegrees(const QVariant& radians);

private:
    typedef struct {
        int     param;
        QString label;
        QString units;
        double  defaultValue;
        int     decimalPlaces;
    } ParamInfo_t;

    typedef struct {
        MAV_CMD                 command;
        QString                 rawName;
        QString                 friendlyName;
        QString                 description;
        bool                    specifiesCoordinate;
        bool                    friendlyEdit;
        QMap<int, ParamInfo_t>  paramInfoMap;
    } MavCmdInfo_t;
    
    bool        _rawEdit;
    bool        _dirty;
    int         _sequenceNumber;
    bool        _isCurrentItem;
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
    static FactMetaData*    _supportedCommandMetaData;

    FactMetaData    _param1MetaData;
    FactMetaData    _param2MetaData;
    FactMetaData    _param3MetaData;
    FactMetaData    _param4MetaData;

    /// This is used to reference any subsequent mission items which do not specify a coordinate.
    QmlObjectListModel  _childItems;

    bool _syncingAltitudeRelativeToHomeAndFrame;    ///< true: already in a sync signal, prevents signal loop
    bool _syncingHeadingDegreesAndParam4;           ///< true: already in a sync signal, prevents signal loop
    bool _syncingSupportedCommandAndCommand;         ///< true: already in a sync signal, prevents signal loop

    static QMap<MAV_CMD, MavCmdInfo_t> _mavCmdInfoMap;

    static const QString _decimalPlacesJsonKey;
    static const QString _defaultJsonKey;
    static const QString _descriptionJsonKey;
    static const QString _friendlyNameJsonKey;
    static const QString _friendlyEditJsonKey;
    static const QString _idJsonKey;
    static const QString _labelJsonKey;
    static const QString _mavCmdInfoJsonKey;
    static const QString _param1JsonKey;
    static const QString _param2JsonKey;
    static const QString _param3JsonKey;
    static const QString _param4JsonKey;
    static const QString _paramJsonKeyFormat;
    static const QString _rawNameJsonKey;
    static const QString _specifiesCoordinateJsonKey;
    static const QString _unitsJsonKey;
    static const QString _versionJsonKey;

    static const QString _degreesUnits;
};

QDebug operator<<(QDebug dbg, const MissionItem& missionItem);
QDebug operator<<(QDebug dbg, const MissionItem* missionItem);

#endif
