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
#include <QtCore/QStringView>
#include <QtCore/QVersionNumber>

#include "FactMetaData.h"
#include "MAVLinkLib.h"
#include "ParameterMetaData.h"

Q_DECLARE_LOGGING_CATEGORY(APMParameterMetaDataLog)

class QXmlStreamReader;

/// Loads and holds parameter fact meta data for ArduPilot stack
class APMParameterMetaData : public ParameterMetaData
{
    Q_OBJECT

public:
    explicit APMParameterMetaData(QObject *parent = nullptr);
    ~APMParameterMetaData() override;

    /// Parse the ArduPilot XML meta‑data file (thread‑safe; subsequent calls are ignored).
    void loadFromFile(const QString &xmlPath) final;

    /// Build QGC FactMetaData for @p paramName and @p vehicleType.
    FactMetaData *getMetaDataForFact(const QString &paramName, MAV_TYPE vehicleType, FactMetaData::ValueType_t type) final;

    /// Return <major, minor> meta‑data version encoded in the filename (e.g. v4.7.xml).
    static QVersionNumber versionFromFilename(QStringView path);

private:
    struct APMFactMetaDataRaw {
        QString name;
        QString category;
        QString group;
        QString shortDescription;
        QString longDescription;
        QString min;
        QString max;
        QString incrementSize;
        QString units;
        QString unitText;
        bool rebootRequired = false;
        bool readOnly = false;
        using Value = QPair<QString, QString>;
        QList<Value> values;
        using Bit = QPair<QString, QString>;
        QList<Bit> bitmask;
    };

    using RawPtr = std::shared_ptr<APMFactMetaDataRaw>;
    using ParamMap = QHash<QString, RawPtr>;

    bool _parseParameterAttributes(QXmlStreamReader &xml, APMFactMetaDataRaw &raw);
    static QString _mavTypeToString(MAV_TYPE mavType);
    static QString _groupFromParameterName(const QString &name);
    static void _correctGroupMemberships(const ParamMap &paramToMeta, QHash<QString, QStringList> &groupMembers);
    static bool _parseRange(QStringView text, APMFactMetaDataRaw &out);
    static bool _parseBitmask(QStringView text, APMFactMetaDataRaw &out);

    QHash<QString, ParamMap> _params;
};
