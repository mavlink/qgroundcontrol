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
 *   @brief Definition of class AudioOutputWidget
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef AUDIOOUTPUTWIDGET_H
#define AUDIOOUTPUTWIDGET_H

#include <QtGui/QWidget>

namespace Ui
{
class AudioOutputWidget;
}

/**
 * @brief Widget to control audio/volume/mute
 */
class AudioOutputWidget : public QWidget
{
    Q_OBJECT
public:
    AudioOutputWidget(QWidget *parent = 0);
    ~AudioOutputWidget();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::AudioOutputWidget *m_ui;
};

#endif // AUDIOOUTPUTWIDGET_H
