#pragma once

#include "PlanExporter.h"

/// Exports plans to OGC GeoPackage (.gpkg) format
class GeoPackagePlanExporter : public PlanExporter
{
    Q_OBJECT

public:
    bool exportToFile(const QString& filename,
                      MissionController* missionController,
                      QString& errorString) override;

    QString fileExtension() const override { return QStringLiteral("gpkg"); }
    QString formatName() const override { return tr("OGC GeoPackage"); }
    QString fileFilter() const override { return tr("GeoPackage Files (*.gpkg)"); }

private:
    explicit GeoPackagePlanExporter(QObject* parent = nullptr);
    DECLARE_PLAN_EXPORTER_SINGLETON(GeoPackagePlanExporter)
};
