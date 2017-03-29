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
///     @author Matthew Coleman <uavflightdirector@gmail.com>

#include "MixersComponentController.h"
#include "MixersManager.h"
#include "QGCApplication.h"

#include <QSettings>

QGC_LOGGING_CATEGORY(MixersComponentControllerLog, "MixersComponentControllerLog")
QGC_LOGGING_CATEGORY(MixersComponentControllerVerboseLog, "MixersComponentControllerVerboseLog")

//#ifdef UNITTEST_BUILD
//// Nasty hack to expose controller to unit test code
//MixersComponentController* MixersComponentController::_unitTestController = NULL;
//#endif

const int MixersComponentController::_updateInterval = 150;              ///< Interval for timer which updates radio channel widgets

MixersComponentController::MixersComponentController(void)
    : _mixersManagerStatusText(NULL)
    , _mixers(new QmlObjectListModel(this))
    , _selectedParameter(nullptr)
    , _selectedGroup(0)
    , _selectedParamID(0)
{
//    _getMixersCountButton = NULL;
//#ifdef UNITTEST_BUILD
//    // Nasty hack to expose controller to unit test code
//    _unitTestController = this;
//#endif

    // Connect external connections
    connect(_vehicle->mixersManager(), &MixersManager::mixerDataReadyChanged, this, &MixersComponentController::_updateMixers);
    connect(_vehicle->mixersManager(), &MixersManager::mixerManagerStatusChanged, this, &MixersComponentController::_updateMixersManagerStatus);
    connect(this, &MixersComponentController::selectedGroupChanged, this, &MixersComponentController::_updateSelectedGroup);
    connect(this, &MixersComponentController::selectedParamChanged, this, &MixersComponentController::_updateSelectedParam);
}



MixersComponentController::~MixersComponentController()
{
    delete _mixers;
}


void MixersComponentController::guiUpdated(void)
{
    _updateMixersManagerStatus(_vehicle->mixersManager()->mixerManagerStatus());
    _vehicle->mixersManager()->searchAllMixerGroupsAndDownload();
}


void MixersComponentController::_updateMixers(bool dataReady){

    if(dataReady) {
        // Pick a default mixer group selection
        if(_vehicle->mixersManager()->getMixerGroup(1) != nullptr){
            _selectedGroup = 1;
        } else if(_vehicle->mixersManager()->getMixerGroup(2) != nullptr) {
            _selectedGroup = 2;
        } else
            _selectedGroup = 0;

        _updateSelectedGroup(_selectedGroup);
    } else {
        _mixers->clear();
    }
}

void MixersComponentController::_updateMixersManagerStatus(MixersManager::MIXERS_MANAGER_STATUS_e mixerManagerStatus) {
    switch(mixerManagerStatus){
    case MixersManager::MIXERS_MANAGER_WAITING:
        _mixersManagerStatusText->setProperty("text", "WAITING");
        break;
    case MixersManager::MIXERS_MANAGER_DOWNLOADING_ALL:
         _mixersManagerStatusText->setProperty("text", "DOWNLOADING ALL");
        break;
    case MixersManager::MIXERS_MANAGER_DOWNLOADING_MISSING:
         _mixersManagerStatusText->setProperty("text", "DOWNLOADING MISSING");
        break;
    default:
         _mixersManagerStatusText->setProperty("text", "");
        break;
    }
}


void MixersComponentController::_updateSelectedGroup(unsigned int groupID){
    Q_UNUSED(groupID)

    MixerGroup *mixerGroup = _vehicle->mixersManager()->getMixerGroup(_selectedGroup);
    if(mixerGroup != nullptr){
        _mixers->swapObjectList(mixerGroup->parameters());
    } else {
        _mixers->clear();
    }
}


void MixersComponentController::_updateSelectedParam(unsigned int paramID){
    Q_UNUSED(paramID)

    MixerGroup *mixerGroup = _vehicle->mixersManager()->getMixerGroup(_selectedGroup);
    if(mixerGroup == nullptr){
        _selectedParameter = nullptr;
        return;
    }

    MixerParameter* selParam = mixerGroup->getParameter(paramID);
    if(selParam != nullptr){
        _selectedParameter = selParam->param();
    } else {
        _selectedParameter = nullptr;
    }
}
