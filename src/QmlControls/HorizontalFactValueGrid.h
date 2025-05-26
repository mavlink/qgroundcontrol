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

    Q_PROPERTY(QString telemetryBarSettingsGroup    MEMBER telemetryBarSettingsGroup    CONSTANT)
    Q_PROPERTY(QString vehicleCardSettingsGroup     MEMBER vehicleCardSettingsGroup     CONSTANT)

    static const QString telemetryBarSettingsGroup;
    static const QString vehicleCardSettingsGroup;

private:
    Q_DISABLE_COPY(HorizontalFactValueGrid)
};

QML_DECLARE_TYPE(HorizontalFactValueGrid)
