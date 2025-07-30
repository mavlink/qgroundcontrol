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
#include <QtCore/QObject>

#include "FactMetaData.h"
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(PX4ParameterMetaDataLog)

/// Loads and holds parameter fact meta data for PX4 stack
class PX4ParameterMetaData : public QObject
{
    Q_OBJECT

public:
    explicit PX4ParameterMetaData(QObject *parent = nullptr);
    ~PX4ParameterMetaData();

    /// Parse the PX4 XML meta‑data file (thread‑safe; subsequent calls are ignored).
    void loadFromFile(const QString &xmlPath);

    /// Build QGC FactMetaData for @p paramName and @p vehicleType.
    FactMetaData *getMetaDataForFact(const QString &paramName, MAV_TYPE vehicleType, FactMetaData::ValueType_t type);

    /// Return <major, minor> meta‑data version encoded in the filename (e.g. v1.7.xml).
    static QVersionNumber versionFromFilename(QStringView path);

private:
    enum class XmlState : quint8 {
        None,
        ParamFileFound,
        FoundVehicles,
        FoundLibraries,
        FoundParameters,
        FoundParameter,
        FoundGroup,
        FoundVersion,
        Done
    };

    void _generateParameterJson();
    static void _jsonWriteLine(QFile &file, int indent, const QString &line);
    static void _outputFileWarning(const QString &metaDataFile, const QString &error1, const QString &error2);

    std::atomic_bool _parsed = false;
    FactMetaData::NameToMetaDataMap_t _params;
};
