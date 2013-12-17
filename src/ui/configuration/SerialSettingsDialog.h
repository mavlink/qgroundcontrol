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
 *   @brief Serial Settings View.
 *
 *   @author Bill Bonney <billbonney@communistech.com>
 *
 * Influenced from Qt examples by :-
 * Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
 * Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
 *
 */

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <qserialport.h>

namespace Ui {
class SettingsDialog;
}

class QIntValidator;

class SerialSettings {
public:
    SerialSettings() : name(""),
        baudRate(115200),
        dataBits(QSerialPort::Data8),
        parity(QSerialPort::NoParity),
        stopBits(QSerialPort::OneStop),
        flowControl(QSerialPort::NoFlowControl){}
public:
    QString name;
    qint32 baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;
    QSerialPort::FlowControl flowControl;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

    const SerialSettings &settings() const;

private slots:
    void showPortInfo(int idx);
    void apply();
    void checkCustomBaudRatePolicy(int idx);

private:
    void fillPortsParameters();
    void fillPortsInfo();
    void updateSettings();

private:
    Ui::SettingsDialog *ui;
    SerialSettings m_currentSettings;
    QIntValidator *m_intValidator;
};

#endif // SETTINGSDIALOG_H
