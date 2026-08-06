#pragma once

#include "QmlUITestBase.h"

/// UI smoke test for the FlyView proximity radar map overlay. Connects a MockLink
/// vehicle with the simulated proximity sensor ring enabled and verifies the radar
/// map item becomes visible, plus the negative case without proximity telemetry.
class FlyViewProximityRadarUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testRadarVisibleWithProximity();
    void _testRadarHiddenWithoutProximity();
};
