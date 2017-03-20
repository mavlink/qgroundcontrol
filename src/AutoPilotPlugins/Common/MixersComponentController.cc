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
    , _groups(new QmlObjectListModel(this))
    , _selectedGroup(0)
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
}



MixersComponentController::~MixersComponentController()
{
    delete _mixers;
//    _storeSettings();
}


void MixersComponentController::guiUpdated(void)
{
    _updateMixersManagerStatus(_vehicle->mixersManager()->mixerManagerStatus());
    _vehicle->mixersManager()->searchAllMixerGroupsAndDownload();
}


unsigned int MixersComponentController::groupValue(void)
{    
//    return _selectedGroup;
    return 0;
}

float MixersComponentController::parameterValue(void)
{
    return 0.0;
}

void MixersComponentController::_updateMixers(bool dataReady){

    if(dataReady) {
        MixerGroup *mixerGroup;
        _groups->clear();

//        QMap<int, MixerGroup*>* mixerGroups = _vehicle->mixersManager()->getMixerGroups()->getMixerGroups();
//  For the combobox that just doesn't work
//        QObjectList newGroupsList;
//        foreach(mixerGroup, *mixerGroups){
//            newGroupsList.append(mixerGroup);
//        }
//        _groups->swapObjectList(newGroupsList);

        // Pick a default mixer group selection
        if(_vehicle->mixersManager()->getMixerGroup(0) != nullptr){
            _selectedGroup = 0;
        } else if(_vehicle->mixersManager()->getMixerGroup(1) != nullptr) {
            _selectedGroup = 1;
        } else {
            _mixers->clear();
            return;
        }

        mixerGroup = _vehicle->mixersManager()->getMixerGroup(_selectedGroup);
        if(mixerGroup != nullptr){
            _mixers->swapObjectList(mixerGroup->mixers());
        } else {
            _mixers->clear();
        }
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
    case MixersManager::MIXERS_MANAGER_DOWNLOADING_MIXER_INFO:
         _mixersManagerStatusText->setProperty("text", "DOWNLOADING MIXER INFO");
        break;
    default:
         _mixersManagerStatusText->setProperty("text", "");
        break;
    }
}


void MixersComponentController::_updateSelectedGroup(unsigned int group){
    Q_UNUSED(group)

    MixerGroup *mixerGroup = _vehicle->mixersManager()->getMixerGroup(_selectedGroup);
    if(mixerGroup != nullptr){
        _mixers->swapObjectList(mixerGroup->mixers());
    } else {
        _mixers->clear();
    }
}
