#pragma once

#include "PlanCreator.h"

class StructureScanPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    StructureScanPlanCreator(PlanMasterController* planMasterController, QObject* parent = nullptr);

    Q_INVOKABLE void createPlan(const QGeoCoordinate& mapCenterCoord) final;
};
