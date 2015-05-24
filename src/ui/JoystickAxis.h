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

#ifndef JOYSTICKAXIS_H
#define JOYSTICKAXIS_H

#include <QWidget>
#include "JoystickInput.h"

namespace Ui {
class JoystickAxis;
}

class JoystickAxis : public QWidget
{
    Q_OBJECT
    
public:
    explicit JoystickAxis(int id, QWidget *parent = 0);
    ~JoystickAxis();
    void setMapping(JoystickInput::JOYSTICK_INPUT_MAPPING newMapping);
    void setInverted(bool newValue);
    void setRangeLimit(bool newValue);
    void setAxisRangeMin(float min);
    void setAxisRangeMax(float max);

signals:
    /** @brief Signal a change in this axis' yaw/pitch/roll mapping */
    void mappingChanged(int id, JoystickInput::JOYSTICK_INPUT_MAPPING newMapping);
    /** @brief Signal a change in this axis' inversion status */
    void inversionChanged(int id, bool inversion);
    /** @brief Signal a change in this axis' range setting. If limited is true then only the positive values should be read from this axis. */
    void rangeChanged(int id, bool limited);

public slots:
    /** @brief Update the displayed value of the included progressbar.
     * @param value A value between -1.0 and 1.0.
     */
    void setValue(float value);
    /** @brief Specify the UAS that this axis should track for displaying throttle properly. */
    void setActiveUAS(UASInterface* uas);
    
private:
    int id; ///< The ID for this axis. Corresponds to the IDs used by JoystickInput.
    Ui::JoystickAxis *ui;
    /**
     * @brief Update the UI based on both the current UAS and the current axis mapping.
     * @param uas The currently-active UAS.
     * @param axisMapping The new mapping for this axis.
     */
    void updateUIBasedOnUAS(UASInterface* uas, JoystickInput::JOYSTICK_INPUT_MAPPING axisMapping);

private slots:
    /** @brief Handle changes to the mapping dropdown bar. */
    void mappingComboBoxChanged(int newMapping);
    /** @brief Emit signal when the inversion checkbox is changed. */
    void inversionCheckBoxChanged(bool inverted);
    /** @brief Emit signal when the range checkbox is changed. */
    void rangeCheckBoxChanged(bool inverted);
};

#endif // JOYSTICKAXIS_H
