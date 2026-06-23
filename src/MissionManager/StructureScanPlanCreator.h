#pragma once

#include "PlanCreator.h"

class StructureScanPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    StructureScanPlanCreator(PlanMasterController* planMasterController);

    Q_INVOKABLE void createPlan(const QGeoCoordinate& mapCenterCoord) final;
};
