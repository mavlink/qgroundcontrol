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

#ifndef FLIGHTMODECONFIGTEST_H
#define FLIGHTMODECONFIGTEST_H

#include "AutoTest.h"
#include "MockUASManager.h"
#include "MockUAS.h"
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>

class FlightModeConfig;

/// @file
///     @brief FlightModeConfig unit test
///
///     @author Don Gagne <don@thegagnes.com>

class FlightModeConfigUnitTest : public QObject
{
    Q_OBJECT
    
public:
    FlightModeConfigUnitTest(void);
    
private slots:
    void init(void);
    void cleanup(void);
    
    void _nullUAS_test(void);
    void _validUAS_test(void);
    void _nullToValidUAS_test(void);
    void _simpleModeFixedWing_test(void);
    void _simpleModeRover_test(void);
    void _simpleModeRotor_test(void);
    void _modeSwitchParam_test(void);
    void _pwmFixedWing_test(void);
    void _pwmRotor_test(void);
    void _pwmRover_test(void);
    void _pwmInvalidChannel_test(void);
    void _unknownSystemType_test(void);
    void _unknownMode_test(void);
    
private:
    void _findControls(QObject* fmc);
    void _pwmTestWorker(int systemType, int modeSwitchChannel, const char* modeSwitchParam);
    
private:
    MockUASManager* _mockUASManager;
    MockUAS*        _mockUAS;
    
    // FlightModeConfig ui elements
    static const size_t     _cCombo = 6;
    QLabel*                 _rgLabel[_cCombo];
    QComboBox*              _rgCombo[_cCombo];
    QCheckBox*              _rgSimpleModeCheckBox[_cCombo];
    QLabel*                 _rgPWMLabel[_cCombo];
    QPushButton*            _saveButton;
};

DECLARE_TEST(FlightModeConfigUnitTest)

#endif
