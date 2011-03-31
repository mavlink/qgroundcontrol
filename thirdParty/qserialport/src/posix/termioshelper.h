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

#ifndef TNX_TERMIOSHELPER_H__
#define TNX_TERMIOSHELPER_H__

#include <QString>
#include <QStringList>
#include <QDebug>
#include <termios.h>
#include "qportsettings.h"
#include "qserialport.h"

struct termios;

namespace TNX {

/*!
  Wrapper class for termios structure on Posix compatible systems.
 */

class TermiosHelper
{
  enum { kDefaultReadTimeout = 0, kDefaultNumOfBytes = 1 }; // counted read

  enum ControlSignals {
    CSIGNAL_DSR = TIOCM_LE,
    CSIGNAL_DTR = TIOCM_DTR,
    CSIGNAL_RTS = TIOCM_RTS,
    CSIGNAL_CTS = TIOCM_CTS,
    CSIGNAL_DCD = TIOCM_CD,
    CSIGNAL_RNG = TIOCM_RNG
  };

public:
  explicit TermiosHelper(int fileDescriptor);
  ~TermiosHelper();

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

  bool commTimeouts(CommTimeouts &timeouts) const;
  void setCommTimeouts(const CommTimeouts commtimeouts);

  inline QSerialPort::CommSignalValues rts() const {
    return ctrSignal(CSIGNAL_RTS);
  }
  inline bool setRts(bool value = true) {
    return setCtrSignal(CSIGNAL_RTS, value);
  }

  inline QSerialPort::CommSignalValues dtr() const {
    return ctrSignal(CSIGNAL_DTR);
  }
  inline bool setDtr(bool value = true) {
    return setCtrSignal(CSIGNAL_DTR, value);
  }

  inline QSerialPort::CommSignalValues cts() const {
    return ctrSignal(CSIGNAL_CTS);
  }
  inline bool setCts(bool value = true) {
    return setCtrSignal(CSIGNAL_CTS, value);
  }

  inline QSerialPort::CommSignalValues dsr() const {
    return ctrSignal(CSIGNAL_DSR);
  }
  inline bool setDsr(bool value = true) {
    return setCtrSignal(CSIGNAL_DSR, value);
  }

  inline QSerialPort::CommSignalValues dcd() const {
    return ctrSignal(CSIGNAL_DCD);
  }
  inline bool setDcd(bool value = true) {
    return setCtrSignal(CSIGNAL_DCD, value);
  }

  inline QSerialPort::CommSignalValues ri() const {
    return ctrSignal(CSIGNAL_RNG);
  }
  inline bool setRi(bool value = true) {
    return setCtrSignal(CSIGNAL_RNG, value);
  }

  // helper methods to manage port attributes in 'termios' data structure
private:
  void saveTermios();
  void restoreTermios();
  void initTermios();

  bool setCtrSignal(ControlSignals csig, bool value = true);
  QSerialPort::CommSignalValues ctrSignal(ControlSignals csig) const;

private:
  int fileDescriptor_;
  struct termios *originalAttrs_;
  struct termios *currentAttrs_;

private:
  TermiosHelper();
};

}

#endif // TNX_TERMIOSHELPER_H__
