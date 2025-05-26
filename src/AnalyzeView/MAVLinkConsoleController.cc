/****************************************************************************
 *
 *   (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkConsoleController.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QGCPalette.h"
#include "Vehicle.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QClipboard>

QGC_LOGGING_CATEGORY(MAVLinkConsoleControllerLog, "qgc.analyzeview.mavlinkconsolecontroller")

MAVLinkConsoleController::MAVLinkConsoleController(QObject *parent)
    : QStringListModel(parent)
    , _palette(new QGCPalette(this))
{
    // qCDebug(MAVLinkConsoleControllerLog) << Q_FUNC_INFO << this;

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &MAVLinkConsoleController::_setActiveVehicle);

    _setActiveVehicle(MultiVehicleManager::instance()->activeVehicle());
}

MAVLinkConsoleController::~MAVLinkConsoleController()
{
    if (_vehicle) {
        QByteArray msg;
        _sendSerialData(msg, true);
    }

    // qCDebug(MAVLinkConsoleControllerLog) << Q_FUNC_INFO << this;
}

void MAVLinkConsoleController::sendCommand(const QString &command)
{
    QString output = command;

    // there might be multiple commands, add them separately to the history
    const QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (!line.isEmpty()) {
            _history.append(line);
        }
    }

    (void) output.append("\n");
    _sendSerialData(qPrintable(output));
    _cursorHomePos = -1;
}

QString MAVLinkConsoleController::handleClipboard(const QString &command_pre)
{
    QString clipboardData = command_pre + QGuiApplication::clipboard()->text();

    const int lastLinePos = clipboardData.lastIndexOf('\n');
    if (lastLinePos != -1) {
        const QString commands = clipboardData.mid(0, lastLinePos);
        sendCommand(commands);
        clipboardData = clipboardData.mid(lastLinePos + 1);
    }

    return clipboardData;
}

void MAVLinkConsoleController::_setActiveVehicle(Vehicle *vehicle)
{
    for (QMetaObject::Connection &con : _connections) {
        (void) disconnect(con);
    }
    _connections.clear();

    _vehicle = vehicle;
    if (_vehicle) {
        _incomingBuffer.clear();
        // Reset the model
        setStringList(QStringList());
        _cursorY = 0;
        _cursorX = 0;
        _cursorHomePos = -1;
        _connections << connect(_vehicle, &Vehicle::mavlinkSerialControl, this, &MAVLinkConsoleController::_receiveData);
    }
}

void MAVLinkConsoleController::_receiveData(uint8_t device, uint8_t flags, uint16_t timeout, uint32_t baudrate, const QByteArray &data)
{
    Q_UNUSED(flags); Q_UNUSED(timeout); Q_UNUSED(baudrate);

    if (device != SERIAL_CONTROL_DEV_SHELL) {
        return;
    }

    // Append incoming data and parse for ANSI codes
    (void) _incomingBuffer.append(data);

    while (!_incomingBuffer.isEmpty()) {
        bool newline = false;

        int idx = _incomingBuffer.indexOf('\n');
        if (idx == -1) {
            // Read the whole incoming buffer
            idx = _incomingBuffer.size();
        } else {
            newline = true;
        }

        QByteArray fragment = _incomingBuffer.mid(0, idx);
        if (!_processANSItext(fragment)) {
            // ANSI processing failed, need more data
            return;
        }

        _writeLine(_cursorY, fragment);
        if (newline) {
            _cursorY++;
            _cursorX = 0;
            // ensure line exists
            const int rc = rowCount();
            if (_cursorY >= rc) {
                (void) insertRows(rc, 1 + _cursorY - rc);
            }
        }

        (void) _incomingBuffer.remove(0, idx + (newline ? 1 : 0));
    }
}

void MAVLinkConsoleController::_sendSerialData(const QByteArray &data, bool close)
{
    if (!_vehicle) {
        qCWarning(MAVLinkConsoleControllerLog) << "Internal error";
        return;
    }

    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    // Send maximum sized chunks until the complete buffer is transmitted
    QByteArray output(data);
    while (output.size()) {
        QByteArray chunk(data.left(MAVLINK_MSG_SERIAL_CONTROL_FIELD_DATA_LEN));
        const int dataSize = chunk.size();

        // Ensure the buffer is large enough, as the MAVLink parser expects MAVLINK_MSG_SERIAL_CONTROL_FIELD_DATA_LEN bytes
        (void) chunk.append(MAVLINK_MSG_SERIAL_CONTROL_FIELD_DATA_LEN - chunk.size(), '\0');

        const uint8_t flags = close ? 0 : SERIAL_CONTROL_FLAG_EXCLUSIVE |  SERIAL_CONTROL_FLAG_RESPOND | SERIAL_CONTROL_FLAG_MULTI;

        mavlink_message_t msg;
        (void) mavlink_msg_serial_control_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            SERIAL_CONTROL_DEV_SHELL,
            flags,
            0,
            0,
            dataSize,
            reinterpret_cast<uint8_t*>(chunk.data()),
            _vehicle->id(),
            _vehicle->defaultComponentId()
        );

        (void) _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        (void) output.remove(0, chunk.size());
    }
}

bool MAVLinkConsoleController::_processANSItext(QByteArray &line)
{
    // Iterate over the incoming buffer to parse off known ANSI control codes
    for (int i = 0; i < line.size(); i++) {
        if (line[i] != '\x1B') {
            continue;
        }

        // For ANSI codes we expect at least 3 incoming chars
        if ((i >= (line.size() - 2)) || (line[i + 1] != '[')) {
            // We can reasonably expect a control code was fragemented
            // Stop parsing here and wait for it to come in
            return false;
        }

        switch (line[i + 2]) {
        case 'H':
            if (_cursorHomePos == -1) {
                // Assign new home position if home is unset
                _cursorHomePos = _cursorY;
            } else {
                // Rewind write cursor position to home
                _cursorY = _cursorHomePos;
                _cursorX = 0;
            }
            break;
        case 'K':
            // Erase the current line to the end
            if (_cursorY < rowCount()) {
                const QModelIndex idx = index(_cursorY);
                QString updated = data(idx, Qt::DisplayRole).toString();
                const int eraseIdx = _cursorX + i;
                if (eraseIdx < updated.length()) {
                    (void) setData(idx, updated.remove(eraseIdx, updated.length()));
                }
            }
            break;
        case '2':
            // Check for sufficient buffer size
            if (i >= (line.size() - 3)) {
                return false;
            }

            if ((line[i + 3] == 'J') && (_cursorHomePos != -1)) {
                // Erase everything and rewind to home
                const bool blocked = blockSignals(true);
                for (int j = _cursorHomePos; j < rowCount(); j++) {
                    (void) setData(index(j), "");
                }
                (void) blockSignals(blocked);

                const QVector<int> roles({Qt::DisplayRole, Qt::EditRole});
                emit dataChanged(index(_cursorY), index(rowCount()), roles);
            }

            // Even if we didn't understand this ANSI code, remove the 4th char
            (void) line.remove(i + 3,1);
            break;
        default:
            continue;
        }

        // Remove the parsed ANSI code and decrement the bufferpos
        (void) line.remove(i, 3);
        i--;
    }

    return true;
}

QString MAVLinkConsoleController::_transformLineForRichText(const QString &line) const
{
    QString ret = line.toHtmlEscaped().replace(" ","&nbsp;").replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");

    if (ret.startsWith("WARN", Qt::CaseSensitive)) {
        (void) ret.replace(0, 4, "<font color=\"" + _palette->colorOrange().name() + "\">WARN</font>");
    } else if (ret.startsWith("ERROR", Qt::CaseSensitive)) {
        (void) ret.replace(0, 5, "<font color=\"" + _palette->colorRed().name() + "\">ERROR</font>");
    }

    return ret;
}

QString MAVLinkConsoleController::_getText() const
{
    QString ret;
    if (rowCount() > 0) {
        ret = _transformLineForRichText(data(index(0), Qt::DisplayRole).toString());
    }

    for (int i = 1; i < rowCount(); ++i) {
        ret += ("<br>" + _transformLineForRichText(data(index(i), Qt::DisplayRole).toString()));
    }

    return ret;
}

void MAVLinkConsoleController::_writeLine(int line, const QByteArray &text)
{
    const int rc = rowCount();
    if (line >= rc) {
        (void) insertRows(rc, 1 + line - rc);
    }

    if (rowCount() > kMaxNumLines) {
        const int count = rowCount() - kMaxNumLines;
        (void) removeRows(0, count);
        line -= count;
        _cursorY -= count;
        _cursorHomePos -= count;
        if (_cursorHomePos < 0) {
            _cursorHomePos = -1;
        }
    }

    const QModelIndex idx = index(line);
    QString updated = data(idx, Qt::DisplayRole).toString();
    (void) updated.replace(_cursorX, text.size(), text);
    (void) setData(idx, updated);
    _cursorX += text.size();
}

void MAVLinkConsoleController::CommandHistory::append(const QString &command)
{
    if (!command.isEmpty()) {
        // do not append duplicates
        if ((_history.isEmpty()) || (_history.last() != command)) {
            if (_history.length() >= CommandHistory::kMaxHistoryLength) {
                _history.removeFirst();
            }
            _history.append(command);
        }
    }

    _index = _history.length();
}

QString MAVLinkConsoleController::CommandHistory::up(const QString &current)
{
    if (_index <= 0) {
        return current;
    }

    --_index;
    if (_index < _history.length()) {
        return _history[_index];
    }

    return QStringLiteral("");
}

QString MAVLinkConsoleController::CommandHistory::down(const QString &current)
{
    if (_index >= _history.length()) {
        return current;
    }

    ++_index;
    if (_index < _history.length()) {
        return _history[_index];
    }

    return QStringLiteral("");
}
