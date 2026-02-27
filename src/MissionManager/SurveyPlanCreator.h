#pragma once

#include "PlanCreator.h"

class SurveyPlanCreator : public PlanCreator
{
    Q_OBJECT

public:
    SurveyPlanCreator(PlanMasterController* planMasterController, QObject* parent = nullptr);

    Q_INVOKABLE void createPlan(const QGeoCoordinate& mapCenterCoord) final;
};
