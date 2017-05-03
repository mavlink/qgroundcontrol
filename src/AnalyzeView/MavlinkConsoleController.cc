/****************************************************************************
 *
 *   (c) 2009-2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MavlinkConsoleController.h"
#include "QGCApplication.h"
#include "UAS.h"

MavlinkConsoleController::MavlinkConsoleController()
    : _cursor_home_pos{-1},
      _cursor{0},
      _vehicle{nullptr}
{
    auto *manager = qgcApp()->toolbox()->multiVehicleManager();
    connect(manager, &MultiVehicleManager::activeVehicleChanged, this, &MavlinkConsoleController::_setActiveVehicle);
    _setActiveVehicle(manager->activeVehicle());
}

MavlinkConsoleController::~MavlinkConsoleController()
{
    if (_vehicle) {
        QByteArray msg("");
        _sendSerialData(msg, true);
    }
}

void
MavlinkConsoleController::sendCommand(QString command)
{
    command.append("\n");
    _sendSerialData(qPrintable(command));
    _cursor_home_pos = -1;
    _cursor = _console_text.length();
}

void
MavlinkConsoleController::_setActiveVehicle(Vehicle* vehicle)
{
    for (auto &con : _uas_connections)
        disconnect(con);
    _uas_connections.clear();

    _vehicle = vehicle;

    if (_vehicle) {
        _incoming_buffer.clear();
        _console_text.clear();
        emit cursorChanged(0);
        emit textChanged(_console_text);
        _uas_connections << connect(_vehicle, &Vehicle::mavlinkSerialControl, this, &MavlinkConsoleController::_receiveData);
    }
}

void
MavlinkConsoleController::_receiveData(uint8_t device, uint8_t, uint16_t, uint32_t, QByteArray data)
{
    if (device != SERIAL_CONTROL_DEV_SHELL)
        return;

    // Append incoming data and parse for ANSI codes
    _incoming_buffer.append(data);
    auto old_size = _console_text.size();
    _processANSItext();
    auto new_size = _console_text.size();

    // Update QML and cursor
    if (old_size > new_size) {
        // Rewind back so we don't get a warning to stderr
        emit cursorChanged(new_size);
    }
    emit textChanged(_console_text);
    emit cursorChanged(new_size);
}

void
MavlinkConsoleController::_sendSerialData(QByteArray data, bool close)
{
    if (!_vehicle) {
        qWarning() << "Internal error";
        return;
    }

    // Send maximum sized chunks until the complete buffer is transmitted
    while(data.size()) {
        QByteArray chunk{data.left(MAVLINK_MSG_SERIAL_CONTROL_FIELD_DATA_LEN)};
        uint8_t flags = SERIAL_CONTROL_FLAG_EXCLUSIVE |  SERIAL_CONTROL_FLAG_RESPOND | SERIAL_CONTROL_FLAG_MULTI;
        if (close) flags = 0;
        auto protocol = qgcApp()->toolbox()->mavlinkProtocol();
        auto priority_link = _vehicle->priorityLink();
        mavlink_message_t msg;
        mavlink_msg_serial_control_pack_chan(
                    protocol->getSystemId(),
                    protocol->getComponentId(),
                    priority_link->mavlinkChannel(),
                    &msg,
                    SERIAL_CONTROL_DEV_SHELL,
                    flags,
                    0,
                    0,
                    chunk.size(),
                    reinterpret_cast<uint8_t*>(chunk.data()));
        _vehicle->sendMessageOnLink(priority_link, msg);
        data.remove(0, chunk.size());
    }
}

void
MavlinkConsoleController::_processANSItext()
{
    int i; // Position into the parsed buffer

    // Iterate over the incoming buffer to parse off known ANSI control codes
    for (i = 0; i < _incoming_buffer.size(); i++) {
        if (_incoming_buffer.at(i) == '\x1B') {
            // For ANSI codes we expect at most 4 incoming chars
            if (i < _incoming_buffer.size() - 3 && _incoming_buffer.at(i+1) == '[') {
                // Parse ANSI code
                switch(_incoming_buffer.at(i+2)) {
                    default:
                        continue;
                    case 'H':
                        if (_cursor_home_pos == -1) {
                            // Assign new home position if home is unset
                            _cursor_home_pos = _cursor;
                        } else {
                            // Rewind write cursor position to home
                            _cursor = _cursor_home_pos;
                        }
                        break;
                    case 'K':
                        // Erase the current line to the end
                    {
                        auto next_lf = _console_text.indexOf('\n', _cursor);
                        if (next_lf > 0)
                            _console_text.remove(_cursor, next_lf + 1 - _cursor);
                    }
                        break;
                    case '2':
                        // Erase everything and rewind to home
                        if (_incoming_buffer.at(i+3) == 'J' && _cursor_home_pos != -1) {
                            // Keep newlines so textedit doesn't scroll annoyingly
                            int newlines = _console_text.mid(_cursor_home_pos).count('\n');
                            _console_text.remove(_cursor_home_pos, _console_text.size());
                            _console_text.append(QByteArray(newlines, '\n'));
                            _cursor = _cursor_home_pos;
                        }
                        // Even if we didn't understand this ANSI code, remove the 4th char
                        _incoming_buffer.remove(i+3,1);
                        break;
                }
                // Remove the parsed ANSI code and decrement the bufferpos
                _incoming_buffer.remove(i, 3);
                i--;
            } else {
                // We can reasonably expect a control code was fragemented
                // Stop parsing here and wait for it to come in
                break;
            }
        }
    }

    // Insert the new data and increment the write cursor
    _console_text.insert(_cursor, _incoming_buffer.left(i));
    _cursor += i;

    // Remove written data from the incoming buffer
    _incoming_buffer.remove(0, i);
}
