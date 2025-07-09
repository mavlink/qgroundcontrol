/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtQml/QQmlEngine>

#include "LogDownloadController.h"
#include "MAVLinkChartController.h"
#include "MAVLinkConsoleController.h"
#include "GeoTagController.h"
#include "MAVLinkInspectorController.h"

namespace AnalyzeView
{
    void registerQmlTypes()
    {
        (void) qmlRegisterUncreatableType<MAVLinkChartController>("QGroundControl", 1, 0, "MAVLinkChart", "Reference only");
        (void) qmlRegisterType<MAVLinkInspectorController>("QGroundControl.Controllers", 1, 0, "MAVLinkInspectorController");
        (void) qmlRegisterType<GeoTagController>("QGroundControl.Controllers", 1, 0, "GeoTagController");
        (void) qmlRegisterType<LogDownloadController>("QGroundControl.Controllers", 1, 0, "LogDownloadController");
        (void) qmlRegisterType<MAVLinkConsoleController>("QGroundControl.Controllers", 1, 0, "MAVLinkConsoleController");
    }
}
