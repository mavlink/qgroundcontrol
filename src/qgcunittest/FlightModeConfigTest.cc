/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "FlightModeConfigTest.h"
#include "FlightModeConfig.h"
#include "UASManager.h"
#include "MockQGCUASParamManager.h"

/// @file
///     @brief FlightModeConfig unit test
///
///     @author Don Gagne <don@thegagnes.com>

FlightModeConfigUnitTest::FlightModeConfigUnitTest(void) :
    _mockUASManager(NULL)
{
    
}

void FlightModeConfigUnitTest::init(void)
{
    _mockUASManager = new MockUASManager();
    Q_ASSERT(_mockUASManager);
    
    UASManager::setMockUASManager(_mockUASManager);
    
    _mockUAS = new MockUAS();
    Q_ASSERT(_mockUAS);
}

void FlightModeConfigUnitTest::cleanup(void)
{
    Q_ASSERT(_mockUAS);
    delete _mockUAS;
    
    UASManager::setMockUASManager(NULL);

    Q_ASSERT(_mockUASManager);
    delete _mockUASManager;
}

void FlightModeConfigUnitTest::_nullUAS_test(void)
{
    // When there is no UAS the widget should be disabled
    FlightModeConfig* fmc = new FlightModeConfig();
    QCOMPARE(fmc->isEnabled(), false);
}

void FlightModeConfigUnitTest::_validUAS_test(void)
{
    // With an active UAS widget should be enabled
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    QCOMPARE(fmc->isEnabled(), true);
}

void FlightModeConfigUnitTest::_nullToValidUAS_test(void)
{
    // start with no UAS
    FlightModeConfig* fmc = new FlightModeConfig();
    
    // clear it out and validate widget gets disabled
    _mockUASManager->setMockActiveUAS(NULL);
    QCOMPARE(fmc->isEnabled(), false);
}

void FlightModeConfigUnitTest::_simpleModeFixedWing_test(void)
{
    // Fixed wing does not have simple mode, all checkboxes should be disabled
    _mockUAS->setMockSystemType(MAV_TYPE_FIXED_WING);
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    _findControls(fmc);

    for (size_t i=0; i<_cCombo; i++) {
        QCOMPARE(_rgSimpleModeCheckBox[i]->isEnabled(), false);
    }
}

void FlightModeConfigUnitTest::_simpleModeRover_test(void)
{
    // Rover does not have simple mode, all checkboxes should be disabled
    _mockUAS->setMockSystemType(MAV_TYPE_GROUND_ROVER);
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    _findControls(fmc);
    
    for (size_t i=0; i<_cCombo; i++) {
        QCOMPARE(_rgSimpleModeCheckBox[i]->isEnabled(), false);
    }
}

void FlightModeConfigUnitTest::_simpleModeRotor_test(void)
{
    // Rotor has simple mode, all checkboxes should be enabled
    _mockUAS->setMockSystemType(MAV_TYPE_QUADROTOR);
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    _findControls(fmc);
    
    for (size_t i=0; i<_cCombo; i++) {
        QCOMPARE(_rgSimpleModeCheckBox[i]->isEnabled(), true);
    }
}

void FlightModeConfigUnitTest::_modeSwitchParam_test(void)
{
    _mockUAS->setMockSystemType(MAV_TYPE_QUADROTOR);
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    _findControls(fmc);
    
    const char* rgModeSwitchParam[_cCombo] = { "FLTMODE1", "FLTMODE2", "FLTMODE3", "FLTMODE4", "FLTMODE5", "FLTMODE6" };
    const int rgModeSwitchValue[_cCombo] = { 0, 2, 4, 6, 8, 10 };
    
    MockQGCUASParamManager::ParamMap_t mapParams;
    for (size_t i=0; i<_cCombo; i++) {
        mapParams[rgModeSwitchParam[i]] = QVariant(QChar(rgModeSwitchValue[i]));
    }
    
    int simpleModeBitMask = 21;
    mapParams["SIMPLE"] = QVariant(QChar(simpleModeBitMask));
    _mockUAS->setMockParametersAndSignal(mapParams);
    
    // Check that the UI is showing the correct information
    for (size_t i=0; i<_cCombo; i++) {
        QComboBox* combo = _rgCombo[i];
        
        // Check for the correct selection in the combo boxes. Combo boxes store the mode
        // in the item data, so use that to compare
        QCOMPARE(combo->itemData(combo->currentIndex()), mapParams[rgModeSwitchParam[i]]);
        
        // Make sure the text for the current selection doesn't contain the text Unknown
        // which means that it received an unsupported mode
        QCOMPARE(combo->currentText().contains("unknown", Qt::CaseInsensitive), false);

        // Check that the right simple mode check boxes are checked
        QCOMPARE(_rgSimpleModeCheckBox[i]->isChecked(), !!((1 << i) & simpleModeBitMask));
    }
    
    
    // Click Save button and make sure we get the same values back through setParameter calls
    QTest::mouseClick(_saveButton, Qt::LeftButton);
    MockQGCUASParamManager* paramMgr = _mockUAS->getMockQGCUASParamManager();
    MockQGCUASParamManager::ParamMap_t mapParamsSet = paramMgr->getMockSetParameters();
    QMapIterator<QString, QVariant> i(mapParams);
    while (i.hasNext()) {
        i.next();
        QCOMPARE(mapParamsSet.contains(i.key()), true);
        int receivedValue = mapParamsSet[i.key()].toInt();
        int expectedValue = i.value().toInt();
        QCOMPARE(receivedValue, expectedValue);
    }
}

