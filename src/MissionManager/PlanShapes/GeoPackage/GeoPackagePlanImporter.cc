#include "GeoPackagePlanImporter.h"
#include "GeoFormatRegistry.h"

GeoPackagePlanImporter::GeoPackagePlanImporter(QObject* parent)
    : PlanImporter(parent)
{
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(GeoPackagePlanImporter)

PlanImportResult GeoPackagePlanImporter::importFromFile(const QString& filename)
{
    return importWithRegistry(filename, QStringLiteral("GeoPackage"));
}
