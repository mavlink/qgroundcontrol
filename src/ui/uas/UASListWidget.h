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
 *   @brief List of unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _UASLISTWIDGET_H_
#define _UASLISTWIDGET_H_

#include <QWidget>
#include <QMap>
#include <QVBoxLayout>
#include "UASInterface.h"
#include "UASView.h"
#include "QGCUnconnectedInfoWidget.h"
#include "ui_UASList.h"

class UASListWidget : public QWidget
{
    Q_OBJECT

public:
    UASListWidget(QWidget *parent = 0);
    ~UASListWidget();

public slots:
    void addUAS(UASInterface* uas);
    void activeUAS(UASInterface* uas);
    void removeUAS(UASInterface* uas);

protected:
    QMap<UASInterface*, UASView*> uasViews;
    QVBoxLayout* listLayout;
    QGCUnconnectedInfoWidget* uWidget;
    void changeEvent(QEvent *e);

private:
    Ui::UASList* m_ui;

};

#endif // _UASLISTWIDGET_H_
