#include "CsvPlanExporter.h"
#include "PlanExporter.h"
#include "MissionController.h"
#include "CsvPlanDocument.h"

CsvPlanExporter::CsvPlanExporter(QObject* parent)
    : PlanExporter(parent)
{
}

IMPLEMENT_PLAN_EXPORTER_SINGLETON(CsvPlanExporter)

bool CsvPlanExporter::exportToFile(const QString& filename,
                                    MissionController* missionController,
                                    QString& errorString)
{
    ExportData data = prepareMissionData(missionController, errorString);
    if (!data.valid) {
        return false;
    }

    CsvPlanDocument planCsv;
    planCsv.addMission(data.vehicle, data.visualItems, data.missionItems);

    return planCsv.saveToFile(filename, errorString);
}
