#pragma once

#include <QtQml/qqmlregistration.h>

#include "GPSConnectionState.h"
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
