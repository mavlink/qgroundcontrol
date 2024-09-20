/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>

#include "ADSB.h"

Q_DECLARE_LOGGING_CATEGORY(ADSBTCPLinkLog)

class QTcpSocket;
class QTimer;

/// The ADSBTCPLink class handles the TCP connection to an ADS-B server
/// and processes incoming ADS-B data.
class ADSBTCPLink : public QObject
{
    Q_OBJECT

public:
    /// Constructs an ADSBTCPLink object.
    ///     @param hostAddress The address of the host to connect to.
    ///     @param port The port to connect to on the host.
    ///     @param parent The parent object.
    explicit ADSBTCPLink(const QHostAddress &hostAddress, quint16 port = 30003, QObject *parent = nullptr);

    /// Destroys the ADSBTCPLink object.
    ~ADSBTCPLink();

    /// Attempts connection to a host.
    bool init();

signals:
    /// Emitted when an ADS-B vehicle update is received.
    ///     @param vehicleInfo The updated vehicle information.
    void adsbVehicleUpdate(const ADSB::VehicleInfo_t &vehicleInfo);

    /// Emitted when an error occurs.
    ///     @param errorMsg The error message.
    void errorOccurred(const QString &errorMsg, bool stopped = false);

private slots:
    /// Reads bytes from the TCP socket.
    void _readBytes();

    /// Processes buffered lines of ADS-B data.
    void _processLines();

private:
    /// Parses a line of ADS-B data.
    ///     @param line The line to parse.
    void _parseLine(const QString &line);

    /// Parses and emits the callsign from ADS-B data.
    ///     @param adsbInfo The ADS-B vehicle info structure to update.
    ///     @param values The values parsed from the line.
    void _parseAndEmitCallsign(ADSB::VehicleInfo_t &adsbInfo, const QStringList &values);

    /// Parses and emits the location from ADS-B data.
    ///     @param adsbInfo The ADS-B vehicle info structure to update.
    ///     @param values The values parsed from the line.
    void _parseAndEmitLocation(ADSB::VehicleInfo_t &adsbInfo, const QStringList &values);

    /// Parses and emits the heading from ADS-B data.
    ///     @param adsbInfo The ADS-B vehicle info structure to update.
    ///     @param values The values parsed from the line.
    void _parseAndEmitHeading(ADSB::VehicleInfo_t &adsbInfo, const QStringList &values);

    QHostAddress _hostAddress;
    quint16 _port = 30003;

    QTcpSocket *_socket = nullptr;     ///< Pointer to the TCP socket used for connection
    QTimer *_processTimer = nullptr;   ///< Timer for periodic processing of ADS-B data
    QStringList _lineBuffer;           ///< Buffer for storing incoming lines of ADS-B data

    static constexpr int _processInterval = 50;     ///< Interval for processing lines
    static constexpr int _maxLinesToProcess = 100;  ///< Maximum number of lines to process per timer timeout
};
