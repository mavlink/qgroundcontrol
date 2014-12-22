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

#ifndef FlightModeConfigTest_H
#define FlightModeConfigTest_H

#include "UnitTest.h"
#include "MockUASManager.h"
#include "MockUAS.h"
#include "AutoPilotPlugins/PX4/FlightModeConfig.h"

/// @file
///     @brief FlightModeConfig Widget unit test
///
///     @author Don Gagne <don@thegagnes.com>

class FlightModeConfigTest : public UnitTest
{
    Q_OBJECT
    
public:
    FlightModeConfigTest(void);
    
private slots:
    void initTestCase(void);
    void init(void);
    void cleanup(void);
    
    void _create_test(void);
    void _validateInitialState_test(void);
    void _validateRCCalCheck_test(void);
    void _attempChannelReuse_test(void);
    void _validateRadioButtonEnableDisable_test(void);
    void _validateModeSwitchMustBeEnabled_test(void);
    
private:
    int _getChannelMapForParam(const QString& parameter);

    MockUASManager*                 _mockUASManager;
    MockUAS*                        _mockUAS;
    QGCUASParamManagerInterface*    _paramMgr;
    FlightModeConfig*               _configWidget;
    int                             _defaultComponentId;
    
    QMap<QGCComboBox*, QString>         _mapChannelCombo2Param;
    QMap<QGCComboBox*, QButtonGroup*>   _mapChannelCombo2ButtonGroup;

	static const int _availableChannels = 18;	///< Simulate 18 channel RC Transmitter
};

#endif
