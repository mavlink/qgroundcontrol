#include "PlanCreator.h"
#include "PlanMasterController.h"

PlanCreator::PlanCreator(PlanMasterController* planMasterController, QString name, QString imageResource, QObject* parent)
    : QObject               (parent)
    , _planMasterController (planMasterController)
    , _missionController    (planMasterController->missionController())
    , _name                 (name)
    , _imageResource        (imageResource)
{

}
