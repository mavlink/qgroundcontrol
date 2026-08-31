#pragma once

#include "PlanCreator.h"

class PerimeterScanPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    explicit PerimeterScanPlanCreator(PlanMasterController *planMasterController);

    Q_INVOKABLE void createPlan(const QGeoCoordinate &mapCenterCoord) final;
};
