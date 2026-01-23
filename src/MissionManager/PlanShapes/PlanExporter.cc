#include "PlanExporter.h"
#include "CsvPlanExporter.h"
#include "GeoJsonPlanExporter.h"
#include "GeoPackagePlanExporter.h"
#include "GpxPlanExporter.h"
#include "KmlPlanExporter.h"
#include "KmzPlanExporter.h"
#include "ShpPlanExporter.h"
#include "WktPlanExporter.h"
#include "MissionController.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFileInfo>

QGC_LOGGING_CATEGORY(PlanExporterLog, "PlanManager.PlanExporter")

QHash<QString, PlanExporter*> PlanExporter::s_exporters;
bool PlanExporter::s_initialized = false;

void PlanExporter::registerExporter(const QString& extension, PlanExporter* exporter)
{
    const QString lowerExt = extension.toLower();
    if (s_exporters.contains(lowerExt)) {
        qCWarning(PlanExporterLog) << "Replacing existing exporter for extension:" << lowerExt;
    }
    s_exporters.insert(lowerExt, exporter);
    qCDebug(PlanExporterLog) << "Registered exporter for extension:" << lowerExt;
}

PlanExporter* PlanExporter::exporterForExtension(const QString& extension)
{
    if (!s_initialized) {
        initializeExporters();
    }
    return s_exporters.value(extension.toLower(), nullptr);
}

PlanExporter* PlanExporter::exporterForFile(const QString& filename)
{
    const QFileInfo fileInfo(filename);
    return exporterForExtension(fileInfo.suffix());
}

QStringList PlanExporter::registeredExtensions()
{
    if (!s_initialized) {
        initializeExporters();
    }
    return s_exporters.keys();
}

QStringList PlanExporter::fileDialogFilters()
{
    if (!s_initialized) {
        initializeExporters();
    }

    QStringList filters;
    for (auto it = s_exporters.constBegin(); it != s_exporters.constEnd(); ++it) {
        filters.append(it.value()->fileFilter());
    }
    return filters;
}

void PlanExporter::initializeExporters()
{
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    // Initialize built-in exporters (they register themselves)
    CsvPlanExporter::instance();
    GeoJsonPlanExporter::instance();
    GeoPackagePlanExporter::instance();
    GpxPlanExporter::instance();
    KmlPlanExporter::instance();
    KmzPlanExporter::instance();
    ShpPlanExporter::instance();
    WktPlanExporter::instance();

    qCDebug(PlanExporterLog) << "PlanExporter system initialized with" << s_exporters.count() << "exporters";
}

PlanExporter::ExportData PlanExporter::prepareMissionData(MissionController* missionController, QString& errorString)
{
    ExportData data;

    if (!missionController) {
        errorString = tr("No mission controller provided");
        return data;
    }

    data.visualItems = missionController->visualItems();
    if (!data.visualItems || data.visualItems->count() == 0) {
        errorString = tr("No mission items to export");
        return data;
    }

    data.vehicle = missionController->controllerVehicle();
    data.itemParent = std::make_unique<QObject>();

    if (!missionController->convertToMissionItems(data.missionItems, data.itemParent.get())) {
        errorString = tr("Failed to convert mission items");
        return data;
    }

    data.valid = true;
    return data;
}
