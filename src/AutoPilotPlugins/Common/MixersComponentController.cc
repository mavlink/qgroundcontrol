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

MixerGroupUIData::MixerGroupUIData(QObject *parent)
    :QObject(parent)
    , _groupName("DEFAULT_NAME")
    , _mixerID(99)
{
    setObjectName("DEFAULT_OBJECT_NAME");
}


const int MixersComponentController::_updateInterval = 150;              ///< Interval for timer which updates radio channel widgets

MixersComponentController::MixersComponentController(void)
    : _refreshGUIButton(NULL)
    , _mixersManagerStatusText(NULL)
    , _mixers(new QmlObjectListModel(this))
    , _groups(new QmlObjectListModel(this))
//    , _selectedGroup(0)
    , _mockMetaData(FactMetaData::valueTypeString, this)
    , _mockFactList()
    , _guiInit(false)
{
//    _getMixersCountButton = NULL;
//#ifdef UNITTEST_BUILD
//    // Nasty hack to expose controller to unit test code
//    _unitTestController = this;
//#endif

    _mockMetaData.setName("MOCK_METADATA");
    _mockMetaData.setGroup("MIXER_COMP_CONTROLLER");
    _mockMetaData.setRawDefaultValue("MOCK_STRING");

    Fact *fact;

    fact = new Fact(-1, "Mock Fact 1", FactMetaData::valueTypeString, this);
    fact->setMetaData(&_mockMetaData);
    fact->setRawValue("1");
    _mockFactList.append(fact);

    fact = new Fact(-1, "Mock Fact 2", FactMetaData::valueTypeString, this);
    fact->setMetaData(&_mockMetaData);
    fact->setRawValue("2");
    _mockFactList.append(fact);

    // Connect external connections
    connect(_vehicle->mixersManager(), &MixersManager::mixerDataReadyChanged, this, &MixersComponentController::_updateMixers);
    connect(_vehicle->mixersManager(), &MixersManager::mixerManagerStatusChanged, this, &MixersComponentController::_updateMixersManagerStatus);
    connect(_vehicle->mixersManager(), &MixersManager::mixerGroupStatusChanged, this, &MixersComponentController::_updateMixerGroupStatus);

}



MixersComponentController::~MixersComponentController()
{
    delete _mixers;
//    _storeSettings();
}


void MixersComponentController::guiUpdated(void)
{
    _guiInit = true;
    _updateMixersManagerStatus(_vehicle->mixersManager()->mixerManagerStatus());
    _vehicle->mixersManager()->searchAllMixerGroupsAndDownload();
}


//void MixersComponentController::getMixersCountButtonClicked(void)
//{
//    _vehicle->mixersManager()->requestMixerCount(0);
//}


//void MixersComponentController::requestAllButtonClicked(void)
//{
//    _vehicle->mixersManager()->requestMixerAll(0);
//}

//void MixersComponentController::requestMissingButtonClicked(void)
//{
//    _vehicle->mixersManager()->requestMissingData(0);
//}

//void MixersComponentController::requestSubmixerCountButtonClicked(void)
//{
//    _vehicle->mixersManager()->requestSubmixerCount(0, 0);
//}

void MixersComponentController::refreshGUIButtonClicked(void){
    QObjectList newMixerList;
    Fact* fact;

    foreach(fact, _mockFactList){
        newMixerList.append(fact);
    }

    _mixers->swapObjectList(newMixerList);
}


//void RadioComponentController::_loadSettings(void)
//{
//    QSettings settings;
    
//    settings.beginGroup(_settingsGroup);
//    _transmitterMode = settings.value(_settingsKeyTransmitterMode, 2).toInt();
//    settings.endGroup();
    
//    if (_transmitterMode != 1 || _transmitterMode != 2) {
//        _transmitterMode = 2;
//    }
//}

//void RadioComponentController::_storeSettings(void)
//{
//    QSettings settings;
    
//    settings.beginGroup(_settingsGroup);
//    settings.setValue(_settingsKeyTransmitterMode, _transmitterMode);
//    settings.endGroup();
//}


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
        QMap<int, MixerGroup*>* mixerGroups = _vehicle->mixersManager()->getMixerGroups()->getMixerGroups();
        MixerGroup *mixerGroup;

        _groups->clear();

        QObjectList newGroupsList;
//        Fact *groupData;
        foreach(mixerGroup, *mixerGroups){
            newGroupsList.append(new MixerGroupUIData());
            newGroupsList.append(new MixerGroupUIData());
            newGroupsList.append(new MixerGroupUIData());
//            groupData = new Fact(-1, mixerGroup->groupName(), FactMetaData::valueTypeUint16, this);
//            groupData->setMetaData(&_mockMetaData);
//            groupData->setRawValue(mixerGroup->groupID());
//            newGroupsList.append(groupData);
        }
        _groups->swapObjectList(newGroupsList);

        mixerGroup = _vehicle->mixersManager()->getMixerGroup(0);
        if(mixerGroup != nullptr){
            _mixers->swapObjectList(mixerGroup->mixers());
        } else {
            QObjectList newMixerList;
            Fact* fact;

            foreach(fact, _mockFactList){
                newMixerList.append(fact);
            }
        _mixers->swapObjectList(newMixerList);
        }
    } else {
        _mixers->clear();
    }
}

void MixersComponentController::_updateMixersManagerStatus(MixersManager::MIXERS_MANAGER_STATUS_e mixerManagerStatus) {
//    if(!_guiInit)
//        return;
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


void MixersComponentController::_updateMixerGroupStatus(MixerGroup *mixerGroup){
    unsigned int status = mixerGroup->getGroupStatus();
    Q_UNUSED(status)
}
