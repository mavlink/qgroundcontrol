/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VerticalFactValueGrid.h"
#include "InstrumentValueData.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QSettings>

const QString VerticalFactValueGrid::_valuePageUserSettingsGroup  ("ValuePageUserSettings2");
const QString VerticalFactValueGrid::valuePageDefaultSettingsGroup("ValuePageDefaultSettings2");

VerticalFactValueGrid::VerticalFactValueGrid(QQuickItem* parent)
    : FactValueGrid(parent)
{
    _orientation = VerticalOrientation;
}

VerticalFactValueGrid::VerticalFactValueGrid(const QString& defaultSettingsGroup)
    : FactValueGrid(defaultSettingsGroup)
{
    _orientation = VerticalOrientation;
}
