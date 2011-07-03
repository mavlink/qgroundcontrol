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
#ifdef _WIN32
#include <QextSerialEnumerator.h>
#endif
#if defined (__APPLE__) && defined (__MACH__)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <AvailabilityMacros.h>

#ifdef __MWERKS__
#define __CF_USE_FRAMEWORK_INCLUDES__
#endif


#include <CoreFoundation/CoreFoundation.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#if defined(MAC_OS_X_VERSION_10_3) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3)
#include <IOKit/serial/ioss.h>
#endif
#include <IOKit/IOBSD.h>

// Apple internal modems default to local echo being on. If your modem has local echo disabled,
// undefine the following macro.
#define LOCAL_ECHO

#define kATCommandString  "AT\r"

#ifdef LOCAL_ECHO
#define kOKResponseString  "AT\r\r\nOK\r\n"
#else
#define kOKResponseString  "\r\nOK\r\n"
#endif
#endif


// Some helper functions for serial port enumeration
#if defined (__APPLE__) && defined (__MACH__)

enum {
    kNumRetries = 3
};

// Function prototypes
static kern_return_t FindModems(io_iterator_t *matchingServices);
static kern_return_t GetModemPath(io_iterator_t serialPortIterator, char *bsdPath, CFIndex maxPathSize);

// Returns an iterator across all known modems. Caller is responsible for
// releasing the iterator when iteration is complete.
static kern_return_t FindModems(io_iterator_t *matchingServices)
{
    kern_return_t      kernResult;
    CFMutableDictionaryRef  classesToMatch;

    /*! @function IOServiceMatching
    @abstract Create a matching dictionary that specifies an IOService class match.
    @discussion A very common matching criteria for IOService is based on its class. IOServiceMatching will create a matching dictionary that specifies any IOService of a class, or its subclasses. The class is specified by C-string name.
    @param name The class name, as a const C-string. Class matching is successful on IOService's of this class or any subclass.
    @result The matching dictionary created, is returned on success, or zero on failure. The dictionary is commonly passed to IOServiceGetMatchingServices or IOServiceAddNotification which will consume a reference, otherwise it should be released with CFRelease by the caller. */

    // Serial devices are instances of class IOSerialBSDClient
    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL) {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    } else {
        /*!
          @function CFDictionarySetValue
          Sets the value of the key in the dictionary.
          @param theDict The dictionary to which the value is to be set. If this
            parameter is not a valid mutable CFDictionary, the behavior is
            undefined. If the dictionary is a fixed-capacity dictionary and
            it is full before this operation, and the key does not exist in
            the dictionary, the behavior is undefined.
          @param key The key of the value to set into the dictionary. If a key
            which matches this key is already present in the dictionary, only
            the value is changed ("add if absent, replace if present"). If
            no key matches the given key, the key-value pair is added to the
            dictionary. If added, the key is retained by the dictionary,
            using the retain callback provided
            when the dictionary was created. If the key is not of the sort
            expected by the key retain callback, the behavior is undefined.
          @param value The value to add to or replace into the dictionary. The value
            is retained by the dictionary using the retain callback provided
            when the dictionary was created, and the previous value if any is
            released. If the value is not of the sort expected by the
            retain or release callbacks, the behavior is undefined.
        */
        CFDictionarySetValue(classesToMatch,
                             CFSTR(kIOSerialBSDTypeKey),
                             CFSTR(kIOSerialBSDModemType));

        // Each serial device object has a property with key
        // kIOSerialBSDTypeKey and a value that is one of kIOSerialBSDAllTypes,
        // kIOSerialBSDModemType, or kIOSerialBSDRS232Type. You can experiment with the
        // matching by changing the last parameter in the above call to CFDictionarySetValue.

        // As shipped, this sample is only interested in modems,
        // so add this property to the CFDictionary we're matching on.
        // This will find devices that advertise themselves as modems,
        // such as built-in and USB modems. However, this match won't find serial modems.
    }

    /*! @function IOServiceGetMatchingServices
        @abstract Look up registered IOService objects that match a matching dictionary.
        @discussion This is the preferred method of finding IOService objects currently registered by IOKit. IOServiceAddNotification can also supply this information and install a notification of new IOServices. The matching information used in the matching dictionary may vary depending on the class of service being looked up.
        @param masterPort The master port obtained from IOMasterPort().
        @param matching A CF dictionary containing matching information, of which one reference is consumed by this function. IOKitLib can contruct matching dictionaries for common criteria with helper functions such as IOServiceMatching, IOOpenFirmwarePathMatching.
        @param existing An iterator handle is returned on success, and should be released by the caller when the iteration is finished.
        @result A kern_return_t error code. */

    kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, matchingServices);
    if (KERN_SUCCESS != kernResult) {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
        goto exit;
    }

