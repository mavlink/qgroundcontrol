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
//    , _getMixersCountButton(NULL)
{
    _getMixersCountButton = NULL;
//#ifdef UNITTEST_BUILD
//    // Nasty hack to expose controller to unit test code
//    _unitTestController = this;
//#endif
}

MixersComponentController::~MixersComponentController()
{
//    _storeSettings();
}


void MixersComponentController::getMixersCountButtonClicked(void)
{
    _vehicle->mixersManager()->requestMixerCount(0);
}


void MixersComponentController::requestAllButtonClicked(void)
{
    _vehicle->mixersManager()->requestMixerAll(0);
}

void MixersComponentController::requestMissingButtonClicked(void)
{
    _vehicle->mixersManager()->requestMissingData(0);
}

void MixersComponentController::requestSubmixerCountButtonClicked(void)
{
    _vehicle->mixersManager()->requestSubmixerCount(0, 0);
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

unsigned int MixersComponentController::mixerIndexValue(void)
{
    return 0;
}

unsigned int MixersComponentController::submixerIndexValue(void)
{
    return 0;
}

float MixersComponentController::parameterValue(void)
{
    return 0.0;
}


