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

#include <QFile>
#include "qserialportunittest.h"

void QSerialPortUnitTest::initTestCase()
{
  using namespace TNX;

  serialPortInvalid_ = new QSerialPort("/dev/invaliddevicename");

  QString validSerPortName;
  {
    QString filePath = "../serport";
    if ( !QFile::exists(filePath) )
      filePath = "../../serport";
    QFile serportName(filePath);
    if ( !serportName.open(QIODevice::ReadOnly) )
      QFAIL("Cannot locate the file containing the serial port name that will be used for unit testing. Quitting.");

    validSerPortName = serportName.read(256);
  }

  QPortSettings portSettings;
  serialPortValid_ = new TNX::QSerialPort(validSerPortName.trimmed(), portSettings);
}

void QSerialPortUnitTest::cleanupTestCase()
{
  delete serialPortInvalid_;
  delete serialPortValid_;
}

void QSerialPortUnitTest::init()
{
  QVERIFY( !serialPortInvalid_->open() );
  QVERIFY( serialPortValid_->open() );
}

void QSerialPortUnitTest::cleanup()
{
  serialPortInvalid_->close();
  serialPortValid_->close();
}

void QSerialPortUnitTest::test_portSettings()
{
  using namespace TNX;

  QPortSettings portSettings;

  portSettings.setBaudRate(QPortSettings::BAUDR_115200);
  QCOMPARE( portSettings.baudRate(), QPortSettings::BAUDR_115200);

  portSettings.setDataBits(QPortSettings::DB_5);
  QCOMPARE( portSettings.dataBits(), QPortSettings::DB_5);

  portSettings.setDataBits(QPortSettings::DB_6);
  QCOMPARE( portSettings.dataBits(), QPortSettings::DB_6);

  portSettings.setDataBits(QPortSettings::DB_7);
  QCOMPARE( portSettings.dataBits(), QPortSettings::DB_7);

  portSettings.setDataBits(QPortSettings::DB_8);
  QCOMPARE( portSettings.dataBits(), QPortSettings::DB_8);

  portSettings.setFlowControl(QPortSettings::FLOW_HARDWARE);
  QCOMPARE( portSettings.flowControl(), QPortSettings::FLOW_HARDWARE);

  portSettings.setFlowControl(QPortSettings::FLOW_OFF);
  QCOMPARE( portSettings.flowControl(), QPortSettings::FLOW_OFF);

  portSettings.setFlowControl(QPortSettings::FLOW_XONXOFF);
  QCOMPARE( portSettings.flowControl(), QPortSettings::FLOW_XONXOFF);

  portSettings.setParity(QPortSettings::PAR_EVEN);
  QCOMPARE( portSettings.parity(), QPortSettings::PAR_EVEN);

  portSettings.setParity(QPortSettings::PAR_NONE);
  QCOMPARE( portSettings.parity(), QPortSettings::PAR_NONE);

  portSettings.setParity(QPortSettings::PAR_ODD);
  QCOMPARE( portSettings.parity(), QPortSettings::PAR_ODD);

  portSettings.setParity(QPortSettings::PAR_SPACE);
  QCOMPARE( portSettings.parity(), QPortSettings::PAR_SPACE);

  portSettings.setStopBits(QPortSettings::STOP_1);
  QCOMPARE( portSettings.stopBits(), QPortSettings::STOP_1);

  portSettings.setStopBits(QPortSettings::STOP_2);
  QCOMPARE( portSettings.stopBits(), QPortSettings::STOP_2);
}

void QSerialPortUnitTest::test_invalidport_signals()
{
  using namespace TNX;

  QCOMPARE( serialPortInvalid_->dcd(),  QSerialPort::Signal_Unknown );
  QCOMPARE( serialPortInvalid_->cts(),  QSerialPort::Signal_Unknown );
  QCOMPARE( serialPortInvalid_->dsr(),  QSerialPort::Signal_Unknown );
  QCOMPARE( serialPortInvalid_->ri(),  QSerialPort::Signal_Unknown );

  QVERIFY( !serialPortInvalid_->setDtr() );
  QVERIFY( !serialPortInvalid_->setRts() );
}

