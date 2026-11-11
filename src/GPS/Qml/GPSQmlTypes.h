#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSReceiverReports.h"

// QGCGPSCore builds without QML, so its types are registered here.

// Registers GPSFixQuality's values as a namespace, so QML names fix types instead of numbers.
namespace GPSFixQualityForeign {
Q_NAMESPACE
QML_NAMED_ELEMENT(GPSFixQuality)
QML_FOREIGN_NAMESPACE(GPSFixQualities)
}  // namespace GPSFixQualityForeign
