#include "KmlPlanExporter.h"
#include "PlanExporter.h"
#include "MissionController.h"
#include "KMLPlanDomDocument.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

KmlPlanExporter::KmlPlanExporter(QObject* parent)
    : PlanExporter(parent)
{
}

IMPLEMENT_PLAN_EXPORTER_SINGLETON(KmlPlanExporter)

bool KmlPlanExporter::exportToFile(const QString& filename,
                                    MissionController* missionController,
                                    QString& errorString)
{
    ExportData data = prepareMissionData(missionController, errorString);
    if (!data.valid) {
        return false;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorString = tr("Cannot open file for writing: %1").arg(file.errorString());
        return false;
    }

    KMLPlanDomDocument planKML;
    planKML.addMission(data.vehicle, data.visualItems, data.missionItems);

    QTextStream stream(&file);
    stream << planKML.toString();
    file.close();

    return true;
}
