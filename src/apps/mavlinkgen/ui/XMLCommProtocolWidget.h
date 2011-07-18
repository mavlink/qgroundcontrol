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
 *   @brief Definition of class XMLCommProtocolWidget
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef XMLCOMMPROTOCOLWIDGET_H
#define XMLCOMMPROTOCOLWIDGET_H

#include <QtGui/QWidget>
#include "DomModel.h"

namespace Ui
{
class XMLCommProtocolWidget;
}

/**
 * @brief Tool to generate MAVLink code out of XML protocol definitions
 * @see http://doc.trolltech.com/4.6/itemviews-simpledommodel.html for a XML view tutorial
 */
class XMLCommProtocolWidget : public QWidget
{
    Q_OBJECT
public:
    XMLCommProtocolWidget(QWidget *parent = 0);
    ~XMLCommProtocolWidget();

protected slots:
    /** @brief Select input XML protocol definition */
    void selectXMLFile();
    /** @brief Select output directory for generated .h files */
    void selectOutputDirectory();
    /** @brief Set the XML this widget currently operates on */
    void setXML(const QString& xml);
    /** @brief Parse XML file and generate .h files */
    void generate();
    /** @brief Save the edited file */
    void save();

protected:
    DomModel* model;
    void changeEvent(QEvent *e);

private:
    Ui::XMLCommProtocolWidget *m_ui;
};

#endif // XMLCOMMPROTOCOLWIDGET_H
