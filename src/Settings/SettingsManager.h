/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef SettingsManager_H
#define SettingsManager_H

#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "MultiVehicleManager.h"
#include "QGCToolbox.h"
#include "AppSettings.h"
#include "UnitsSettings.h"
#include "AutoConnectSettings.h"
#include "VideoSettings.h"
#include "FlightMapSettings.h"
#include "RTKSettings.h"
#include "GuidedSettings.h"
#include "BrandImageSettings.h"

#include <QVariantList>

/// Provides access to all app settings
class SettingsManager : public QGCTool
{
    Q_OBJECT
    
public:
    SettingsManager(QGCApplication* app, QGCToolbox* toolbox);

    Q_PROPERTY(QObject* appSettings         READ appSettings            CONSTANT)
    Q_PROPERTY(QObject* unitsSettings       READ unitsSettings          CONSTANT)
    Q_PROPERTY(QObject* autoConnectSettings READ autoConnectSettings    CONSTANT)
    Q_PROPERTY(QObject* videoSettings       READ videoSettings          CONSTANT)
    Q_PROPERTY(QObject* flightMapSettings   READ flightMapSettings      CONSTANT)
    Q_PROPERTY(QObject* rtkSettings         READ rtkSettings            CONSTANT)
    Q_PROPERTY(QObject* guidedSettings      READ guidedSettings         CONSTANT)
    Q_PROPERTY(QObject* brandImageSettings  READ brandImageSettings     CONSTANT)

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    AppSettings*            appSettings         (void) { return _appSettings; }
    UnitsSettings*          unitsSettings       (void) { return _unitsSettings; }
    AutoConnectSettings*    autoConnectSettings (void) { return _autoConnectSettings; }
    VideoSettings*          videoSettings       (void) { return _videoSettings; }
    FlightMapSettings*      flightMapSettings   (void) { return _flightMapSettings; }
    RTKSettings*            rtkSettings         (void) { return _rtkSettings; }
    GuidedSettings*         guidedSettings      (void) { return _guidedSettings; }
    BrandImageSettings*     brandImageSettings  (void) { return _brandImageSettings; }

private:
    AppSettings*            _appSettings;
    UnitsSettings*          _unitsSettings;
    AutoConnectSettings*    _autoConnectSettings;
    VideoSettings*          _videoSettings;
    FlightMapSettings*      _flightMapSettings;
    RTKSettings*            _rtkSettings;
    GuidedSettings*         _guidedSettings;
    BrandImageSettings*     _brandImageSettings;
};

#endif
