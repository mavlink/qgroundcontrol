#pragma once

#include "PlanImporter.h"

/// Imports airspace polygons from OpenAir format files
class OpenAirPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("txt"); }
    QString formatName() const override { return tr("OpenAir Airspace"); }
    QString fileFilter() const override { return tr("OpenAir Files (*.txt *.air)"); }

private:
    explicit OpenAirPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(OpenAirPlanImporter)
};
