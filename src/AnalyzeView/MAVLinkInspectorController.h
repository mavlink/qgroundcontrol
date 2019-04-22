/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "MAVLinkProtocol.h"
#include "Vehicle.h"

#include <QObject>
#include <QString>
#include <QDebug>

class MAVLinkInspectorController : public QObject
{
    Q_OBJECT
public:
    MAVLinkInspectorController();
    ~MAVLinkInspectorController();

private slots:
    void _receiveMessage        (LinkInterface* link, mavlink_message_t message);
    void _vehicleAdded          (Vehicle* vehicle);
    void _vehicleRemoved        (Vehicle* vehicle);

private:
    void _reset                 ();

private:
    int         _selectedSystemID       = 0;                    ///< Currently selected system
    int         _selectedComponentID    = 0;                    ///< Currently selected component
    QList<int>  _vehicleIDs;
    QMap<int, mavlink_message_t*>       _uasMessageStorage;     ///< Stores the messages for every UAS
    QMap<int, QMap<int, quint64>*>      _uasLastMessageUpdate;  ///< Stores the time of the last message for each message of each UAS
    QMap<int, QMap<int, float>*>        _uasMessageHz;          ///< Stores the frequency of each message of each UAS
    QMap<int, QMap<int, unsigned int>*> _uasMessageCount;       ///< Stores the message count of each message of each UAS
};
