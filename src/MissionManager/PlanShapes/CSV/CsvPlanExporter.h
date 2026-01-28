#pragma once

#include "PlanExporter.h"

/// Exports plans to CSV (Comma-Separated Values) format
class CsvPlanExporter : public PlanExporter
{
    Q_OBJECT

public:
    bool exportToFile(const QString& filename,
                      MissionController* missionController,
                      QString& errorString) override;

    QString fileExtension() const override { return QStringLiteral("csv"); }
    QString formatName() const override { return tr("Comma-Separated Values"); }
    QString fileFilter() const override { return tr("CSV Files (*.csv)"); }

private:
    explicit CsvPlanExporter(QObject* parent = nullptr);
    DECLARE_PLAN_EXPORTER_SINGLETON(CsvPlanExporter)
};