void FlightModeConfigUnitTest::_pwmTestWorker(int systemType, int modeSwitchChannel, const char* modeSwitchParam)
{
    _mockUAS->setMockSystemType(systemType);
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    _findControls(fmc);
    
    MockQGCUASParamManager::ParamMap_t mapParams;
    if (modeSwitchParam != NULL) {
        // Param is 1-based, code in here is all 0-base
        mapParams[modeSwitchParam] = QVariant(QChar(modeSwitchChannel+1));
        _mockUAS->setMockParametersAndSignal(mapParams);
    }
    
    const int pwmBoundary[] = { 1230, 1360, 1490, 1620, 1749, 1900 };
    
    int lowerPWM = 0;
    for (size_t i=0; i<_cCombo; i++) {
        // emit a PWM value at the mid point of the switch position
        int pwmMidPoint = ((pwmBoundary[i] - lowerPWM) / 2) + lowerPWM;
        _mockUAS->emitRemoteControlChannelRawChanged(modeSwitchChannel, pwmMidPoint);
        
        // Make sure that only the correct pwm label has a style set on it for highlight
        for (size_t j=0; j<_cCombo; j++) {
            if (j == i) {
                QVERIFY(_rgPWMLabel[j]->styleSheet().length() > 0);
            } else {
                QCOMPARE(_rgPWMLabel[j]->styleSheet().length(), 0);
            }
        }
        
        lowerPWM = pwmBoundary[i];
    }
}

void FlightModeConfigUnitTest::_pwmFixedWing_test(void)
{
    // FixedWing has mode switch channel on FLTMODE_CH param
    _pwmTestWorker(MAV_TYPE_FIXED_WING, 7, "FLTMODE_CH");
}

void FlightModeConfigUnitTest::_pwmRotor_test(void)
{
    // Rotor is hardwired to 0-based rc channel 4 for mode wsitch
    _pwmTestWorker(MAV_TYPE_QUADROTOR, 4, NULL);
}

void FlightModeConfigUnitTest::_pwmRover_test(void)
{
    // Rover has mode switch channel on MODE_CH param
    _pwmTestWorker(MAV_TYPE_GROUND_ROVER, 7, "MODE_CH");
}

void FlightModeConfigUnitTest::_pwmInvalidChannel_test(void)
{
    // Rover has mode switch channel on MODE_CH param
    _mockUAS->setMockSystemType(MAV_TYPE_GROUND_ROVER);
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    _findControls(fmc);
    
    int modeSwitchChannel = 7;
    MockQGCUASParamManager::ParamMap_t mapParams;
    mapParams["MODE_CH"] = QVariant(QChar(modeSwitchChannel+1));    // 1-based
    _mockUAS->setMockParametersAndSignal(mapParams);
    
    const int pwmBoundary[] = { 1230, 1360, 1490, 1620, 1749, 1900 };
    
    int lowerPWM = 0;
    for (size_t i=0; i<_cCombo; i++) {
        // emit a PWM value at the mid point of the switch position
        int pwmMidPoint = ((pwmBoundary[i] - lowerPWM) / 2) + lowerPWM;
        // emit rc values on wrong channel
        _mockUAS->emitRemoteControlChannelRawChanged(modeSwitchChannel-1, pwmMidPoint);
        
        // Make sure no label have a style set on it for highlight
        for (size_t j=0; j<_cCombo; j++) {
            QCOMPARE(_rgPWMLabel[j]->styleSheet().length(), 0);
        }
        
        lowerPWM = pwmBoundary[i];
    }
}

void FlightModeConfigUnitTest::_unknownSystemType_test(void)
{
    // Set the system type to something we can't handle, make sure we are disabled
    _mockUAS->setMockSystemType(MAV_TYPE_ENUM_END);
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    QCOMPARE(fmc->isEnabled(), false);
}

void FlightModeConfigUnitTest::_unknownMode_test(void)
{
    _mockUAS->setMockSystemType(MAV_TYPE_QUADROTOR);
    _mockUASManager->setMockActiveUAS(_mockUAS);
    FlightModeConfig* fmc = new FlightModeConfig();
    _findControls(fmc);
    
    // Set an unknown mode
    MockQGCUASParamManager::ParamMap_t mapParams;
    mapParams["FLTMODE1"] = QVariant(QChar(100));
    _mockUAS->setMockParametersAndSignal(mapParams);
    
    // Check for the correct selection in the combo boxes. Combo boxes store the mode
    // in the item data, so use that to compare
    QCOMPARE(_rgCombo[0]->itemData(_rgCombo[0]->currentIndex()), mapParams["FLTMODE1"]);
    
    // Make sure the text for the current selection contains the text Unknown
    QCOMPARE(_rgCombo[0]->currentText().contains("unknown", Qt::CaseInsensitive), true);
}

void FlightModeConfigUnitTest::_findControls(QObject* fmc)
{
   // Find all the controls
   for (size_t i=0; i<_cCombo; i++) {
       _rgLabel[i] = fmc->findChild<QLabel*>(QString("mode%1Label").arg(i));
       _rgCombo[i] = fmc->findChild<QComboBox*>(QString("mode%1ComboBox").arg(i));
       _rgSimpleModeCheckBox[i] = fmc->findChild<QCheckBox*>(QString("mode%1SimpleCheckBox").arg(i));
       _rgPWMLabel[i] = fmc->findChild<QLabel*>(QString("mode%1PWMLabel").arg(i));
       Q_ASSERT(_rgLabel[i]);
       Q_ASSERT(_rgCombo[i]);
       Q_ASSERT(_rgSimpleModeCheckBox[i]);
       Q_ASSERT(_rgPWMLabel[i]);
   }
    
    _saveButton = fmc->findChild<QPushButton*>("savePushButton");
    Q_ASSERT(_saveButton);
}

