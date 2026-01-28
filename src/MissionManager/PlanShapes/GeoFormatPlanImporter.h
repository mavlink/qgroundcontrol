#pragma once

#include "PlanImporter.h"

/// Generic plan importer that uses GeoFormatRegistry for format detection and loading.
/// This importer serves as a fallback for formats not handled by specific importers,
/// and provides automatic support for GDAL formats when available.
class GeoFormatPlanImporter : public PlanImporter
{
    Q_OBJECT

public:
    PlanImportResult importFromFile(const QString& filename) override;

    QString fileExtension() const override { return QStringLiteral("*"); }
    QString formatName() const override { return tr("Geographic Format (Auto-detect)"); }
    QString fileFilter() const override;

    /// Check if a file can be imported by this importer (based on GeoFormatRegistry support)
    static bool canImport(const QString& filename);

    /// Import directly using GeoFormatRegistry (convenience method)
    static PlanImportResult importFile(const QString& filename);

private:
    explicit GeoFormatPlanImporter(QObject* parent = nullptr);

    DECLARE_PLAN_IMPORTER_SINGLETON(GeoFormatPlanImporter)
};
