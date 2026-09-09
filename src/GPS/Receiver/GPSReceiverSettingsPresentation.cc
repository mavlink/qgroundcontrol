#include "GPSReceiverSettingsPresentation.h"

#include "GPSConfigurationReport.h"
#include "GPSReceiverCapabilities.h"

namespace {
QString kindName(GPSReceiverCapabilities::SettingDescriptor::Kind kind)
{
    using Kind = GPSReceiverCapabilities::SettingDescriptor::Kind;
    switch (kind) {
        case Kind::Flags:
            return QStringLiteral("flags");
        case Kind::Enum:
            return QStringLiteral("enum");
        case Kind::Number:
            return QStringLiteral("number");
    }
    return {};
}
}  // namespace

QVariantList GPSReceiverSettingsPresentation::settingDescriptors(const GPSReceiverCapabilities& capabilities,
                                                                 bool baseStation)
{
    QVariantList result;
    for (const auto& descriptor : capabilities.settings(baseStation)) {
        QVariantList values;
        for (int value : descriptor.values) {
            values.append(value);
        }
        result.append(QVariantMap{{QStringLiteral("key"), GPSReceiverSettings::key(descriptor.id)},
                                  {QStringLiteral("label"), descriptor.label},
                                  {QStringLiteral("units"), descriptor.units},
                                  {QStringLiteral("kind"), kindName(descriptor.kind)},
                                  {QStringLiteral("defaultValue"), descriptor.defaultValue},
                                  {QStringLiteral("minimum"), descriptor.minimum},
                                  {QStringLiteral("maximum"), descriptor.maximum},
                                  {QStringLiteral("values"), values},
                                  {QStringLiteral("labels"), descriptor.labels},
                                  {QStringLiteral("support"), static_cast<int>(descriptor.support)},
                                  {QStringLiteral("requiredMask"), descriptor.requiredMask},
                                  {QStringLiteral("requiresReconnect"), descriptor.requiresReconnect}});
    }
    return result;
}

QVariantList GPSReceiverSettingsPresentation::configurationReport(const GPSConfigurationReport& report)
{
    QVariantList result;
    for (const auto& setting : report.settings) {
        result.append(QVariantMap{{QStringLiteral("key"), GPSReceiverSettings::key(setting.id)},
                                  {QStringLiteral("label"), setting.label},
                                  {QStringLiteral("units"), setting.units},
                                  {QStringLiteral("requestedValue"), setting.requestedValue},
                                  {QStringLiteral("reportedValue"), setting.reportedValue},
                                  {QStringLiteral("requestState"), static_cast<int>(setting.requestState)},
                                  {QStringLiteral("readbackState"), static_cast<int>(setting.readbackState)},
                                  {QStringLiteral("comparisonApplicable"), setting.comparisonApplicable},
                                  {QStringLiteral("matchesRequested"), setting.matchesRequested},
                                  {QStringLiteral("detail"), setting.detail}});
    }
    return result;
}
