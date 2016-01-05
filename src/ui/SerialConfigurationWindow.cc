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
 *   @brief Implementation of SerialConfigurationWindow
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QDir>
#include <QSettings>
#include <QFileInfoList>
#include <QDebug>

#ifdef __android__
#include "qserialportinfo.h"
#else
#include <QSerialPortInfo>
#endif

#include <SerialConfigurationWindow.h>
#include <SerialLink.h>

#ifndef USE_ANCIENT_RATES
#define USE_ANCIENT_RATES 0
#endif

SerialConfigurationWindow::SerialConfigurationWindow(SerialConfiguration *config, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags)
{
    _ui.setupUi(this);
    Q_ASSERT(config != NULL);
    _config = config;

    // Scan for serial ports. Let the user know if none were found for debugging purposes
    if (!setupPortList()) {
        qDebug() << "No serial ports found.";
    }

    // Set up baud rates
    _ui.baudRate->clear();

    // Keep track of all desired baud rates by OS. These are iterated through
    // later and added to _ui.baudRate.
    QStringList supportedBaudRates = SerialConfiguration::supportedBaudRates();

    // Now actually add all of our supported baud rates to the ui.
    for (int i = 0; i < supportedBaudRates.size(); ++i) {
        _ui.baudRate->addItem(supportedBaudRates.at(i), supportedBaudRates.at(i).toInt());
    }

    // Connect the individual user interface inputs
    connect(_ui.portName,           static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,  &SerialConfigurationWindow::setPortName);
    connect(_ui.baudRate,           static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this,            &SerialConfigurationWindow::setBaudRate);
    connect(_ui.dataBitsSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,         &SerialConfigurationWindow::setDataBits);
    connect(_ui.stopBitsSpinBox,    static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,         &SerialConfigurationWindow::setStopBits);

    connect(_ui.flowControlCheckBox,&QCheckBox::toggled, this,             &SerialConfigurationWindow::enableFlowControl);
    connect(_ui.parNone,            &QRadioButton::toggled, this,             &SerialConfigurationWindow::setParityNone);
    connect(_ui.parOdd,             &QRadioButton::toggled, this,             &SerialConfigurationWindow::setParityOdd);
    connect(_ui.parEven,            &QRadioButton::toggled, this,             &SerialConfigurationWindow::setParityEven);
    connect(_ui.advCheckBox,        &QCheckBox::clicked, _ui.advGroupBox,  &QWidget::setVisible);

    _ui.advCheckBox->setCheckable(true);
    _ui.advCheckBox->setChecked(false);
    _ui.advGroupBox->setVisible(false);

    switch(_config->parity()) {
    case QSerialPort::NoParity:
        _ui.parNone->setChecked(true);
        break;
    case QSerialPort::OddParity:
        _ui.parOdd->setChecked(true);
        break;
    case QSerialPort::EvenParity:
        _ui.parEven->setChecked(true);
        break;
    default:
        // Enforce default: no parity in link
        setParityNone(true);
        _ui.parNone->setChecked(true);
        break;
    }

    int idx = 0;
    _ui.flowControlCheckBox->setChecked(_config->flowControl() == QSerialPort::HardwareControl);
    idx = _ui.baudRate->findText(QString("%1").arg(_config->baud()));
    if(idx < 0) idx = _ui.baudRate->findText("57600");
    if(idx < 0) idx = 0;
    _ui.baudRate->setCurrentIndex(idx);
    _ui.dataBitsSpinBox->setValue(_config->dataBits());
    _ui.stopBitsSpinBox->setValue(_config->stopBits());
    _portCheckTimer = new QTimer(this);
    _portCheckTimer->setInterval(1000);
    connect(_portCheckTimer, &QTimer::timeout, this, &SerialConfigurationWindow::setupPortList);

    // Display the widget
    setWindowTitle(tr("Serial Communication Settings"));
}

SerialConfigurationWindow::~SerialConfigurationWindow()
{

}

void SerialConfigurationWindow::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    _portCheckTimer->start();
}

void SerialConfigurationWindow::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    _portCheckTimer->stop();
}

bool SerialConfigurationWindow::setupPortList()
{
    bool changed = false;
    // Iterate found ports
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo &info, portList)
    {
        QString name = info.portName();
        // Append newly found port to the list
        if (_ui.portName->findText(name) < 0)
        {
            // We show the user the "short name" but store the full port name
            _ui.portName->addItem(name, QVariant(info.systemLocation()));
            changed = true;
        }
    }
    // See if configured port (if any) is present
    if(changed) {
        int idx = _ui.portName->count() - 1;
        if(!_config->portName().isEmpty()) {
            idx = _ui.portName->findData(QVariant(_config->portName()));
            if(idx < 0) {
                idx = 0;
            }
        }
        _ui.portName->setCurrentIndex(idx);
        if(_ui.portName->count() > 0) {
            _ui.portName->setEditText(_ui.portName->itemText(idx));
        }
        if(_config->portName().isEmpty()) {
            setPortName(idx);
        }
    }
    return (_ui.portName->count() > 0);
}

void SerialConfigurationWindow::enableFlowControl(bool flow)
{
    _config->setFlowControl(flow ? QSerialPort::HardwareControl : QSerialPort::NoFlowControl);
    //-- If this was dynamic, it's now edited and persistent
    _config->setDynamic(false);
}

void SerialConfigurationWindow::setParityNone(bool accept)
{
    if (accept) {
        _config->setParity(QSerialPort::NoParity);
        //-- If this was dynamic, it's now edited and persistent
        _config->setDynamic(false);
    }
}

void SerialConfigurationWindow::setParityOdd(bool accept)
{
    if (accept) {
        _config->setParity(QSerialPort::OddParity);
        //-- If this was dynamic, it's now edited and persistent
        _config->setDynamic(false);
    }
}

void SerialConfigurationWindow::setParityEven(bool accept)
{
    if (accept) {
        _config->setParity(QSerialPort::EvenParity);
        //-- If this was dynamic, it's now edited and persistent
        _config->setDynamic(false);
    }
}

void SerialConfigurationWindow::setPortName(int index)
{
    // Get the full port name and store it in the config
    QString pname = _ui.portName->itemData(index).toString();
    if (_config->portName() != pname) {
        _config->setPortName(pname);
        //-- If this was dynamic, it's now edited and persistent
        _config->setDynamic(false);
    }
}

void SerialConfigurationWindow::setBaudRate(int index)
{
    int baud = _ui.baudRate->itemData(index).toInt();
    _config->setBaud(baud);
    //-- If this was dynamic, it's now edited and persistent
    _config->setDynamic(false);
}

void SerialConfigurationWindow::setDataBits(int bits)
{
    _config->setDataBits(bits);
    //-- If this was dynamic, it's now edited and persistent
    _config->setDynamic(false);
}

void SerialConfigurationWindow::setStopBits(int bits)
{
    _config->setStopBits(bits);
    //-- If this was dynamic, it's now edited and persistent
    _config->setDynamic(false);
}
