#pragma once

#include "PlanExporter.h"

/// GPX format plan exporter
class GpxPlanExporter : public PlanExporter
{
    Q_OBJECT

public:
    explicit GpxPlanExporter(QObject* parent = nullptr);
    ~GpxPlanExporter() override = default;

    bool exportToFile(const QString& filename,
                      MissionController* missionController,
                      QString& errorString) override;

    QString fileExtension() const override { return QStringLiteral("gpx"); }
    QString formatName() const override { return tr("GPX"); }
    QString fileFilter() const override { return tr("GPX Files (*.gpx)"); }

private:
    DECLARE_PLAN_EXPORTER_SINGLETON(GpxPlanExporter)
};
