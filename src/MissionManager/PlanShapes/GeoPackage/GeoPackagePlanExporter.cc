#include "GeoPackagePlanExporter.h"
#include "PlanExporter.h"
#include "MissionController.h"
#include "GeoPackagePlanDocument.h"

GeoPackagePlanExporter::GeoPackagePlanExporter(QObject* parent)
    : PlanExporter(parent)
{
}

IMPLEMENT_PLAN_EXPORTER_SINGLETON(GeoPackagePlanExporter)

bool GeoPackagePlanExporter::exportToFile(const QString& filename,
                                           MissionController* missionController,
                                           QString& errorString)
{
    ExportData data = prepareMissionData(missionController, errorString);
    if (!data.valid) {
        return false;
    }

    GeoPackagePlanDocument planGpkg;
    planGpkg.addMission(data.vehicle, data.visualItems, data.missionItems);

    return planGpkg.exportToFile(filename, errorString);
}
