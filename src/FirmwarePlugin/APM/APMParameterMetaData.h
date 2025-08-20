/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QXmlStreamReader>

#include "MAVLinkLib.h"
#include "FactMetaData.h"

Q_DECLARE_LOGGING_CATEGORY(APMParameterMetaDataLog)
Q_DECLARE_LOGGING_CATEGORY(APMParameterMetaDataVerboseLog)

class APMFactMetaDataRaw : public QObject
{
    Q_OBJECT
public:
    explicit APMFactMetaDataRaw(QObject *parent = nullptr)
        : QObject(parent)
    { }
    ~APMFactMetaDataRaw() {}

    QString name;
    QString category;
    QString group;
    QString shortDescription;
    QString longDescription;
    QString min;
    QString max;
    QString incrementSize;
    QString units;
    bool rebootRequired = false;
    bool readOnly;
    QList<QPair<QString, QString>> values;
    QList<QPair<QString, QString>> bitmask;
};

/*===========================================================================*/

typedef QMap<QString, APMFactMetaDataRaw*> ParameterNametoFactMetaDataMap;

/// Collection of Parameter Facts for ArduPilot
class APMParameterMetaData : public QObject
{
    Q_OBJECT

public:
    explicit APMParameterMetaData(QObject *parent = nullptr);
    ~APMParameterMetaData();

    FactMetaData *getMetaDataForFact(const QString &name, MAV_TYPE vehicleType, FactMetaData::ValueType_t type);
    void loadParameterFactMetaDataFile(const QString &metaDataFile);

    static void getParameterMetaDataVersionInfo(const QString &metaDataFile, int &majorVersion, int &minorVersion);

private:
    enum XmlState {
        None,
        ParamFileFound,
        FoundVehicles,
        FoundLibraries,
        FoundParameters,
        FoundVersion,
        FoundGroup,
        FoundParameter,
        Done
    };

    static bool _skipXMLBlock(QXmlStreamReader &xml, const QString &blockName);
    bool _parseParameterAttributes(QXmlStreamReader &xml, APMFactMetaDataRaw *rawMetaData);
    static void _correctGroupMemberships(ParameterNametoFactMetaDataMap &parameterToFactMetaDataMap, QMap<QString,QStringList> &groupMembers);
    static QString _mavTypeToString(MAV_TYPE vehicleTypeEnum);
    static QString _groupFromParameterName(const QString &name);

    bool _parameterMetaDataLoaded = false; ///< true: parameter meta data already loaded
    // FIXME: metadata is vehicle type specific now
    QMap<QString, ParameterNametoFactMetaDataMap> _vehicleTypeToParametersMap; ///< Maps from a vehicle type to paramametertoFactMeta map>
};