void QSerialPortUnitTest::test_validport_signals()
{
  using namespace TNX;

  QCOMPARE( serialPortInvalid_->dcd(),  QSerialPort::Signal_Unknown );
  QCOMPARE( serialPortInvalid_->cts(),  QSerialPort::Signal_Unknown );
  QCOMPARE( serialPortInvalid_->dsr(),  QSerialPort::Signal_Unknown );
  QCOMPARE( serialPortInvalid_->ri(),  QSerialPort::Signal_Unknown );

  QVERIFY( !serialPortInvalid_->setDtr() );
  QVERIFY( !serialPortInvalid_->setRts() );
}

void QSerialPortUnitTest::test_validport_set_portSettings()
{
  using namespace TNX;

  QPortSettings portSettings;

  // #1
  #if defined(Q_OS_WIN)
  portSettings.setBaudRate(QPortSettings::BAUDR_110);
  #else
  portSettings.setBaudRate(QPortSettings::BAUDR_150);
  #endif

  portSettings.setDataBits(QPortSettings::DB_7);
  portSettings.setFlowControl(QPortSettings::FLOW_XONXOFF);
  portSettings.setParity(QPortSettings::PAR_ODD);
  portSettings.setStopBits(QPortSettings::STOP_1);

  QVERIFY( serialPortValid_->setPortSettings(portSettings) );
  #if defined(Q_OS_WIN)
  QCOMPARE( serialPortValid_->baudRate(), QPortSettings::BAUDR_110 );
  #else
  QCOMPARE( serialPortValid_->baudRate(), QPortSettings::BAUDR_150 );
  #endif

  QCOMPARE( serialPortValid_->dataBits(), QPortSettings::DB_7 );
  QCOMPARE( serialPortValid_->flowControl(), QPortSettings::FLOW_XONXOFF );
  QCOMPARE( serialPortValid_->parity(), QPortSettings::PAR_ODD );
  QCOMPARE( serialPortValid_->stopBits(), QPortSettings::STOP_1 );

  // #2
  portSettings.setBaudRate(QPortSettings::BAUDR_115200);
  portSettings.setDataBits(QPortSettings::DB_7);
  portSettings.setFlowControl(QPortSettings::FLOW_OFF);
  portSettings.setParity(QPortSettings::PAR_EVEN);
  portSettings.setStopBits(QPortSettings::STOP_2);

  QVERIFY( serialPortValid_->setPortSettings(portSettings) );
  QCOMPARE( serialPortValid_->baudRate(), QPortSettings::BAUDR_115200 );
  QCOMPARE( serialPortValid_->dataBits(), QPortSettings::DB_7 );
  QCOMPARE( serialPortValid_->flowControl(), QPortSettings::FLOW_OFF );
  QCOMPARE( serialPortValid_->parity(), QPortSettings::PAR_EVEN );

  QCOMPARE( serialPortValid_->stopBits(), QPortSettings::STOP_2 );

  // #3
  portSettings.setBaudRate(QPortSettings::BAUDR_300);
  portSettings.setDataBits(QPortSettings::DB_8);
  portSettings.setFlowControl(QPortSettings::FLOW_HARDWARE);
  portSettings.setParity(QPortSettings::PAR_NONE);
  portSettings.setStopBits(QPortSettings::STOP_1);

  QVERIFY( serialPortValid_->setPortSettings(portSettings) );
  QCOMPARE( serialPortValid_->baudRate(), QPortSettings::BAUDR_300 );
  QCOMPARE( serialPortValid_->dataBits(), QPortSettings::DB_8 );
#if !defined(Q_OS_MAC)
  QCOMPARE( serialPortValid_->flowControl(), QPortSettings::FLOW_HARDWARE );
#endif
  QCOMPARE( serialPortValid_->parity(), QPortSettings::PAR_NONE );
  QCOMPARE( serialPortValid_->stopBits(), QPortSettings::STOP_1 );

  // #4
  portSettings.setBaudRate(QPortSettings::BAUDR_38400);
  portSettings.setDataBits(QPortSettings::DB_5);
  portSettings.setFlowControl(QPortSettings::FLOW_OFF);
  portSettings.setParity(QPortSettings::PAR_EVEN);
  portSettings.setStopBits(QPortSettings::STOP_2);

#if defined(Q_OS_WIN)
  QVERIFY( !serialPortValid_->setPortSettings(portSettings) );
#else
  QVERIFY( serialPortValid_->setPortSettings(portSettings) );

  QCOMPARE( serialPortValid_->baudRate(), QPortSettings::BAUDR_38400 );
  QCOMPARE( serialPortValid_->dataBits(), QPortSettings::DB_5 );
  QCOMPARE( serialPortValid_->flowControl(), QPortSettings::FLOW_OFF );
  QCOMPARE( serialPortValid_->parity(), QPortSettings::PAR_EVEN );
  QCOMPARE( serialPortValid_->stopBits(), QPortSettings::STOP_2 );
#endif
}

