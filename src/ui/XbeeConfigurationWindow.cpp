#include <QDebug>

#include <XbeeConfigurationWindow.h>
#include <XbeeLinkInterface.h>
#include <QDir>
#include <QSettings>
#include <QFileInfoList>
#include <qdatastream.h>

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

XbeeConfigurationWindow::XbeeConfigurationWindow(LinkInterface* link, QWidget *parent, Qt::WindowFlags flags): QWidget(parent, flags), 
	userConfigured(false)
{
	XbeeLinkInterface *xbeeLink = dynamic_cast<XbeeLinkInterface*>(link);

	if(xbeeLink != 0)
	{
		this->link = xbeeLink;

		action = new QAction(QIcon(":/images/devices/network-wireless.svg"), "", link);

		baudLabel = new QLabel;
		baudLabel->setText(tr("Baut Rate"));
		baudBox = new QComboBox;
		baudLabel->setBuddy(baudBox);
		portLabel = new QLabel;
		portLabel->setText(tr("SerialPort"));
		portBox = new QComboBox;
		portBox->setEditable(true);
		portLabel->setBuddy(portBox);
		highAddrLabel = new QLabel;
		highAddrLabel->setText(tr("Remote hex Address &High"));
		highAddr = new HexSpinBox(this);
		highAddrLabel->setBuddy(highAddr);
		lowAddrLabel = new QLabel;
		lowAddrLabel->setText(tr("Remote hex Address &Low"));
		lowAddr = new HexSpinBox(this);
		lowAddrLabel->setBuddy(lowAddr);
		actionLayout = new QGridLayout;
		actionLayout->addWidget(baudLabel,1,1);
		actionLayout->addWidget(baudBox,1,2);
		actionLayout->addWidget(portLabel,2,1);
		actionLayout->addWidget(portBox,2,2);
		actionLayout->addWidget(highAddrLabel,3,1);
		actionLayout->addWidget(highAddr,3,2);
		actionLayout->addWidget(lowAddrLabel,4,1);
		actionLayout->addWidget(lowAddr,4,2);
		tmpLayout = new QVBoxLayout;
		tmpLayout->addStretch();
		tmpLayout->addLayout(actionLayout);
		xbeeLayout = new QHBoxLayout;
		xbeeLayout->addStretch();
		xbeeLayout->addLayout(tmpLayout);
		this->setLayout(xbeeLayout);

		//connect(portBox,SIGNAL(activated(QString)),this,SLOT(setPortName(QString)));
		//connect(baudBox,SIGNAL(activated(QString)),this,SLOT(setBaudRateString(QString)));
		connect(portBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(setPortName(QString)));
		connect(portBox,SIGNAL(editTextChanged(QString)),this,SLOT(setPortName(QString)));
		connect(baudBox,SIGNAL(currentIndexChanged(QString)),this,SLOT(setBaudRateString(QString)));
		connect(highAddr,SIGNAL(valueChanged(int)),this,SLOT(addrChangedHigh(int)));
		connect(lowAddr,SIGNAL(valueChanged(int)),this,SLOT(addrChangedLow(int)));
		connect(this,SIGNAL(addrHighChanged(quint32)),xbeeLink,SLOT(setRemoteAddressHigh(quint32)));
		connect(this,SIGNAL(addrLowChanged(quint32)),xbeeLink,SLOT(setRemoteAddressLow(quint32)));

		baudBox->addItem("1200",1200);
		baudBox->addItem("2400",2400);
		baudBox->addItem("4800",4800);
		baudBox->addItem("9600",9600);
		baudBox->addItem("19200",19200);
		baudBox->addItem("38400",38400);
		baudBox->addItem("57600",57600);
		baudBox->setCurrentIndex(6);

		// try to open xbeeConf file for last remote address
		QFile in("Xbeeconf.txt");
		if(in.open(QIODevice::ReadOnly))
		{
			QDataStream inStr(&in);
			int tmpaddrHigh;
			int tmpaddrLow;
			inStr >> tmpaddrHigh;
			inStr >> tmpaddrLow;
			highAddr->setValue(tmpaddrHigh);
			lowAddr->setValue(tmpaddrLow);
		}
		else
		{
			highAddr->setValue(0x13A200);
			lowAddr->setValue(0x40DDDDDD);
		}



		this->setupPortList();

		portCheckTimer = new QTimer(this);
        portCheckTimer->setInterval(1000);
        connect(portCheckTimer, SIGNAL(timeout()), this, SLOT(setupPortList()));

        // Display the widget
        this->window()->setWindowTitle(tr("Xbee Communication Settings"));
	}
	else
	{
		qDebug() << "This is not a Xbee Link";
	}
}

