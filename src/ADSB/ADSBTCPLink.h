#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "ADSB.h"

class QTcpSocket;

/// \brief The ADSBTCPLink class handles the TCP connection to an ADS-B server
/// and processes incoming ADS-B data.
///
/// The link is designed to be moved to a worker thread so that socket reads and
/// SBS-1 parsing stay off the ui thread. init() must be invoked on the link's
/// thread (e.g. via QThread::started).

class ADSBTCPLink : public QObject
{
    Q_OBJECT

public:
    /// Constructs an ADSBTCPLink object.
    ///     @param hostAddress The IP address or hostname of the host to connect to.
    ///     @param port The port to connect to on the host.
    ///     @param parent The parent object.
    explicit ADSBTCPLink(const QString &hostAddress, quint16 port = 30003, QObject *parent = nullptr);

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
    /// Reads and parses lines from the TCP socket.
    void _readBytes();

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

    QString _hostAddress;
    quint16 _port = 30003;

    QTcpSocket *_socket = nullptr;     ///< Pointer to the TCP socket used for connection
};
