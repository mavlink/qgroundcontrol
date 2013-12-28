/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2013 Michael Carpenter (malcom2073@gmail.com)

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
 *   @brief Airframe type configuration widget header.
 *
 *   @author Michael Carpenter <malcom2073@gmail.com>
 *
 */

#ifndef FRAMETYPECONFIG_H
#define FRAMETYPECONFIG_H

#include <QWidget>
#include "ui_FrameTypeConfig.h"
#include "AP2ConfigWidget.h"

class FrameTypeConfig : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit FrameTypeConfig(QWidget *parent = 0);
    ~FrameTypeConfig();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void xFrameSelected();
    void plusFrameSelected();
    void vFrameSelected();
private:
    Ui::FrameTypeConfig ui;
};

#endif // FRAMETYPECONFIG_H
