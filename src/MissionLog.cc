/*=====================================================================
 
PIXHAWK Micro Air Vehicle Flying Robotics Toolkit
 
(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>
 
This file is part of the PIXHAWK project
 
    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
 
    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.
 
======================================================================*/
 
/**
 * @file
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QFile>
#include <QFlags>
#include <QThread>
#include <QSplashScreen>
#include <QPixmap>
#include <QDesktopWidget>
#include <QPainter>
#include <QStyleFactory>
#include <QAction>

#include <MissionLog.h>
#include <MG.h>

#include <QDebug>


/**
 * @brief Constructor for the mission log
 *
 **/

MissionLog::MissionLog(QObject* parent) : QObject(parent)
{
    logLines = new QMap<QString, LogLine*>();
    logFiles = new QMap<QString, QFile*>();
}

/**
 * @brief Destructor for the mission log. It closes all files
 *
 **/
MissionLog::~MissionLog()
{
    delete logFiles;
    delete logLines;
}

void MissionLog::startLog(UASInterface* uas, QString format)
{
    QString separator;

    if (format.contains(",")) separator = ",";
    if (format.contains(";")) separator = ";";
    if (format.contains("\t")) separator = "\t";

    QStringList fields = format.split(separator);
}

void MissionLog::stopLog(UASInterface* uas, QString format)
{
    // TODO Check if file has to be closed explicitely
    logFiles->remove(format);
}