XbeeConfigurationWindow::~XbeeConfigurationWindow()
{

}

void XbeeConfigurationWindow::setupPortList()
{
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
        if (fileInfo.fileName().contains(QString("ttyUSB")) || fileInfo.fileName().contains(QString("ttyS"))) {
            if (portBox->findText(fileInfo.canonicalFilePath()) == -1) {
                portBox->addItem(fileInfo.canonicalFilePath());
                if (!userConfigured) portBox->setEditText(fileInfo.canonicalFilePath());
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
        if (portBox->findText(QString(bsdPath)) == -1) {
            portBox->addItem(QString(bsdPath));
            if (!userConfigured) portBox->setEditText(QString(bsdPath));
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
        if (fileInfo.fileName().contains(QString("ttyUSB")) || fileInfo.fileName().contains(QString("ttyS")) || fileInfo.fileName().contains(QString("tty.usbserial"))) {
            if (portBox->findText(fileInfo.canonicalFilePath()) == -1) {
                portBox->addItem(fileInfo.canonicalFilePath());
                if (!userConfigured) portBox->setEditText(fileInfo.canonicalFilePath());
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
        if (portBox->findText(portString) == -1) {
            portBox->insertItem(0, portString);
            if (!userConfigured) portBox->setEditText(portString);
        }
    }

    //printf("port name: %s\n", ports.at(i).portName.toLocal8Bit().constData());
    //printf("friendly name: %s\n", ports.at(i).friendName.toLocal8Bit().constData());
    //printf("physical name: %s\n", ports.at(i).physName.toLocal8Bit().constData());
    //printf("enumerator name: %s\n", ports.at(i).enumName.toLocal8Bit().constData());
    //printf("===================================\n\n");
#endif
}

void XbeeConfigurationWindow::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    portCheckTimer->start();
}

void XbeeConfigurationWindow::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    portCheckTimer->stop();
}

QAction* XbeeConfigurationWindow::getAction()
{
    return action;
}

void XbeeConfigurationWindow::configureCommunication()
{
	this->setupPortList();
	this->show();
}

void XbeeConfigurationWindow::setPortName(QString port)
{
	link->setPortName(port);
}

void XbeeConfigurationWindow::setBaudRateString(QString baud)
{
	int rate = baud.toInt();
	this->link->setBaudRate(rate);
}

void XbeeConfigurationWindow::addrChangedHigh(int addr)
{
	quint32 uaddr = static_cast<quint32>(addr);
	QFile out("Xbeeconf.txt");
	if(out.open(QIODevice::WriteOnly))
	{
		QDataStream outStr(&out);
		outStr << this->highAddr->value();
		outStr << this->lowAddr->value();
	}
	emit addrHighChanged(uaddr);
}

void XbeeConfigurationWindow::addrChangedLow(int addr)
{
	quint32 uaddr = static_cast<quint32>(addr);
	QFile out("Xbeeconf.txt");
	if(out.open(QIODevice::WriteOnly))
	{
		QDataStream outStr(&out);
		outStr << this->highAddr->value();
		outStr << this->lowAddr->value();
	}
	emit addrLowChanged(uaddr);
}