/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtCore/QStringView>
#include <QtCore/QVersionNumber>

#include "FactMetaData.h"
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(ParameterMetaDataLog)

class QXmlStreamReader;

/// Loads and holds parameter fact meta data for firmware stack
class ParameterMetaData : public QObject
{
    Q_OBJECT

public:
    explicit ParameterMetaData(QObject *parent = nullptr) = default;
    virtual ~ParameterMetaData() = default;

    /// Parse the XML meta‑data file (thread‑safe; subsequent calls are ignored).
    void loadFromFile(const QString &xmlPath);

    /// Build QGC FactMetaData for @p paramName and @p vehicleType.
    FactMetaData *getMetaDataForFact(const QString &paramName, MAV_TYPE vehicleType, FactMetaData::ValueType_t type);

    /// Return <major, minor> meta‑data version encoded in the filename (e.g. v4.7.xml).
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

    using RawPtr = std::shared_ptr<APMFactMetaDataRaw>;
    using ParamMap = QHash<QString, RawPtr>;

    bool _parseParameterAttributes(QXmlStreamReader &xml, APMFactMetaDataRaw &raw);
    static QString _mavTypeToString(MAV_TYPE mavType);
    static QString _groupFromParameterName(const QString &name);
    static void _correctGroupMemberships(const ParamMap &paramToMeta, QHash<QString, QStringList> &groupMembers);
    static bool _parseRange(QStringView text, APMFactMetaDataRaw &out);
    static bool _parseBitmask(QStringView text, APMFactMetaDataRaw &out);

    std::atomic_bool _parsed = false;
    QHash<QString, ParamMap> _params;

    mutable QReadWriteLock _lock;
};
