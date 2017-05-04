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
#include <QThread>
#include <QFileInfoList>
#include <QElapsedTimer>
#include <QDebug>
#include <QGeoCoordinate>
#include <QMetaObject>


// Fordward decls
class Vehicle;


/// Controller for MavlinkConsole.qml.
class MavlinkConsoleController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int     cursor     READ cursor        NOTIFY cursorChanged)
    Q_PROPERTY(QString text       READ text          NOTIFY textChanged)

public:
    MavlinkConsoleController();
    ~MavlinkConsoleController();

    int  cursor()    const { return _console_text.size(); }
    QString text()   const { return _console_text; }
public slots:
    void sendCommand(QString command);

signals:
    void cursorChanged(int);
    void textChanged(QString text);

private slots:
    void _setActiveVehicle  (Vehicle* vehicle);
    void _receiveData(uint8_t device, uint8_t flags, uint16_t timeout, uint32_t baudrate, QByteArray data);

private:
    void _processANSItext();
    void _sendSerialData(QByteArray, bool close = false);

    int           _cursor_home_pos;
    int           _cursor;
    QByteArray    _incoming_buffer;
    QString       _console_text;
    Vehicle*      _vehicle;
    QList<QMetaObject::Connection> _uas_connections;

};
