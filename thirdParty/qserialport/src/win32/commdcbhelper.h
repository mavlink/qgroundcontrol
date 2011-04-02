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

#ifndef TNX_COMMDCBHELPER_H__
#define TNX_COMMDCBHELPER_H__

#include <QString>
#include <QStringList>
#include <QDebug>
#include "qportsettings.h"
#include "qserialport.h"
#include <windows.h>

namespace TNX {

/*!
  Wrapper class for DCB structure on Windows systems.
 */

class CommDCBHelper
{
  enum { kDefaultReadTimeout = 0, kDefaultNumOfBytes = 1 };
  enum ControlSignals {
    CSIGNAL_DSR,
    CSIGNAL_DTR,
    CSIGNAL_RTS,
    CSIGNAL_CTS,
    CSIGNAL_DCD,
    CSIGNAL_RNG
  };

public:
  explicit CommDCBHelper(HANDLE fileDescriptor);
  ~CommDCBHelper();

  bool applyChanges(ChangeApplyTypes apptype = AllAppTy);

  QPortSettings::BaudRate baudRate() const;
  void setBaudRate(QPortSettings::BaudRate baudRate);

  QPortSettings::DataBits dataBits() const;
  void setDataBits(QPortSettings::DataBits dataBits);

  QPortSettings::Parity parity() const;
  void setParity(QPortSettings::Parity parity);

  QPortSettings::StopBits stopBits() const;
  void setStopBits(QPortSettings::StopBits stopBits);

  QPortSettings::FlowControl flowControl() const;
  void setFlowControl(QPortSettings::FlowControl flow);

  bool commTimeouts(CommTimeouts &timeouts);
  void setCommTimeouts(const CommTimeouts commtimeouts);

  inline bool setRts(bool value = true) {
    return setCtrSignal(CSIGNAL_RTS, value);
  }

  inline bool setDtr(bool value = true) {
    return setCtrSignal(CSIGNAL_DTR, value);
  }

  inline QSerialPort::CommSignalValues cts() const {
    return ctrSignal(CSIGNAL_CTS);
  }
  
  inline QSerialPort::CommSignalValues dsr() const {
    return ctrSignal(CSIGNAL_DSR);
  }
  
  inline QSerialPort::CommSignalValues dcd() const {
    return ctrSignal(CSIGNAL_DCD);
  }
  
  inline QSerialPort::CommSignalValues ri() const {
    return ctrSignal(CSIGNAL_RNG);
  }
  
  static QString errorText(DWORD err);

  // helper methods to manage port attributes in 'termios' data structure
private:
  void saveDCB();
  bool getCommTimeouts();
  void restoreDCB();
  void initDCB();

  bool setCtrSignal(ControlSignals csig, bool value = true);
  QSerialPort::CommSignalValues ctrSignal(ControlSignals csig) const;

private:
  HANDLE fileHandle_;
  DCB *originalAttrs_;
  DCB *currentAttrs_;
  COMMTIMEOUTS commTimeouts_;

private:
  CommDCBHelper();
  Q_DISABLE_COPY(CommDCBHelper);
};

}

#endif // TNX_COMMDCBHELPER_H__
