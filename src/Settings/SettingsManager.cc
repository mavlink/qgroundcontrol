/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SettingsManager.h"

#include <QQmlEngine>
#include <QtQml>

SettingsManager::SettingsManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _appSettings          (NULL)
    , _unitsSettings        (NULL)
    , _autoConnectSettings  (NULL)
    , _videoSettings        (NULL)
    , _flightMapSettings    (NULL)
    , _rtkSettings          (NULL)
    , _guidedSettings       (NULL)
    , _brandImageSettings   (NULL)
{

}

void SettingsManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    qmlRegisterUncreatableType<SettingsManager>("QGroundControl.SettingsManager", 1, 0, "SettingsManager", "Reference only");

    _unitsSettings =        new UnitsSettings(this);        // Must be first since AppSettings references it
    _appSettings =          new AppSettings(this);
    _autoConnectSettings =  new AutoConnectSettings(this);
    _videoSettings =        new VideoSettings(this);
    _flightMapSettings =    new FlightMapSettings(this);
    _rtkSettings =          new RTKSettings(this);
    _guidedSettings =       new GuidedSettings(this);
    _brandImageSettings =   new BrandImageSettings(this);
}
