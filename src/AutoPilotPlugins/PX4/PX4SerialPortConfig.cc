#include "PX4SerialPortConfig.h"

#include <algorithm>

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>

namespace PX4SerialPortConfig {

namespace {

// Port index -> tag, from serial_ports in PX4's Tools/serial/generate_config.py. PX4 never reuses or changes an index.
const QHash<int, QString>& portTags()
{
    static const QHash<int, QString> tags = {
        {1, QStringLiteral("URT1")},   {2, QStringLiteral("URT2")},   {3, QStringLiteral("URT3")},
        {4, QStringLiteral("URT4")},   {5, QStringLiteral("URT5")},   {6, QStringLiteral("URT6")},
        {7, QStringLiteral("URT7")},   {8, QStringLiteral("URT8")},   {9, QStringLiteral("URT9")},
        {101, QStringLiteral("TEL1")}, {102, QStringLiteral("TEL2")}, {103, QStringLiteral("TEL3")},
        {104, QStringLiteral("TEL4")}, {201, QStringLiteral("GPS1")}, {202, QStringLiteral("GPS2")},
        {203, QStringLiteral("GPS3")}, {300, QStringLiteral("RC")},   {301, QStringLiteral("WIFI")},
        {401, QStringLiteral("EXT2")},
    };
    return tags;
}

constexpr int kFirstNonUartPortId = 101;
constexpr int kRcPortId = 300;

const Function* findFunction(const QList<Function>& functions, const QString& paramName)
{
    const auto it = std::find_if(functions.cbegin(), functions.cend(),
                                 [&paramName](const Function& function) { return function.paramName == paramName; });
    return (it == functions.cend()) ? nullptr : &(*it);
}

bool allowsPort(const Function& function, int portId)
{
    return std::any_of(function.ports.cbegin(), function.ports.cend(),
                       [portId](const Port& port) { return port.id == portId; });
}

/// Matches the functions a port is normally used for, or nullptr if the port has none
const QRegularExpression* usualFunctionsRegex(int portId)
{
    static const QRegularExpression gpsRegex(QStringLiteral("^(GPS_\\d_CONFIG|SEP_PORT\\d_CFG)$"));
    static const QRegularExpression telemetryRegex(QStringLiteral("^(MAV_\\d_CONFIG|UXRCE_DDS_CFG)$"));
    static const QRegularExpression rcRegex(QStringLiteral("^RC_"));

    const QString tag = portTags().value(portId);
    if (tag.startsWith(QStringLiteral("TEL")) || (tag == QStringLiteral("WIFI")) || (portId == kEthernetPortId)) {
        return &telemetryRegex;
    }
    if (tag.startsWith(QStringLiteral("GPS"))) {
        return &gpsRegex;
    }
    if (tag == QStringLiteral("RC")) {
        return &rcRegex;
    }
    return nullptr;
}

}  // namespace

bool isFunctionParam(const QVariantList& enumValues)
{
    bool hasDisabled = false;
    bool hasNonUartPort = false;

    for (const QVariant& enumValue : enumValues) {
        bool ok = false;
        const int value = enumValue.toInt(&ok);
        if (!ok) {
            return false;
        }
        if (value == kDisabledPortId) {
            hasDisabled = true;
        } else if (portTags().contains(value)) {
            // UART indices 1-9 overlap ordinary small enums, so they don't identify a port selection on their own
            hasNonUartPort = hasNonUartPort || (value >= kFirstNonUartPortId);
        } else if (value != kEthernetPortId) {
            return false;
        }
    }

    return hasDisabled && hasNonUartPort;
}

QString baudParamName(int portId)
{
    const QString tag = portTags().value(portId);
    return tag.isEmpty() ? QString() : QStringLiteral("SER_%1_BAUD").arg(tag);
}

QString functionLabel(const QString& paramName, const QString& shortDescription)
{
    static const QString prefix = QStringLiteral("Serial Configuration for ");

    if (shortDescription.startsWith(prefix)) {
        return shortDescription.mid(prefix.length());
    }
    return shortDescription.isEmpty() ? paramName : shortDescription;
}

Function makeFunction(const QString& paramName, const QString& shortDescription, const QVariantList& enumValues,
                      const QStringList& enumStrings, int value)
{
    Function function{
        .paramName = paramName,
        .label = functionLabel(paramName, shortDescription),
        .ports = {},
    };

    for (qsizetype i = 0; i < enumValues.count(); i++) {
        const int portId = enumValues[i].toInt();
        if (portId != kDisabledPortId) {
            function.ports.append({.id = portId, .label = enumStrings.value(i, QString::number(portId))});
        }
    }

    if ((paramName == QStringLiteral("RC_PORT_CONFIG")) && allowsPort(function, kRcPortId)) {
        function.defaultPortId = kRcPortId;
    }
    function.currentPortId = (value == kDisabledPortId) ? function.defaultPortId : value;

    return function;
}

QList<Port> ports(const QList<Function>& functions)
{
    QList<Port> result;

    for (const Function& function : functions) {
        for (const Port& port : function.ports) {
            const bool known = std::any_of(result.cbegin(), result.cend(),
                                           [&port](const Port& existing) { return existing.id == port.id; });
            if (!known) {
                result.append(port);
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const Port& a, const Port& b) { return a.id < b.id; });
    return result;
}

QStringList functionsOnPort(const QList<Function>& functions, int portId)
{
    QStringList result;

    if (portId == kDisabledPortId) {
        return result;
    }
    for (const Function& function : functions) {
        if (function.currentPortId == portId) {
            result.append(function.paramName);
        }
    }

    return result;
}

QList<Function> functionsForPort(const QList<Function>& functions, int portId)
{
    QList<Function> usual;
    QList<Function> others;
    const QRegularExpression* const usualRegex = usualFunctionsRegex(portId);

    for (const Function& function : functions) {
        if (!allowsPort(function, portId)) {
            continue;
        }
        if (usualRegex && usualRegex->match(function.paramName).hasMatch()) {
            usual.append(function);
        } else {
            others.append(function);
        }
    }

    return usual + others;
}

QList<ParamWrite> assignPort(const QList<Function>& functions, int portId, const QString& functionParamName)
{
    QList<ParamWrite> writes;

    if (!functionParamName.isEmpty()) {
        const Function* const function = findFunction(functions, functionParamName);
        if (!function || !allowsPort(*function, portId)) {
            return writes;
        }
    }

    if (!isSharedPort(portId)) {
        for (const Function& function : functions) {
            if ((function.currentPortId == portId) && (function.paramName != functionParamName) &&
                (function.defaultPortId != portId)) {
                writes.append({.paramName = function.paramName, .value = kDisabledPortId});
            }
        }
    }

    if (!functionParamName.isEmpty() && (findFunction(functions, functionParamName)->currentPortId != portId)) {
        writes.append({.paramName = functionParamName, .value = portId});
    }

    return writes;
}

QList<ParamWrite> removeFromPort(const QList<Function>& functions, int portId, const QString& functionParamName)
{
    const Function* const function = findFunction(functions, functionParamName);
    if (!function || (portId == kDisabledPortId) || (function->currentPortId != portId) ||
        (function->defaultPortId == portId)) {
        return {};
    }
    return {{.paramName = functionParamName, .value = kDisabledPortId}};
}

bool isSharedPort(int portId)
{
    return portId == kEthernetPortId;
}

QStringList optionParamNames(const QString& functionParamName, int portId)
{
    static const QRegularExpression mavlinkRegex(QStringLiteral("^MAV_(\\d)_CONFIG$"));
    static const QRegularExpression gpsRegex(QStringLiteral("^GPS_(\\d)_CONFIG$"));

    if (portId == kDisabledPortId) {
        return {};
    }

    QStringList suffixes;
    QString prefix;

    if (const QRegularExpressionMatch mavlinkMatch = mavlinkRegex.match(functionParamName); mavlinkMatch.hasMatch()) {
        prefix = QStringLiteral("MAV_%1_").arg(mavlinkMatch.captured(1));
        suffixes = {QStringLiteral("MODE"), QStringLiteral("RATE"), QStringLiteral("FORWARD")};
        if (portId == kEthernetPortId) {
            suffixes += {QStringLiteral("UDP_PRT"), QStringLiteral("REMOTE_PRT"), QStringLiteral("BROADCAST")};
        } else {
            suffixes += QStringLiteral("FLOW_CTRL");
        }
    } else if (const QRegularExpressionMatch gpsMatch = gpsRegex.match(functionParamName); gpsMatch.hasMatch()) {
        prefix = QStringLiteral("GPS_%1_").arg(gpsMatch.captured(1));
        suffixes = {QStringLiteral("PROTOCOL")};
    }

    QStringList names;
    for (const QString& suffix : suffixes) {
        names.append(prefix + suffix);
    }
    return names;
}

}  // namespace PX4SerialPortConfig
