/****************************************************************************
 *
 * (c) 2009-2025 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactValueGrid.h"

class InstrumentValueData;

class MultiVehicleFactValueGrid : public FactValueGrid
{
    Q_OBJECT

public:
    MultiVehicleFactValueGrid(QQuickItem *parent = nullptr);
    MultiVehicleFactValueGrid(const QString& defaultSettingsGroup);

    Q_PROPERTY(QString vehicleCardDefaultSettingsGroup MEMBER vehicleCardDefaultSettingsGroup CONSTANT)
    Q_PROPERTY(QString vehicleCardUserSettingsGroup    MEMBER vehicleCardUserSettingsGroup    CONSTANT)

    static const QString vehicleCardDefaultSettingsGroup;
    static const QString vehicleCardUserSettingsGroup;

private:
    Q_DISABLE_COPY(MultiVehicleFactValueGrid)
};

QML_DECLARE_TYPE(MultiVehicleFactValueGrid)
