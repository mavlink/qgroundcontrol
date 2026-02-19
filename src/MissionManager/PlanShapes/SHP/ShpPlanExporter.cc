#include "ShpPlanExporter.h"
#include "PlanExporter.h"
#include "MissionController.h"
#include "ShpPlanDocument.h"

ShpPlanExporter::ShpPlanExporter(QObject* parent)
    : PlanExporter(parent)
{
}

IMPLEMENT_PLAN_EXPORTER_SINGLETON(ShpPlanExporter)

bool ShpPlanExporter::exportToFile(const QString& filename,
                                    MissionController* missionController,
                                    QString& errorString)
{
    ExportData data = prepareMissionData(missionController, errorString);
    if (!data.valid) {
        return false;
    }

    ShpPlanDocument planShp;
    planShp.addMission(data.vehicle, data.visualItems, data.missionItems);

    if (!planShp.exportToFiles(filename, errorString)) {
        return false;
    }

    _lastCreatedFiles = planShp.createdFiles();
    return true;
}

QStringList ShpPlanExporter::lastCreatedFiles() const
{
    return _lastCreatedFiles;
}
