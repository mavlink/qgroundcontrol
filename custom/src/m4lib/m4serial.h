/**
 * @file SerialComm.h
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include "TyphoonHCommon.h"
#if defined(__androidx86__)
#include <termios.h>
#endif
class M4SerialComm : public QObject
{
    Q_OBJECT
public:
    M4SerialComm  (QObject* parent = NULL);
    ~M4SerialComm ();
    bool        init    (QString port, int baud);
    bool        open    ();
    void        close   ();
    bool        write   (QByteArray data, bool debug = false);
    bool        write   (void* data, int length);
    void        tryRead ();

private:
    int         _openPort       (const char* port);
    int         _writePort      (void *buffer, int len);
    bool        _setupPort      (int baud);
    void        _readPacket     (uint8_t length);
    bool        _readData       (void *buffer, int len);
signals:
    void        bytesReady      (QByteArray data);
private:
    enum {
        // Status flags
        SERIAL_PORT_CLOSED,
        SERIAL_PORT_OPEN,
        SERIAL_PORT_ERROR
    };
    enum {
        // Packet Status
        PACKET_NONE,
        PACKET_FIRST_ID,
        PACKET_SECOND_ID
    };
    int         _fd;
    int         _baudrate;
    int         _status;
    QString     _uart_name;
    int         _currentPacketStatus;
#if defined(__androidx86__)
    struct termios  _savedtio;
#endif
};

