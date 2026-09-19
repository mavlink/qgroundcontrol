#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionEventModel.h"
#include "GPSPositionService.h"
#include "GPSRTKFactGroup.h"
#include "GPSSourceHealth.h"
#include "NTRIPConnectionStats.h"
#include "NTRIPSourceTableController.h"

struct GPSPositionServiceQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSPositionService)
    QML_NAMED_ELEMENT(GPSPositionService)
    QML_UNCREATABLE("Provided by the position manager")
};

struct GPSSourceHealthQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSSourceHealth)
    QML_ANONYMOUS
};

struct GPSRTKFactGroupQmlType
{
    Q_GADGET
    QML_FOREIGN(GPSRTKFactGroup)
    QML_ANONYMOUS
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
