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

/// Collection of Parameter Facts for PX4 AutoPilot
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

    /// Converts a string to a typed QVariant
    ///     @param string String to convert
    ///     @param type Type for Fact which dictates the QVariant type as well
    ///     @param convertOk Returned: true: conversion success, false: conversion failure
    /// @return Returns the correctly type QVariant
    static QVariant _stringToTypedVariant(const QString &string, FactMetaData::ValueType_t type, bool *convertOk);
    static bool _skipXMLBlock(QXmlStreamReader &xml, const QString &blockName);
    bool _parseParameterAttributes(QXmlStreamReader &xml, APMFactMetaDataRaw *rawMetaData);
    static void _correctGroupMemberships(ParameterNametoFactMetaDataMap &parameterToFactMetaDataMap, QMap<QString,QStringList> &groupMembers);
    static QString _mavTypeToString(MAV_TYPE vehicleTypeEnum);
    static QString _groupFromParameterName(const QString &name);

    bool _parameterMetaDataLoaded = false; ///< true: parameter meta data already loaded
    // FIXME: metadata is vehicle type specific now
    QMap<QString, ParameterNametoFactMetaDataMap> _vehicleTypeToParametersMap; ///< Maps from a vehicle type to paramametertoFactMeta map>

    static constexpr const char *kInvalidConverstion = "Internal Error: No support for string parameters";
};
