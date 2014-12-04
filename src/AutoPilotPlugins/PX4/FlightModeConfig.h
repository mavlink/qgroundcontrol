/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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


/// @file
///     @brief Flight Mode Configuration
///     @author Don Gagne <don@thegagnes.com

#ifndef FlightModeConfig_H
#define FlightModeConfig_H

#include <QWidget>

#include "UASInterface.h"
#include "ui_FlightModeConfig.h"

namespace Ui {
    class FlightModeConfig;
}


class FlightModeConfig : public QWidget
{
    Q_OBJECT
    
    friend class FlightModeConfigTest; ///< This allows our unit test to access internal information needed.
    
public:
    explicit FlightModeConfig(QWidget *parent = 0);
    
private slots:
    void _channelSelected(int index);
    void _parameterUpdated(int componentId, QString parameter, QVariant value);
    
private:
    void _initUi(void);
    void _updateSwitchUiGroup(QGCComboBox* combo);
    void _selectChannelForParam(QGCComboBox* combo, const QString& parameter);
    void _updateAllSwitches(void);
    int _getChannelMapForParam(const QString& parameter);
    
    UASInterface*                   _mav;           ///< The current MAV
    int                             _componentId;   ///< Default component id
    QGCUASParamManagerInterface*    _paramMgr;
    
    static const int _maxChannels = 18;
    
    QMap<QGCComboBox*, QString>         _mapChannelCombo2Param;
    QMap<QGCComboBox*, QButtonGroup*>   _mapChannelCombo2ButtonGroup;
    QMap<QString, QString>              _mapParam2FunctionInfo;
    
    static const char* _modeSwitchParam;
    static const char* _manualSwitchParam;
    static const char* _assistSwitchParam;
    static const char* _returnSwitchParam;
    static const char* _loiterSwitchParam;
    
    Ui::FlightModeConfig* _ui;
};

#endif
