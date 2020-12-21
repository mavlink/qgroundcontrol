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
#include "QGCPalette.h"
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
    virtual ~MavlinkConsoleController();

    Q_INVOKABLE void sendCommand(QString command);

    Q_INVOKABLE QString historyUp(const QString& current);
    Q_INVOKABLE QString historyDown(const QString& current);

    /**
     * Get clipboard data and if it has N lines, execute first N-1 commands
     * @param command_pre prefix to data from clipboard
     * @return last line of the clipboard data
     */
    Q_INVOKABLE QString handleClipboard(const QString& command_pre);

    Q_PROPERTY(QString text                     READ getText                    CONSTANT)

private slots:
    void _setActiveVehicle  (Vehicle* vehicle);
    void _receiveData(uint8_t device, uint8_t flags, uint16_t timeout, uint32_t baudrate, QByteArray data);

private:
    bool _processANSItext(QByteArray &line);
    void _sendSerialData(QByteArray, bool close = false);
    void writeLine(int line, const QByteArray &text);

    QString transformLineForRichText(const QString& line) const;

    QString getText() const;

    class CommandHistory
    {
    public:
        void append(const QString& command);
        QString up(const QString& current);
        QString down(const QString& current);
    private:
        static constexpr int maxHistoryLength = 100;
        QList<QString> _history;
        int _index = 0;
    };

    static constexpr int _max_num_lines = 500; ///< history size (affects CPU load)

    int           _cursor_home_pos{-1};
    int           _cursorY{0};
    int           _cursorX{0};
    QByteArray    _incoming_buffer;
    Vehicle*      _vehicle{nullptr};
    QList<QMetaObject::Connection> _uas_connections;
    CommandHistory _history;
    QGCPalette     _palette;
};
