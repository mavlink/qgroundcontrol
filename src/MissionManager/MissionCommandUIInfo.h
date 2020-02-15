/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef MissionCommandUIInfo_H
#define MissionCommandUIInfo_H

#include "QGCToolbox.h"
#include "QGCMAVLink.h"

#include <QString>
#include <QVariant>

class MissionCommandTree;
class MissionCommandUIInfo;
#ifdef UNITTEST_BUILD
class MissionCommandTreeTest;
#endif

/// UI Information associated with a mission command (MAV_CMD) parameter
///
/// MissionCommandParamInfo is used to automatically generate editing ui for a parameter associated with a MAV_CMD.
///
/// The json format for a MissionCmdParamInfo object is:
///
/// Key             Type    Default     Description
/// label           string  required    Label for text field
/// units           string              Units for value, should use FactMetaData units strings in order to get automatic translation
/// default         double  0.0/NaN     Default value for param. If no default value specified and nanUnchanged == true, then defaultValue is NaN.
/// decimalPlaces   int     7           Number of decimal places to show for value
/// enumStrings     string              Strings to show in combo box for selection
/// enumValues      string              Values associated with each enum string
/// nanUnchanged    bool    false       True: value can be set to NaN to signal unchanged
///
class MissionCmdParamInfo : public QObject {

    Q_OBJECT

public:
    MissionCmdParamInfo(QObject* parent = nullptr);

    MissionCmdParamInfo(const MissionCmdParamInfo& other, QObject* parent = nullptr);
    const MissionCmdParamInfo& operator=(const MissionCmdParamInfo& other);

    Q_PROPERTY(int          decimalPlaces   READ decimalPlaces  CONSTANT)
    Q_PROPERTY(double       defaultValue    READ defaultValue   CONSTANT)
    Q_PROPERTY(QStringList  enumStrings     READ enumStrings    CONSTANT)
    Q_PROPERTY(QVariantList enumValues      READ enumValues     CONSTANT)
    Q_PROPERTY(QString      label           READ label          CONSTANT)
    Q_PROPERTY(int          param           READ param          CONSTANT)
    Q_PROPERTY(QString      units           READ units          CONSTANT)
    Q_PROPERTY(bool         nanUnchanged    READ nanUnchanged   CONSTANT)

    int             decimalPlaces   (void) const { return _decimalPlaces; }
    double          defaultValue    (void) const { return _defaultValue; }
    QStringList     enumStrings     (void) const { return _enumStrings; }
    QVariantList    enumValues      (void) const { return _enumValues; }
    QString         label           (void) const { return _label; }
    int             param           (void) const { return _param; }
    QString         units           (void) const { return _units; }
    bool            nanUnchanged    (void) const { return _nanUnchanged; }

private:
    int             _decimalPlaces;
    double          _defaultValue;
    QStringList     _enumStrings;
    QVariantList    _enumValues;
    QString         _label;
    int             _param;
    QString         _units;
    bool            _nanUnchanged;

    friend class MissionCommandTree;
    friend class MissionCommandUIInfo;
};

/// UI Information associated with a mission command (MAV_CMD)
///
/// MissionCommandUIInfo is used to automatically generate editing ui for a MAV_CMD. This object also supports the concept of only having a set of partial
/// information for the command. This is used to create overrides of the base command information. For on override just specify the keys you want to modify
/// from the base command ui info. To override param ui info you must specify the entire MissionParamInfo object.
///
/// The json format for a MissionCommandUIInfo object is:
///
/// Key                     Type    Default     Description
/// id                      int     reauired    MAV_CMD id
/// comment                 string              Used to add a comment
/// rawName                 string  required    MAV_CMD enum name, should only be set of base tree information
/// friendlyName            string  rawName     Short description of command
/// description             string              Long description of command
/// specifiesCoordinate     bool    false       true: Command specifies a lat/lon/alt coordinate
/// specifiesAltitudeOnly   bool    false       true: Command specifies an altitude only (no coordinate)
/// standaloneCoordinate    bool    false       true: Vehicle does not fly through coordinate associated with command (exampl: ROI)
/// friendlyEdit            bool    false       true: Command supports friendly editing dialog, false: Command supports 'Show all values" style editing only
/// category                string  Advanced    Category which this command belongs to
/// paramRemove             string              Used by an override to remove params, example: "1,3" will remove params 1 and 3 on the override
/// param[1-7]              object              MissionCommandParamInfo object
///
class MissionCommandUIInfo : public QObject {
    Q_OBJECT

public:
    MissionCommandUIInfo(QObject* parent = nullptr);

