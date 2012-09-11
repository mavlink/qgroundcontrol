/*=====================================================================
======================================================================*/
/**
 * @file
 *   @brief Cross-platform support for serial ports
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTimer>
#include <QDebug>
#include <QSettings>
#include <QMutexLocker>
#include "SerialLink.h"
#include "LinkManager.h"
#include "QGC.h"
#include <MG.h>
#include <iostream>
#ifdef _WIN32
#include "windows.h"
#endif

#ifdef _WIN32
#include <qextserialenumerator.h>
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

using namespace TNX;

SerialLink::SerialLink(QString portname, int baudRate, bool hardwareFlowControl, bool parity,
                       int dataBits, int stopBits) :
    port(NULL),
    ports(new QVector<QString>()),
    m_stopp(false)
{
    // Setup settings
    this->porthandle = portname.trimmed();

    if (this->porthandle == "" && getCurrentPorts()->size() > 0)
    {
        this->porthandle = getCurrentPorts()->first().trimmed();
    }

#ifdef _WIN32
    // Port names above 20 need the network path format - if the port name is not already in this format
    // catch this special case
    if (this->porthandle.size() > 0 && !this->porthandle.startsWith("\\")) {
        // Append \\.\ before the port handle. Additional backslashes are used for escaping.
        this->porthandle = "\\\\.\\" + this->porthandle;
    }
#endif
    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();

    setBaudRate(baudRate);
    if (hardwareFlowControl)
    {
        portSettings.setFlowControl(QPortSettings::FLOW_HARDWARE);
    }
    else
    {
        portSettings.setFlowControl(QPortSettings::FLOW_OFF);
    }
    if (parity)
    {
        portSettings.setParity(QPortSettings::PAR_EVEN);
    }
    else
    {
        portSettings.setParity(QPortSettings::PAR_NONE);
    }
    setDataBits(dataBits);
    setStopBits(stopBits);

    // Set the port name
    if (porthandle == "")
    {
        name = tr("Serial Link ") + QString::number(getId());
    }
    else
    {
        name = portname.trimmed();
    }
    loadSettings();
}

SerialLink::~SerialLink()
{
    disconnect();
    if(port) delete port;
    port = NULL;
    if (ports) delete ports;
    ports = NULL;
}

QVector<QString>* SerialLink::getCurrentPorts()
{
    ports->clear();
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
        if (fileInfo.fileName().contains(QString("ttyUSB")) || fileInfo.fileName().contains(QString("ttyS")) || fileInfo.fileName().contains(QString("ttyACM")))
        {
            ports->append(fileInfo.canonicalFilePath());
        }
    }
#endif

#if defined (__APPLE__) && defined (__MACH__)

    // Enumerate serial ports
    kern_return_t    kernResult; // on PowerPC this is an int (4 bytes)
    io_iterator_t    serialPortIterator;
    char        bsdPath[MAXPATHLEN];
    kernResult = FindModems(&serialPortIterator);
    kernResult = GetModemPath(serialPortIterator, bsdPath, sizeof(bsdPath));
    IOObjectRelease(serialPortIterator);    // Release the iterator.

    // Add found modems
    if (bsdPath[0])
    {
        ports->append(QString(bsdPath));
    }

    // Add USB serial port adapters
    // TODO Strangely usb serial port adapters are not enumerated, even when connected
    QString devdir = "/dev";
    QDir dir(devdir);
    dir.setFilter(QDir::System);

    QFileInfoList list = dir.entryInfoList();
    for (int i = list.size() - 1; i >= 0; i--) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.fileName().contains(QString("ttyUSB")) ||
                fileInfo.fileName().contains(QString("tty.")) ||
                fileInfo.fileName().contains(QString("ttyS")) ||
                fileInfo.fileName().contains(QString("ttyACM")))
        {
            ports->append(fileInfo.canonicalFilePath());
        }
    }
#endif

#ifdef _WIN32
    // Get the ports available on this system
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();

    // Add the ports in reverse order, because we prepend them to the list
    for (int i = ports.size() - 1; i >= 0; i--)
    {
        QextPortInfo portInfo = ports.at(i);
        QString portString = QString(portInfo.portName.toLocal8Bit().constData()) + " - " + QString(ports.at(i).friendName.toLocal8Bit().constData()).split("(").first();
        // Prepend newly found port to the list
        this->ports->append(portString);
    }

    //printf("port name: %s\n", ports.at(i).portName.toLocal8Bit().constData());
    //printf("friendly name: %s\n", ports.at(i).friendName.toLocal8Bit().constData());
    //printf("physical name: %s\n", ports.at(i).physName.toLocal8Bit().constData());
    //printf("enumerator name: %s\n", ports.at(i).enumName.toLocal8Bit().constData());
    //printf("===================================\n\n");
#endif
    return this->ports;
}

void SerialLink::loadSettings()
{
    // Load defaults from settings
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    settings.sync();
    if (settings.contains("SERIALLINK_COMM_PORT"))
    {
        setPortName(settings.value("SERIALLINK_COMM_PORT").toString());
        setBaudRateType(settings.value("SERIALLINK_COMM_BAUD").toInt());
        setParityType(settings.value("SERIALLINK_COMM_PARITY").toInt());
        setStopBits(settings.value("SERIALLINK_COMM_STOPBITS").toInt());
        setDataBits(settings.value("SERIALLINK_COMM_DATABITS").toInt());
        setFlowType(settings.value("SERIALLINK_COMM_FLOW_CONTROL").toInt());
    }
}

void SerialLink::writeSettings()
{
    // Store settings
    QSettings settings(QGC::COMPANYNAME, QGC::APPNAME);
    settings.setValue("SERIALLINK_COMM_PORT", getPortName());
    settings.setValue("SERIALLINK_COMM_BAUD", getBaudRateType());
    settings.setValue("SERIALLINK_COMM_PARITY", getParityType());
    settings.setValue("SERIALLINK_COMM_STOPBITS", getStopBits());
    settings.setValue("SERIALLINK_COMM_DATABITS", getDataBits());
    settings.setValue("SERIALLINK_COMM_FLOW_CONTROL", getFlowType());
    settings.sync();
}


/**
 * @brief Runs the thread
 *
 **/
