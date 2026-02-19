#include "GeoJsonPlanImporter.h"
#include "GeoFormatRegistry.h"

GeoJsonPlanImporter::GeoJsonPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
    // Register for .json extension (the singleton macro registers for .geojson via fileExtension())
    registerImporter(QStringLiteral("json"), this);
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(GeoJsonPlanImporter)

PlanImportResult GeoJsonPlanImporter::importFromFile(const QString& filename)
{
    return importWithRegistry(filename, QStringLiteral("GeoJSON"));
}
