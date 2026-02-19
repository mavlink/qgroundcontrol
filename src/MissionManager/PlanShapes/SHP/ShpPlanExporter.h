#pragma once

#include "PlanExporter.h"

#include <QtCore/QStringList>

/// Shapefile format plan exporter
/// Creates multiple files due to SHP format limitation (one geometry type per file):
///   - {basename}_waypoints.shp - Point features with attributes
///   - {basename}_path.shp - LineString for flight path
///   - {basename}_areas.shp - Polygons for survey/structure areas
class ShpPlanExporter : public PlanExporter
{
    Q_OBJECT

public:
    explicit ShpPlanExporter(QObject* parent = nullptr);
    ~ShpPlanExporter() override = default;

    bool exportToFile(const QString& filename,
                      MissionController* missionController,
                      QString& errorString) override;

    QString fileExtension() const override { return QStringLiteral("shp"); }
    QString formatName() const override { return tr("Shapefile"); }
    QString fileFilter() const override { return tr("Shapefiles (*.shp)"); }

    /// Returns list of files created by the last export
    QStringList lastCreatedFiles() const;

private:
    DECLARE_PLAN_EXPORTER_SINGLETON(ShpPlanExporter)
    QStringList _lastCreatedFiles;
};
