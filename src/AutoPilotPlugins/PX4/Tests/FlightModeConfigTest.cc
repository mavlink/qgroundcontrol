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
#include "UASManager.h"
#include "MockQGCUASParamManager.h"
#include "QGCMessageBox.h"

/// @file
///     @brief FlightModeConfig Widget unit test
///
///     @author Don Gagne <don@thegagnes.com>

UT_REGISTER_TEST(FlightModeConfigTest)

FlightModeConfigTest::FlightModeConfigTest(void) :
    _mockUASManager(NULL),
    _paramMgr(NULL),
    _configWidget(NULL),
    _defaultComponentId(0)
{

}

void FlightModeConfigTest::initTestCase(void)
{

}

void FlightModeConfigTest::init(void)
{
    UnitTest::init();
    
    _mockUASManager = new MockUASManager();
    Q_ASSERT(_mockUASManager);
    
    UASManager::setMockUASManager(_mockUASManager);
    
    _mockUAS = new MockUAS();
    Q_CHECK_PTR(_mockUAS);
    _paramMgr = _mockUAS->getParamManager();
    _defaultComponentId = _paramMgr->getDefaultComponentId();
    
    _mockUASManager->setMockActiveUAS(_mockUAS);
    
    struct SetupParamInfo {
        const char* param;
        int         mappedChannel;
    };
    
    struct SetupParamInfo rgSetupParamInfo[] = {
        { "RC_MAP_MODE_SW",   5 },
        { "RC_MAP_ACRO_SW",   0 },
        { "RC_MAP_POSCTL_SW", 0 },
        { "RC_MAP_RETURN_SW", 0 },
        { "RC_MAP_LOITER_SW", 0 },
        { "RC_MAP_THROTTLE",  1 },
        { "RC_MAP_YAW",       2 },
        { "RC_MAP_ROLL",      3 },
        { "RC_MAP_PITCH",     4 },
        { "RC_MAP_AUX1",      0 },
        { "RC_MAP_AUX2",      0 },
        { "RC_MAP_AUX3",      0 }
    };
    size_t crgSetupParamInfo = sizeof(rgSetupParamInfo) / sizeof(rgSetupParamInfo[0]);

    // Set initial parameter values
    for (size_t i=0; i<crgSetupParamInfo; i++) {
        struct SetupParamInfo* info = &rgSetupParamInfo[i];
        
        _paramMgr->setParameter(_defaultComponentId, info->param, info->mappedChannel);
    }

    _configWidget = new FlightModeConfig;
    Q_CHECK_PTR(_configWidget);
    _configWidget->setVisible(true);
    
    // Setup combo maps
    
    struct SetupComboInfo {
        QGCComboBox*    combo;
        QButtonGroup*   buttonGroup;
        const char*     param;
    };
    
    struct SetupComboInfo rgSetupComboInfo[] = {
        { _configWidget->_ui->modeSwitchChannel,    _configWidget->_ui->modeSwitchGroup,    "RC_MAP_MODE_SW" },
        { _configWidget->_ui->manualSwitchChannel,  _configWidget->_ui->manualSwitchGroup,  "RC_MAP_ACRO_SW" },
        { _configWidget->_ui->assistSwitchChannel,  _configWidget->_ui->assistSwitchGroup,  "RC_MAP_POSCTL_SW" },
        { _configWidget->_ui->returnSwitchChannel,  _configWidget->_ui->returnSwitchGroup,  "RC_MAP_RETURN_SW" },
        { _configWidget->_ui->loiterSwitchChannel,  _configWidget->_ui->loiterSwitchGroup,  "RC_MAP_LOITER_SW" }
    };
    size_t crgSetupComboInfo = sizeof(rgSetupComboInfo) / sizeof(rgSetupComboInfo[0]);

    Q_ASSERT(_mapChannelCombo2Param.count() == 0);
    for (size_t i=0; i<crgSetupComboInfo; i++) {
        struct SetupComboInfo* info = &rgSetupComboInfo[i];
        
        Q_ASSERT(!_mapChannelCombo2Param.contains(info->combo));
        _mapChannelCombo2Param[info->combo] = info->param;
        
        Q_ASSERT(!_mapChannelCombo2ButtonGroup.contains(info->combo));
        _mapChannelCombo2ButtonGroup[info->combo] = info->buttonGroup;
    }
}

void FlightModeConfigTest::cleanup(void)
{
    UnitTest::cleanup();
    
    Q_ASSERT(_configWidget);
    delete _configWidget;
    
    Q_ASSERT(_mockUAS);
    delete _mockUAS;
    
    UASManager::setMockUASManager(NULL);
    
    Q_ASSERT(_mockUASManager);
    delete _mockUASManager;
    
    _mapChannelCombo2Param.clear();
}

/// @brief Returns channel mapping for the specified parameters
int FlightModeConfigTest::_getChannelMapForParam(const QString& paramId)
{
    QVariant value;
    bool found = _paramMgr->getParameterValue(_defaultComponentId, paramId, value);
    Q_UNUSED(found);
    Q_ASSERT(found);
    
    bool conversionOk;
    int channel = value.toInt(&conversionOk);
    Q_UNUSED(conversionOk);
    Q_ASSERT(conversionOk);
    
    return channel;
}

void FlightModeConfigTest::_create_test(void)
{
    // This just test widget creation which is handled in init
}

