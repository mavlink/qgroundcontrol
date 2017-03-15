/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGCSettings.h"

class TyphoonHM4Interface;
class TyphoonHOptions;
class TyphoonHSettings;

class TyphoonHPlugin : public QGCCorePlugin
{
    Q_OBJECT
public:
    TyphoonHPlugin(QGCApplication* app);
    ~TyphoonHPlugin();

    TyphoonHM4Interface*   handler                      () { return _pHandler; }

    // Overrides from QGCCorePlugin
    QGCOptions*         options                         () final;
    QVariantList&       settingsPages                   () final;
    bool                overrideSettingsGroupVisibility (QString name) final;
    bool                adjustSettingMetaData           (FactMetaData& metaData) final;
    QString             brandImageIndoor                () const final;
    QString             brandImageOutdoor               () const final;

    // Overrides from QGCTool
    void                setToolbox                      (QGCToolbox* toolbox);

private slots:
    void                _showAdvancedPages              ();

private:
    TyphoonHOptions*    _pOptions;
    QGCSettings*        _pTyphoonSettings;
    QGCSettings*        _pGeneral;
    QGCSettings*        _pOfflineMaps;
    QGCSettings*        _pMAVLink;
#ifdef QT_DEBUG
    QGCSettings*        _pMockLink;
#endif
    QGCSettings*        _pConsole;
    QVariantList        _settingsList;
    TyphoonHM4Interface*_pHandler;
};
