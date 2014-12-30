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
 *   @brief Connection to OpalRT
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#ifndef OPALLINK_H
#define OPALLINK_H

#include <QMutex>
#include <QDebug>
#include <QTextStreamManipulator>
#include <QTimer>
#include <QQueue>
#include <QByteArray>
#include <QObject>
#include <stdlib.h>


#include "LinkInterface.h"
#include "LinkManager.h"
#include "MG.h"
#include "QGCMAVLink.h"
#include "QGCConfig.h"
#include "OpalRT.h"
#include "ParameterList.h"
#include "Parameter.h"
#include "QGCParamID.h"
#include "OpalApi.h"
#include "errno.h"
#include "string.h"

/**
 * @brief Interface to OpalRT targets
 *
 * This is an interface to the OpalRT hardware-in-the-loop (HIL) simulator.
 * This class receives MAVLink packets as if it is a true link, but it
 * interprets the packets internally, and calls the appropriate api functions.
 *
 * @author Bryan Godbolt <godbolt@ualberta.ca>
 * @ref http://www.opal-rt.com/
 */
class OpalLink : public LinkInterface
{
    Q_OBJECT

public:
    OpalLink();
    /* Connection management */

    QString getName() const;
    bool isConnected() const;

    qint64 getConnectionSpeed() const;
    qint64 getCurrentInDataRate() const;
    qint64 getCurrentOutDataRate() const;

    void run();

    int getOpalInstID() {
        return static_cast<int>(opalInstID);
    }
    
    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

public slots:

    void writeBytes(const char *bytes, qint64 length);

    void readBytes();

    void heartbeat();

    void getSignals();

    void setOpalInstID(int instID) {
        opalInstID = static_cast<unsigned short>(instID);
    }

protected slots:

    void receiveMessage(mavlink_message_t message);
    void setSignals(double *values);

protected:
    QString name;
    int id;
    bool connectState;

    quint64 connectionStartTime;

    QMutex receiveDataMutex;

    void setName(QString name);

    QTimer* heartbeatTimer;    ///< Timer to emit heartbeats
    int heartbeatRate;         ///< Heartbeat rate, controls the timer interval
    bool m_heartbeatsEnabled;  ///< Enabled/disable heartbeat emission

    QTimer* getSignalsTimer;
    int getSignalsPeriod;

    QQueue<QByteArray>* receiveBuffer;
    QByteArray* sendBuffer;

    const int systemID;
    const int componentID;

    void getParameterList();
    OpalRT::ParameterList *params;

    unsigned short opalInstID;

    uint16_t duty2PulseMicros(double duty);
    uint8_t rescaleNorm(double norm, int ch);
    int8_t rescaleControllerOutput(double raw);

    bool sendRCValues;
    bool sendRawController;
    bool sendPosition;
    
private:
    // From LinkInterface
    virtual bool _connect(void);
    virtual bool _disconnect(void);
};

#endif // OPALLINK_H
