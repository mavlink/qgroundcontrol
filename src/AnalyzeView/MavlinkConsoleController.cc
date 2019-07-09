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
    : QStringListModel(),
      _cursor_home_pos{-1},
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
        QByteArray msg;
        _sendSerialData(msg, true);
    }
}

void
MavlinkConsoleController::sendCommand(QString command)
{
    _history.append(command);
    command.append("\n");
    _sendSerialData(qPrintable(command));
    _cursor_home_pos = -1;
    _cursor = rowCount();
}

QString
MavlinkConsoleController::historyUp(const QString& current)
{
    return _history.up(current);
}

QString
MavlinkConsoleController::historyDown(const QString& current)
{
    return _history.down(current);
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
        // Reset the model
        setStringList(QStringList());
        _cursor = 0;
        _cursor_home_pos = -1;
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
    while(!_incoming_buffer.isEmpty()) {
        bool newline = false;
        int idx = _incoming_buffer.indexOf('\n');
        if (idx == -1) {
            // Read the whole incoming buffer
            idx = _incoming_buffer.size();
        } else {
            newline = true;
        }

        QByteArray fragment = _incoming_buffer.mid(0, idx);
        if (_processANSItext(fragment)) {
            writeLine(_cursor, fragment);
            if (newline)
                _cursor++;
            _incoming_buffer.remove(0, idx + (newline ? 1 : 0));
        } else {
            // ANSI processing failed, need more data
            return;
        }
    }
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

bool
MavlinkConsoleController::_processANSItext(QByteArray &line)
{
    // Iterate over the incoming buffer to parse off known ANSI control codes
    for (int i = 0; i < line.size(); i++) {
        if (line.at(i) == '\x1B') {
            // For ANSI codes we expect at least 3 incoming chars
            if (i < line.size() - 2 && line.at(i+1) == '[') {
                // Parse ANSI code
                switch(line.at(i+2)) {
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
                        if (_cursor < rowCount()) {
                            setData(index(_cursor), "");
                        }
                        break;
                    case '2':
                        // Check for sufficient buffer size
                        if ( i >= line.size() - 3)
                            return false;

                        if (line.at(i+3) == 'J' && _cursor_home_pos != -1) {
                            // Erase everything and rewind to home
                            bool blocked = blockSignals(true);
                            for (int j = _cursor_home_pos; j < rowCount(); j++)
                                setData(index(j), "");
                            blockSignals(blocked);
                            QVector<int> roles;
                            roles.reserve(2);
                            roles.append(Qt::DisplayRole);
                            roles.append(Qt::EditRole);
                            emit dataChanged(index(_cursor), index(rowCount()), roles);
                        }
                        // Even if we didn't understand this ANSI code, remove the 4th char
                        line.remove(i+3,1);
                        break;
                }
                // Remove the parsed ANSI code and decrement the bufferpos
                line.remove(i, 3);
                i--;
            } else {
                // We can reasonably expect a control code was fragemented
                // Stop parsing here and wait for it to come in
                return false;
            }
        }
    }
    return true;
}

void
MavlinkConsoleController::writeLine(int line, const QByteArray &text)
{
    auto rc = rowCount();
    if (line >= rc) {
        insertRows(rc, 1 + line - rc);
    }
    auto idx = index(line);
    setData(idx, data(idx, Qt::DisplayRole).toString() + text);
}

void MavlinkConsoleController::CommandHistory::append(const QString& command)
{
    if (command.length() > 0) {

        // do not append duplicates
        if (_history.length() == 0 || _history.last() != command) {

            if (_history.length() >= maxHistoryLength) {
                _history.removeFirst();
            }
            _history.append(command);
        }
    }
    _index = _history.length();
}

QString MavlinkConsoleController::CommandHistory::up(const QString& current)
{
    if (_index <= 0)
        return current;

    --_index;
    if (_index < _history.length()) {
        return _history[_index];
    }
    return "";
}

QString MavlinkConsoleController::CommandHistory::down(const QString& current)
{
    if (_index >= _history.length())
        return current;

    ++_index;
    if (_index < _history.length()) {
        return _history[_index];
    }
    return "";
}