    MissionCommandUIInfo(const MissionCommandUIInfo& other, QObject* parent = nullptr);
    const MissionCommandUIInfo& operator=(const MissionCommandUIInfo& other);

    Q_PROPERTY(QString  category                READ category               CONSTANT)
    Q_PROPERTY(QString  description             READ description            CONSTANT)
    Q_PROPERTY(bool     friendlyEdit            READ friendlyEdit           CONSTANT)
    Q_PROPERTY(QString  friendlyName            READ friendlyName           CONSTANT)
    Q_PROPERTY(QString  rawName                 READ rawName                CONSTANT)
    Q_PROPERTY(bool     isStandaloneCoordinate  READ isStandaloneCoordinate CONSTANT)
    Q_PROPERTY(bool     specifiesCoordinate     READ specifiesCoordinate    CONSTANT)
    Q_PROPERTY(bool     specifiesAltitudeOnly   READ specifiesAltitudeOnly  CONSTANT)
    Q_PROPERTY(int      command                 READ intCommand             CONSTANT)

    MAV_CMD command(void) const { return _command; }
    int     intCommand(void) const { return (int)_command; }

    QString category                (void) const;
    QString description             (void) const;
    bool    friendlyEdit            (void) const;
    QString friendlyName            (void) const;
    QString rawName                 (void) const;
    bool    isStandaloneCoordinate  (void) const;
    bool    specifiesCoordinate     (void) const;
    bool    specifiesAltitudeOnly   (void) const;

    /// Load the data in the object from the specified json
    ///     @param jsonObject Json object to load from
    ///     @param requireFullObject true: not a partial load, false: partial load allowed
    /// @return true: success, false: failure, errorString set
    bool loadJsonInfo(const QJsonObject& jsonObject, bool requireFullObject, QString& errorString);

    /// Retruns parameter information for specified parameter
    ///     @param index paremeter index to retrieve, 1-7
    ///     @param showUI true: show parameter in editor, false: hide parameter in editor
    /// @return Param info for index, NULL for none available
    const MissionCmdParamInfo* getParamInfo(int index, bool& showUI) const;

private:
    QString _loadErrorString(const QString& errorString) const;

    /// Returns whether the specific information value is available
    bool _infoAvailable(const QString& key) const { return _infoMap.contains(key); }

    /// Returns the values for the specified value
    const QVariant _infoValue(const QString& key) const { return _infoMap[key]; }

    /// Set the value for the specified piece of information
    void _setInfoValue(const QString& key, const QVariant& value) { _infoMap[key] = value; }

    /// Overrides the existing values with new ui info
    ///     @param uiInfo New ui info to override existing info
    void _overrideInfo(MissionCommandUIInfo* uiInfo);

    MAV_CMD                         _command;
    QMap<QString, QVariant>         _infoMap;
    QMap<int, MissionCmdParamInfo*> _paramInfoMap;
    QList<int>                      _paramRemoveList;

    static const char* _categoryJsonKey;
    static const char* _decimalPlacesJsonKey;
    static const char* _defaultJsonKey;
    static const char* _descriptionJsonKey;
    static const char* _enumStringsJsonKey;
    static const char* _enumValuesJsonKey;
    static const char* _nanUnchangedJsonKey;
    static const char* _friendlyNameJsonKey;
    static const char* _friendlyEditJsonKey;
    static const char* _idJsonKey;
    static const char* _labelJsonKey;
    static const char* _mavCmdInfoJsonKey;
    static const char* _param1JsonKey;
    static const char* _param2JsonKey;
    static const char* _param3JsonKey;
    static const char* _param4JsonKey;
    static const char* _param5JsonKey;
    static const char* _param6JsonKey;
    static const char* _param7JsonKey;
    static const char* _paramJsonKeyFormat;
    static const char* _paramRemoveJsonKey;
    static const char* _rawNameJsonKey;
    static const char* _standaloneCoordinateJsonKey;
    static const char* _specifiesCoordinateJsonKey;
    static const char* _specifiesAltitudeOnlyJsonKey;
    static const char* _unitsJsonKey;
    static const char* _commentJsonKey;    
    static const char* _advancedCategory;

    friend class MissionCommandTree;    
#ifdef UNITTEST_BUILD
    friend class MissionCommandTreeTest;
#endif
};

#endif
