#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QStringView>

#include <optional>

#include "GPSType.h"

struct GPSReceiverConfig;

/// Family-level support is refined after the receiver identifies its model.
struct GPSReceiverCapabilities
{
    enum class Support
    {
        Unknown,
        Unsupported,
        Supported,
    };

    GPSType type = GPSType::u_blox;
    QString name;
    QString model;
    QString firmware;
    int manufacturerId = -1;
    Support nativePosition = Support::Unsupported;
    Support rtkBase = Support::Unsupported;
    Support nmeaOutput = Support::Unsupported;

    bool recognized() const { return manufacturerId >= 0; }

    QString validationError(const GPSReceiverConfig& config) const;

    static GPSReceiverCapabilities forType(GPSType type);
    static std::optional<GPSType> typeForName(QStringView name);
};
Q_DECLARE_METATYPE(GPSReceiverCapabilities)
