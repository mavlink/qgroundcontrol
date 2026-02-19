#pragma once

#include "PlanImporter.h"

/// Imports geometries from WKT (Well-Known Text) files
class WktPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("wkt"); }
    QString formatName() const override { return tr("Well-Known Text"); }
    QString fileFilter() const override { return tr("WKT Files (*.wkt)"); }

private:
    explicit WktPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(WktPlanImporter)
};
