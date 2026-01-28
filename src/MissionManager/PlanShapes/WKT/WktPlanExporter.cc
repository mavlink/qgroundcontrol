#include "WktPlanExporter.h"
#include "PlanExporter.h"
#include "MissionController.h"
#include "WktPlanDocument.h"

WktPlanExporter::WktPlanExporter(QObject* parent)
    : PlanExporter(parent)
{
}

IMPLEMENT_PLAN_EXPORTER_SINGLETON(WktPlanExporter)

bool WktPlanExporter::exportToFile(const QString& filename,
                                    MissionController* missionController,
                                    QString& errorString)
{
    ExportData data = prepareMissionData(missionController, errorString);
    if (!data.valid) {
        return false;
    }

    WktPlanDocument planWkt;
    planWkt.addMission(data.vehicle, data.visualItems, data.missionItems);

    return planWkt.saveToFile(filename, errorString);
}
