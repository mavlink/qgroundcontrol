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

#include <QDebug>

#include <SerialConfigurationWindow.h>
#include <SerialLinkInterface.h>
#include <QDir>
#include <QSettings>
#include <QFileInfoList>

SerialConfigurationWindow::SerialConfigurationWindow(LinkInterface* link, QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags),
    userConfigured(false)
{
    SerialLinkInterface* serialLink = dynamic_cast<SerialLinkInterface*>(link);

    if(serialLink != 0)
    {
        serialLink->loadSettings();
        this->link = serialLink;

        // Setup the user interface according to link type
        ui.setupUi(this);

        // Create action to open this menu
        // Create configuration action for this link
        // Connect the current UAS
        action = new QAction(QIcon(":/files/images/devices/network-wireless.svg"), "", link);
        setLinkName(link->getName());

        setupPortList();

        // Set up baud rates
        ui.baudRate->clear();
		
		// Keep track of all desired baud rates by OS. These are iterated through
		// later and added to ui.baudRate.
		QList<int> supportedBaudRates;

		// Baud rates supported only by POSIX systems
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
		supportedBaudRates << 50;
		supportedBaudRates << 75;
		supportedBaudRates << 134;
		supportedBaudRates << 150;
		supportedBaudRates << 200;
		supportedBaudRates << 1800;
#endif

		// Baud rates supported only by Windows
#if defined(Q_OS_WIN)
		supportedBaudRates << 14400;
		supportedBaudRates << 56000;
		supportedBaudRates << 128000;
		supportedBaudRates << 256000;
#endif

		// Baud rates supported by everyone
		supportedBaudRates << 110;
		supportedBaudRates << 300;
		supportedBaudRates << 600;
		supportedBaudRates << 1200;
		supportedBaudRates << 2400;
		supportedBaudRates << 4800;
		supportedBaudRates << 9600;
		supportedBaudRates << 19200;
		supportedBaudRates << 38400;
		supportedBaudRates << 57600;
		supportedBaudRates << 115200;
        supportedBaudRates << 230400;
        supportedBaudRates << 460800;

#if defined(Q_OS_LINUX)
        // Baud rates supported only by Linux
        supportedBaudRates << 500000;
        supportedBaudRates << 576000;
#endif

        supportedBaudRates << 921600;
		
		// Now actually add all of our supported baud rates to the UI.
		qSort(supportedBaudRates.begin(), supportedBaudRates.end());
		for (int i = 0; i < supportedBaudRates.size(); ++i) {
			ui.baudRate->addItem(QString::number(supportedBaudRates.at(i)), supportedBaudRates.at(i));
		}

        // Load current link config
        ui.portName->setCurrentIndex(ui.baudRate->findText(QString("%1").arg(this->link->getPortName())));

        connect(action, SIGNAL(triggered()), this, SLOT(configureCommunication()));

        // Make sure that a change in the link name will be reflected in the UI
        connect(link, SIGNAL(nameChanged(QString)), this, SLOT(setLinkName(QString)));

        // Connect the individual user interface inputs
        connect(ui.portName, SIGNAL(editTextChanged(QString)), this, SLOT(setPortName(QString)));
        connect(ui.portName, SIGNAL(currentIndexChanged(QString)), this, SLOT(setPortName(QString)));
        connect(ui.baudRate, SIGNAL(activated(QString)), this->link, SLOT(setBaudRateString(QString)));
        connect(ui.flowControlCheckBox, SIGNAL(toggled(bool)), this, SLOT(enableFlowControl(bool)));
        connect(ui.parNone, SIGNAL(toggled(bool)), this, SLOT(setParityNone(bool)));
        connect(ui.parOdd, SIGNAL(toggled(bool)), this, SLOT(setParityOdd(bool)));
        connect(ui.parEven, SIGNAL(toggled(bool)), this, SLOT(setParityEven(bool)));
        connect(ui.dataBitsSpinBox, SIGNAL(valueChanged(int)), this->link, SLOT(setDataBits(int)));
        connect(ui.stopBitsSpinBox, SIGNAL(valueChanged(int)), this->link, SLOT(setStopBits(int)));

        //connect(this->link, SIGNAL(connected(bool)), this, SLOT());
        ui.portName->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
        ui.baudRate->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);

        switch(this->link->getParityType()) {
        case 0:
            ui.parNone->setChecked(true);
            break;
        case 1:
            ui.parOdd->setChecked(true);
            break;
        case 2:
            ui.parEven->setChecked(true);
            break;
        default:
            // Enforce default: no parity in link
            setParityNone(true);
            ui.parNone->setChecked(true);
            break;
        }

        switch(this->link->getFlowType()) {
        case 0:
            ui.flowControlCheckBox->setChecked(false);
            break;
        case 1:
            ui.flowControlCheckBox->setChecked(true);
            break;
        default:
            ui.flowControlCheckBox->setChecked(false);
            enableFlowControl(false);
        }

        ui.baudRate->setCurrentIndex(ui.baudRate->findText(QString("%1").arg(this->link->getBaudRate())));

        ui.dataBitsSpinBox->setValue(this->link->getDataBits());
        ui.stopBitsSpinBox->setValue(this->link->getStopBits());

        portCheckTimer = new QTimer(this);
        portCheckTimer->setInterval(1000);
        connect(portCheckTimer, SIGNAL(timeout()), this, SLOT(setupPortList()));

        // Display the widget
        this->window()->setWindowTitle(tr("Serial Communication Settings"));
    }
    else
    {
        qDebug() << "Link is NOT a serial link, can't open configuration window";
    }
}

