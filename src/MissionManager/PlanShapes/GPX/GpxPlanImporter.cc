#include "GpxPlanImporter.h"
#include "GeoFormatRegistry.h"

GpxPlanImporter::GpxPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(GpxPlanImporter)

PlanImportResult GpxPlanImporter::importFromFile(const QString& filename)
{
    return importWithRegistry(filename, QStringLiteral("GPX"));
}
