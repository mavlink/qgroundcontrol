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

#include "LogReplayLinkConfigurationWidget.h"
#include "QGCFileDialog.h"
#include "QGCApplication.h"

LogReplayLinkConfigurationWidget::LogReplayLinkConfigurationWidget(LogReplayLinkConfiguration *config, QWidget *parent, Qt::WindowFlags flags) :
    QWidget(parent, flags)
{
    _ui.setupUi(this);
    
    Q_ASSERT(config != NULL);
    _config = config;
    
    _ui.logFilename->setText(_config->logFilename());
    
    connect(_ui.selectLogFileButton, &QPushButton::clicked, this, &LogReplayLinkConfigurationWidget::_selectLogFile);
}

void LogReplayLinkConfigurationWidget::_selectLogFile(bool checked)
{
    Q_UNUSED(checked);
    
    QString logFile = QGCFileDialog::getOpenFileName(this,
                                                     "Select log file to replay",
                                                     qgcApp()->mavlinkLogFilesLocation(),
                                                     "MAVLink Log Files (*.mavlink);;All Files (*)");
    if (!logFile.isEmpty()) {
        _ui.logFilename->setText(logFile);
        _config->setLogFilename(logFile);
    }
}