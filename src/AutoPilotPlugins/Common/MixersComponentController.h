/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/



/// @file
///     @brief Mixers Config Qml Controller
///     @author

#ifndef MixersComponentController_H
#define MixersComponentController_H

#include <QTimer>

#include "FactPanelController.h"
#include "UASInterface.h"
#include "QGCLoggingCategory.h"
#include "AutoPilotPlugin.h"
#include <FactMetaData.h>
#include "MixersManager.h"

Q_DECLARE_LOGGING_CATEGORY(MixersComponentControllerLog)
Q_DECLARE_LOGGING_CATEGORY(MixersComponentControllerVerboseLog)

namespace Ui {
    class MixersComponentController;
}


class MixersComponentController : public FactPanelController
{
    Q_OBJECT

public:
    MixersComponentController(void);
    ~MixersComponentController();

    Q_PROPERTY(QQuickItem* mixersManagerStatusText  MEMBER _mixersManagerStatusText     NOTIFY mixersManagerStatusTextChanged)
    Q_PROPERTY(QQuickItem* percentDownloadedText    MEMBER _percentDownloadedText       NOTIFY percentDownloadedTextChanged)

    Q_PROPERTY(QmlObjectListModel*  mixersList          MEMBER _mixers              CONSTANT)
    Q_PROPERTY(unsigned int         selectedGroup       MEMBER _selectedGroup       NOTIFY selectedGroupChanged)

    Q_INVOKABLE void guiUpdated(void);
    Q_INVOKABLE void storeSelectedGroup(void);
    Q_INVOKABLE QString getMixerTypeAlias(unsigned int typeID);
    Q_INVOKABLE QString getValuesString(QObject* paramObj);
    
signals:
    void mixersManagerStatusTextChanged(void);
    void percentDownloadedTextChanged(void);
    void selectedGroupChanged(unsigned int group);
    void parameterValueChanged(float paramValue);
        
private slots:
    void _updateMixers(bool dataReady);
    void _updateMixersManagerStatus(MixersManager::MIXERS_MANAGER_STATUS_e mixerManagerStatus);
    void _updateSelectedGroup(unsigned int groupID);
    void _updatePercentDownloaded(float percent);
    void  _guiUpdate(void);

private:    
    static const int _updateInterval;   ///< Interval for ui update timer

    ///* Timer to avoid overloading AP with parameter value changes*/
    QTimer              _guiUpdateTimer;

    QQuickItem* _mixersManagerStatusText;
    QQuickItem* _percentDownloadedText;

    QmlObjectListModel* _mixers;
    unsigned int        _selectedGroup;
    float               _percentDownloaded;
    QStringList         _mixerTypeAliases;


//#ifdef UNITTEST_BUILD
//    // Nasty hack to expose controller to unit test code
//    static RadioComponentController*    _unitTestController;
//#endif
};

#endif // MixersComponentController_H
