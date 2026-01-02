#include "PlanElementController.h"
#include "PlanMasterController.h"

PlanElementController::PlanElementController(PlanMasterController* masterController, QObject* parent)
    : QObject           (parent)
    , _masterController (masterController)
    , _flyView          (false)
{

}

PlanElementController::~PlanElementController()
{

}

void PlanElementController::start(bool flyView)
{
    _flyView = flyView;
}
