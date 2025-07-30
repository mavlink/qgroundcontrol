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
#include <QtCore/QReadWriteLock>
#include <QtCore/QStringView>
#include <QtCore/QVersionNumber>

#include "FactMetaData.h"
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(ParameterMetaDataLog)

/// Loads and holds parameter fact meta data for firmware stack
class ParameterMetaData : public QObject
{
    Q_OBJECT

public:
    explicit ParameterMetaData(QObject *parent = nullptr)
        : QObject(parent)
    {}

    virtual ~ParameterMetaData() = default;

    /// Parse the XML meta‑data file (thread‑safe; subsequent calls are ignored).
    virtual void loadFromFile(const QString &xmlPath) = 0;

    /// Build QGC FactMetaData for @p paramName and @p vehicleType.
    virtual FactMetaData *getMetaDataForFact(const QString &paramName, MAV_TYPE vehicleType, FactMetaData::ValueType_t type) = 0;

    /// Return <major, minor> meta‑data version encoded in the filename (e.g. v4.7.xml).
    // static QVersionNumber versionFromFilename(QStringView path);

protected:
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

    std::atomic_bool _parsed = false;
    mutable QReadWriteLock _lock;
};
