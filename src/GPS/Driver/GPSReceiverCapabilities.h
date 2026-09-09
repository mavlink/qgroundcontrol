#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QVariantList>

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
    Support correctionInput = Support::Unsupported;
    Support constellationSelection = Support::Unsupported;
    Support dynamicModelSelection = Support::Unsupported;
    Support outputRateSelection = Support::Unsupported;
    Support headingOffsetSelection = Support::Unsupported;
    int supportedConstellations = 31;

    struct SettingDescriptor
    {
        QString key;
        QString label;
        QString units;
        QString kind;
        double defaultValue = 0;
        double minimum = 0;
        double maximum = 0;
        QList<int> values;
        QStringList labels;
        Support support = Support::Unsupported;
        bool requiresReconnect = true;
        int requiredMask = 0;

        bool accepts(double value) const;
    };

    QList<SettingDescriptor> settings(bool baseStation = false) const;
    QVariantList settingDescriptors(bool baseStation = false) const;

    bool recognized() const { return manufacturerId >= 0; }

    QString validationError(const GPSReceiverConfig& config) const;

    static GPSReceiverCapabilities forType(GPSType type);
    static std::optional<GPSType> typeForName(QStringView name);
};
Q_DECLARE_METATYPE(GPSReceiverCapabilities)
