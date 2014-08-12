/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

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
 *   @brief Definition of joystick widget. Provides a UI for configuring the joystick settings.
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef JOYSTICKWIDGET_H
#define JOYSTICKWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QMap>
#include "JoystickInput.h"
#include "MainWindow.h"
#include "JoystickAxis.h"
#include "JoystickButton.h"

namespace Ui
{
class JoystickWidget;
}

class JoystickWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(JoystickWidget)
public:
    explicit JoystickWidget(JoystickInput* joystick, QWidget *parent = 0);
    virtual ~JoystickWidget();

public slots:
    /** @brief Update the UI for a new joystick based on SDL ID. */
    void createUIForJoystick();
    /**
     * @brief Update a given axis with a new value
     * @param axis The index of the axis to update.
     * @param value The new value for the axis, [-1.0:1.0].
     * @see JoystickInput:axisValueChanged
     */
    void updateAxisValue(int axis, float value);
    /** @brief Update the UI with new values for the hat.
     *  @see JoystickInput::hatDirectionChanged
     */
    void setHat(qint8 x, qint8 y);
    /** @brief Trigger a UI change based on a button being pressed */
    void joystickButtonPressed(int key);
    /** @brief Trigger a UI change based on a button being released */
    void joystickButtonReleased(int key);
    /** @brief Toggle the calibration button */
    void cycleCalibrationButton();
    /** @brief Update the UI color scheme when the MainWindow theme changes. */
    void styleChanged(bool styleIsDark);
    /** Update the UI assuming the joystick has stayed the same. */
    void updateUI();

protected:
    /** @brief UI change event */
    virtual void changeEvent(QEvent *e);
    JoystickInput* joystick;  ///< Reference to the joystick
    /** @brief a list of all button labels generated for this joystick. */
    QList<JoystickButton*> buttons;
    /** @brief a lit of all joystick axes generated for this joystick. */
    QList<JoystickAxis*> axes;
    /** @brief The color to use for button labels when their corresponding button is pressed */
    QColor buttonLabelColor;

private:
    Ui::JoystickWidget *m_ui;
    /** @brief Initialize all dynamic UI elements (button list, joystick names, etc.).
     * Only done once at startup.
     */
    void initUI();
};

#endif // JOYSTICKWIDGET_H
