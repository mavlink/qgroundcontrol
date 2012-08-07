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

#ifdef WIN32
# include <guiddef.h>
#endif
#include <QSerialPort>
#include <QtTest/QtTest>

class QSerialPortUnitTest : public QObject
{
Q_OBJECT
private slots:
  void initTestCase();
  void cleanupTestCase();
  void init();
  void cleanup();
  void test_validport_signals();
  void test_invalidport_signals();
  void test_portSettings();
  void test_validport_set_portSettings();
  void test_invalidport_set_portSettings();
  void test_validport_set_commtimeouts();
  void test_invalidport_set_commtimeouts();
  void test_validport_readWrite();
  void test_invalidport_readWrite();

private:
  TNX::QSerialPort *serialPortInvalid_;
  TNX::QSerialPort *serialPortValid_;
};
