/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Definition of the configuration Window for Slugs' HIL Simulator
 *   @author Mariano Lizarraga <malife@gmail.com>
 */

#ifndef SLUGSHILSIM_H
#define SLUGSHILSIM_H

#include <QWidget>
#include <QHostAddress>
#include <QUdpSocket>
#include <QMessageBox>

#include "LinkInterface.h"
#include "UAS.h"
#include <stdint.h>


namespace Ui {
    class SlugsHilSim;
}

class SlugsHilSim : public QWidget
{
    Q_OBJECT

public:
    explicit SlugsHilSim(QWidget *parent = 0);
    ~SlugsHilSim();

protected:
    LinkInterface* hilLink;
    QHostAddress* simulinkIp;
    QUdpSocket* txSocket;
    QUdpSocket* rxSocket;
    UAS* activeUas;

public slots:
    void linkAdded (void);

    /**
     * @brief Adds a link to the combo box listing so the user can select a link
     *
     * Populates the Combo box that allows the user to select the link with which Slugs will
     * receive the simulated sensor data from Simulink
     *
     * @param theLink the link that is being added to the combo box
     */
    void addToCombo(LinkInterface* theLink);

    /**
     * @brief Puts Slugs in HIL Mode
     *
     * Sends the required messages through the main communication link to set Slugs in HIL Mode
     *
     */
    void putInHilMode(void);

    /**
     * @brief Receives a datagram from Simulink containing the sensor data.
     *
     * Receives a datagram from Simulink containing the simulated sensor data. This data is then
     * forwarded to Slugs to use as input to the attitude estimation and navigation algorithms.
     *
     */
    void readDatagram(void);

    /**
     * @brief Called when the a new UAS is set to active.
     *
     * Called when the a new UAS is set to active.
     *
     * @param uas The new active UAS
     */
    void activeUasSet(UASInterface* uas);


private:

	typedef union _tFloatToChar {
		unsigned char   chData[4];
		float   		flData;
	} tFloatToChar;

	typedef union _tUint16ToChar {
		unsigned char   chData[2];
		uint16_t   		uiData;
	} tUint16ToChar;

    Ui::SlugsHilSim *ui;
    QHash <int, LinkInterface*> linksAvailable;

    void processHilDatagram (const QByteArray* datagram);
    float getFloatFromDatagram (const QByteArray* datagram, unsigned char * i);
    uint16_t getUint16FromDatagram (const QByteArray* datagram, unsigned char * i);

};

#endif // SLUGSHILSIM_H