SerialConfigurationWindow::~SerialConfigurationWindow()
{

}

void SerialConfigurationWindow::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    portCheckTimer->start();
}

void SerialConfigurationWindow::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    portCheckTimer->stop();
}

QAction* SerialConfigurationWindow::getAction()
{
    return action;
}

void SerialConfigurationWindow::configureCommunication()
{
    QString selected = ui.portName->currentText();
    setupPortList();
    ui.portName->setEditText(selected);
    this->show();
}

void SerialConfigurationWindow::setupPortList()
{
    if (!link) return;

    // Get the ports available on this system
    QVector<QString>* ports = link->getCurrentPorts();

    // Add the ports in reverse order, because we prepend them to the list
    for (int i = ports->size() - 1; i >= 0; --i)
    {
        // Prepend newly found port to the list
        if (ui.portName->findText(ports->at(i)) == -1)
        {
            ui.portName->insertItem(0, ports->at(i));
            if (!userConfigured) ui.portName->setEditText(ports->at(i));
        }
    }

    ui.portName->setEditText(this->link->getPortName());
}

void SerialConfigurationWindow::enableFlowControl(bool flow)
{
    if(flow)
    {
        link->setFlowType(1);
    }
    else
    {
        link->setFlowType(0);
    }
}

void SerialConfigurationWindow::setParityNone(bool accept)
{
    if (accept) link->setParityType(0);
}

void SerialConfigurationWindow::setParityOdd(bool accept)
{
    if (accept) link->setParityType(1);
}

void SerialConfigurationWindow::setParityEven(bool accept)
{
    if (accept) link->setParityType(2);
}

void SerialConfigurationWindow::setPortName(QString port)
{
#ifdef Q_OS_WIN
    port = port.split("-").first();
#endif
    port = port.remove(" ");

    if (this->link->getPortName() != port) {
        link->setPortName(port);
    }
    userConfigured = true;
}

void SerialConfigurationWindow::setLinkName(QString name)
{
    Q_UNUSED(name);
    // FIXME
    action->setText(tr("Configure ") + link->getName());
    action->setStatusTip(tr("Configure ") + link->getName());
    setWindowTitle(tr("Configuration of ") + link->getName());
}