void QSerialPortUnitTest::test_invalidport_set_portSettings()
{
  using namespace TNX;

  QPortSettings portSettings;

  // #1
  #if defined(Q_OS_WIN)
  portSettings.setBaudRate(QPortSettings::BAUDR_110);
  #else
  portSettings.setBaudRate(QPortSettings::BAUDR_150);
  #endif
  portSettings.setDataBits(QPortSettings::DB_6);
  portSettings.setFlowControl(QPortSettings::FLOW_XONXOFF);
  portSettings.setParity(QPortSettings::PAR_ODD);
  portSettings.setStopBits(QPortSettings::STOP_2);

  QVERIFY( serialPortInvalid_->setPortSettings(portSettings) );
  #if defined(Q_OS_WIN)
  QCOMPARE( serialPortInvalid_->baudRate(), QPortSettings::BAUDR_110 );
  #else
  QCOMPARE( serialPortInvalid_->baudRate(), QPortSettings::BAUDR_150 );
  #endif
  QCOMPARE( serialPortInvalid_->dataBits(), QPortSettings::DB_6 );
  QCOMPARE( serialPortInvalid_->flowControl(), QPortSettings::FLOW_XONXOFF );
  QCOMPARE( serialPortInvalid_->parity(), QPortSettings::PAR_ODD );
  QCOMPARE( serialPortInvalid_->stopBits(), QPortSettings::STOP_2 );

  // #2
  portSettings.setBaudRate(QPortSettings::BAUDR_115200);
  portSettings.setDataBits(QPortSettings::DB_7);
  portSettings.setFlowControl(QPortSettings::FLOW_OFF);
  portSettings.setParity(QPortSettings::PAR_EVEN);
  portSettings.setStopBits(QPortSettings::STOP_1);

  QVERIFY( serialPortInvalid_->setPortSettings(portSettings) );
  QCOMPARE( serialPortInvalid_->baudRate(), QPortSettings::BAUDR_115200 );
  QCOMPARE( serialPortInvalid_->dataBits(), QPortSettings::DB_7 );
  QCOMPARE( serialPortInvalid_->flowControl(), QPortSettings::FLOW_OFF );
  QCOMPARE( serialPortInvalid_->parity(), QPortSettings::PAR_EVEN );
  QCOMPARE( serialPortInvalid_->stopBits(), QPortSettings::STOP_1 );

  // #3
  portSettings.setBaudRate(QPortSettings::BAUDR_300);
  portSettings.setDataBits(QPortSettings::DB_8);
  portSettings.setFlowControl(QPortSettings::FLOW_HARDWARE);
  portSettings.setParity(QPortSettings::PAR_NONE);
  portSettings.setStopBits(QPortSettings::STOP_1);

  QVERIFY( serialPortInvalid_->setPortSettings(portSettings) );
  QCOMPARE( serialPortInvalid_->baudRate(), QPortSettings::BAUDR_300 );
  QCOMPARE( serialPortInvalid_->dataBits(), QPortSettings::DB_8 );
  QCOMPARE( serialPortInvalid_->flowControl(), QPortSettings::FLOW_HARDWARE );
  QCOMPARE( serialPortInvalid_->parity(), QPortSettings::PAR_NONE );
  QCOMPARE( serialPortInvalid_->stopBits(), QPortSettings::STOP_1 );

  // #4
  portSettings.setBaudRate(QPortSettings::BAUDR_38400);
  portSettings.setDataBits(QPortSettings::DB_5);
  portSettings.setFlowControl(QPortSettings::FLOW_OFF);
  portSettings.setParity(QPortSettings::PAR_EVEN);
  portSettings.setStopBits(QPortSettings::STOP_2);

  QVERIFY( serialPortInvalid_->setPortSettings(portSettings) );
  QCOMPARE( serialPortInvalid_->baudRate(), QPortSettings::BAUDR_38400 );
  QCOMPARE( serialPortInvalid_->dataBits(), QPortSettings::DB_5 );
  QCOMPARE( serialPortInvalid_->flowControl(), QPortSettings::FLOW_OFF );
  QCOMPARE( serialPortInvalid_->parity(), QPortSettings::PAR_EVEN );
  QCOMPARE( serialPortInvalid_->stopBits(), QPortSettings::STOP_2 );
}

