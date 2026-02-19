#include "GeoJsonPlanExporter.h"
#include "PlanExporter.h"
#include "MissionController.h"
#include "GeoJsonPlanDocument.h"

#include <QtCore/QFile>

GeoJsonPlanExporter::GeoJsonPlanExporter(QObject* parent)
    : PlanExporter(parent)
{
}

IMPLEMENT_PLAN_EXPORTER_SINGLETON(GeoJsonPlanExporter)

bool GeoJsonPlanExporter::exportToFile(const QString& filename,
                                        MissionController* missionController,
                                        QString& errorString)
{
    ExportData data = prepareMissionData(missionController, errorString);
    if (!data.valid) {
        return false;
    }

    GeoJsonPlanDocument planGeoJson;
    planGeoJson.addMission(data.vehicle, data.visualItems, data.missionItems);

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = tr("Cannot open file for writing: %1").arg(file.errorString());
        return false;
    }

    file.write(planGeoJson.toJson());
    file.close();

    return true;
}
