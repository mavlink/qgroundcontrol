/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef APMParameterMetaData_H
#define APMParameterMetaData_H

#include <QObject>
#include <QMap>
#include <QPointer>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include "FactSystem.h"
#include "AutoPilotPlugin.h"
#include "Vehicle.h"

Q_DECLARE_LOGGING_CATEGORY(APMParameterMetaDataLog)
Q_DECLARE_LOGGING_CATEGORY(APMParameterMetaDataVerboseLog)

class APMFactMetaDataRaw : public QObject
{
    Q_OBJECT
public:
    APMFactMetaDataRaw(QObject *parent = nullptr)
        : QObject(parent), rebootRequired(false)
    { }

    QString name;
    QString category;
    QString group;
    QString shortDescription;
    QString longDescription;
    QString min;
    QString max;
    QString incrementSize;
    QString units;
    bool    rebootRequired;
    bool    readOnly;
    QList<QPair<QString, QString> > values;
    QList<QPair<QString, QString> > bitmask;
};


/// Collection of Parameter Facts for PX4 AutoPilot

typedef QMap<QString, APMFactMetaDataRaw*> ParameterNametoFactMetaDataMap;

class APMParameterMetaData : public QObject
{
    Q_OBJECT
    
public:
    APMParameterMetaData(void);

    FactMetaData* getMetaDataForFact(const QString& name, MAV_TYPE vehicleType, FactMetaData::ValueType_t type);
    void loadParameterFactMetaDataFile(const QString& metaDataFile);

    static void getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion);

private:
    enum {
        XmlStateNone,
        XmlstateParamFileFound,
        XmlStateFoundVehicles,
        XmlStateFoundLibraries,
        XmlStateFoundParameters,
        XmlStateFoundVersion,
        XmlStateFoundGroup,
        XmlStateFoundParameter,
        XmlStateDone
    };    

    QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool* convertOk);
    bool skipXMLBlock(QXmlStreamReader& xml, const QString& blockName);
    bool parseParameterAttributes(QXmlStreamReader& xml, APMFactMetaDataRaw *rawMetaData);
    void correctGroupMemberships(ParameterNametoFactMetaDataMap& parameterToFactMetaDataMap, QMap<QString,QStringList>& groupMembers);
    QString mavTypeToString(MAV_TYPE vehicleTypeEnum);
    QString _groupFromParameterName(const QString& name);

    bool                                            _parameterMetaDataLoaded        = false;    ///< true: parameter meta data already loaded
    // FIXME: metadata is vehicle type specific now
    QMap<QString, ParameterNametoFactMetaDataMap>   _vehicleTypeToParametersMap;                ///< Maps from a vehicle type to paramametertoFactMeta map>
};

#endif
