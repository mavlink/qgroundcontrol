#pragma once

#include "PlanCreator.h"

class BlankPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    BlankPlanCreator(PlanMasterController* planMasterController, QObject* parent = nullptr);

    Q_INVOKABLE void createPlan(const QGeoCoordinate& mapCenterCoord) final;
};
