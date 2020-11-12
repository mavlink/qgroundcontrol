/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactSystem.h"
#include "QmlObjectListModel.h"
#include "QGCApplication.h"
#include "FactValueGrid.h"

class InstrumentValueData;

class HorizontalFactValueGrid : public FactValueGrid
{
    Q_OBJECT

public:
    HorizontalFactValueGrid(QQuickItem *parent = nullptr);
    HorizontalFactValueGrid(const QString& defaultSettingsGroup);

    Q_PROPERTY(QString telemetryBarDefaultSettingsGroup MEMBER telemetryBarDefaultSettingsGroup  CONSTANT)
    Q_PROPERTY(QString telemetryBarUserSettingsGroup    MEMBER _toolbarUserSettingsGroup    CONSTANT)

    static const QString telemetryBarDefaultSettingsGroup;

private:
    Q_DISABLE_COPY(HorizontalFactValueGrid)

    static const QString _toolbarUserSettingsGroup;
};

QML_DECLARE_TYPE(HorizontalFactValueGrid)
