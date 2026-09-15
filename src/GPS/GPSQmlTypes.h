#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionEventModel.h"
#include "GPSPositionService.h"
#include "GPSSourceHealth.h"

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

struct GPSCorrectionEventModelQml
{
    Q_GADGET
    QML_FOREIGN(GPSCorrectionEventModel)
    QML_NAMED_ELEMENT(GPSCorrectionEventModel)
    QML_UNCREATABLE("")
};