exit:
    return kernResult;
}

/** Given an iterator across a set of modems, return the BSD path to the first one.
 *  If no modems are found the path name is set to an empty string.
 */
static kern_return_t GetModemPath(io_iterator_t serialPortIterator, char *bsdPath, CFIndex maxPathSize)
{
    io_object_t    modemService;
    kern_return_t  kernResult = KERN_FAILURE;
    Boolean      modemFound = false;

    // Initialize the returned path
    *bsdPath = '\0';

    // Iterate across all modems found. In this example, we bail after finding the first modem.

    while ((modemService = IOIteratorNext(serialPortIterator)) && !modemFound) {
        CFTypeRef  bsdPathAsCFString;

        // Get the callout device's path (/dev/cu.xxxxx). The callout device should almost always be
        // used: the dialin device (/dev/tty.xxxxx) would be used when monitoring a serial port for
        // incoming calls, e.g. a fax listener.

        bsdPathAsCFString = IORegistryEntryCreateCFProperty(modemService,
                            CFSTR(kIOCalloutDeviceKey),
                            kCFAllocatorDefault,
                            0);
        if (bsdPathAsCFString) {
            Boolean result;

            // Convert the path from a CFString to a C (NUL-terminated) string for use
            // with the POSIX open() call.

            result = CFStringGetCString((CFStringRef)bsdPathAsCFString,
                                        bsdPath,
                                        maxPathSize,
                                        kCFStringEncodingUTF8);
            CFRelease(bsdPathAsCFString);

            if (result) {
                //printf("Modem found with BSD path: %s", bsdPath);
                modemFound = true;
                kernResult = KERN_SUCCESS;
            }
        }

        printf("\n");

        // Release the io_service_t now that we are done with it.

        (void) IOObjectRelease(modemService);
    }

    return kernResult;
}
#endif

SerialConfigurationWindow::SerialConfigurationWindow(LinkInterface* link, QWidget *parent, Qt::WindowFlags flags) : QWidget(parent, flags),
    userConfigured(false)
{
    SerialLinkInterface* serialLink = dynamic_cast<SerialLinkInterface*>(link);

    if(serialLink != 0) {
        serialLink->loadSettings();
        this->link = serialLink;

        // Setup the user interface according to link type
        ui.setupUi(this);
        //this->setVisible(false);
        //this->hide();

        // Create action to open this menu
        // Create configuration action for this link
        // Connect the current UAS
        action = new QAction(QIcon(":/images/devices/network-wireless.svg"), "", link);
        setLinkName(link->getName());

        setupPortList();

        // Set up baud rates
        ui.baudRate->addItem("115200", 115200);

        ui.baudRate->clear();
        ui.baudRate->addItem("50", 50);
        ui.baudRate->addItem("70", 70);
        ui.baudRate->addItem("110", 110);
        ui.baudRate->addItem("134", 134);
        ui.baudRate->addItem("150", 150);
        ui.baudRate->addItem("200", 200);
        ui.baudRate->addItem("300", 300);
        ui.baudRate->addItem("600", 600);
        ui.baudRate->addItem("1200", 1200);
        ui.baudRate->addItem("1800", 1800);
        ui.baudRate->addItem("2400", 2400);
        ui.baudRate->addItem("4800", 4800);
        ui.baudRate->addItem("9600", 9600);
#ifdef Q_OS_WIN
        ui.baudRate->addItem("14400", 14400);
#endif
        ui.baudRate->addItem("19200", 19200);
        ui.baudRate->addItem("34800", 34800);
#ifdef Q_OS_WIN
        ui.baudRate->addItem("56000", 56000);
#endif
        ui.baudRate->addItem("57600", 57600);
#ifdef Q_OS_WIN
        ui.baudRate->addItem("76800", 76800);
#endif
        ui.baudRate->addItem("115200", 115200);
#ifdef Q_OS_WIN
        ui.baudRate->addItem("128000", 128000);
        ui.baudRate->addItem("230400", 230400);
        ui.baudRate->addItem("256000", 256000);
        ui.baudRate->addItem("460800", 460800);
#endif
        ui.baudRate->addItem("921600", 921600);

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
        //this->show();
    } else {
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
    //ui.portName->clear();
    // Get list of existing items


#ifdef __linux

    // TODO Linux has no standard way of enumerating serial ports
    // However the device files are only present when the physical
    // device is connected, therefore listing the files should be
    // sufficient.

    QString devdir = "/dev";
    QDir dir(devdir);
    dir.setFilter(QDir::System);

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.fileName().contains(QString("ttyUSB")) || fileInfo.fileName().contains(QString("ttyS")) || fileInfo.fileName().contains(QString("ttyACM"))) {
            if (ui.portName->findText(fileInfo.canonicalFilePath()) == -1) {
                ui.portName->addItem(fileInfo.canonicalFilePath());
                if (!userConfigured) ui.portName->setEditText(fileInfo.canonicalFilePath());
            }
        }
    }
