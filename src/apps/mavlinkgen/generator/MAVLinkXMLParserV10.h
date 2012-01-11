/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/**
 * @file
 *   @brief Definition of class MAVLinkXMLParserV10
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#ifndef MAVLINKXMLPARSERV10_H
#define MAVLINKXMLPARSERV10_H

#include <QObject>
#include <QDomDocument>
#include <QString>
#include <QProcess>

#include <inttypes.h>

/**
 * @brief MAVLink micro air vehicle protocol generator
 *
 * MAVLink is a generic communication protocol for micro air vehicles.
 * for more information, please see the official website.
 * @ref http://pixhawk.ethz.ch/software/mavlink/
 **/
class MAVLinkXMLParserV10 : public QObject
{
    Q_OBJECT
public:
    MAVLinkXMLParserV10(QDomDocument* document, QString outputDirectory, QObject* parent=0);
    MAVLinkXMLParserV10(QString document, QString outputDirectory, QObject* parent=0);
    ~MAVLinkXMLParserV10();

public slots:
    /** @brief Parse XML and generate C files */
    bool generate();

    /** @brief Handle process errors */
    void processError(QProcess::ProcessError err);

    /** @brief Redirect standard output */
    void readStdOut();
    /** @brief Redirect standard error output */
    void readStdErr();

signals:
    /** @brief Status message on the parsing */
    void parseState(QString message);

protected:
//    /** @brief Accumulate the X.25 CRC by adding one char at a time. */
//    void crcAccumulate(uint8_t data, uint16_t *crcAccum);

//    /** @brief Initialize the buffer for the X.25 CRC */
//    void crcInit(uint16_t* crcAccum);

    QDomDocument* doc;
    QString outputDirName;
    QString fileName;
    QProcess* process;
};

#endif // MAVLINKXMLPARSERV10_H
