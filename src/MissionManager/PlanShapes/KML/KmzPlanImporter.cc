#include "KmzPlanImporter.h"
#include "GeoFormatRegistry.h"

KmzPlanImporter::KmzPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(KmzPlanImporter)

PlanImportResult KmzPlanImporter::importFromFile(const QString& filename)
{
    // GeoFormatRegistry handles KMZ files transparently (decompresses internally)
    return importWithRegistry(filename, QStringLiteral("KMZ"));
}
