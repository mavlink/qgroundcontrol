/*!
 *   @brief Typhoon H Plugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGCSettings.h"

class TyphoonHCore;
class TyphoonHOptions;
class TyphoonHSettings;

class TyphoonHPlugin : public QGCCorePlugin
{
    Q_OBJECT
public:
    TyphoonHPlugin(QGCApplication* app);
    ~TyphoonHPlugin();

    QGCOptions*     options     ();
    QVariantList&   settings    ();
    TyphoonHCore*   core        () { return _pCore; }

    void            setToolbox  (QGCToolbox* toolbox);

private slots:
    void            _vehicleReady(bool parameterReadyVehicleAvailable);

private:
    TyphoonHCore*       _pCore;
    TyphoonHOptions*    _pOptions;
    QGCSettings*        _pTyphoonSettings;
    QGCSettings*        _pGeneral;
    QGCSettings*        _pOfflineMaps;
    QGCSettings*        _pMAVLink;
    QVariantList        settingsList;
};
