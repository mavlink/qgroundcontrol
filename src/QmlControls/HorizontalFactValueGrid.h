/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactValueGrid.h"

class InstrumentValueData;

class HorizontalFactValueGrid : public FactValueGrid
{
    Q_OBJECT

public:
    HorizontalFactValueGrid(QQuickItem *parent = nullptr);
    HorizontalFactValueGrid(const QString& defaultSettingsGroup);

    Q_PROPERTY(QString telemetryBarDefaultSettingsGroup MEMBER telemetryBarDefaultSettingsGroup CONSTANT)
    Q_PROPERTY(QString telemetryBarUserSettingsGroup    MEMBER telemetryBarUserSettingsGroup    CONSTANT)

    Q_PROPERTY(QString vehicleCardDefaultSettingsGroup  MEMBER vehicleCardDefaultSettingsGroup  CONSTANT)
    Q_PROPERTY(QString vehicleCardUserSettingsGroup     MEMBER vehicleCardUserSettingsGroup     CONSTANT)

    static const QString telemetryBarDefaultSettingsGroup;
    static const QString telemetryBarUserSettingsGroup;

    static const QString vehicleCardDefaultSettingsGroup;
    static const QString vehicleCardUserSettingsGroup;

private:
    Q_DISABLE_COPY(HorizontalFactValueGrid)
};

QML_DECLARE_TYPE(HorizontalFactValueGrid)
