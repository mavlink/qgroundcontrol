/****************************************************************************
 * FailureInjection.cc
 ****************************************************************************/
#include "FailureInjection.h"

#include <QtCore/QMetaEnum>
#include <QtCore/QVariantMap>

#include "MAVLinkEnumsQml.h"  // MAVLinkEnums::FAILURE_UNIT / FAILURE_TYPE, Q_ENUM_NS-reflected from the MAVLink dialect
#include "MAVLinkLib.h"       // MAV_RESULT_* for resolveResult()
#include "QGCMAVLink.h"       // QGCMAVLink::mavResultToString() fallback for resolveResult()

namespace {

QVariantMap _makeUnit(const QString& name, int unit)
{
    return QVariantMap{{"name", name}, {"unit", unit}};
}

QVariantMap _makeType(const QString& name, int type)
{
    return QVariantMap{{"name", name}, {"type", type}};
}

/// Strip the longest matching MAVLink enum prefix, e.g. FAILURE_UNIT_SENSOR_GYRO -> GYRO.
QString _stripPrefix(const QString& key, const QStringList& prefixesLongestFirst)
{
    for (const QString& prefix : prefixesLongestFirst) {
        if (key.startsWith(prefix)) {
            return key.mid(prefix.length());
        }
    }
    return key;
}

/// Builds a {name, value} catalog from the Q_ENUM_NS-exposed enum on MAVLinkEnums::staticMetaObject,
/// looked up by name.
QVariantList _buildCatalog(const char* enumName, const QStringList& prefixesLongestFirst,
                           QVariantMap (*makeEntry)(const QString&, int))
{
    QVariantList list;
    const QMetaObject& enumsMetaObject = MAVLinkEnums::staticMetaObject;
    const QMetaEnum me = enumsMetaObject.enumerator(enumsMetaObject.indexOfEnumerator(enumName));
    for (int i = 0; i < me.keyCount(); ++i) {
        const QString key = QString::fromLatin1(me.key(i));
        if (key.endsWith(QStringLiteral("_ENUM_END"))) {
            continue;  // dialect sentinel, not a real value
        }
        list.append(makeEntry(_stripPrefix(key, prefixesLongestFirst), me.value(i)));
    }
    return list;
}

}  // namespace

FailureInjection::FailureInjection(QObject* parent) : QObject(parent)
{
    _units = _buildCatalog("FAILURE_UNIT",
                           {QStringLiteral("FAILURE_UNIT_SENSOR_"), QStringLiteral("FAILURE_UNIT_SYSTEM_"),
                            QStringLiteral("FAILURE_UNIT_")},
                           _makeUnit);
    _types = _buildCatalog("FAILURE_TYPE", {QStringLiteral("FAILURE_TYPE_")}, _makeType);
}

void FailureInjection::logRow(const QString& unitName, const QString& typeName, const QString& instanceLabel,
                              const QString& time)
{
    _activity.prepend(QVariantMap{{"time", time},
                                  {"unitName", unitName},
                                  {"typeName", typeName},
                                  {"instance", instanceLabel},
                                  {"result", QStringLiteral("pending")}});
    emit activityChanged();
}

void FailureInjection::logInjection(const QString& unitName, const QString& typeName, int unitEnum,
                                    const QString& instanceLabel, const QString& time)
{
    logRow(unitName, typeName, instanceLabel, time);
    if (!_injectedUnits.contains(unitEnum)) {
        _injectedUnits.append(unitEnum);
    }
}

void FailureInjection::resolveResult(int ackResult)
{
    // ACKs for MAV_CMD_INJECT_FAILURE arrive in send order; resolve the oldest pending row (highest index, since newest
    // is prepended).
    int pendingIndex = -1;
    for (int i = _activity.size() - 1; i >= 0; --i) {
        if (_activity.at(i).toMap().value(QStringLiteral("result")).toString() == QStringLiteral("pending")) {
            pendingIndex = i;
            break;
        }
    }
    if (pendingIndex < 0) {
        return;
    }

    if (ackResult == MAV_RESULT_IN_PROGRESS) {
        return;  // not a terminal result; leave pending
    }

    QString reason;
    switch (ackResult) {
        // Not tr()'d: FailureInjectionComponent.qml matches this exact literal to style the row green.
        case MAV_RESULT_ACCEPTED:
            reason = QStringLiteral("accepted");
            break;
        case MAV_RESULT_TEMPORARILY_REJECTED:
            reason = tr("Temporarily rejected");
            break;
        case MAV_RESULT_DENIED:
            reason = tr("Denied");
            break;
        case MAV_RESULT_UNSUPPORTED:
            reason = tr("Unsupported");
            break;
        case MAV_RESULT_FAILED:
            reason = tr("Failed");
            break;
        case MAV_RESULT_CANCELLED:
            reason = tr("Cancelled");
            break;
        default:
            reason = QGCMAVLink::mavResultToString(static_cast<uint8_t>(ackResult));
            break;
    }

    QVariantMap row = _activity.at(pendingIndex).toMap();
    row[QStringLiteral("result")] = reason;
    _activity[pendingIndex] = row;
    emit activityChanged();
}

QVariantList FailureInjection::injectedUnits(void) const
{
    QVariantList list;
    for (int unitEnum : _injectedUnits) {
        list.append(unitEnum);
    }
    return list;
}

void FailureInjection::clearInjectedUnits(void)
{
    _injectedUnits.clear();
}

QVariantList FailureInjection::detailParams(int unitEnum, int typeEnum) const
{
    // Vehicle parameters that refine how a failure manifests, keyed by (unit, type). The page shows
    // an editor per entry, but only when the connected vehicle actually exposes the parameter.
    // Adding a new combo is one more table line.
    struct DetailParam
    {
        int unitEnum;
        int typeEnum;
        const char* param;
        const char* label;
    };

    static const DetailParam table[] = {
        {FAILURE_UNIT_SYSTEM_BATTERY, FAILURE_TYPE_WRONG, "SYS_FAIL_BAT_LVL", QT_TR_NOOP("Battery level")},
    };

    QVariantList list;
    for (const DetailParam& entry : table) {
        if (entry.unitEnum == unitEnum && entry.typeEnum == typeEnum) {
            list.append(QVariantMap{{"param", QString::fromLatin1(entry.param)}, {"label", tr(entry.label)}});
        }
    }
    return list;
}
