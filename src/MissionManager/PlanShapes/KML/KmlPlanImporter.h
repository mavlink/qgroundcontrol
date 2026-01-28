#pragma once

#include "PlanImporter.h"

/// Imports waypoints, paths, and polygons from KML files
class KmlPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("kml"); }
    QString formatName() const override { return tr("Keyhole Markup Language"); }
    QString fileFilter() const override { return tr("KML Files (*.kml)"); }

private:
    explicit KmlPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(KmlPlanImporter)
};
