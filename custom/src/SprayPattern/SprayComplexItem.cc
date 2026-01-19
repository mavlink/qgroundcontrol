/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SprayComplexItem.h"
#include "JsonHelper.h"
#include "QGCGeo.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "PlanMasterController.h"
#include "MissionItem.h"
#include "QGCApplication.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

#include <QtGui/QPolygonF>
#include <QtCore/QJsonArray>
#include <QtCore/QLineF>

QGC_LOGGING_CATEGORY(SprayComplexItemLog, "SprayComplexItemLog")

const QString SprayComplexItem::name(SprayComplexItem::tr("Spray"));

SprayComplexItem::SprayComplexItem(PlanMasterController* masterController, bool flyView)
    : ComplexMissionItem(masterController, flyView)
    , _metaDataMap(FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/Spray.SettingsGroup.json"), this))
    , _speedFact(settingsGroup, _metaDataMap[speedName])
    , _altitudeFact(settingsGroup, _metaDataMap[altitudeName])
    , _sprayWidthFact(settingsGroup, _metaDataMap[sprayWidthName])
    , _complexDistance(0.0)
{
    _editorQml = "qrc:/qml/QGroundControl/Controls/SprayItemEditor.qml";

}

