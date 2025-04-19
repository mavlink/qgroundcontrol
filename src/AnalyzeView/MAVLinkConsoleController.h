/****************************************************************************
 *
 *   (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringListModel>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(MAVLinkConsoleControllerLog)

class QGCPalette;
class Vehicle;

class MAVLinkConsoleController : public QStringListModel
{
    Q_OBJECT
    // QML_ELEMENT
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(QString text READ _getText CONSTANT)

    class CommandHistory
    {
    public:
        void append(const QString &command);
        QString up(const QString &current);
        QString down(const QString &current);

    private:
        QList<QString> _history;
        int _index = 0;
        static constexpr int kMaxHistoryLength = 100;
    };

public:
    explicit MAVLinkConsoleController(QObject *parent = nullptr);
    ~MAVLinkConsoleController();

    Q_INVOKABLE void sendCommand(const QString &command);
    Q_INVOKABLE QString historyUp(const QString &current) { return _history.up(current); }
    Q_INVOKABLE QString historyDown(const QString &current) { return _history.down(current); }

    /// Get clipboard data and if it has N lines, execute first N-1 commands
    ///     @param command_pre prefix to data from clipboard
    ///     @return last line of the clipboard data
    Q_INVOKABLE QString handleClipboard(const QString &command_pre);

private slots:
    void _setActiveVehicle(Vehicle *vehicle);
    void _receiveData(uint8_t device, uint8_t flags, uint16_t timeout, uint32_t baudrate, const QByteArray &data);

private:
    bool _processANSItext(QByteArray &line);
    void _sendSerialData(const QByteArray &data, bool close = false);
    void _writeLine(int line, const QByteArray &text);
    QString _transformLineForRichText(const QString &line) const;
    QString _getText() const;

    QGCPalette *_palette = nullptr;
    int _cursorHomePos = -1;
    int _cursorY = 0;
    int _cursorX = 0;
    QByteArray _incomingBuffer;
    QList<QMetaObject::Connection> _connections;
    CommandHistory _history;
    Vehicle *_vehicle = nullptr;

    static constexpr int kMaxNumLines = 500;    ///< history size (affects CPU load)
};
