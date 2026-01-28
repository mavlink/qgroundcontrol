#include "KmlPlanImporter.h"
#include "GeoFormatRegistry.h"

KmlPlanImporter::KmlPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(KmlPlanImporter)

PlanImportResult KmlPlanImporter::importFromFile(const QString& filename)
{
    return importWithRegistry(filename, QStringLiteral("KML"));
}
