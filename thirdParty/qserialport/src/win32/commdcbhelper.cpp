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
 * author labs@inbiza.com
 */

#include <QDebug>
#include "commdcbhelper.h"

namespace TNX {

/*!
  Constructs a CommDCBHelper object with the given \a file handle.
*/
CommDCBHelper::CommDCBHelper(HANDLE fileHandle)
  : fileHandle_(fileHandle), originalAttrs_(NULL), currentAttrs_(NULL)
{
  Q_ASSERT(fileHandle_ > 0);
  
  originalAttrs_ = new DCB();
  currentAttrs_ = new DCB();

  // save the current serial port attributes
  // see restoreDCB()

  saveDCB();
    
  // clone the original attributes

  *currentAttrs_ = *originalAttrs_;

  // initialize port attributes for serial port communication

  initDCB();
}


CommDCBHelper::~CommDCBHelper()
{
  // It is good practice to reset a serial port back to the state in
  // which you found it. This is why we saved the original DCB struct
  
  restoreDCB();

  delete originalAttrs_;
  delete currentAttrs_;
}

/*!
   Sets the DCB structure.
 */
bool CommDCBHelper::applyChanges(ChangeApplyTypes apptype)
{
  if ( apptype == PortAttrOnlyAppTy || apptype == AllAppTy ) {
    if ( !SetCommState(fileHandle_, currentAttrs_) ) {
      qDebug() << QString("CommDCBHelper::applyChanges(file: %1, PortAttributes) failed: %2(Err #%3)")
                       .arg((quintptr)fileHandle_)
                       .arg(errorText(GetLastError()))
                       .arg(GetLastError());
      return false;
    }
  }

  // Communication Timeouts

  if ( apptype == CommTimeoutsOnlyAppTy || apptype == AllAppTy ) {
    if ( !SetCommTimeouts(fileHandle_, &commTimeouts_) ) {
      qDebug() << QString("CommDCBHelper::applyChanges(file: %1, CommTimeouts) failed: %2(Err #%3)")
                      .arg((quintptr)fileHandle_)
                      .arg(errorText(GetLastError()))
                      .arg(GetLastError());
      return false;
    }
  }

  return true;
}

/*!

 */
bool CommDCBHelper::setCtrSignal(ControlSignals csig, bool value)
{
  DWORD sig;

  switch ( csig ) {
    case CSIGNAL_RTS:
      sig = value ? SETRTS : CLRRTS;
      break;

    case CSIGNAL_DTR:
      sig = value ? SETDTR : CLRDTR;
      break;

    default:
      qDebug() << QString("CommDCBHelper::setCtrSignal(file: %1, csig: %2) failed." \
                            "Given signal is read only.")
                 .arg((quintptr)fileHandle_)
                 .arg(csig);
      return false;
  }

  if ( !EscapeCommFunction(fileHandle_, sig) ) {
    qDebug() <<  QString("CommDCBHelper::setCtrSignal(file: %1, csig: %2) failed" \
                         "when fetching control signal values : %3(Err #%4)")
                 .arg((quintptr)fileHandle_)
                 .arg(csig)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());
    return false;
  }

  return true;
}

/*!

 */
QSerialPort::CommSignalValues CommDCBHelper::ctrSignal(ControlSignals csig) const
{
  DWORD status = 0;

  if ( !GetCommModemStatus(fileHandle_, &status) ) {
    qDebug() << QString("CommDCBHelper::ctrSignal(file: %1, csig: %2) failed" \
                        "when fetching control signal values : %3(Err #%4)")
                 .arg((quintptr)fileHandle_)
                 .arg(csig)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());
    return QSerialPort::Signal_Unknown;
  }

  DWORD sig;
  switch ( csig ) {
    case CSIGNAL_CTS:
      sig = MS_CTS_ON;
      break;

    case CSIGNAL_DSR:
      sig = MS_DSR_ON;
      break;

    case CSIGNAL_DCD:
      sig = MS_RLSD_ON;
      break;

    case CSIGNAL_RNG:
      sig = MS_RING_ON;
      break;

    default:
      qDebug() << QString("CommDCBHelper::ctrSignal(file: %1, csig: %2) failed." \
                            "Invalid signal.")
                 .arg((quintptr)fileHandle_)
                 .arg(csig);
      return QSerialPort::Signal_Unknown;
  }

  return ((status & sig) ? QSerialPort::Signal_On : QSerialPort::Signal_Off);
}

