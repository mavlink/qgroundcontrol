#include "ShpPlanImporter.h"
#include "GeoFormatRegistry.h"

ShpPlanImporter::ShpPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(ShpPlanImporter)

PlanImportResult ShpPlanImporter::importFromFile(const QString& filename)
{
    return importWithRegistry(filename, QStringLiteral("Shapefile"));
}