void QSerialPortUnitTest::test_validport_set_commtimeouts()
{
  using namespace TNX;
  // Mac OSX doesn't allow us to set anything higher than 255

  CommTimeouts commtimeouts;

#if !defined(Q_OS_WIN)

  commtimeouts.PosixVMIN = 0;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortValid_->commTimeouts().PosixVMIN, 0 );

  commtimeouts.PosixVMIN = 255;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortValid_->commTimeouts().PosixVMIN, 255 );

  commtimeouts.PosixVMIN = 128;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortValid_->commTimeouts().PosixVMIN, 128 );

  commtimeouts.PosixVTIME = 0;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortValid_->commTimeouts().PosixVTIME, 0 );

  commtimeouts.PosixVTIME = 255;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortValid_->commTimeouts().PosixVTIME, 255 );

  commtimeouts.PosixVTIME = 128;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortValid_->commTimeouts().PosixVTIME, 128 );

#else
  commtimeouts.Win32ReadIntervalTimeout = 0;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadIntervalTimeout, (DWORD)0 );

  commtimeouts.Win32ReadIntervalTimeout = 1000;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadIntervalTimeout, (DWORD)1000 );

  commtimeouts.Win32ReadIntervalTimeout = -1;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadIntervalTimeout, (DWORD)-1 );

  //
  commtimeouts.Win32ReadTotalTimeoutConstant = 0;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadTotalTimeoutConstant, (DWORD)0 );

  commtimeouts.Win32ReadTotalTimeoutConstant = 1000;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadTotalTimeoutConstant, (DWORD)1000 );

  commtimeouts.Win32ReadTotalTimeoutConstant = -1;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadTotalTimeoutConstant, (DWORD)-1 );

  //
  commtimeouts.Win32ReadTotalTimeoutMultiplier = 0;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadTotalTimeoutMultiplier, (DWORD)0 );

  commtimeouts.Win32ReadTotalTimeoutMultiplier = 1000;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadTotalTimeoutMultiplier, (DWORD)1000 );

  commtimeouts.Win32ReadTotalTimeoutMultiplier = -1;
  //QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  //QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32ReadTotalTimeoutMultiplier, (DWORD)-1 );
  QVERIFY( !serialPortValid_->setCommTimeouts(commtimeouts) );

  // Reset the read timout values
  commtimeouts.Win32ReadTotalTimeoutMultiplier = 0;
  commtimeouts.Win32ReadTotalTimeoutConstant = 0;
  commtimeouts.Win32ReadIntervalTimeout = 0;

  //
  commtimeouts.Win32WriteTotalTimeoutConstant = 0;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32WriteTotalTimeoutConstant, (DWORD)0 );

  commtimeouts.Win32WriteTotalTimeoutConstant = 1000;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32WriteTotalTimeoutConstant, (DWORD)1000 );

  commtimeouts.Win32WriteTotalTimeoutConstant = -1;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32WriteTotalTimeoutConstant, (DWORD)-1 );

  //
  commtimeouts.Win32WriteTotalTimeoutMultiplier = 0;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32WriteTotalTimeoutMultiplier, (DWORD)0 );

  commtimeouts.Win32WriteTotalTimeoutMultiplier = 1000;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32WriteTotalTimeoutMultiplier, (DWORD)1000 );

  commtimeouts.Win32WriteTotalTimeoutMultiplier = -1;
  QCOMPARE( serialPortValid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortValid_->commTimeouts().Win32WriteTotalTimeoutMultiplier, (DWORD)-1 );

