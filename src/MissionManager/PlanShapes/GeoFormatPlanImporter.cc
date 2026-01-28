#include "GeoFormatPlanImporter.h"
#include "GeoFormatRegistry.h"

#include <QtCore/QFileInfo>

GeoFormatPlanImporter::GeoFormatPlanImporter(QObject* parent)
    : PlanImporter(parent)
{
}

IMPLEMENT_PLAN_IMPORTER_SINGLETON(GeoFormatPlanImporter)

bool GeoFormatPlanImporter::canImport(const QString& filename)
{
    QFileInfo fileInfo(filename);
    return GeoFormatRegistry::isSupported(fileInfo.suffix());
}

PlanImportResult GeoFormatPlanImporter::importFile(const QString& filename)
{
    return instance()->importFromFile(filename);
}

PlanImportResult GeoFormatPlanImporter::importFromFile(const QString& filename)
{
    return importWithRegistry(filename, tr("Geographic"));
}

QString GeoFormatPlanImporter::fileFilter() const
{
    return GeoFormatRegistry::readFileFilter();
}
