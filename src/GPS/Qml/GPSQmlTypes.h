#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSReceiverDescriptor.h"
#include "GPSReceiverReports.h"
#include "RTCMMessageCountModel.h"

// QGCGPSCore builds without QML, so its types are registered here.

struct GPSReceiverPresentationQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSReceiverPresentation)
    QML_VALUE_TYPE(gpsReceiverPresentation)
    QML_STRUCTURED_VALUE
};

struct RTCMMessageCountModelQmlType
{
    Q_GADGET
    QML_FOREIGN(RTCMMessageCountModel)
    QML_NAMED_ELEMENT(RTCMMessageCountModel)
    QML_UNCREATABLE("Provided by the correction manager")
};

// Registers GPSFixQuality's values as a namespace, so QML names fix types instead of numbers.
namespace GPSFixQualityForeign {
Q_NAMESPACE
QML_NAMED_ELEMENT(GPSFixQuality)
QML_FOREIGN_NAMESPACE(GPSFixQualities)
}  // namespace GPSFixQualityForeign
