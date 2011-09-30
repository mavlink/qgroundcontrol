/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef SERIALLINKINTERFACE_H_
#define SERIALLINKINTERFACE_H_

#include <QObject>
#include <QString>
#include <QVector>
#include <LinkInterface.h>

class SerialLinkInterface : public LinkInterface
{
    Q_OBJECT

public:
    virtual QVector<QString>* getCurrentPorts() = 0;
    virtual QString getPortName() = 0;
    virtual int getBaudRate() = 0;
    virtual int getDataBits() = 0;
    virtual int getStopBits() = 0;

    virtual int getBaudRateType() = 0;
    virtual int getFlowType() = 0;
    virtual int getParityType() = 0;
    virtual int getDataBitsType() = 0;
    virtual int getStopBitsType() = 0;

public slots:
    virtual bool setPortName(QString portName) = 0;
    virtual bool setBaudRate(int rate) = 0;
    virtual bool setBaudRateType(int rateIndex) = 0;
    virtual bool setFlowType(int flow) = 0;
    virtual bool setParityType(int parity) = 0;
    virtual bool setDataBitsType(int dataBits) = 0;
    virtual bool setStopBitsType(int stopBits) = 0;
    virtual void loadSettings() = 0;
    virtual void writeSettings() = 0;

};


/* Declare C++ interface as Qt interface */
//Q_DECLARE_INTERFACE(SerialLinkInterface, "org.openground.comm.links.SerialLinkInterface/1.0")

#endif // SERIALLINKINTERFACE_H_
