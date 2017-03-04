/*!
 *   @brief Typhoon H Plugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGCSettings.h"

class TyphoonM4Handler;
class TyphoonHOptions;
class TyphoonHSettings;

class TyphoonHPlugin : public QGCCorePlugin
{
    Q_OBJECT
public:
    TyphoonHPlugin(QGCApplication* app);
    ~TyphoonHPlugin();

    TyphoonM4Handler*   handler                         () { return _pHandler; }

    // Overrides from QGCCorePlugin
    QGCOptions*         options                         () final;
    QVariantList&       settings                        () final;
    bool                overrideSettingsGroupVisibility (QString name) final;
    bool                adjustSettingMetaData           (FactMetaData& metaData) final;

    // Overrides from QGCTool
    void                setToolbox          (QGCToolbox* toolbox);

private:
    TyphoonHOptions*    _pOptions;
    QGCSettings*        _pTyphoonSettings;
    QGCSettings*        _pGeneral;
    QGCSettings*        _pOfflineMaps;
    QGCSettings*        _pMAVLink;
#ifdef QT_DEBUG
    QGCSettings*        _pMockLink;
    QGCSettings*        _pConsole;
#endif
    QVariantList        _settingsList;
    TyphoonM4Handler*   _pHandler;
};
