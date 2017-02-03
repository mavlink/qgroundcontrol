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

    void            setToolbox  (QGCToolbox* toolbox);

private:
    TyphoonHOptions*    _pOptions;
    QGCSettings*        _pTyphoonSettings;
    QGCSettings*        _pGeneral;
    QGCSettings*        _pOfflineMaps;
    QGCSettings*        _pMAVLink;
    QVariantList        settingsList;
};
