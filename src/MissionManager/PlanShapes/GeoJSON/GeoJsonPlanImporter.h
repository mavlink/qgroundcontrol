#pragma once

#include "PlanImporter.h"

/// Imports waypoints, tracks, and polygons from GeoJSON files
class GeoJsonPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("geojson"); }
    QString formatName() const override { return tr("GeoJSON"); }
    QString fileFilter() const override { return tr("GeoJSON Files (*.geojson *.json)"); }

private:
    explicit GeoJsonPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(GeoJsonPlanImporter)
};