/*!

 */
void CommDCBHelper::initDCB()
{   
  currentAttrs_->DCBlength          = sizeof(DCB);

  currentAttrs_->EvtChar            =   '\0';
  currentAttrs_->EofChar            =   '\0';
  currentAttrs_->ErrorChar          =   '\0';
  currentAttrs_->XonChar            =   '\0';
  currentAttrs_->XoffChar           =   '\0';
  currentAttrs_->fDtrControl        =   DTR_CONTROL_DISABLE;
  currentAttrs_->fRtsControl        =   RTS_CONTROL_DISABLE;
  currentAttrs_->fOutxCtsFlow       =   0;
  currentAttrs_->fOutxDsrFlow       =   0;
  currentAttrs_->fDsrSensitivity    =   0;
  currentAttrs_->fOutX              =   0;
  currentAttrs_->fInX               =   0;
  currentAttrs_->fTXContinueOnXoff  =   0;
  currentAttrs_->fErrorChar         =   0;
  currentAttrs_->XonLim             =   0;
  currentAttrs_->XoffLim            =   0;
  currentAttrs_->fBinary            =   1;
  currentAttrs_->fNull              =   0;

  // If a communications port has been set up with a TRUE value for the fAbortOnError member 
  // of the setup DCB structure, the communications software will terminate all read and write 
  // operations on the communications port when a communications error occurs. No new read or write 
  // operations will be accepted until the application acknowledges the communications error by 
  // calling the ClearCommError function.

  currentAttrs_->fAbortOnError      =   0;
  currentAttrs_->BaudRate           =   CBR_9600;
  currentAttrs_->fParity            =   0;
  currentAttrs_->Parity             =   0;
  currentAttrs_->StopBits           =   0;
  currentAttrs_->ByteSize           =   8;
  
  // ensure the new attributes take effect immediately

  applyChanges(AllAppTy);
}

/*!

*/
void CommDCBHelper::saveDCB()
{
  // get the current serial port attributes

  if ( !GetCommState(fileHandle_, originalAttrs_) ) {
    qDebug() << QString("CommDCBHelper::saveDCB(file: %1) failed when" \
                          " getting original port attributes: %2(Err #%3)")
                 .arg((quintptr)fileHandle_)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());
  }
}

/*!

*/
bool CommDCBHelper::getCommTimeouts()
{
  // get the current communication timeouts

  if ( !GetCommTimeouts(fileHandle_, &commTimeouts_) ) {
    qDebug() << QString("CommDCBHelper::getCommTimeouts(file: %1) failed when" \
                          " getting communication timeout values: %2(Err #%3)")
                 .arg((quintptr)fileHandle_)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());
    return false;
  }
  return true;
}

/*!

*/
void CommDCBHelper::restoreDCB()
{
  if ( !originalAttrs_ || !SetCommState(fileHandle_, originalAttrs_) ) {
    qDebug() << QString("CommDCBHelper::restoreDCB(file: %1) failed when resetting " \
                        "serial port attributes: %2(Err #%3)")
                  .arg((quintptr)fileHandle_)
                  .arg(errorText(GetLastError()))
                  .arg(GetLastError());
  }
}

