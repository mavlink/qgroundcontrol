/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FactSystem.h"
#include "FactGroup.h"
#include "FactPanelController.h"
#include "FactValueSliderListModel.h"
#include "ParameterManager.h"

#include <QtQml/QtQml>

FactSystem::FactSystem(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{

}

void FactSystem::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    qmlRegisterType<Fact>               ("QGroundControl.FactSystem", 1, 0, "Fact");
    qmlRegisterType<FactMetaData>       ("QGroundControl.FactSystem", 1, 0, "FactMetaData");
    qmlRegisterType<FactPanelController>("QGroundControl.FactSystem", 1, 0, "FactPanelController");

    qmlRegisterUncreatableType<FactGroup>               ("QGroundControl.FactSystem",   1, 0, "FactGroup",                  "Reference only");
    qmlRegisterUncreatableType<FactValueSliderListModel>("QGroundControl.FactControls", 1, 0, "FactValueSliderListModel",   "Reference only");
    qmlRegisterUncreatableType<ParameterManager>        ("QGroundControl.Vehicle",      1, 0, "ParameterManager",           "Reference only");
}
