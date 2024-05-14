/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>

#include "ADSBVehicle.h"

Q_DECLARE_LOGGING_CATEGORY(ADSBTCPLinkLog)

class QTcpSocket;
class QTimer;

namespace ADSB {

/// Enum for ADS-B message types.
enum ADSBMessageType {
    IdentificationAndCategory = 1,
    SurfacePosition = 2,
    AirbornePosition = 3,
    AirborneVelocity = 4,
    SurveillanceAltitude = 5,
    SurveillanceId = 6,
    Unsupported = -1
};

} // namespace ADSB

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
    ADSBTCPLink(const QString &hostAddress, quint16 port, QObject *parent = nullptr);

    /// Destroys the ADSBTCPLink object.
    ~ADSBTCPLink();

signals:
    /// Emitted when an ADS-B vehicle update is received.
    ///     @param vehicleInfo The updated vehicle information.
    void adsbVehicleUpdate(const ADSBVehicle::ADSBVehicleInfo_t &vehicleInfo);

    /// Emitted when an error occurs.
    ///     @param errorMsg The error message.
    void errorOccurred(const QString &errorMsg);

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
    void _parseAndEmitCallsign(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, const QStringList &values);

    /// Parses and emits the location from ADS-B data.
    ///     @param adsbInfo The ADS-B vehicle info structure to update.
    ///     @param values The values parsed from the line.
    void _parseAndEmitLocation(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, const QStringList &values);

    /// Parses and emits the heading from ADS-B data.
    ///     @param adsbInfo The ADS-B vehicle info structure to update.
    ///     @param values The values parsed from the line.
    void _parseAndEmitHeading(ADSBVehicle::ADSBVehicleInfo_t &adsbInfo, const QStringList &values);

    QTcpSocket *m_socket = nullptr;     ///< Pointer to the TCP socket used for connection
    QTimer *m_processTimer = nullptr;   ///< Timer for periodic processing of ADS-B data
    QStringList m_lineBuffer;           ///< Buffer for storing incoming lines of ADS-B data

    static constexpr int s_processInterval = 50;     ///< Interval for processing lines
    static constexpr int s_maxLinesToProcess = 100;  ///< Maximum number of lines to process per timer timeout
};