#endif

#if defined (__APPLE__) && defined (__MACH__)

    // Enumerate serial ports
    //int            fileDescriptor;
    kern_return_t    kernResult; // on PowerPC this is an int (4 bytes)

    io_iterator_t    serialPortIterator;
    char        bsdPath[MAXPATHLEN];

    kernResult = FindModems(&serialPortIterator);

    kernResult = GetModemPath(serialPortIterator, bsdPath, sizeof(bsdPath));

    IOObjectRelease(serialPortIterator);    // Release the iterator.

    // Add found modems
    if (bsdPath[0]) {
        if (ui.portName->findText(QString(bsdPath)) == -1) {
            ui.portName->addItem(QString(bsdPath));
            if (!userConfigured) ui.portName->setEditText(QString(bsdPath));
        }
    }

    // Add USB serial port adapters
    // TODO Strangely usb serial port adapters are not enumerated, even when connected
    QString devdir = "/dev";
    QDir dir(devdir);
    dir.setFilter(QDir::System);

    QFileInfoList list = dir.entryInfoList();
    for (int i = list.size() - 1; i >= 0; i--) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.fileName().contains(QString("ttyUSB")) || fileInfo.fileName().contains(QString("ttyS")) || fileInfo.fileName().contains(QString("tty.usbserial")) || fileInfo.fileName().contains(QString("ttyACM"))) {
            if (ui.portName->findText(fileInfo.canonicalFilePath()) == -1) {
                ui.portName->addItem(fileInfo.canonicalFilePath());
                if (!userConfigured) ui.portName->setEditText(fileInfo.canonicalFilePath());
            }
        }
    }


#endif

#ifdef _WIN32
    // Get the ports available on this system
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

    // Add the ports in reverse order, because we prepend them to the list
    for (int i = ports.size() - 1; i >= 0; i--) {
        QString portString = QString(ports.at(i).portName.toLocal8Bit().constData()) + " - " + QString(ports.at(i).friendName.toLocal8Bit().constData()).split("(").first();
        // Prepend newly found port to the list
        if (ui.portName->findText(portString) == -1) {
            ui.portName->insertItem(0, portString);
            if (!userConfigured) ui.portName->setEditText(portString);
        }
    }

    //printf("port name: %s\n", ports.at(i).portName.toLocal8Bit().constData());
    //printf("friendly name: %s\n", ports.at(i).friendName.toLocal8Bit().constData());
    //printf("physical name: %s\n", ports.at(i).physName.toLocal8Bit().constData());
    //printf("enumerator name: %s\n", ports.at(i).enumName.toLocal8Bit().constData());
    //printf("===================================\n\n");
#endif

    if (this->link) {
        if (this->link->getPortName() != "") {
            ui.portName->setEditText(this->link->getPortName());
        }
    }
}

void SerialConfigurationWindow::enableFlowControl(bool flow)
{
    if(flow) {
        link->setFlowType(1);
    } else {
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

