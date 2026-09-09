#pragma once

#include <QtCore/QVariantList>

struct GPSReceiverCapabilities;
struct GPSConfigurationReport;

namespace GPSReceiverSettingsPresentation {
QVariantList settingDescriptors(const GPSReceiverCapabilities& capabilities, bool baseStation = false);
QVariantList configurationReport(const GPSConfigurationReport& report);
}  // namespace GPSReceiverSettingsPresentation
