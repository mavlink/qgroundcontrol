#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "FactValueGrid.h"

class InstrumentValueData;

class HorizontalFactValueGrid : public FactValueGrid
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HorizontalFactValueGridTemplate)

public:
    explicit HorizontalFactValueGrid(QQuickItem *parent = nullptr);

    Q_PROPERTY(QString telemetryBarSettingsGroup    MEMBER telemetryBarSettingsGroup    CONSTANT)
    Q_PROPERTY(QString vehicleCardSettingsGroup     MEMBER vehicleCardSettingsGroup     CONSTANT)

    static const QString telemetryBarSettingsGroup;
    static const QString vehicleCardSettingsGroup;

private:
    Q_DISABLE_COPY(HorizontalFactValueGrid)
};

QML_DECLARE_TYPE(HorizontalFactValueGrid)
