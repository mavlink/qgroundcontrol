#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionDiagnostics.h"
#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionManager.h"
#include "GPSPositionService.h"
#include "GPSReceiverDescriptor.h"
#include "GPSRtk.h"
#include "GPSSourceHealth.h"
#include "NTRIPConnectionStats.h"
#include "NTRIPManager.h"
#include "NTRIPSourceTableController.h"
#include "RTCMMavlink.h"

struct GPSPositionServiceQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSPositionService)
    QML_NAMED_ELEMENT(GPSPositionService)
    QML_UNCREATABLE("Provided by the position manager")
};

struct GPSReceiverPresentationQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSReceiverPresentation)
    QML_VALUE_TYPE(gpsReceiverPresentation)
    QML_STRUCTURED_VALUE
};

struct GPSRtkQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSRtk)
    QML_NAMED_ELEMENT(GPSRtk)
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

struct RTCMMavlinkQmlType
{
    Q_GADGET
    QML_FOREIGN(RTCMMavlink)
    QML_ANONYMOUS
};

struct NTRIPManagerQmlType
{
    Q_GADGET
    QML_FOREIGN(NTRIPManager)
    QML_NAMED_ELEMENT(NTRIPManager)
    QML_UNCREATABLE("Provided by the GPS manager")
};