/*!

*/
void CommDCBHelper::setBaudRate(QPortSettings::BaudRate baudRate)
{
  DWORD baud = CBR_9600;

  switch ( baudRate ) {
    case QPortSettings::BAUDR_110:
      baud = CBR_110;
      break;      
    case QPortSettings::BAUDR_300:
      baud = CBR_300;
      break;      
    case QPortSettings::BAUDR_600:
      baud = CBR_600;
      break;
    case QPortSettings::BAUDR_1200:
      baud = CBR_1200;
      break;
    case QPortSettings::BAUDR_2400:
      baud = CBR_2400;
      break;
    case QPortSettings::BAUDR_4800:
      baud = CBR_4800;
      break;
    case QPortSettings::BAUDR_9600:
      baud = CBR_9600;
      break;
    case QPortSettings::BAUDR_14400:
      baud = CBR_14400;
      break;
    case QPortSettings::BAUDR_19200:
      baud = CBR_19200;
      break;
    case QPortSettings::BAUDR_38400:
      baud = CBR_38400;
      break;
    case QPortSettings::BAUDR_56000:
      baud = CBR_56000;
      break;
    case QPortSettings::BAUDR_57600:
      baud = CBR_57600;
      break;
    case QPortSettings::BAUDR_115200:
      baud = CBR_115200;
      break;
    case QPortSettings::BAUDR_128000:
      baud = CBR_128000;
      break;
    case QPortSettings::BAUDR_230400:
      baud = 230400;
      break;
    case QPortSettings::BAUDR_256000:
      baud = CBR_256000;
      break;
    case QPortSettings::BAUDR_460800:
      baud = 460800;
      break;
    case QPortSettings::BAUDR_921600:
      baud = 921600;
      break;
    default:
      qWarning() << "CommDCBHelper::setBaudRate(" << baudRate << "): " \
                    "Unsupported baud rate";
  }
  currentAttrs_->BaudRate = baud;
}

/*!

*/
QPortSettings::BaudRate CommDCBHelper::baudRate() const
{
  DWORD baud;
  DCB dcb;

  // although we store the last value of the baud rate attribute in currentAttrs_ structure, 
  // it is better practice to request the actual value from the OS.

  if ( !GetCommState(fileHandle_, &dcb) ) {
    qDebug() << QString("CommDCBHelper::baudRate(file: %1) failed when" \
                          " getting baud rate: %2(Err #%3)")
                 .arg((quintptr)fileHandle_)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());

    baud = currentAttrs_->BaudRate;
  }
  else {
      baud = dcb.BaudRate;
  }

  Q_ASSERT(currentAttrs_->BaudRate == baud);

  switch ( baud ) {
  case CBR_110:
      return QPortSettings::BAUDR_110;
  case CBR_300:
      return QPortSettings::BAUDR_300;
  case CBR_600:
      return QPortSettings::BAUDR_600;
  case CBR_1200:
      return QPortSettings::BAUDR_1200;
  case CBR_2400:
      return QPortSettings::BAUDR_2400;
  case CBR_4800:
      return QPortSettings::BAUDR_4800;
  case CBR_9600:
      return QPortSettings::BAUDR_9600;
  case CBR_14400:
      return QPortSettings::BAUDR_14400;
  case CBR_19200:
      return QPortSettings::BAUDR_19200;
  case CBR_38400:
      return QPortSettings::BAUDR_38400;
  case CBR_56000:
      return QPortSettings::BAUDR_56000;
  case CBR_57600:
      return QPortSettings::BAUDR_57600;
  case CBR_115200:
      return QPortSettings::BAUDR_115200;
  case CBR_128000:
      return QPortSettings::BAUDR_128000;
  case 230400:
      return QPortSettings::BAUDR_230400;
  case CBR_256000:
      return QPortSettings::BAUDR_256000;
  case 460800:
      return QPortSettings::BAUDR_460800;
  case 921600:
      return QPortSettings::BAUDR_921600;
      break;
  default:
      qWarning() << "CommDCBHelper::baudRate(): Unknown baud rate";
  }

  return QPortSettings::BAUDR_UNKNOWN;
}

/*!

*/
QPortSettings::DataBits CommDCBHelper::dataBits() const
{
  DCB dcb;
  BYTE dataBits;

  // get the current serial port attributes

  if ( !GetCommState(fileHandle_, &dcb) ) {
    qDebug() << QString("CommDCBHelper::dataBits(file: %1) failed when" \
                        " getting original port attributes: %2(Err #%3)")
                 .arg((quintptr)fileHandle_)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());

    dataBits = currentAttrs_->ByteSize;
  }
  else {
    dataBits = dcb.ByteSize;
  }

  Q_ASSERT(currentAttrs_->ByteSize == dataBits);

  if ( dataBits == 5 )
    return QPortSettings::DB_5;
  else if ( dataBits == 6 )
    return QPortSettings::DB_6;
  else if ( dataBits == 7 )
    return QPortSettings::DB_7;
  else if ( dataBits == 8 )
    return QPortSettings::DB_8;
  else
    return QPortSettings::DB_UNKNOWN;
}

