/**
 * @file SerialComm.h
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

// TODO: include to be removed
#include "TyphoonHCommon.h"

// Only to be compiled on ST16 hardware (Android x86)
#if defined(__androidx86__)

#include <termios.h>
#include <functional>
#include <vector>

class HelperInterface;

class M4SerialComm : public QObject
{
    Q_OBJECT
public:
    M4SerialComm  (HelperInterface& helper);
    ~M4SerialComm ();
    bool        init    (std::string port, int baud);
    bool        open    ();
    void        close   ();
    bool        write   (std::vector<uint8_t> data, bool debug = false);
    bool        write   (void* data, int length);
    void        tryRead ();
    void        setBytesReadyCallback(std::function<void(std::vector<uint8_t>)> callback);

private:
    int         _openPort       (const char* port);
    int         _writePort      (void *buffer, int len);
    bool        _setupPort      (int baud);
    void        _readPacket     (uint8_t length);
    bool        _readData       (void *buffer, int len);

private:
    enum class SerialPortState {
        CLOSED,
        OPEN,
        ERROR
    };
    enum class PacketState {
        NONE,
        FIRST_ID,
        SECOND_ID
    };

    HelperInterface& _helper;
    int         _fd = -1;
    int         _baudrate = 230400;
    SerialPortState _serialPortStatus = SerialPortState::CLOSED;
    std::string     _uart_name {};
    PacketState _currentPacketStatus = PacketState::NONE;
    struct termios  _savedtio;
    std::function<void(std::vector<uint8_t>)> _bytesReadyCallback = nullptr;
};

#endif
