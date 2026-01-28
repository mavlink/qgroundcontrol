#pragma once

#include "PlanImporter.h"

/// Imports waypoints, tracks, and polygons from ESRI Shapefile format
class ShpPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("shp"); }
    QString formatName() const override { return tr("ESRI Shapefile"); }
    QString fileFilter() const override { return tr("Shapefiles (*.shp)"); }

private:
    explicit ShpPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(ShpPlanImporter)
};
