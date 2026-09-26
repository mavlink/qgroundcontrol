#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionDiagnostics.h"
#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionManager.h"
#include "GPSFixQuality.h"
#include "GPSPositionService.h"
#include "GPSReceiver.h"
#include "GPSReceiverDescriptor.h"
#include "GPSSourceHealth.h"
#include "NTRIPConnectionStats.h"
#include "NTRIPManager.h"
#include "NTRIPSourceTableController.h"
#include "PositionManager.h"
#include "RTCMMAVLink.h"

struct GPSPositionServiceQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSPositionService)
    QML_NAMED_ELEMENT(GPSPositionService)
    QML_UNCREATABLE("Provided by the position manager")
};

struct PositionManagerQmlType
{
    Q_GADGET
    QML_FOREIGN(PositionManager)
    QML_NAMED_ELEMENT(PositionManager)
    QML_UNCREATABLE("Created by QGroundControl")
};

struct GPSReceiverPresentationQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSReceiverPresentation)
    QML_VALUE_TYPE(gpsReceiverPresentation)
    QML_STRUCTURED_VALUE
};

struct GPSReceiverQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSReceiver)
    QML_NAMED_ELEMENT(GPSReceiver)
    QML_UNCREATABLE("Managed by GPSManager")
};

struct GPSSourceHealthQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSSourceHealth)
    QML_ANONYMOUS
};

struct RTCMMessageCountQmlType
{
    Q_GADGET
    QML_FOREIGN(RTCMMessageCount)
    QML_VALUE_TYPE(rtcmMessageCount)
    QML_STRUCTURED_VALUE
};

struct GPSCorrectionStreamDiagnosticQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSCorrectionStreamDiagnostic)
    QML_VALUE_TYPE(gpsCorrectionStream)
    QML_STRUCTURED_VALUE
};

struct GPSCorrectionEventModelQml
{
    Q_GADGET
    QML_FOREIGN(GPSCorrectionEventModel)
    QML_NAMED_ELEMENT(GPSCorrectionEventModel)
    QML_UNCREATABLE("")
};

struct NTRIPConnectionStatsQmlType
{
    Q_GADGET
    QML_FOREIGN(NTRIPConnectionStats)
    QML_NAMED_ELEMENT(NTRIPConnectionStats)
    QML_UNCREATABLE("")
};

struct NTRIPSourceTableControllerQmlType
{
    Q_GADGET
    QML_FOREIGN(NTRIPSourceTableController)
    QML_NAMED_ELEMENT(NTRIPSourceTableController)
    QML_UNCREATABLE("")
};

struct GPSCorrectionManagerQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSCorrectionManager)
    QML_NAMED_ELEMENT(GPSCorrectionManager)
    QML_UNCREATABLE("Provided by the GPS manager")
};

struct RTCMMAVLinkQmlType
{
    Q_GADGET
    QML_FOREIGN(RTCMMAVLink)
    QML_ANONYMOUS
};

struct NTRIPManagerQmlType
{
    Q_GADGET
    QML_FOREIGN(NTRIPManager)
    QML_NAMED_ELEMENT(NTRIPManager)
    QML_UNCREATABLE("Provided by the GPS manager")
};

// Registers GPSFixQuality's values as a namespace, so QML names fix types instead of numbers.
namespace GPSFixQualityForeign {
Q_NAMESPACE
QML_NAMED_ELEMENT(GPSFixQuality)
QML_FOREIGN_NAMESPACE(GPSFixQualities)
}  // namespace GPSFixQualityForeign
