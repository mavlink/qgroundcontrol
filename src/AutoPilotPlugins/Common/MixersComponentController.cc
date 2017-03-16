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
    : _mixers(new QmlObjectListModel(this))
    , _mockMetaData(FactMetaData::valueTypeString, this)
    , _mockFactList()
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

    connect(_vehicle->mixersManager(), &MixersManager::mixerDataReadyChanged, this, &MixersComponentController::_updateMixers);

    _vehicle->mixersManager()->requestMixerDownload(0);
}



MixersComponentController::~MixersComponentController()
{
//    _storeSettings();
}


void MixersComponentController::guiUpdated(void)
{

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
    return 0;
}

float MixersComponentController::parameterValue(void)
{
    return 0.0;
}

void MixersComponentController::_updateMixers(void){

    MixerGroup *mixerGroup = _vehicle->mixersManager()->getMixerGroup(0);
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
}

