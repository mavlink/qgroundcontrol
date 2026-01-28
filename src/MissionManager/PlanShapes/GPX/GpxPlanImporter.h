#pragma once

#include "PlanImporter.h"

/// Imports waypoints and tracks from GPX files
class GpxPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("gpx"); }
    QString formatName() const override { return tr("GPS Exchange Format"); }
    QString fileFilter() const override { return tr("GPX Files (*.gpx)"); }

private:
    explicit GpxPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(GpxPlanImporter)
};
