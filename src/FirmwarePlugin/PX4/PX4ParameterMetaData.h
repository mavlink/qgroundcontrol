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
#include "ParameterMetaData.h"

Q_DECLARE_LOGGING_CATEGORY(PX4ParameterMetaDataLog)

/// Loads and holds parameter fact meta data for PX4 stack
class PX4ParameterMetaData : public ParameterMetaData
{
    Q_OBJECT

public:
    explicit PX4ParameterMetaData(QObject *parent = nullptr);
    ~PX4ParameterMetaData() override;

    /// Parse the PX4 XML meta‑data file (thread‑safe; subsequent calls are ignored).
    void loadFromFile(const QString &xmlPath) final;

    /// Build QGC FactMetaData for @p paramName and @p vehicleType.
    FactMetaData *getMetaDataForFact(const QString &paramName, MAV_TYPE vehicleType, FactMetaData::ValueType_t type) final;

    /// Return <major, minor> meta‑data version encoded in the filename (e.g. v1.7.xml).
    static QVersionNumber versionFromFilename(QStringView path);

private:
    void _generateParameterJson();
    static void _jsonWriteLine(QFile &file, int indent, const QString &line);

    FactMetaData::NameToMetaDataMap_t _params;
};