/*!

*/
void CommDCBHelper::setDataBits(QPortSettings::DataBits dataBits)
{
  switch( dataBits ) {
    /*5 data bits*/
    case QPortSettings::DB_5:
      currentAttrs_->ByteSize = 5;
      break;

    /*6 data bits*/
    case QPortSettings::DB_6:
      currentAttrs_->ByteSize = 6;
      break;

    /*7 data bits*/
    case QPortSettings::DB_7:
      currentAttrs_->ByteSize = 7;
      break;

    /*8 data bits*/
    case QPortSettings::DB_8:
      currentAttrs_->ByteSize = 8;
      break;

    default:
      currentAttrs_->ByteSize = 8;
      qWarning() << "CommDCBHelper::setDataBits(enum[" << dataBits << "]): Unsupported data bits";
  }
}

/*!

 */
QPortSettings::Parity CommDCBHelper::parity() const
{
  DCB dcb;
  BYTE parity;
  DWORD fParity;

  // get the current serial port attributes

  if ( !GetCommState(fileHandle_, &dcb) ) {
    qDebug() << QString("CommDCBHelper::parity(file: %1) failed when" \
                        " getting original port attributes: %2(Err #%3)")
                 .arg((quintptr)fileHandle_)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());

    fParity = currentAttrs_->fParity;
    parity = currentAttrs_->Parity;
  }
  else {
    fParity = dcb.fParity;
    parity = dcb.Parity;
  }

  // For some reason windows keep changing fParity value even Parity field is set
  // something other than 0 (no parity)

  //Q_ASSERT(currentAttrs_->fParity == fParity && currentAttrs_->Parity == parity);

  //if ( fParity != 1 ) {
  //  return QPortSettings::PAR_NONE;
  //}
  //else {
    if ( parity == 0 )
      return QPortSettings::PAR_NONE;
    else if ( parity == 1 )
      return QPortSettings::PAR_ODD;
    else if ( parity == 2 )
      return QPortSettings::PAR_EVEN;
    else if ( parity == 3 )
      return QPortSettings::PAR_MARK;
    else if ( parity == 4 )
      return QPortSettings::PAR_SPACE;
    else
      return QPortSettings::PAR_UNKNOWN;
  //}
}

/*!

*/
void CommDCBHelper::setParity(QPortSettings::Parity parity)
{
  switch ( parity ) {
    /*no parity*/
    case QPortSettings::PAR_NONE:
      currentAttrs_->fParity = 0;
      currentAttrs_->Parity = 0;
      break;

    /*odd parity*/
    case QPortSettings::PAR_ODD:
      currentAttrs_->fParity = 1;
      currentAttrs_->Parity = 1;
      break;

    /*even parity*/
    case QPortSettings::PAR_EVEN:
      currentAttrs_->fParity = 1;
      currentAttrs_->Parity = 2;
      break;

    /*mark parity*/
    case QPortSettings::PAR_MARK:
      currentAttrs_->fParity = 1;
      currentAttrs_->Parity = 3;
      break;

    /*space parity*/
    case QPortSettings::PAR_SPACE:
      currentAttrs_->fParity = 1;
      currentAttrs_->Parity = 4;
      break;
      
    default:
      currentAttrs_->fParity = 0;
      currentAttrs_->Parity = 0;
      qWarning() << "CommDCBHelper::setParity(enum[" << parity << "]): Unsupported parity";
  }
}

/*!

 */