void SerialLink::run()
{
    // Initialize the connection
    hardwareConnect();

    // Qt way to make clear what a while(1) loop does
    forever
    {
        {
            QMutexLocker locker(&this->m_stoppMutex);
            if(this->m_stopp)
            {
                this->m_stopp = false;
                break;
            }
        }
        // Check if new bytes have arrived, if yes, emit the notification signal
        checkForBytes();
        /* Serial data isn't arriving that fast normally, this saves the thread
                 * from consuming too much processing time
                 */
        MG::SLEEP::msleep(SerialLink::poll_interval);
    }
    if (port) {
        port->flushInBuffer();
        port->flushOutBuffer();
        port->close();
        delete port;
        port = NULL;
    }
}


void SerialLink::checkForBytes()
{
    /* Check if bytes are available */
    if(port && port->isOpen() && port->isWritable())
    {
        dataMutex.lock();
        qint64 available = port->bytesAvailable();
        dataMutex.unlock();

        if(available > 0)
        {
            readBytes();
        }
        else if (available < 0) {
            /* Error, close port */
            port->close();
            emit disconnected();
            emit connected(false);
            emit communicationError(this->getName(), tr("Could not send data - link %1 is disconnected!").arg(this->getName()));
        }
    }
//    else
//    {
//        emit disconnected();
//        emit connected(false);
//    }

}

