#pragma once

#include <QtQml/qqmlregistration.h>

#include "GPSConnectionState.h"
#include "GPSCorrectionEventModel.h"
#include "GPSRelativePositionModel.h"
#include "GPSSatelliteModel.h"
#include "GPSSourceHealth.h"

// Keep application QML registration separate from the reusable GPS core.
struct GPSConnectionStateQml
{
    Q_GADGET
    QML_FOREIGN(GPSConnectionState)
    QML_NAMED_ELEMENT(GPSConnectionState)
    QML_UNCREATABLE("")
};

struct GPSSourceHealthQml
{
    Q_GADGET
    QML_FOREIGN(GPSSourceHealth)
    QML_NAMED_ELEMENT(GPSSourceHealth)
    QML_UNCREATABLE("")
};

struct GPSCorrectionEventModelQml
{
    Q_GADGET
    QML_FOREIGN(GPSCorrectionEventModel)
    QML_NAMED_ELEMENT(GPSCorrectionEventModel)
    QML_UNCREATABLE("")
};

struct GPSSatelliteModelQml
{
    Q_GADGET
    QML_FOREIGN(GPSSatelliteModel)
    QML_NAMED_ELEMENT(GPSSatelliteModel)
    QML_UNCREATABLE("")
};

struct GPSRelativePositionModelQml
{
    Q_GADGET
    QML_FOREIGN(GPSRelativePositionModel)
    QML_NAMED_ELEMENT(GPSRelativePositionModel)
    QML_UNCREATABLE("")
};
