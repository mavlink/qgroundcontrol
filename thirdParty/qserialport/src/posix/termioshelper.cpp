/*
 * Unofficial Qt Serial Port Library
 *
 * Copyright (c) 2010 Inbiza Systems Inc. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *
 * @file termioshelper.cpp
 */

#include <QDebug>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "termioshelper.h"

namespace TNX {

/*!
  Constructs a TermiosHelper object with the given \a file descriptor.
*/

TermiosHelper::TermiosHelper(int fileDescriptor)
    : fileDescriptor_(fileDescriptor), originalAttrs_(NULL), currentAttrs_(NULL)
{
    Q_ASSERT(fileDescriptor_ > 0);

    originalAttrs_ = new termios();
    currentAttrs_ = new termios();

    // save the current serial port attributes
    // see restoreTermios()

    saveTermios();

    // clone the original attributes

    *currentAttrs_ = *originalAttrs_;

    // initialize port attributes for serial port communication

    initTermios();
}


TermiosHelper::~TermiosHelper()
{
    // It is good practice to reset a serial port back to the state in
    // which you found it. This is why we saved the original termios struct
    // The constant TCSANOW (defined in termios.h) indicates that
    // the change should take effect immediately.

    restoreTermios();

    delete originalAttrs_;
    delete currentAttrs_;
}

/*!
   Sets the termios structure.
 */

bool TermiosHelper::applyChanges(ChangeApplyTypes /*apptype*/)
{
    // there is only termios structure to be applied for posix compatible platforms

    if ( tcsetattr(fileDescriptor_, TCSANOW, currentAttrs_) == -1 ) {
        qCritical() << QString("TermiosHelper::applyChanges(file: %1, applyType: %2) failed " \
                               "when setting new attributes: %3(%4)")
                       .arg(fileDescriptor_)
                       .arg(TCSANOW)
                       .arg(strerror(errno))
                       .arg(errno);
        return false;
    }
    return true;
}

/*!

 */

bool TermiosHelper::setCtrSignal(ControlSignals csig, bool value)
{
    int status;

    if ( ioctl(fileDescriptor_, TIOCMGET, &status) == -1 ) {
        qCritical() <<  QString("TermiosHelper::setCtrSignal(file: %1, csig: %2) failed" \
                                "when fetching control signal values : %3(%4)")
                        .arg(fileDescriptor_)
                        .arg(csig)
                        .arg(strerror(errno))
                        .arg(errno);
        return false;
    }

    if ( value )
        status |= (int)csig;
    else
        status &= ~((int)csig);

    if ( ioctl(fileDescriptor_, TIOCMSET, &status) == -1 ) {
        qCritical() <<  QString("TermiosHelper::setCtrSignal(file: %1, csig: %2) failed" \
                                "when setting control signal values : %3(%4)")
                        .arg(fileDescriptor_)
                        .arg(csig)
                        .arg(strerror(errno))
                        .arg(errno);
        return false;
    }
    return true;
}

/*!

 */

QSerialPort::CommSignalValues TermiosHelper::ctrSignal(ControlSignals csig) const
{
    int status;

    if ( ioctl(fileDescriptor_, TIOCMGET, &status) == -1 ) {
        qCritical() <<  QString("TermiosHelper::ctrSignal(file: %1, csig: %2) failed" \
                                "when fetching control signal values : %3(%4)")
                        .arg(fileDescriptor_)
                        .arg(csig)
                        .arg(strerror(errno))
                        .arg(errno);
        return QSerialPort::Signal_Unknown;
    }

    return (status & ((int)csig)) ? QSerialPort::Signal_On : QSerialPort::Signal_Off;
}

/*!

 */

void TermiosHelper::initTermios()
{
    // Set raw input (non-canonical) mode, with reads blocking until either a single character
    // has been received or a one second timeout expires.
    // See tcsetattr(4) ("man 4 tcsetattr") and termios(4) ("man 4 termios") for details.

    cfmakeraw(currentAttrs_);

    currentAttrs_->c_cflag |= (CLOCAL | CREAD);

    // set communication timeouts

    currentAttrs_->c_cc[VMIN] = kDefaultNumOfBytes;

    // converting ms to one-tenths-of-second (sec * 0.1)
    currentAttrs_->c_cc[VTIME] = (kDefaultReadTimeout / 100);

    // ensure the new attributes take effect immediately

    applyChanges();
}

/*!

*/

void TermiosHelper::saveTermios()
{
    // get the current serial port attributes

    if ( tcgetattr(fileDescriptor_, originalAttrs_) == -1 ) {
        qWarning() << QString("TermiosHelper::saveTermios(file: %1) failed when" \
                              " getting original port attributes: %2(%3)")
                      .arg(fileDescriptor_)
                      .arg(strerror(errno))
                      .arg(errno);
    }
}

/*!

*/

void TermiosHelper::restoreTermios()
{
    if ( !originalAttrs_ || tcsetattr(fileDescriptor_, TCSANOW, originalAttrs_) == -1 ) {
        qWarning() << QString("TermiosHelper::restoreTermios(file: %1) failed when resetting " \
                              "serial port attributes: %2(%3)")
                      .arg(fileDescriptor_)
                      .arg(strerror(errno))
                      .arg(errno);
    }

    // Block until all written output has been sent from the device.
    // Note that this call is simply passed on to the serial device driver.
    // See tcsendbreak(3) ("man 3 tcsendbreak") for details.

    if ( tcdrain(fileDescriptor_) == -1 ) {
        qWarning() << QString("TermiosHelper::restoreTermios(file: %1) failed while waiting for drain: %2(%3)")
                      .arg(fileDescriptor_)
                      .arg(strerror(errno))
                      .arg(errno);
    }
}

/*!

*/

void TermiosHelper::setBaudRate(QPortSettings::BaudRate baudRate)
{
    speed_t baud = B9600;

    switch ( baudRate ) {
    case QPortSettings::BAUDR_50:
        baud = B50;
        break;
    case QPortSettings::BAUDR_75:
        baud = B75;
        break;
    case QPortSettings::BAUDR_110:
        baud = B110;
        break;
    case QPortSettings::BAUDR_134:
        baud = B134;
        break;
    case QPortSettings::BAUDR_150:
        baud = B150;
        break;
    case QPortSettings::BAUDR_200:
        baud = B200;
        break;
    case QPortSettings::BAUDR_300:
        baud = B300;
        break;
    case QPortSettings::BAUDR_600:
        baud = B600;
        break;
    case QPortSettings::BAUDR_1200:
        baud = B1200;
        break;
    case QPortSettings::BAUDR_1800:
        baud = B1800;
        break;
    case QPortSettings::BAUDR_2400:
        baud = B2400;
        break;
    case QPortSettings::BAUDR_4800:
        baud = B4800;
        break;
    case QPortSettings::BAUDR_9600:
        baud = B9600;
        break;
    case QPortSettings::BAUDR_19200:
        baud = B19200;
        break;
    case QPortSettings::BAUDR_38400:
        baud = B38400;
        break;
    case QPortSettings::BAUDR_57600:
        baud = B57600;
        break;
        //case QPortSettings::BAUDR_76800:
        //  baud = B76800;
        //  break;
    case QPortSettings::BAUDR_115200:
        baud = B115200;
        break;
    case QPortSettings::BAUDR_230400:
#ifdef B230400
        baud = B230400;
#else
        baud = (speed_t)230400;
#endif
        break;
    case QPortSettings::BAUDR_460800:
#ifdef B460800
        baud = B460800;
#else
        baud = (speed_t)460800;
#endif
        break;
    case QPortSettings::BAUDR_500000:
#ifdef B500000
        baud = B500000;
#else
        baud = (speed_t)500000;
#endif
        break;
    case QPortSettings::BAUDR_576000:
#ifdef B576000
        baud = B576000;
#else
        baud = (speed_t)576000;
#endif
        break;
    case QPortSettings::BAUDR_921600:
#ifdef B921600
        baud = B921600;
#else
        baud = (speed_t)921600;
#endif
        break;
    default:
        qWarning() << "TermiosHelper::setBaudRate(" << baudRate << "): " \
                      "Unsupported baud rate";
    }

    //#ifdef Q_OS_MAC
    //  if ( ioctl( fileDescriptor_, IOSSIOSPEED, &baud ) == -1 )
    //          {
    //      qCritical() <<  QString("TermiosHelper::setBaudRate(file: %1) failed: %2(%3)")
    //                   .arg(fileDescriptor_)
    //                   .arg(strerror(errno))
    //                   .arg(errno);
    //              return false;
    //          }
    //#else

    qCritical() << "Baud rate is now: " << baud;

    if ( cfsetspeed(currentAttrs_, baud) == -1 ) {
        qCritical() <<  QString("TermiosHelper::setBaudRate(file: %1) failed: %2(%3)")
                        .arg(fileDescriptor_)
                        .arg(strerror(errno))
                        .arg(errno);
    }
    //#endif
}

/*!

*/

QPortSettings::BaudRate TermiosHelper::baudRate() const
{
    speed_t ibaud = cfgetispeed(currentAttrs_);
    speed_t obaud = cfgetospeed(currentAttrs_);

    (obaud == ibaud);

    Q_ASSERT(ibaud == obaud);

    switch ( ibaud ) {
    case B50:
        return QPortSettings::BAUDR_50;
    case B75:
        return QPortSettings::BAUDR_75;
    case B110:
        return QPortSettings::BAUDR_110;
    case B134:
        return QPortSettings::BAUDR_134;
    case B150:
        return QPortSettings::BAUDR_150;
    case B200:
        return QPortSettings::BAUDR_200;
    case B300:
        return QPortSettings::BAUDR_300;
    case B600:
        return QPortSettings::BAUDR_600;
    case B1200:
        return QPortSettings::BAUDR_1200;
    case B1800:
        return QPortSettings::BAUDR_1800;
    case B2400:
        return QPortSettings::BAUDR_2400;
    case B4800:
        return QPortSettings::BAUDR_4800;
    case B9600:
        return QPortSettings::BAUDR_9600;
    case B19200:
        return QPortSettings::BAUDR_19200;
    case B38400:
        return QPortSettings::BAUDR_38400;
    case B57600:
        return QPortSettings::BAUDR_57600;
        //case B76800:
        //  return QPortSettings::BAUDR_76800;
    case B115200:
        return QPortSettings::BAUDR_115200;
#ifdef B230400
    case B230400:
        return QPortSettings::BAUDR_230400;
#else
    case 230400:
        return QPortSettings::BAUDR_230400;
#endif
#ifdef B460800
    case B460800:
        return QPortSettings::BAUDR_460800;
#else
    case 460800:
        return QPortSettings::BAUDR_460800;
#endif
#ifdef B500000
    case B500000:
        return QPortSettings::BAUDR_500000;
#else
    case 500000:
        return QPortSettings::BAUDR_500000;
#endif
#ifdef B576000:
    case B576000:
        return QPortSettings::BAUDR_576000;
#else
    case 576000:
        return QPortSettings::BAUDR_576000;
#endif
#ifdef B921600
    case B921600:
        return QPortSettings::BAUDR_921600;
#else
    case 921600:
        return QPortSettings::BAUDR_921600;
#endif
    default:
        qWarning() << "TermiosHelper::baudRate(): Unknown baud rate";
    }

    return QPortSettings::BAUDR_UNKNOWN;
}

/*!

*/

QPortSettings::DataBits TermiosHelper::dataBits() const
{
    struct termios options;

    // get the current serial port attributes

    if ( tcgetattr(fileDescriptor_, &options) == -1 ) {
        qWarning() << QString("TermiosHelper::dataBits(file: %1) failed when" \
                              " getting original port attributes: %2(%3)")
                      .arg(fileDescriptor_)
                      .arg(strerror(errno))
                      .arg(errno);
        return QPortSettings::DB_UNKNOWN;
    }

    if ( (options.c_cflag & CSIZE) == CS5 )
        return QPortSettings::DB_5;
    else if ( (options.c_cflag & CSIZE) == CS6 )
        return QPortSettings::DB_6;
    else if ( (options.c_cflag & CSIZE) == CS7 )
        return QPortSettings::DB_7;
    else if ( (options.c_cflag & CSIZE) == CS8 )
        return QPortSettings::DB_8;
    else
        return QPortSettings::DB_UNKNOWN;
}


/*!

*/
void TermiosHelper::setDataBits(QPortSettings::DataBits dataBits)
{
    switch( dataBits ) {
    /*5 data bits*/
    case QPortSettings::DB_5:
        currentAttrs_->c_cflag &= (~CSIZE);
        currentAttrs_->c_cflag |= CS5;
        break;
        /*6 data bits*/
    case QPortSettings::DB_6:
        currentAttrs_->c_cflag &= (~CSIZE);
        currentAttrs_->c_cflag |= CS6;
        break;
        /*7 data bits*/
    case QPortSettings::DB_7:
        currentAttrs_->c_cflag &= (~CSIZE);
        currentAttrs_->c_cflag |= CS7;
        break;
        /*8 data bits*/
    case QPortSettings::DB_8:
        currentAttrs_->c_cflag &= (~CSIZE);
        currentAttrs_->c_cflag |= CS8;
        break;
    default:
        currentAttrs_->c_cflag &= (~CSIZE);
        currentAttrs_->c_cflag |= CS8;
        qWarning() << "TermiosHelper::setDataBits(" << dataBits << "): Unsupported data bits";
    }
}

/*!

 */

QPortSettings::Parity TermiosHelper::parity() const
{
    struct termios options;

    // get the current serial port attributes

    if ( tcgetattr(fileDescriptor_, &options) == -1 ) {
        qWarning() << QString("TermiosHelper::parity(file: %1) failed when getting original " \
                              "port attributes: %2(%3)")
                      .arg(fileDescriptor_)
                      .arg(strerror(errno))
                      .arg(errno);
        return QPortSettings::PAR_UNKNOWN;
    }

    if ( options.c_cflag & PARENB ) {
        if ( options.c_cflag & PARODD )
            return QPortSettings::PAR_ODD;
        else
            return QPortSettings::PAR_EVEN;
    }
    else
        return QPortSettings::PAR_NONE;
}

/*!

*/
void TermiosHelper::setParity(QPortSettings::Parity parity)
{
    switch ( parity ) {
    /*no parity*/
    case QPortSettings::PAR_NONE:
        currentAttrs_->c_cflag &= (~PARENB);
        break;
        /*odd parity*/
    case QPortSettings::PAR_ODD:
        currentAttrs_->c_cflag |= (PARENB|PARODD);
        break;
        /*even parity*/
    case QPortSettings::PAR_EVEN:
        currentAttrs_->c_cflag &= (~PARODD);
        currentAttrs_->c_cflag |= PARENB;
        break;
    default:
        currentAttrs_->c_cflag &= (~PARENB);
        qWarning() << "TermiosHelper::setParity(" << parity << "): Unsupported parity";
    }
}

/*!

 */

QPortSettings::StopBits TermiosHelper::stopBits() const
{
    struct termios options;
    // get the current serial port attributes

    if ( tcgetattr(fileDescriptor_, &options) == -1 ) {
        qWarning() << QString("TermiosHelper::stopBits(file: %1) failed when getting original " \
                              "port attributes: %2(%3)")
                      .arg(fileDescriptor_)
                      .arg(strerror(errno))
                      .arg(errno);
        return QPortSettings::STOP_UNKNOWN;
    }

    if ( options.c_cflag & CSTOPB )
        return QPortSettings::STOP_2;
    else
        return QPortSettings::STOP_1;
}

/*!

*/
void TermiosHelper::setStopBits(QPortSettings::StopBits stopBits)
{
    switch( stopBits ) {
    /*one stop bit*/
    case QPortSettings::STOP_1:
        currentAttrs_->c_cflag &= (~CSTOPB);
        break;
        /*two stop bits*/
    case QPortSettings::STOP_2:
        currentAttrs_->c_cflag |= CSTOPB;
        break;
    default:
        currentAttrs_->c_cflag &= (~CSTOPB);
        qWarning() << "TermiosHelper::setStopBits(" << stopBits << "): Unsupported stop bits";
    }
}

/*!
  @return FLOW_UNKNOWN in error case
 */

QPortSettings::FlowControl TermiosHelper::flowControl() const
{
    struct termios options;
    // get the current serial port attributes

    if ( tcgetattr(fileDescriptor_, &options) == -1 ) {
        qWarning() << QString("TermiosHelper::flowControl(file: %1) failed when getting original " \
                              "port attributes: %2(%3)")
                      .arg(fileDescriptor_)
                      .arg(strerror(errno))
                      .arg(errno);
        return QPortSettings::FLOW_UNKNOWN;
    }

    if ( options.c_cflag & CRTSCTS ) {
        return QPortSettings::FLOW_HARDWARE;
    }
    else {
        if ( options.c_iflag & IXON )
            return QPortSettings::FLOW_XONXOFF;
        else
            return QPortSettings::FLOW_OFF;
    }
}

/*!

*/
void TermiosHelper::setFlowControl(QPortSettings::FlowControl flow)
{
    switch( flow ) {
    /*no flow control*/
    case QPortSettings::FLOW_OFF:
        currentAttrs_->c_cflag &= (~CRTSCTS);
        currentAttrs_->c_iflag &= (~(IXON|IXOFF|IXANY));
        break;
        /*software (XON/XOFF) flow control*/
    case QPortSettings::FLOW_XONXOFF:
        currentAttrs_->c_cflag &= (~CRTSCTS);
        currentAttrs_->c_iflag |= (IXON|IXOFF|IXANY);
        break;
    case QPortSettings::FLOW_HARDWARE:
        currentAttrs_->c_cflag |= CRTSCTS;
        currentAttrs_->c_iflag &= (~(IXON|IXOFF|IXANY));
        break;
    default:
        currentAttrs_->c_cflag &= (~CRTSCTS);
        currentAttrs_->c_iflag &= (~(IXON|IXOFF|IXANY));
        qWarning() << "TermiosHelper::setFlowControl(" << flow << "): Unsupported flow control type";
    }
}

//
// information about VMIN and VTIME
// @see www.unixwiz.net/techtips/termios-vmin-vtime.html
//
// VMIN is a character count ranging from 0 to 255 characters, and VTIME is time measured in 0.1
// second intervals, (0 to 25.5 seconds). The value of "zero" is special to both of these parameters,
// and this suggests four combinations that we'll discuss below. In every case, the question is when
// a read() system call is satisfied, and this is our prototype call:
//  int n = read(fd, buffer, nbytes);
//
// Keep in mind that the tty driver maintains an input queue of bytes already read from the
// serial line and not passed to the user, so not every read() call waits for actual I/O - the read
// may very well be satisfied directly from the input queue.
//
// VMIN = 0 and VTIME = 0
// This is a completely non-blocking read - the call is satisfied immediately directly from the
// driver's input queue. If data are available, it's transferred to the caller's buffer up to
// nbytes and returned. Otherwise zero is immediately returned to indicate "no data". We'll note
// that this is "polling" of the serial port, and it's almost always a bad idea. If done repeatedly,
// it can consume enormous amounts of processor time and is highly inefficient. Don't use this mode
// unless you really, really know what you're doing.
//
// VMIN = 0 and VTIME > 0
// This is a pure timed read. If data are available in the input queue, it's transferred to the caller's
// buffer up to a maximum of nbytes, and returned immediately to the caller. Otherwise the driver blocks
// until data arrives, or when VTIME tenths expire from the start of the call. If the timer expires
// without data, zero is returned. A single byte is sufficient to satisfy this read call, but if more
// is available in the input queue, it's returned to the caller. Note that this is an overall timer,
// not an intercharacter one.
//
// VMIN > 0 and VTIME > 0
// A read() is satisfied when either VMIN characters have been transferred to the caller's buffer,
// or when VTIME tenths expire between characters. Since this timer is not started until the first
// character arrives, this call can block indefinitely if the serial line is idle. This is the most
// common mode of operation, and we consider VTIME to be an intercharacter timeout, not an overall one.
// This call should never return zero bytes read.
//
// VMIN > 0 and VTIME = 0
// This is a counted read that is satisfied only when at least VMIN characters have been transferred
// to the caller's buffer - there is no timing component involved. This read can be satisfied from the
// driver's input queue (where the call could return immediately), or by waiting for new data to arrive:
// in this respect the call could block indefinitely. We believe that it's undefined behavior if nbytes
// is less then VMIN.
//

bool TermiosHelper::commTimeouts(CommTimeouts &commtimeouts) const
{
    struct termios options;

    // get the current serial port attributes

    if ( tcgetattr(fileDescriptor_, &options) == -1 ) {
        qWarning() << QString("TermiosHelper::commTimeouts(file: %1) failed when getting original " \
                              "port attributes: %2(%3)")
                      .arg(fileDescriptor_)
                      .arg(strerror(errno))
                      .arg(errno);
        return false;
    }
    commtimeouts.PosixVMIN = options.c_cc[VMIN];
    commtimeouts.PosixVTIME = options.c_cc[VTIME];

    return true;
}

void TermiosHelper::setCommTimeouts(const CommTimeouts commtimeouts)
{
    currentAttrs_->c_cc[VMIN] = (cc_t)commtimeouts.PosixVMIN;
    currentAttrs_->c_cc[VTIME] = (cc_t)commtimeouts.PosixVTIME;
}

} // namespace


