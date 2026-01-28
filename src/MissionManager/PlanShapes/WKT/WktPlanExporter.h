#pragma once

#include "PlanExporter.h"

/// Exports plans to WKT (Well-Known Text) format
class WktPlanExporter : public PlanExporter
{
    Q_OBJECT

public:
    bool exportToFile(const QString& filename,
                      MissionController* missionController,
                      QString& errorString) override;

    QString fileExtension() const override { return QStringLiteral("wkt"); }
    QString formatName() const override { return tr("Well-Known Text"); }
    QString fileFilter() const override { return tr("WKT Files (*.wkt)"); }

private:
    explicit WktPlanExporter(QObject* parent = nullptr);
    DECLARE_PLAN_EXPORTER_SINGLETON(WktPlanExporter)
};
