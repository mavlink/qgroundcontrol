/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QmlObjectListModel.h"
#include "Fact.h"
#include "FactMetaData.h"
#include <QObject>
#include <QString>
#include <QMetaObject>
#include <QStringListModel>

// Fordward decls
class Vehicle;

/// Controller for MavlinkConsole.qml.
class MavlinkConsoleController : public QStringListModel
{
    Q_OBJECT

public:
    MavlinkConsoleController();
    ~MavlinkConsoleController();

public slots:
    void sendCommand(QString command);

signals:
    void cursorChanged(int);

private slots:
    void _setActiveVehicle  (Vehicle* vehicle);
    void _receiveData(uint8_t device, uint8_t flags, uint16_t timeout, uint32_t baudrate, QByteArray data);

private:
    bool _processANSItext(QByteArray &line);
    void _sendSerialData(QByteArray, bool close = false);
    void writeLine(int line, const QByteArray &text);

    int           _cursor_home_pos;
    int           _cursor;
    QByteArray    _incoming_buffer;
    Vehicle*      _vehicle;
    QList<QMetaObject::Connection> _uas_connections;

};
