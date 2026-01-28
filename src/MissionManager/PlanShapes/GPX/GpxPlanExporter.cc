#include "GpxPlanExporter.h"
#include "PlanExporter.h"
#include "MissionController.h"
#include "GpxPlanDocument.h"

GpxPlanExporter::GpxPlanExporter(QObject* parent)
    : PlanExporter(parent)
{
}

IMPLEMENT_PLAN_EXPORTER_SINGLETON(GpxPlanExporter)

bool GpxPlanExporter::exportToFile(const QString& filename,
                                    MissionController* missionController,
                                    QString& errorString)
{
    ExportData data = prepareMissionData(missionController, errorString);
    if (!data.valid) {
        return false;
    }

    GpxPlanDocument planGpx;
    planGpx.addMission(data.vehicle, data.visualItems, data.missionItems);

    return planGpx.saveToFile(filename, errorString);
}
