// Copyright (C) 2011-2012 Denis Shienkov <denis.shienkov@gmail.com>
// Copyright (C) 2011 Sergey Belyashov <Sergey.Belyashov@gmail.com>
// Copyright (C) 2012 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qserialportinfo.h"
#include "qserialportinfo_p.h"
#include "qserialport.h"
#include "qserialport_p.h"

QT_BEGIN_NAMESPACE

// We changed from QScopedPointer to std::unique_ptr, make sure it's
// binary compatible. The QScopedPointer had a non-default deleter, but
// the deleter just provides a static function to use for deletion so we don't
// include it in this template definition (the deleter-class was deleted).
static_assert(sizeof(std::unique_ptr<QSerialPortInfoPrivate>)
              == sizeof(QScopedPointer<QSerialPortInfoPrivate>));

/*!
    \class QSerialPortInfo

    \brief Provides information about existing serial ports.

    \ingroup serialport-main
    \inmodule QtSerialPort
    \since 5.1

    Use the static \l availablePorts() function to generate a list of
    QSerialPortInfo objects. Each QSerialPortInfo object in the list represents
    a single serial port and can be queried for the \l {portName}{port name},
    \l {systemLocation}{system location}, \l description, \l manufacturer, and
    some other hardware parameters. The QSerialPortInfo class can also be
    used as an input parameter for the \l {QSerialPort::}{setPort()} method of
    the QSerialPort class.

    \section1 Example Usage

    The example code enumerates all available serial ports and prints their
    parameters to console:

    \snippet doc_src_serialport.cpp enumerate_ports

    \sa QSerialPort
*/

/*!
    Constructs an empty QSerialPortInfo object.

    \sa isNull()
*/
QSerialPortInfo::QSerialPortInfo()
{
}

/*!
    Constructs a copy of \a other.
*/
QSerialPortInfo::QSerialPortInfo(const QSerialPortInfo &other)
    : d_ptr(other.d_ptr ? new QSerialPortInfoPrivate(*other.d_ptr) : nullptr)
{
}

/*!
    Constructs a QSerialPortInfo object from serial \a port.
*/
QSerialPortInfo::QSerialPortInfo(const QSerialPort &port)
    : QSerialPortInfo(port.portName())
{
}

/*!
    Constructs a QSerialPortInfo object from serial port \a name.

    This constructor finds the relevant serial port among the available ones
    according to the port name \a name, and constructs the serial port info
    instance for that port.
*/
QSerialPortInfo::QSerialPortInfo(const QString &name)
{
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        if (name == info.portName()) {
            *this = info;
            break;
        }
    }
}

QSerialPortInfo::QSerialPortInfo(const QSerialPortInfoPrivate &dd)
    : d_ptr(new QSerialPortInfoPrivate(dd))
{
}

/*!
    Destroys the QSerialPortInfo object. References to the values in the
    object become invalid.
*/
QSerialPortInfo::~QSerialPortInfo()
{
}

/*!
    Swaps QSerialPortInfo \a other with this QSerialPortInfo. This operation is
    very fast and never fails.
*/
void QSerialPortInfo::swap(QSerialPortInfo &other)
{
    d_ptr.swap(other.d_ptr);
}

/*!
    Sets the QSerialPortInfo object to be equal to \a other.
*/
QSerialPortInfo& QSerialPortInfo::operator=(const QSerialPortInfo &other)
{
    QSerialPortInfo(other).swap(*this);
    return *this;
}

/*!
    Returns the name of the serial port.

    \sa systemLocation()
*/
QString QSerialPortInfo::portName() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->portName;
}

/*!
    Returns the system location of the serial port.

    \sa portName()
*/
QString QSerialPortInfo::systemLocation() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->device;
}

/*!
    Returns the description string of the serial port,
    if available; otherwise returns an empty string.

    \sa manufacturer(), serialNumber()
*/
QString QSerialPortInfo::description() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->description;
}

/*!
    Returns the manufacturer string of the serial port,
    if available; otherwise returns an empty string.

    \sa description(), serialNumber()
*/
QString QSerialPortInfo::manufacturer() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->manufacturer;
}

/*!
    \since 5.3

    Returns the serial number string of the serial port,
    if available; otherwise returns an empty string.

    \note The serial number may include letters.

    \sa description(), manufacturer()
*/
QString QSerialPortInfo::serialNumber() const
{
    Q_D(const QSerialPortInfo);
    return !d ? QString() : d->serialNumber;
}

/*!
    Returns the 16-bit vendor number for the serial port, if available;
    otherwise returns zero.

    \sa hasVendorIdentifier(), productIdentifier(), hasProductIdentifier()
*/
quint16 QSerialPortInfo::vendorIdentifier() const
{
    Q_D(const QSerialPortInfo);
    return !d ? 0 : d->vendorIdentifier;
}

/*!
    Returns the 16-bit product number for the serial port, if available;
    otherwise returns zero.

    \sa hasProductIdentifier(), vendorIdentifier(), hasVendorIdentifier()
*/
quint16 QSerialPortInfo::productIdentifier() const
{
    Q_D(const QSerialPortInfo);
    return !d ? 0 : d->productIdentifier;
}

/*!
    Returns \c true if there is a valid \c 16-bit vendor number present; otherwise
    returns \c false.

    \sa vendorIdentifier(), productIdentifier(), hasProductIdentifier()
*/
bool QSerialPortInfo::hasVendorIdentifier() const
{
    Q_D(const QSerialPortInfo);
    return !d ? false : d->hasVendorIdentifier;
}

/*!
    Returns \c true if there is a valid \c 16-bit product number present; otherwise
    returns \c false.

    \sa productIdentifier(), vendorIdentifier(), hasVendorIdentifier()
*/
bool QSerialPortInfo::hasProductIdentifier() const
{
    Q_D(const QSerialPortInfo);
    return !d ? false : d->hasProductIdentifier;
}

/*!
    \fn bool QSerialPortInfo::isNull() const

    Returns whether this QSerialPortInfo object holds a
    serial port definition.
*/

/*!
    \fn QList<qint32> QSerialPortInfo::standardBaudRates()

    Returns a list of available standard baud rates supported
    by the target platform.
*/
QList<qint32> QSerialPortInfo::standardBaudRates()
{
    return QSerialPortPrivate::standardBaudRates();
}

/*!
    \fn QList<QSerialPortInfo> QSerialPortInfo::availablePorts()

    Returns a list of available serial ports on the system.
*/

QT_END_NAMESPACE