void SerialLink::writeBytes(const char* data, qint64 size)
{
    if(port && port->isOpen()) {
        int b = port->write(data, size);

        if (b > 0) {

            //            qDebug() << "Serial link " << this->getName() << "transmitted" << b << "bytes:";

            // Increase write counter
            bitsSentTotal += size * 8;

            //            int i;
            //            for (i=0; i<size; i++)
            //            {
            //                unsigned char v =data[i];
            //                qDebug("%02x ", v);
            //            }
            //            qDebug("\n");
        } else {
            disconnect();
            // Error occured
            emit communicationError(this->getName(), tr("Could not send data - link %1 is disconnected!").arg(this->getName()));
        }
    }
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void SerialLink::readBytes()
{
    dataMutex.lock();
    if(port && port->isOpen()) {
        const qint64 maxLength = 2048;
        char data[maxLength];
        qint64 numBytes = port->bytesAvailable();
        //qDebug() << "numBytes: " << numBytes;

        if(numBytes > 0) {
            /* Read as much data in buffer as possible without overflow */
            if(maxLength < numBytes) numBytes = maxLength;

            port->read(data, numBytes);
            QByteArray b(data, numBytes);
            emit bytesReceived(this, b);

            //qDebug() << "SerialLink::readBytes()" << std::hex << data;
            //            int i;
            //            for (i=0; i<numBytes; i++){
            //                unsigned int v=data[i];
            //
            //                fprintf(stderr,"%02x ", v);
            //            }
            //            fprintf(stderr,"\n");
            bitsReceivedTotal += numBytes * 8;
        }
    }
    dataMutex.unlock();
}


/**
 * @brief Get the number of bytes to read.
 *
 * @return The number of bytes to read
 **/
qint64 SerialLink::bytesAvailable()
{
    if (port) {
        return port->bytesAvailable();
    } else {
        return 0;
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool SerialLink::disconnect()
{
    if(this->isRunning())
    {
        {
            QMutexLocker locker(&this->m_stoppMutex);
            this->m_stopp = true;
        }
        this->wait();

        //    if (port) {
        //#if !defined _WIN32 || !defined _WIN64
        /* Block the thread until it returns from run() */
        //#endif
        //        port->flushInBuffer();
        //        port->flushOutBuffer();
        //        port->close();
        //        delete port;
        //        port = NULL;

        //        if(this->isRunning()) this->terminate(); //stop running the thread, restart it upon connect

        bool closed = true;
        //port->isOpen();

        emit disconnected();
        emit connected(false);
        return closed;
    }
    else {
        // No port, so we're disconnected
        return true;
    }

}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool SerialLink::connect()
{
    if (this->isRunning()) this->disconnect();
    {
        QMutexLocker locker(&this->m_stoppMutex);
        this->m_stopp = false;
    }
    this->start(LowPriority);
    return true;
}

/**
 * @brief This function is called indirectly by the connect() call.
 *
 * The connect() function starts the thread and indirectly calls this method.
 *
 * @return True if the connection could be established, false otherwise
 * @see connect() For the right function to establish the connection.
 **/
bool SerialLink::hardwareConnect()
{
    if(port) {
        port->close();
        delete port;
    }
    port = new QSerialPort(porthandle, portSettings);
    QObject::connect(port,SIGNAL(aboutToClose()),this,SIGNAL(disconnected()));
    port->setCommTimeouts(QSerialPort::CtScheme_NonBlockingRead);
    connectionStartTime = MG::TIME::getGroundTimeNow();

    port->open();

    bool connectionUp = isConnected();
    if(connectionUp) {
        emit connected();
        emit connected(true);
    }

    //qDebug() << "CONNECTING LINK: " << __FILE__ << __LINE__ << "with settings" << port->portName() << getBaudRate() << getDataBits() << getParityType() << getStopBits();


    writeSettings();

    return connectionUp;
}


/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool SerialLink::isConnected()
{
    if (port) {
        return port->isOpen();
    } else {
        return false;
    }
}

int SerialLink::getId()
{
    return id;
}

QString SerialLink::getName()
{
    return name;
}

void SerialLink::setName(QString name)
{
    this->name = name;
    emit nameChanged(this->name);
}


/**
  * This function maps baud rate constants to numerical equivalents.
  * It relies on the mapping given in qportsettings.h from the QSerialPort library.
  */
qint64 SerialLink::getNominalDataRate()
{
    qint64 dataRate = 0;
    switch (portSettings.baudRate()) {

    // Baud rates supported only by POSIX systems
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
    case QPortSettings::BAUDR_50:
        dataRate = 50;
        break;
    case QPortSettings::BAUDR_75:
        dataRate = 75;
        break;
    case QPortSettings::BAUDR_134:
        dataRate = 134;
        break;
    case QPortSettings::BAUDR_150:
        dataRate = 150;
        break;
    case QPortSettings::BAUDR_200:
        dataRate = 200;
        break;
    case QPortSettings::BAUDR_1800:
        dataRate = 1800;
        break;
#endif

        // Baud rates supported only by Windows
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    case QPortSettings::BAUDR_14400:
        dataRate = 14400;
        break;
    case QPortSettings::BAUDR_56000:
        dataRate = 56000;
        break;
    case QPortSettings::BAUDR_128000:
        dataRate = 128000;
        break;
    case QPortSettings::BAUDR_256000:
        dataRate = 256000;
#endif

    case QPortSettings::BAUDR_110:
        dataRate = 110;
        break;
    case QPortSettings::BAUDR_300:
        dataRate = 300;
        break;
    case QPortSettings::BAUDR_600:
        dataRate = 600;
        break;
    case QPortSettings::BAUDR_1200:
        dataRate = 1200;
        break;
    case QPortSettings::BAUDR_2400:
        dataRate = 2400;
        break;
    case QPortSettings::BAUDR_4800:
        dataRate = 4800;
        break;
    case QPortSettings::BAUDR_9600:
        dataRate = 9600;
        break;
    case QPortSettings::BAUDR_19200:
        dataRate = 19200;
        break;
    case QPortSettings::BAUDR_38400:
        dataRate = 38400;
        break;
    case QPortSettings::BAUDR_57600:
        dataRate = 57600;
        break;
    case QPortSettings::BAUDR_115200:
        dataRate = 115200;
        break;
    case QPortSettings::BAUDR_230400:
        dataRate = 230400;
        break;
    case QPortSettings::BAUDR_460800:
        dataRate = 460800;
        break;
    case QPortSettings::BAUDR_500000:
        dataRate = 500000;
        break;
    case QPortSettings::BAUDR_576000:
        dataRate = 576000;
        break;
    case QPortSettings::BAUDR_921600:
        dataRate = 921600;
        break;

        // Otherwise do nothing.
    case QPortSettings::BAUDR_UNKNOWN:
    default:
        break;
    }
    return dataRate;
}

qint64 SerialLink::getTotalUpstream()
{
    statisticsMutex.lock();
    return bitsSentTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
}

qint64 SerialLink::getCurrentUpstream()
{
    return 0; // TODO
}

qint64 SerialLink::getMaxUpstream()
{
    return 0; // TODO
}

qint64 SerialLink::getBitsSent()
{
    return bitsSentTotal;
}

qint64 SerialLink::getBitsReceived()
{
    return bitsReceivedTotal;
}

qint64 SerialLink::getTotalDownstream()
{
    statisticsMutex.lock();
    return bitsReceivedTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
}

qint64 SerialLink::getCurrentDownstream()
{
    return 0; // TODO
}

qint64 SerialLink::getMaxDownstream()
{
    return 0; // TODO
}

bool SerialLink::isFullDuplex()
{
    /* Serial connections are always half duplex */
    return false;
}

int SerialLink::getLinkQuality()
{
    /* This feature is not supported with this interface */
    return -1;
}

QString SerialLink::getPortName()
{
    return porthandle;
}

int SerialLink::getBaudRate()
{
    return getNominalDataRate();
}

int SerialLink::getBaudRateType()
{
    return portSettings.baudRate();
}

int SerialLink::getFlowType()
{
    return portSettings.flowControl();
}

int SerialLink::getParityType()
{
    return portSettings.parity();
}

int SerialLink::getDataBitsType()
{
    return portSettings.dataBits();
}

int SerialLink::getStopBitsType()
{
    return portSettings.stopBits();
}

int SerialLink::getDataBits()
{
    int ret = -1;
    switch (portSettings.dataBits()) {
    case QPortSettings::DB_5:
        ret = 5;
        break;
    case QPortSettings::DB_6:
        ret = 6;
        break;
    case QPortSettings::DB_7:
        ret = 7;
        break;
    case QPortSettings::DB_8:
        ret = 8;
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

int SerialLink::getStopBits()
{
    int ret = -1;
    switch (portSettings.stopBits()) {
    case QPortSettings::STOP_1:
        ret = 1;
        break;
    case QPortSettings::STOP_2:
        ret = 2;
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

bool SerialLink::setPortName(QString portName)
{
    if(portName.trimmed().length() > 0) {
        bool reconnect = false;
        if (isConnected()) reconnect = true;
        disconnect();

        this->porthandle = portName.trimmed();
        setName(tr("serial port ") + portName.trimmed());
#ifdef _WIN32
        // Port names above 20 need the network path format - if the port name is not already in this format
        // catch this special case
        if (!this->porthandle.startsWith("\\")) {
            // Append \\.\ before the port handle. Additional backslashes are used for escaping.
            this->porthandle = "\\\\.\\" + this->porthandle;
        }
#endif

        if(reconnect) connect();
        return true;
    } else {
        return false;
    }
}


bool SerialLink::setBaudRateType(int rateIndex)
{
    bool reconnect = false;
    bool accepted = true; // This is changed if none of the data rates matches
    if(isConnected()) reconnect = true;
    disconnect();

    // These minimum and maximum baud rates were based on those enumerated in qportsettings.h.
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    const int minBaud = (int)QPortSettings::BAUDR_110;
    const int maxBaud = (int)QPortSettings::BAUDR_921600;
#elif defined(Q_OS_LINUX)
    const int minBaud = (int)QPortSettings::BAUDR_50;
    const int maxBaud = (int)QPortSettings::BAUDR_921600;
#elif defined(Q_OS_UNIX) || defined(Q_OS_DARWIN)
    const int minBaud = (int)QPortSettings::BAUDR_50;
    const int maxBaud = (int)QPortSettings::BAUDR_921600;
#endif

    if (rateIndex >= minBaud && rateIndex <= maxBaud)
    {
        portSettings.setBaudRate((QPortSettings::BaudRate)rateIndex);
    }

    if(reconnect) connect();
    return accepted;
}

bool SerialLink::setBaudRateString(const QString& rate)
{
    bool ok;
    int intrate = rate.toInt(&ok);
    if (!ok) return false;
    return setBaudRate(intrate);
}

bool SerialLink::setBaudRate(int rate)
{
    bool reconnect = false;
    bool accepted = true; // This is changed if none of the data rates matches
    if(isConnected()) {
        reconnect = true;
    }
    disconnect();

    // This switch-statment relies on the mapping given in qportsettings.h from the QSerialPort library.
    switch (rate) {

    // Baud rates supported only by non-Windows systems
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
    case 50:
        portSettings.setBaudRate(QPortSettings::BAUDR_50);
        break;
    case 75:
        portSettings.setBaudRate(QPortSettings::BAUDR_75);
        break;
    case 134:
        portSettings.setBaudRate(QPortSettings::BAUDR_134);
        break;
    case 150:
        portSettings.setBaudRate(QPortSettings::BAUDR_150);
        break;
    case 200:
        portSettings.setBaudRate(QPortSettings::BAUDR_200);
        break;
    case 1800:
        portSettings.setBaudRate(QPortSettings::BAUDR_1800);
        break;
#endif

        // Baud rates supported only by windows
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    case 14400:
        portSettings.setBaudRate(QPortSettings::BAUDR_14400);
        break;
    case 56000:
        portSettings.setBaudRate(QPortSettings::BAUDR_56000);
        break;
    case 128000:
        portSettings.setBaudRate(QPortSettings::BAUDR_128000);
        break;
    case 256000:
        portSettings.setBaudRate(QPortSettings::BAUDR_256000);
        break;
#endif

        // Supported by all OSes:
    case 110:
        portSettings.setBaudRate(QPortSettings::BAUDR_110);
        break;
    case 300:
        portSettings.setBaudRate(QPortSettings::BAUDR_300);
        break;
    case 600:
        portSettings.setBaudRate(QPortSettings::BAUDR_600);
        break;
    case 1200:
        portSettings.setBaudRate(QPortSettings::BAUDR_1200);
        break;
    case 2400:
        portSettings.setBaudRate(QPortSettings::BAUDR_2400);
        break;
    case 4800:
        portSettings.setBaudRate(QPortSettings::BAUDR_4800);
        break;
    case 9600:
        portSettings.setBaudRate(QPortSettings::BAUDR_9600);
        break;
    case 19200:
        portSettings.setBaudRate(QPortSettings::BAUDR_19200);
        break;
    case 38400:
        portSettings.setBaudRate(QPortSettings::BAUDR_38400);
        break;
    case 57600:
        portSettings.setBaudRate(QPortSettings::BAUDR_57600);
        break;
    case 115200:
        portSettings.setBaudRate(QPortSettings::BAUDR_115200);
        break;
    case 230400:
        portSettings.setBaudRate(QPortSettings::BAUDR_230400);
        break;
    case 460800:
        portSettings.setBaudRate(QPortSettings::BAUDR_460800);
        break;
    case 500000:
        portSettings.setBaudRate(QPortSettings::BAUDR_500000);
        break;
    case 576000:
        portSettings.setBaudRate(QPortSettings::BAUDR_576000);
        break;
    case 921600:
        portSettings.setBaudRate(QPortSettings::BAUDR_921600);
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();
    return accepted;

}

bool SerialLink::setFlowType(int flow)
{
    bool reconnect = false;
    bool accepted = true;
    if(isConnected()) reconnect = true;
    disconnect();

    switch (flow) {
    case (int)QPortSettings::FLOW_OFF:
        portSettings.setFlowControl(QPortSettings::FLOW_OFF);
        break;
    case (int)QPortSettings::FLOW_HARDWARE:
        portSettings.setFlowControl(QPortSettings::FLOW_HARDWARE);
        break;
    case (int)QPortSettings::FLOW_XONXOFF:
        portSettings.setFlowControl(QPortSettings::FLOW_XONXOFF);
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();
    return accepted;
}

bool SerialLink::setParityType(int parity)
{
    bool reconnect = false;
    bool accepted = true;
    if (isConnected()) reconnect = true;
    disconnect();

    switch (parity) {
    case (int)QPortSettings::PAR_NONE:
        portSettings.setParity(QPortSettings::PAR_NONE);
        break;
    case (int)QPortSettings::PAR_ODD:
        portSettings.setParity(QPortSettings::PAR_ODD);
        break;
    case (int)QPortSettings::PAR_EVEN:
        portSettings.setParity(QPortSettings::PAR_EVEN);
        break;
    case (int)QPortSettings::PAR_SPACE:
        portSettings.setParity(QPortSettings::PAR_SPACE);
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if (reconnect) connect();
    return accepted;
}


bool SerialLink::setDataBits(int dataBits)
{
    //qDebug() << "Setting" << dataBits << "data bits";
    bool reconnect = false;
    if (isConnected()) reconnect = true;
    bool accepted = true;
    disconnect();

    switch (dataBits) {
    case 5:
        portSettings.setDataBits(QPortSettings::DB_5);
        break;
    case 6:
        portSettings.setDataBits(QPortSettings::DB_6);
        break;
    case 7:
        portSettings.setDataBits(QPortSettings::DB_7);
        break;
    case 8:
        portSettings.setDataBits(QPortSettings::DB_8);
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();

    return accepted;
}

bool SerialLink::setStopBits(int stopBits)
{
    bool reconnect = false;
    bool accepted = true;
    if(isConnected()) reconnect = true;
    disconnect();

    switch (stopBits) {
    case 1:
        portSettings.setStopBits(QPortSettings::STOP_1);
        break;
    case 2:
        portSettings.setStopBits(QPortSettings::STOP_2);
        break;
    default:
        // If none of the above cases matches, there must be an error
        accepted = false;
        break;
    }

    if(reconnect) connect();
    return accepted;
}

bool SerialLink::setDataBitsType(int dataBits)
{
    bool reconnect = false;
    bool accepted = false;

    if (isConnected()) reconnect = true;
    disconnect();

    if (dataBits >= (int)QPortSettings::DB_5 && dataBits <= (int)QPortSettings::DB_8) {
        portSettings.setDataBits((QPortSettings::DataBits) dataBits);

        if(reconnect) connect();
        accepted = true;
    }

    return accepted;
}

bool SerialLink::setStopBitsType(int stopBits)
{
    bool reconnect = false;
    bool accepted = false;
    if(isConnected()) reconnect = true;
    disconnect();

    if (stopBits >= (int)QPortSettings::STOP_1 && stopBits <= (int)QPortSettings::STOP_2) {
        portSettings.setStopBits((QPortSettings::StopBits) stopBits);

        if(reconnect) connect();
        accepted = true;
    }

    if(reconnect) connect();
    return accepted;
}
