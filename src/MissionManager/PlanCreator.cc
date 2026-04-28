#include "PlanCreator.h"
#include "PlanMasterController.h"

PlanCreator::PlanCreator(PlanMasterController* planMasterController, QString name, QString imageResource, QList<QGCMAVLinkTypes::VehicleClass_t> supportedVehicleClasses)
    : QObject                   (planMasterController)
    , _planMasterController     (planMasterController)
    , _missionController        (planMasterController->missionController())
    , _name                     (name)
    , _imageResource            (imageResource)
    , _supportedVehicleClasses  (supportedVehicleClasses)
{

}

bool PlanCreator::supportsVehicleClass(QGCMAVLinkTypes::VehicleClass_t vehicleClass) const
{
    return _supportedVehicleClasses.contains(vehicleClass);
}