#endif

  serialPortValid_->setReadNotifyThreshold(5);
  QCOMPARE( serialPortValid_->readNotifyThreshold(), 5 );

  serialPortValid_->setReadNotifyThreshold(0);
  QCOMPARE( serialPortValid_->readNotifyThreshold(), 1 );

  serialPortValid_->setReadNotifyThreshold(-1);
  QCOMPARE( serialPortValid_->readNotifyThreshold(), 1 );

  serialPortValid_->setReadNotifyThreshold(9999);
  QCOMPARE( serialPortValid_->readNotifyThreshold(), 1024 );
}

void QSerialPortUnitTest::test_invalidport_set_commtimeouts()
{
  using namespace TNX;

  CommTimeouts commtimeouts;

#if !defined(Q_OS_WIN)

  commtimeouts.PosixVMIN = 0;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortInvalid_->commTimeouts().PosixVMIN, 0 );

  commtimeouts.PosixVMIN = 255;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortInvalid_->commTimeouts().PosixVMIN, 255 );

  commtimeouts.PosixVMIN = 128;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortInvalid_->commTimeouts().PosixVMIN, 128 );

  commtimeouts.PosixVTIME = 0;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortInvalid_->commTimeouts().PosixVTIME, 0 );

  commtimeouts.PosixVTIME = 255;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortInvalid_->commTimeouts().PosixVTIME, 255 );

  commtimeouts.PosixVTIME = 128;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (int)serialPortInvalid_->commTimeouts().PosixVTIME, 128 );

#else

  commtimeouts.Win32ReadIntervalTimeout = 0;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadIntervalTimeout, (DWORD)0 );

  commtimeouts.Win32ReadIntervalTimeout = 1000;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadIntervalTimeout, (DWORD)1000 );

  commtimeouts.Win32ReadIntervalTimeout = -1;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadIntervalTimeout, (DWORD)-1 );

  //
  commtimeouts.Win32ReadTotalTimeoutConstant = 0;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadTotalTimeoutConstant, (DWORD)0 );

  commtimeouts.Win32ReadTotalTimeoutConstant = 1000;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadTotalTimeoutConstant, (DWORD)1000 );

  commtimeouts.Win32ReadTotalTimeoutConstant = -1;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadTotalTimeoutConstant, (DWORD)-1 );

  //
  commtimeouts.Win32ReadTotalTimeoutMultiplier = 0;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadTotalTimeoutMultiplier, (DWORD)0 );

  commtimeouts.Win32ReadTotalTimeoutMultiplier = 1000;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadTotalTimeoutMultiplier, (DWORD)1000 );

  commtimeouts.Win32ReadTotalTimeoutMultiplier = -1;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32ReadTotalTimeoutMultiplier, (DWORD)-1 );

  //
  commtimeouts.Win32WriteTotalTimeoutConstant = 0;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32WriteTotalTimeoutConstant, (DWORD)0 );

  commtimeouts.Win32WriteTotalTimeoutConstant = 1000;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32WriteTotalTimeoutConstant, (DWORD)1000 );

  commtimeouts.Win32WriteTotalTimeoutConstant = -1;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32WriteTotalTimeoutConstant, (DWORD)-1 );

  //
  commtimeouts.Win32WriteTotalTimeoutMultiplier = 0;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32WriteTotalTimeoutMultiplier, (DWORD)0 );

  commtimeouts.Win32WriteTotalTimeoutMultiplier = 1000;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32WriteTotalTimeoutMultiplier, (DWORD)1000 );

  commtimeouts.Win32WriteTotalTimeoutMultiplier = -1;
  QCOMPARE( serialPortInvalid_->setCommTimeouts(commtimeouts), true );
  QCOMPARE( (DWORD)serialPortInvalid_->commTimeouts().Win32WriteTotalTimeoutMultiplier, (DWORD)-1 );

