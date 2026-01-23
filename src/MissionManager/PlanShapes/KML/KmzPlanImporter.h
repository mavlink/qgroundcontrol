#pragma once

#include "PlanImporter.h"

/// Imports waypoints, tracks, and polygons from KMZ (compressed KML) files
class KmzPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("kmz"); }
    QString formatName() const override { return tr("Compressed KML"); }
    QString fileFilter() const override { return tr("KMZ Files (*.kmz)"); }

private:
    explicit KmzPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(KmzPlanImporter)
};
