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
#include <QKeyEvent>

#include "LinkInterface.h"

namespace Ui
{
class DebugConsole;
}

/**
 * @brief Shows a debug console
 *
 * This class shows the raw data stream of each link
 * and the debug / text messages sent by all systems
 */
class DebugConsole : public QWidget
{
    Q_OBJECT
public:
    DebugConsole(QWidget *parent = 0);
    ~DebugConsole();

public slots:
    /** @brief Add a link to the list of monitored links */
    void addLink(LinkInterface* link);
    /** @brief Remove a link from the list */
    void removeLink(LinkInterface* const link);
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
    /** @brief Set connection state of the current link */
    void setConnectionState(bool);
    /** @brief Handle the connect button */
    void handleConnectButton();
    /** @brief Enable auto-freeze mode if traffic intensity is too high to display */
    void setAutoHold(bool hold);
    /** @brief Receive plain text message to output to the user */
    void receiveTextMessage(int id, int component, int severity, QString text);
    /** @brief Append a special symbol */
    void appendSpecialSymbol(const QString& text);
    /** @brief Append the special symbol currently selected in combo box */
    void appendSpecialSymbol();
    /** @brief A new special symbol is selected */
    void specialSymbolSelected(const QString& text);

protected slots:
    /** @brief Draw information overlay */
    void paintEvent(QPaintEvent *event);
    /** @brief Update traffic measurements */
    void updateTrafficMeasurements();
    void loadSettings();
    void storeSettings();

protected:
    void changeEvent(QEvent *e);
    void hideEvent(QHideEvent* event);
    /** @brief Convert a symbol name to the byte representation */
    QByteArray symbolNameToBytes(const QString& symbol);
    /** @brief Convert a symbol byte to the name */
    QString bytesToSymbolNames(const QByteArray& b);
    /** @brief Handle keypress events */
    void keyPressEvent(QKeyEvent * event);
    /** @brief Cycle through the command history */
    void cycleCommandHistory(bool up);

    QList<LinkInterface*> links;
    LinkInterface* currLink;

    bool holdOn;              ///< Hold current view, ignore new data
    bool convertToAscii;      ///< Convert data to ASCII
    bool filterMAVLINK;       ///< Set true to filter out MAVLink in output
    bool autoHold;            ///< Auto-hold mode sets view into hold if the data rate is too high
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
    QStringList commandHistory;
    QString currCommand;
    int commandIndex;

private:
    Ui::DebugConsole *m_ui;
};

#endif // DEBUGCONSOLE_H
