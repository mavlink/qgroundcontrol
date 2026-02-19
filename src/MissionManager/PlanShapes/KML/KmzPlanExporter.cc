#include "KmzPlanExporter.h"
#include "KMLPlanDomDocument.h"
#include "MissionController.h"
#include "PlanExporter.h"

#include <QtCore/QFile>
#include <private/qzipwriter_p.h>

KmzPlanExporter::KmzPlanExporter(QObject* parent)
    : PlanExporter(parent)
{
}

IMPLEMENT_PLAN_EXPORTER_SINGLETON(KmzPlanExporter)

bool KmzPlanExporter::exportToFile(const QString& filename,
                                    MissionController* missionController,
                                    QString& errorString)
{
    ExportData data = prepareMissionData(missionController, errorString);
    if (!data.valid) {
        return false;
    }

    KMLPlanDomDocument planKML;
    planKML.addMission(data.vehicle, data.visualItems, data.missionItems);
    const QString kmlContent = planKML.toString();

    QZipWriter zipWriter(filename);
    if (zipWriter.status() != QZipWriter::NoError) {
        errorString = tr("Failed to create KMZ file");
        return false;
    }

    zipWriter.addFile(QStringLiteral("doc.kml"), kmlContent.toUtf8());

    if (zipWriter.status() != QZipWriter::NoError) {
        errorString = tr("Failed to write KML data to KMZ");
        return false;
    }

    zipWriter.close();
    return true;
}
