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
 *   @brief Debug console
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef DEBUGCONSOLE_H
#define DEBUGCONSOLE_H

#include <QtGui/QWidget>
#include <QList>
#include <QByteArray>
#include <QTimer>

#include "LinkInterface.h"

namespace Ui {
    class DebugConsole;
}

class DebugConsole : public QWidget {
    Q_OBJECT
public:
    DebugConsole(QWidget *parent = 0);
    ~DebugConsole();

public slots:
    /** @brief Add a link to the list of monitored links */
    void addLink(LinkInterface* link);
    /** @brief Update a link name */
    void updateLinkName(QString name);
    /** @brief Select a link for the active view */
    void linkSelected(int linkId);
    /** @brief Receive bytes from link */
    void receiveBytes(LinkInterface* link, QByteArray bytes);
    /** @brief Send lineedit content over link */
    void sendBytes();
    /** @brief Enable HEX display mode */
    void hexModeEnabled(bool mode);
    /** @brief Filter out MAVLINK raw data */
    void MAVLINKfilterEnabled(bool filter);
    /** @brief Freeze input, do not store new incoming data */
    void hold(bool hold);
    /** @brief Enable auto-freeze mode if traffic intensity is too high to display */
    void setAutoHold(bool hold);

    protected slots:
    /** @brief Draw information overlay */
    void paintEvent(QPaintEvent *event);
    /** @brief Update traffic measurements */
    void updateTrafficMeasurements();

protected:
    void changeEvent(QEvent *e);

    QList<LinkInterface*> links;
    LinkInterface* currLink;

    bool holdOn;              ///< Hold current view, ignore new data
    bool convertToAscii;      ///< Convert data to ASCII
    bool filterMAVLINK;       ///< Set true to filter out MAVLink in output
    int bytesToIgnore;        ///< Number of bytes to ignore
    char lastByte;            ///< The last received byte
    QList<QString> sentBytes; ///< Transmitted bytes, per transmission
    QByteArray holdBuffer;    ///< Buffer where bytes are stored during hold-enable
    QString lineBuffer;       ///< Buffere where bytes are stored before writing them out
    QTimer lineBufferTimer;   ///< Line buffer timer
    QTimer snapShotTimer;     ///< Timer for measuring traffic snapshots
    int snapShotInterval;     ///< Snapshot interval for traffic measurements
    int snapShotBytes;        ///< Number of bytes received in current snapshot
    float dataRate;           ///< Current data rate
    float lowpassDataRate;    ///< Lowpass filtered data rate
    float dataRateThreshold;  ///< Threshold where to enable auto-hold
    bool autoHold;            ///< Auto-hold mode sets view into hold if the data rate is too high

private:
    Ui::DebugConsole *m_ui;
};

#endif // DEBUGCONSOLE_H