QPortSettings::StopBits CommDCBHelper::stopBits() const
{
  DCB dcb;
  BYTE stopBits;

  // get the current serial port attributes

  if ( !GetCommState(fileHandle_, &dcb) ) {
    qDebug() << QString("CommDCBHelper::stopBits(file: %1) failed when" \
                        " getting original port attributes: %2(Err #%3)")
                 .arg((quintptr)fileHandle_)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());

    stopBits = currentAttrs_->StopBits;
  }
  else {
    stopBits = dcb.StopBits;
  }

  Q_ASSERT(currentAttrs_->StopBits == stopBits);

  if ( stopBits == 0 )
    return QPortSettings::STOP_1;
  else if ( stopBits == 1 )
    return QPortSettings::STOP_1_5;
  else if ( stopBits == 2 )
    return QPortSettings::STOP_2;
  else
    return QPortSettings::STOP_UNKNOWN;
}

/*!

*/
void CommDCBHelper::setStopBits(QPortSettings::StopBits stopBits)
{
  switch( stopBits ) {
    /*one stop bit*/
    case QPortSettings::STOP_1:
      currentAttrs_->StopBits = 0;
      break;

    /*one and half bit*/
    case QPortSettings::STOP_1_5:
      currentAttrs_->StopBits = 1;
      break;

    /*two stop bits*/
    case QPortSettings::STOP_2:
      currentAttrs_->StopBits = 2;
      break;

    default:
      currentAttrs_->StopBits = 0;
      qWarning() << "CommDCBHelper::setStopBits(enum[" << stopBits << "]): Unsupported stop bits";
  }
}

/*!
  @return FLOW_UNKNOWN in error case
 */
QPortSettings::FlowControl CommDCBHelper::flowControl() const
{
  DCB dcb;

  // get the current serial port attributes.

  if ( !GetCommState(fileHandle_, &dcb) ) {
    qWarning() << QString("CommDCBHelper::flowControl(file: %1) failed when" \
                          " getting original port attributes: %2(Err #%3)")
                 .arg((quintptr)fileHandle_)
                 .arg(errorText(GetLastError()))
                 .arg(GetLastError());

    return QPortSettings::FLOW_UNKNOWN;
  }
  
  if ( !dcb.fOutxCtsFlow && dcb.fRtsControl == RTS_CONTROL_DISABLE ) {
    if ( dcb.fInX && dcb.fOutX )
      return QPortSettings::FLOW_XONXOFF;
    else if ( !dcb.fInX && !dcb.fOutX )
      return QPortSettings::FLOW_OFF;
    else
      return QPortSettings::FLOW_UNKNOWN;
  }
  else if ( dcb.fOutxCtsFlow && dcb.fRtsControl == RTS_CONTROL_HANDSHAKE &&
            !dcb.fInX && !dcb.fOutX ) {
    return QPortSettings::FLOW_HARDWARE;
  }
  else {
    return QPortSettings::FLOW_UNKNOWN;
  }
}

/*!

*/
void CommDCBHelper::setFlowControl(QPortSettings::FlowControl flow)
{
  switch( flow ) {
    /*no flow control*/
    case QPortSettings::FLOW_OFF:
      currentAttrs_->fOutxCtsFlow = false;
      currentAttrs_->fRtsControl = RTS_CONTROL_DISABLE;
      currentAttrs_->fInX = false;
      currentAttrs_->fOutX = false;
      break;

    /*software (XON/XOFF) flow control*/
    case QPortSettings::FLOW_XONXOFF:
      currentAttrs_->fOutxCtsFlow = false;
      currentAttrs_->fRtsControl = RTS_CONTROL_DISABLE;
      currentAttrs_->fInX = true;
      currentAttrs_->fOutX = true;
      break;

    case QPortSettings::FLOW_HARDWARE:
      currentAttrs_->fOutxCtsFlow = true;
      currentAttrs_->fRtsControl = RTS_CONTROL_HANDSHAKE;
      currentAttrs_->fInX = false;
      currentAttrs_->fOutX = false;
      break;

    default:
      currentAttrs_->fOutxCtsFlow = false;
      currentAttrs_->fRtsControl = RTS_CONTROL_DISABLE;
      currentAttrs_->fInX = false;
      currentAttrs_->fOutX = false;
      qWarning("CommDCBHelper::setFlowControl(%i): Unsupported FlowType", flow);
  }
}

