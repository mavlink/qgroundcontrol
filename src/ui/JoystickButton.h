/*=====================================================================

QGroundControl Open Source Ground Control Station

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
 * This class defines a UI element to represent a single controller axis.
 * It is used by the JoystickWidget to simplify some of the logic in that class.
 */

#ifndef JOYSTICKBUTTON_H
#define JOYSTICKBUTTON_H

#include <QWidget>

#include "Vehicle.h"

namespace Ui
{
class JoystickButton;
}

class JoystickButton : public QWidget
{
    Q_OBJECT

public:
    explicit JoystickButton(int id, QWidget *parent = 0);
    virtual ~JoystickButton();

public slots:
    /** @brief Specify the UAS that this axis should track for displaying throttle properly. */
    void activeVehicleChanged(Vehicle* vehicle);
    /** @brieft Specify which action this button should correspond to.
     * Values 0 and higher indicate a specific action, while -1 indicates no action. */
    void setAction(int index);

signals:
    /** @brief Signal a change in this buttons' action mapping */
    void actionChanged(int id, int index);

private:
    int id;
    Ui::JoystickButton *m_ui;

private slots:
    void actionComboBoxChanged(int index);
};
#endif // JOYSTICKBUTTON_H
