#pragma once

#include "PlanExporter.h"

/// KML format plan exporter
class KmlPlanExporter : public PlanExporter
{
    Q_OBJECT

public:
    explicit KmlPlanExporter(QObject* parent = nullptr);
    ~KmlPlanExporter() override = default;

    bool exportToFile(const QString& filename,
                      MissionController* missionController,
                      QString& errorString) override;

    QString fileExtension() const override { return QStringLiteral("kml"); }
    QString formatName() const override { return tr("KML"); }
    QString fileFilter() const override { return tr("KML Files (*.kml)"); }

private:
    DECLARE_PLAN_EXPORTER_SINGLETON(KmlPlanExporter)
};
