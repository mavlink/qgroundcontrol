#include "PlanImporter.h"
#include "CsvPlanImporter.h"
#include "GeoFormatPlanImporter.h"
#include "GeoFormatRegistry.h"
#include "GeoJsonPlanImporter.h"
#include "GeoPackagePlanImporter.h"
#include "GpxPlanImporter.h"
#include "KmlPlanImporter.h"
#include "KmzPlanImporter.h"
#include "OpenAirPlanImporter.h"
#include "ShpPlanImporter.h"
#include "WktPlanImporter.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QFileInfo>

QGC_LOGGING_CATEGORY(PlanImporterLog, "PlanManager.PlanImporter")

QHash<QString, PlanImporter*> PlanImporter::s_importers;
bool PlanImporter::s_initialized = false;

void PlanImporter::registerImporter(const QString& extension, PlanImporter* importer)
{
    const QString lowerExt = extension.toLower();
    if (s_importers.contains(lowerExt)) {
        qCWarning(PlanImporterLog) << "Replacing existing importer for extension:" << lowerExt;
    }
    s_importers.insert(lowerExt, importer);
    qCDebug(PlanImporterLog) << "Registered importer for extension:" << lowerExt;
}

PlanImportResult PlanImporter::importWithRegistry(const QString& filename, const QString& formatName)
{
    PlanImportResult result;

    GeoFormatRegistry::LoadResult loadResult = GeoFormatRegistry::loadFile(filename, _vertexFilterMeters);

    // Use formatUsed from LoadResult if available, otherwise use provided formatName
    const QString displayName = loadResult.formatUsed.isEmpty() ? formatName : loadResult.formatUsed;

    if (!loadResult.success) {
        result.errorString = loadResult.errorString.isEmpty()
            ? tr("No features found in %1 file").arg(displayName)
            : loadResult.errorString;
        return result;
    }

    // Convert LoadResult to PlanImportResult
    result.waypoints = loadResult.points;

    // Polylines are flattened into track points
    for (const auto& polyline : loadResult.polylines) {
        result.trackPoints.append(polyline);
    }

    // Polygons are preserved as-is
    result.polygons = loadResult.polygons;

    result.success = (result.itemCount() > 0);
    if (result.success) {
        result.formatDescription = tr("%1 (%2 features)").arg(displayName).arg(result.itemCount());
    } else {
        result.errorString = tr("No features found in %1 file").arg(displayName);
    }

    return result;
}

PlanImporter* PlanImporter::importerForExtension(const QString& extension)
{
    if (!s_initialized) {
        initializeImporters();
    }

    const QString lowerExt = extension.toLower();
    PlanImporter* importer = s_importers.value(lowerExt, nullptr);

    // If no specific importer registered, fallback to GeoFormatPlanImporter
    // if GeoFormatRegistry supports the format
    if (!importer && GeoFormatRegistry::isSupported(lowerExt)) {
        qCDebug(PlanImporterLog) << "Using GeoFormatPlanImporter fallback for extension:" << lowerExt;
        importer = GeoFormatPlanImporter::instance();
    }

    return importer;
}

PlanImporter* PlanImporter::importerForFile(const QString& filename)
{
    const QFileInfo fileInfo(filename);
    return importerForExtension(fileInfo.suffix());
}

QStringList PlanImporter::registeredExtensions()
{
    if (!s_initialized) {
        initializeImporters();
    }
    return s_importers.keys();
}

QStringList PlanImporter::fileDialogFilters()
{
    if (!s_initialized) {
        initializeImporters();
    }

    QStringList filters;
    for (auto it = s_importers.constBegin(); it != s_importers.constEnd(); ++it) {
        filters.append(it.value()->fileFilter());
    }
    return filters;
}

void PlanImporter::initializeImporters()
{
    if (s_initialized) {
        return;
    }
    s_initialized = true;

    // Initialize built-in importers (they register themselves)
    CsvPlanImporter::instance();
    GeoJsonPlanImporter::instance();
    GeoPackagePlanImporter::instance();
    GpxPlanImporter::instance();
    KmlPlanImporter::instance();
    KmzPlanImporter::instance();
    OpenAirPlanImporter::instance();
    ShpPlanImporter::instance();
    WktPlanImporter::instance();

    qCDebug(PlanImporterLog) << "PlanImporter system initialized with" << s_importers.count() << "importers";
}