void FlightModeConfigTest::_validateInitialState_test(void)
{
    // Make sure that the combo boxes are set to the correct selections
    foreach(QGCComboBox* combo, _mapChannelCombo2Param.keys()) {
        QVariant value;
        
        Q_ASSERT(_mapChannelCombo2Param.contains(combo));
        int expectedMappedChannel = _getChannelMapForParam(_mapChannelCombo2Param[combo]);
        
        int currentIndex = combo->currentIndex();
        QVERIFY(currentIndex != -1);
        
        int comboMappedChannel = combo->itemData(currentIndex).toInt();
        
        QCOMPARE(comboMappedChannel, expectedMappedChannel);
    }
    
    QGCComboBox* combo = _configWidget->_ui->modeSwitchChannel;

    // combo should have entry for each channel plus disabled
    QCOMPARE(combo->count(), _availableChannels + 1);
             
    for (int i=0; i<combo->count(); i++) {
        // Combo data equates to channel mapping value. These are one-base channel values with
        // 0 meaning disabled. The channel should match the index.
        QCOMPARE(i, combo->itemData(i).toInt());
        
        // Channels which are mapped are displayed after the channel number in the form "(name)".
        // Check for the ending parentheses to indicate mapped channels
        QString text = combo->itemText(i);
        if (text.endsWith(")")) {
            // Channels 1-5 are mapped
            QVERIFY(i >= 1 && i <= 5);
        } else {
            QVERIFY(!(i >= 1 && i <= 5));
        }
    }
}

void FlightModeConfigTest::_validateRCCalCheck_test(void)
{
    // Mode switch mapped is used to signal RC Calibration not complete. You must complete RC Cal before
    // doing Flight Mode Config.
    
    // Set mode switch mapping to not set
    Q_ASSERT(_mapChannelCombo2Param.contains(_configWidget->_ui->modeSwitchChannel));
    _paramMgr->setParameter(_defaultComponentId, _mapChannelCombo2Param[_configWidget->_ui->modeSwitchChannel], 0);
    
    // Widget should disable
    QCOMPARE(_configWidget->isEnabled(), false);
    
    // Set mode switch mapping to mapped
    Q_ASSERT(_mapChannelCombo2Param.contains(_configWidget->_ui->modeSwitchChannel));
    _paramMgr->setParameter(_defaultComponentId, _mapChannelCombo2Param[_configWidget->_ui->modeSwitchChannel], 5);
    
    // Widget should re-enable
    QCOMPARE(_configWidget->isEnabled(), true);
}

void FlightModeConfigTest::_attempChannelReuse_test(void)
{
    // Attempt to select a channel that is already in use for a switch mapping
    foreach(QGCComboBox* combo, _mapChannelCombo2Param.keys()) {
        
        Q_ASSERT(_mapChannelCombo2Param.contains(combo));
        int beforeMapping = _getChannelMapForParam(_mapChannelCombo2Param[combo]);
        
        setExpectedMessageBox(QMessageBox::Ok);
        
        // Channel 1 mapped to throttle. This should pop a message box and the parameter value should
        // not change.
        combo->simulateUserSetCurrentIndex(1);
        
        checkExpectedMessageBox();
        
        int afterMapping = _getChannelMapForParam(_mapChannelCombo2Param[combo]);
        
        QCOMPARE(beforeMapping, afterMapping);
    }
}

void FlightModeConfigTest::_validateRadioButtonEnableDisable_test(void)
{
    // Radio button should enable/disable according to selection
    foreach(QGCComboBox* combo, _mapChannelCombo2Param.keys()) {
        // Mode switch can't be disabled
        if (combo == _configWidget->_ui->modeSwitchChannel) {
            continue;
        }
        
        // Index 0 is disabled
        combo->simulateUserSetCurrentIndex(0);
        
        Q_ASSERT(_mapChannelCombo2ButtonGroup.contains(combo));
        QButtonGroup* buttonGroup = _mapChannelCombo2ButtonGroup[combo];
        
        foreach(QAbstractButton* button, buttonGroup->buttons()) {
            QRadioButton* radio = qobject_cast<QRadioButton*>(button);
            Q_ASSERT(radio);
            
            QCOMPARE(radio->isEnabled(), false);
        }
        
        // Index 6 should be a valid mapping
        combo->simulateUserSetCurrentIndex(6);
        
        foreach(QAbstractButton* button, buttonGroup->buttons()) {
            QRadioButton* radio = qobject_cast<QRadioButton*>(button);
            Q_ASSERT(radio);
            
            QCOMPARE(radio->isEnabled(), true);
        }

        // Set back to disabled for next iteration through loop. Otherwise we'll error out
        // with the channel already in use
        combo->simulateUserSetCurrentIndex(0);
    }
}

void FlightModeConfigTest::_validateModeSwitchMustBeEnabled_test(void)
{
    QGCComboBox* modeCombo = _configWidget->_ui->modeSwitchChannel;
    
    Q_ASSERT(_mapChannelCombo2Param.contains(modeCombo));
    int beforeMapping = _getChannelMapForParam(_mapChannelCombo2Param[modeCombo]);
    
    setExpectedMessageBox(QMessageBox::Ok);
    
    // Index 0 is disabled. This should pop a message box and the parameter value should
    // not change.
    modeCombo->simulateUserSetCurrentIndex(0);
    
    checkExpectedMessageBox();
    
    int afterMapping = _getChannelMapForParam(_mapChannelCombo2Param[modeCombo]);
    
    QCOMPARE(beforeMapping, afterMapping);
}