#endif

  serialPortInvalid_->setReadNotifyThreshold(5);
  QCOMPARE( serialPortInvalid_->readNotifyThreshold(), 5 );

  serialPortInvalid_->setReadNotifyThreshold(0);
  QCOMPARE( serialPortInvalid_->readNotifyThreshold(), 1 );

  serialPortInvalid_->setReadNotifyThreshold(-1);
  QCOMPARE( serialPortInvalid_->readNotifyThreshold(), 1 );

  serialPortInvalid_->setReadNotifyThreshold(9999);
  QCOMPARE( serialPortInvalid_->readNotifyThreshold(), 1024 );
}

void QSerialPortUnitTest::test_validport_readWrite()
{
  using namespace TNX;

  char data[1024];

  CommTimeouts commTimeouts;

  commTimeouts.PosixVMIN = 0;
  commTimeouts.PosixVTIME = 0;
  commTimeouts.Win32ReadIntervalTimeout = CommTimeouts::NoTimeout;
  commTimeouts.Win32ReadTotalTimeoutMultiplier = 0;
  commTimeouts.Win32ReadTotalTimeoutConstant = 0;
  QVERIFY( serialPortValid_->setCommTimeouts(commTimeouts) );
  QCOMPARE( (int)serialPortValid_->read(data, 1024LL), 0 );

  // when VMIN is set Mac OSX doesn't timeout read operation
  //TODO: hmm LInux behaves the same way too.. Investigate!!

//  commTimeouts.PosixVMIN = 1;
//  commTimeouts.PosixVTIME = 1;
//  commTimeouts.Win32ReadIntervalTimeout = 75;
//  commTimeouts.Win32ReadTotalTimeoutMultiplier = 0;
//  commTimeouts.Win32ReadTotalTimeoutConstant = 200;
//  QVERIFY( serialPortValid_->setCommTimeouts(commTimeouts) );
//  QCOMPARE( (int)serialPortValid_->read(data, 1024LL), 0 );

  commTimeouts.PosixVMIN = 0;
  commTimeouts.PosixVTIME = 10;
  commTimeouts.Win32ReadIntervalTimeout = 75;
  commTimeouts.Win32ReadTotalTimeoutMultiplier = 0;
  commTimeouts.Win32ReadTotalTimeoutConstant = 200;
  QVERIFY( serialPortValid_->setCommTimeouts(commTimeouts) );
  QCOMPARE( (int)serialPortValid_->read(data, 1024LL), 0 );
}

void QSerialPortUnitTest::test_invalidport_readWrite()
{
  using namespace TNX;

  char data[1024];

  CommTimeouts commTimeouts;

  commTimeouts.PosixVMIN = 0;
  commTimeouts.PosixVTIME = 0;
  commTimeouts.Win32ReadIntervalTimeout = CommTimeouts::NoTimeout;
  commTimeouts.Win32ReadTotalTimeoutMultiplier = 0;
  commTimeouts.Win32ReadTotalTimeoutConstant = 0;
  QVERIFY( serialPortValid_->setCommTimeouts(commTimeouts) );
  QCOMPARE( (int)serialPortInvalid_->read(data, 1024LL), -1 );
}

QTEST_MAIN(QSerialPortUnitTest)
