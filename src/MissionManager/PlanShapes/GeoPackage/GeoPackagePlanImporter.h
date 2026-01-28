#pragma once

#include "PlanImporter.h"

/// Imports waypoints, tracks, and polygons from OGC GeoPackage files
class GeoPackagePlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("gpkg"); }
    QString formatName() const override { return tr("OGC GeoPackage"); }
    QString fileFilter() const override { return tr("GeoPackage Files (*.gpkg)"); }

private:
    explicit GeoPackagePlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(GeoPackagePlanImporter)
};
