#pragma once

#include "PlanExporter.h"

/// KMZ (compressed KML) format plan exporter
class KmzPlanExporter : public PlanExporter
{
    Q_OBJECT

public:
    explicit KmzPlanExporter(QObject* parent = nullptr);
    ~KmzPlanExporter() override = default;

    bool exportToFile(const QString& filename,
                      MissionController* missionController,
                      QString& errorString) override;

    QString fileExtension() const override { return QStringLiteral("kmz"); }
    QString formatName() const override { return tr("KMZ (Compressed KML)"); }
    QString fileFilter() const override { return tr("KMZ Files (*.kmz)"); }

private:
    DECLARE_PLAN_EXPORTER_SINGLETON(KmzPlanExporter)
};
