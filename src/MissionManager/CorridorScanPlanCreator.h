#pragma once

#include "PlanCreator.h"

class CorridorScanPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    CorridorScanPlanCreator(PlanMasterController* planMasterController);

    Q_INVOKABLE void createPlan(const QGeoCoordinate& mapCenterCoord) final;
};
