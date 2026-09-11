#pragma once

#include <QtQml/qqmlregistration.h>

#include "GPSConnectionState.h"
#include "GPSCorrectionEventModel.h"
#include "GPSPositionService.h"
#include "GPSReceiverAutoConnect.h"
#include "GPSRecordingController.h"
#include "GPSRelativePositionModel.h"
#include "GPSSatelliteModel.h"
#include "GPSSourceHealth.h"
#include "NMEASourceManager.h"
#include "NTRIPConnectionStats.h"
#include "NTRIPSourceTableController.h"

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

struct GPSReceiverAutoConnectQml
{
    Q_GADGET
    QML_FOREIGN(GPSReceiverAutoConnect)
    QML_NAMED_ELEMENT(GPSReceiverAutoConnect)
    QML_UNCREATABLE("")
};

struct NMEASourceManagerQml
{
    Q_GADGET
    QML_FOREIGN(NMEASourceManager)
    QML_NAMED_ELEMENT(NMEASourceManager)
    QML_UNCREATABLE("")
};

struct GPSPositionServiceQml
{
    Q_GADGET
    QML_FOREIGN(GPSPositionService)
    QML_NAMED_ELEMENT(GPSPositionService)
    QML_UNCREATABLE("")
};

struct GPSRecordingControllerQml
{
    Q_GADGET
    QML_FOREIGN(GPSRecordingController)
    QML_NAMED_ELEMENT(GPSRecordingController)
    QML_UNCREATABLE("")
};

struct NTRIPConnectionStatsQml
{
    Q_GADGET
    QML_FOREIGN(NTRIPConnectionStats)
    QML_NAMED_ELEMENT(NTRIPConnectionStats)
    QML_UNCREATABLE("")
};

struct NTRIPSourceTableControllerQml
{
    Q_GADGET
    QML_FOREIGN(NTRIPSourceTableController)
    QML_NAMED_ELEMENT(NTRIPSourceTableController)
    QML_UNCREATABLE("")
};
