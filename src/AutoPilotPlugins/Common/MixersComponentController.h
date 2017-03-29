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

    Q_PROPERTY(QQuickItem* mixersManagerStatusText   MEMBER _mixersManagerStatusText      NOTIFY mixersManagerStatusTextChanged)

    Q_PROPERTY(QmlObjectListModel*  mixersList          MEMBER _mixers              CONSTANT)
    Q_PROPERTY(QObject*             selectedParam       MEMBER _selectedParameter   CONSTANT)
    Q_PROPERTY(unsigned int         selectedGroup       MEMBER _selectedGroup       NOTIFY selectedGroupChanged)
    Q_PROPERTY(unsigned int         selectedParamID     MEMBER _selectedParamID       NOTIFY selectedParamChanged)

    Q_INVOKABLE void guiUpdated(void);
        
    
signals:
    void mixersManagerStatusTextChanged(void);
    void selectedGroupChanged(unsigned int group);
    void selectedParamChanged(unsigned int param);
    void parameterValueChanged(float paramValue);
        
private slots:
    void _updateMixers(bool dataReady);
    void _updateMixersManagerStatus(MixersManager::MIXERS_MANAGER_STATUS_e mixerManagerStatus);
    void _updateSelectedGroup(unsigned int groupID);
    void _updateSelectedParam(unsigned int paramID);

private:    
    static const int _updateInterval;   ///< Interval for ui update timer

    QQuickItem* _mixersManagerStatusText;

    QmlObjectListModel* _mixers;
    MixerParameter*     _selectedParameter;
    unsigned int        _selectedGroup;
    unsigned int        _selectedParamID;


//#ifdef UNITTEST_BUILD
//    // Nasty hack to expose controller to unit test code
//    static RadioComponentController*    _unitTestController;
//#endif
};

#endif // MixersComponentController_H
