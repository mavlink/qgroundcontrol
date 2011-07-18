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
 *   @brief Definition of class MAVLinkXMLParser
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#ifndef MAVLINKXMLPARSER_H
#define MAVLINKXMLPARSER_H

#include <QObject>
#include <QDomDocument>
#include <QString>

/**
 * @brief MAVLink micro air vehicle protocol generator
 *
 * MAVLink is a generic communication protocol for micro air vehicles.
 * for more information, please see the official website.
 * @ref http://pixhawk.ethz.ch/software/mavlink/
 **/
class MAVLinkXMLParser : public QObject
{
    Q_OBJECT
public:
    MAVLinkXMLParser(QDomDocument* document, QString outputDirectory, QObject* parent=0);
    MAVLinkXMLParser(QString document, QString outputDirectory, QObject* parent=0);
    ~MAVLinkXMLParser();

public slots:
    /** @brief Parse XML and generate C files */
    bool generate();

signals:
    /** @brief Status message on the parsing */
    void parseState(QString message);

protected:
    QDomDocument* doc;
    QString outputDirName;
    QString fileName;
};

#endif // MAVLINKXMLPARSER_H