//
//  ReadIntervalTimeout
//    Maximum time allowed to elapse between the arrival of two characters on the communications
//    line, in milliseconds. During a ReadFile operation, the time period begins when the first
//    character is received. If the interval between the arrival of any two characters
//    exceeds this amount, the ReadFile operation is completed and any buffered data is returned.
//    A value of zero indicates that interval time-outs are not used. A value of MAXDWORD,
//    combined with zero values for both the ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier
//    parameters, specifies that the read operation is to return immediately with the characters that
//    have already been received, even if no characters have been received.
//
//  ReadTotalTimeoutMultiplier
//    Multiplier used to calculate the total time-out period for read operations,
//    in milliseconds. For each read operation, this value is multiplied by the requested
//    number of bytes to be read.
//
//  ReadTotalTimeoutConstant
//    Constant used to calculate the total time-out period for read operations, in milliseconds.
//    For each read operation, this value is added to the product of the ReadTotalTimeoutMultiplier
//    parameter and the requested number of bytes. A value of zero for both the ReadTotalTimeoutMultiplier
//    and ReadTotalTimeoutConstant members indicates that total time-outs are not used for read operations.
//
//  WriteTotalTimeoutMultiplier
//    Multiplier used to calculate the total time-out period for write operations, in milliseconds.
//    For each write operation, this value is multiplied by the number of bytes to be written.
//
//  WriteTotalTimeoutConstant
//    Constant used to calculate the total time-out period for write operations, in milliseconds.
//    For each write operation, this value is added to the product of the WriteTotalTimeoutMultiplier
//    member and the number of bytes to be written. A value of zero for both the WriteTotalTimeoutMultiplier
//    and WriteTotalTimeoutConstant parameters indicates that total time-outs are not used for write operations.
//

bool CommDCBHelper::commTimeouts(CommTimeouts &commtimeouts)
{
  // get the current serial port attributes

  if ( !getCommTimeouts() )
    return false;

  commtimeouts.Win32ReadIntervalTimeout = commTimeouts_.ReadIntervalTimeout;
  commtimeouts.Win32ReadTotalTimeoutConstant = commTimeouts_.ReadTotalTimeoutConstant;
  commtimeouts.Win32ReadTotalTimeoutMultiplier = commTimeouts_.ReadTotalTimeoutMultiplier;
  commtimeouts.Win32WriteTotalTimeoutConstant = commTimeouts_.WriteTotalTimeoutConstant;
  commtimeouts.Win32WriteTotalTimeoutMultiplier = commTimeouts_.WriteTotalTimeoutMultiplier;

  return true;
}

void CommDCBHelper::setCommTimeouts(const CommTimeouts commtimeouts)
{
  commTimeouts_.ReadIntervalTimeout = commtimeouts.Win32ReadIntervalTimeout;
  commTimeouts_.ReadTotalTimeoutConstant = commtimeouts.Win32ReadTotalTimeoutConstant;
  commTimeouts_.ReadTotalTimeoutMultiplier = commtimeouts.Win32ReadTotalTimeoutMultiplier;
  commTimeouts_.WriteTotalTimeoutConstant = commtimeouts.Win32WriteTotalTimeoutConstant;
  commTimeouts_.WriteTotalTimeoutMultiplier = commtimeouts.Win32WriteTotalTimeoutMultiplier;
}

/*!

*/
QString CommDCBHelper::errorText(DWORD err)
{
  unsigned short *pMsg = 0;

  // if character set is Ascii, pMsg should be defined as char *string = 0
  // and QString::fromLocal8Bit(pMsg) should be used instead.

  if ( FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                     0,
                     err,
                     MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                     (LPTSTR)&pMsg,
                     0,
                     NULL) == 0 ) {
    return "There is no message associated with that error code.";
  }
  else {
    QString errMsg = QString::fromUtf16(pMsg);
    LocalFree(pMsg); // free memory allocated by the OS

    return errMsg;
  }
}

} // namespace


