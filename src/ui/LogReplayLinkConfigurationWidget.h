/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef _LogReplayLinkConfigurationWidget_H_
#define _LogReplayLinkConfigurationWidget_H_

#include <QWidget>

#include "LogReplayLink.h"
#include "ui_LogReplayLinkConfigurationWidget.h"

class LogReplayLinkConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    LogReplayLinkConfigurationWidget(LogReplayLinkConfiguration* config, QWidget *parent = 0, Qt::WindowFlags flags = Qt::Sheet);
    
private slots:
    void _selectLogFile(bool checked);

private:
    Ui::LogReplayLinkConfigurationWidget    _ui;
    LogReplayLinkConfiguration*             _config;
};


#endif
