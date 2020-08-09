/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "HorizontalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QSettings>

const QString HorizontalFactValueGrid::_toolbarUserSettingsGroup        ("TelemetryBarUserSettingsWIP0");
const QString HorizontalFactValueGrid::telemetryBarDefaultSettingsGroup ("TelemetryBarDefaultSettingsWIP0");

HorizontalFactValueGrid::HorizontalFactValueGrid(QQuickItem* parent)
    : FactValueGrid(parent)
{
    _orientation = HorizontalOrientation;
}

HorizontalFactValueGrid::HorizontalFactValueGrid(const QString& defaultSettingsGroup)
    : FactValueGrid(defaultSettingsGroup)
{
    _orientation = HorizontalOrientation;
}
