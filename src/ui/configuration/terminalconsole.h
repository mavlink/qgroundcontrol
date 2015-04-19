/*=====================================================================

APM_PLANNER Open Source Ground Control Station

(c) 2013, Bill Bonney <billbonney@communistech.com>

This file is part of the APM_PLANNER project

    APM_PLANNER is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    APM_PLANNER is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with APM_PLANNER. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Terminal Console display View.
 *
 *   @author Bill Bonney <billbonney@communistech.com>
 *
 * Influenced from Qt examples by :-
 * Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
 * Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
 *
 */

#ifndef TERMINALCONSOLE_H
#define TERMINALCONSOLE_H

#include "SerialSettingsDialog.h"

#include <QWidget>
#ifdef __android__
#include "qserialport.h"
#else
#include <QSerialPort>
#endif

namespace Ui {
class TerminalConsole;
}

class Console;
class SettingsDialog;
class QStatusBar;
class QComboBox;

class TerminalConsole : public QWidget
{
    Q_OBJECT
    
public:
    enum ConsoleMode { APM, PX4 };
public:
    explicit TerminalConsole(QWidget *parent = 0);
    ~TerminalConsole();

private slots:
    void openSerialPort();
    void openSerialPort(const SerialSettings &settings);
    void closeSerialPort();
    void writeData(const QByteArray &data);
    void readData();
    void sendResetCommand();

    void handleError(QSerialPort::SerialPortError error);

private slots:
    void setBaudRate(int index);
    void setLink(int index);

private:
    void initConnections();
    void addBaudComboBoxConfig();
    void fillPortsInfo(QComboBox &comboxBox);
    void addConsoleModesComboBoxConfig();
    void writeSettings();
    void loadSettings();

    
private:
    Ui::TerminalConsole *ui;

    Console *m_console;
    QStatusBar *m_statusBar;
    SerialSettingsDialog *m_settingsDialog;
    QSerialPort *m_serial;
    SerialSettings m_settings;
    ConsoleMode m_consoleMode;
};

#endif // TERMINALCONSOLE_H
