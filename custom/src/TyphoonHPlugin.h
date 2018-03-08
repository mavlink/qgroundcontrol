/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGCSettings.h"

#include <QTranslator>

#if defined(__androidx86__)
class TyphoonHM4Interface;
#endif

class TyphoonHOptions;
class TyphoonHSettings;
class TyphoonHQuickInterface;

class TyphoonHPlugin : public QGCCorePlugin
{
    Q_OBJECT
public:
    TyphoonHPlugin(QGCApplication* app, QGCToolbox *toolbox);
    ~TyphoonHPlugin();

#if defined(__androidx86__)
    TyphoonHM4Interface*   handler                  () { return _pHandler; }
#endif

    // Overrides from QGCCorePlugin
    QGCOptions*     options                         () final;
    QVariantList&   settingsPages                   () final;
    bool            overrideSettingsGroupVisibility (QString name) final;
    bool            adjustSettingMetaData           (FactMetaData& metaData) final;
    QString         brandImageIndoor                () const final;
    QString         brandImageOutdoor               () const final;

#if defined(__androidx86__)
    QGeoPositionInfoSource* createPositionSource    (QObject* parent);
#endif

#if defined (__planner__)
    virtual QQmlApplicationEngine* createRootWindow(QObject* parent);
#endif

    // Overrides from QGCTool
    void                setToolbox                      (QGCToolbox* toolbox);

    TyphoonHQuickInterface* pQFace                      () { return _pQFace; }
    void                    setQFace                    (TyphoonHQuickInterface* pQFace) { _pQFace = pQFace; }

private slots:
    void                _showAdvancedPages              ();

private:
    TyphoonHOptions*        _pOptions;
    TyphoonHQuickInterface* _pQFace;
    QGCSettings*            _pTyphoonSettings;
    QGCSettings*            _pGeneral;
    QGCSettings*            _pOfflineMaps;
    QGCSettings*            _pMAVLink;
    QGCSettings*            _pRCCal;
    QGCSettings*            _pLogDownload;
    QGCSettings*            _pPlannerSync;
#ifdef QT_DEBUG
    QGCSettings*            _pMockLink;
#endif
    QGCSettings*            _pConsole;
    QVariantList            _settingsList;
#if defined(__androidx86__)
    TyphoonHM4Interface*    _pHandler;
#endif
    QTranslator             _YuneecTranslator;
};
