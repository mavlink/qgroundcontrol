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
 *   @brief Definition of joystick widget
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef JOYSTICKWIDGET_H
#define JOYSTICKWIDGET_H

#include <QtGui/QDialog>
#include <QLabel>
#include "JoystickInput.h"
#include "MainWindow.h"

namespace Ui
{
class JoystickWidget;
}

class JoystickWidget : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(JoystickWidget)
public:
    explicit JoystickWidget(JoystickInput* joystick, QWidget *parent = 0);
    virtual ~JoystickWidget();

public slots:
    /**
     * @brief Receive raw joystick values
     *
     * @param roll forward / pitch / x axis, front: 32'767, center: 0, back: -32'768
     * @param pitch left / roll / y axis, left: -32'768, middle: 0, right: 32'767
     * @param yaw turn axis, left-turn: -32'768, centered: 0, right-turn: 32'767
     * @param thrust Thrust, 0%: 0, 100%: 65535
     * @param xHat hat vector in forward-backward direction, +1 forward, 0 center, -1 backward
     * @param yHat hat vector in left-right direction, -1 left, 0 center, +1 right
     */
    void updateJoystick(double roll, double pitch, double yaw, double thrust, int xHat, int yHat);
    /** @brief Throttle lever */
    void setThrottle(float thrust);
    /** @brief Back/forth movement */
    void setX(float x);
    /** @brief Left/right movement */
    void setY(float y);
    /** @brief Wrist rotation */
    void setZ(float z);
    /** @brief Hat switch position */
    void setHat(float x, float y);
    /** @brief Trigger a UI change based on a button being pressed */
    void joystickButtonPressed(int key);
    /** @brief Trigger a UI change based on a button being released */
    void joystickButtonReleased(int key);
    /** @brief Update status string */
    void updateStatus(const QString& status);
    /** @brief Update the UI color scheme when the MainWindow theme changes. */
    void styleChanged(MainWindow::QGC_MAINWINDOW_STYLE);

protected:
    /** @brief Update the proper number of buttons for the current joystick. */
    void updateButtons();
    /** @brief UI change event */
    virtual void changeEvent(QEvent *e);
    JoystickInput* joystick;  ///< Reference to the joystick
    /** @brief a list of all button labels generated for this joystick. */
    QList<QLabel*> buttonLabels;
    /** @brief The color to use for button labels when their corresponding button is pressed */
    QColor buttonLabelColor;

protected slots:
    /** @brief Update the UI for a new joystick based on SDL ID. */
    void updateUIForJoystick(int id);

private:
    Ui::JoystickWidget *m_ui;
    /** @brief Initialize all dynamic UI elements (button list, joystick names, etc.) */
    void initUI();
};

#endif // JOYSTICKWIDGET_H
