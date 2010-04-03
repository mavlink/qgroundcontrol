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

#ifndef _MISSIONLOG_H_
#define _MISSIONLOG_H_

#include <QString>
#include <QFile>
#include <QMap>
#include <UASInterface.h>

/**
 * @brief Log the mission executed by this groundstation
 * 
 * Displays all events in a console window and writes (if enabled) all events to a logfile
 *
 **/
class MissionLog : public QObject {
    Q_OBJECT

public:
    MissionLog(QObject* parent = NULL);
    ~MissionLog();

public slots:
    void startLog(UASInterface* uas, QString format);
    void stopLog(UASInterface* uas, QString format);

protected:
    /**
         * @brief This nested class is just a data container for a log line
         */
    class LogFile
    {
    public:
        void setField(QString id, double value)
        {
            //fields->value(id.trimmed()) = value;
            fieldsDone++;
            if (fieldsDone == fields->size()) writeLine();
        }

            protected:
        QMap<QString, double>* fields;
        int fieldsDone;
        QFile* file;

        LogFile(QFile* file, QString format)
        {
            fields = new QMap<QString, double>();
            fieldsDone = 0;
            file->open(QIODevice::WriteOnly | QIODevice::Text);
        }

        void addField(QString id)
        {
            fields->insert(id, 0.0f);
        }

        void writeLine()
        {
            QString line = "";
            // Iterate through the fields

            // Reset value to zero after write
            line += "\n";
            file->write(line.toAscii());
            file->flush();
        }

    };
    QMap<QString, LogFile*>* logFiles;

private:

};

#endif // _MISSIONLOG_H_
