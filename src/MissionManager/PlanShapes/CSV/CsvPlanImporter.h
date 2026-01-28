#pragma once

#include "PlanImporter.h"

/// Imports waypoints from CSV files
/// Delegates to CSVHelper for parsing
class CsvPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("csv"); }
    QString formatName() const override { return tr("Comma-Separated Values"); }
    QString fileFilter() const override { return tr("CSV Files (*.csv)"); }

private:
    explicit CsvPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(CsvPlanImporter)
};
