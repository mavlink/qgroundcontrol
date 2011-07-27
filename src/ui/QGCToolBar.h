/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#ifndef QGCTOOLBAR_H
#define QGCTOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QToolButton>
#include <QLabel>
#include "UASInterface.h"

class QGCToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit QGCToolBar(QWidget *parent = 0);
    void addPerspectiveChangeAction(QAction* action);
    ~QGCToolBar();

public slots:
    /** @brief Set the system that is currently displayed by this widget */
    void setActiveUAS(UASInterface* active);
    /** @brief Set the system state */
    void updateState(UASInterface* system, QString name, QString description);
    /** @brief Set the system mode */
    void updateMode(int system, QString name, QString description);
    /** @brief Update the system name */
    void updateName(const QString& name);
    /** @brief Set the MAV system type */
    void setSystemType(UASInterface* uas, unsigned int systemType);

protected:
    void createCustomWidgets();

    QAction* toggleLoggingAction;
    QAction* logReplayAction;
    UASInterface* mav;
    QToolButton* symbolButton;
    QLabel* nameLabel;
    QLabel* modeLabel;
    QLabel* stateLabel;
    QLabel* wpLabel;
    QLabel* distlabel;

};

#endif // QGCTOOLBAR_H
