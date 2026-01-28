#pragma once

#include "PlanExporter.h"

/// GeoJSON format plan exporter
class GeoJsonPlanExporter : public PlanExporter
{
    Q_OBJECT

public:
    explicit GeoJsonPlanExporter(QObject* parent = nullptr);
    ~GeoJsonPlanExporter() override = default;

    bool exportToFile(const QString& filename,
                      MissionController* missionController,
                      QString& errorString) override;

    QString fileExtension() const override { return QStringLiteral("geojson"); }
    QString formatName() const override { return tr("GeoJSON"); }
    QString fileFilter() const override { return tr("GeoJSON Files (*.geojson)"); }

private:
    DECLARE_PLAN_EXPORTER_SINGLETON(GeoJsonPlanExporter)
};
