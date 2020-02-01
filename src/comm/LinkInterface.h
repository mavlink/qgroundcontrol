/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QThread>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaType>
#include <QSharedPointer>
#include <QDebug>
#include <QTimer>

#include "QGCMAVLink.h"
#include "LinkConfiguration.h"
#include "MavlinkMessagesTimer.h"

class LinkManager;

/**
* @brief The link interface defines the interface for all links used to communicate
* with the ground station application.
**/
class LinkInterface : public QThread
{
    Q_OBJECT

    // Only LinkManager is allowed to create/delete or _connect/_disconnect a link
    friend class LinkManager;

public:    
    virtual ~LinkInterface() {
        stopMavlinkMessagesTimer();
        _config->setLink(nullptr);
    }

    Q_PROPERTY(bool active      READ active     NOTIFY activeChanged)
    Q_PROPERTY(bool isPX4Flow   READ isPX4Flow  CONSTANT)

    Q_INVOKABLE bool link_active(int vehicle_id) const;
    Q_INVOKABLE bool getHighLatency(void) const { return _highLatency; }

    // Property accessors
    bool active() const;
    bool isPX4Flow(void) const { return _isPX4Flow; }

    LinkConfiguration* getLinkConfiguration(void) { return _config.data(); }

    /* Connection management */

    /**
     * @brief Get the human readable name of this link
     */
    Q_INVOKABLE virtual QString getName() const = 0;

    virtual void requestReset() = 0;

    /**
     * @brief Determine the connection status
     *
     * @return True if the connection is established, false otherwise
     **/
    virtual bool isConnected() const = 0;

    /* Connection characteristics */

    /**
     * @Brief Get the maximum connection speed for this interface.
     *
     * The nominal data rate is the theoretical maximum data rate of the
     * interface. For 100Base-T Ethernet this would be 100 Mbit/s (100'000'000
     * Bit/s, NOT 104'857'600 Bit/s).
     *
     * @return The nominal data rate of the interface in bit per second, 0 if unknown
     **/
    virtual qint64 getConnectionSpeed() const = 0;
    
    /// @return true: This link is replaying a log file, false: Normal two-way communication link
    virtual bool isLogReplay(void) { return false; }

    /**
     * @Brief Enable/Disable data rate collection
     **/
    void enableDataRate(bool enable)
    {
        _enableRateCollection = enable;
    }

    /**
     * @Brief Get the current incoming data rate.
     *
     * This should be over a short timespan, something like 100ms. A precise value isn't necessary,
     * and this can be filtered, but should be a reasonable estimate of current data rate.
     *
     * @return The data rate of the interface in bits per second, 0 if unknown
     **/
    qint64 getCurrentInputDataRate() const
    {
        return _getCurrentDataRate(_inDataIndex, _inDataWriteTimes, _inDataWriteAmounts);
    }

    /**
     * @Brief Get the current outgoing data rate.
     *
     * This should be over a short timespan, something like 100ms. A precise value isn't necessary,
     * and this can be filtered, but should be a reasonable estimate of current data rate.
     *
     * @return The data rate of the interface in bits per second, 0 if unknown
     **/
    qint64 getCurrentOutputDataRate() const
    {
        return _getCurrentDataRate(_outDataIndex, _outDataWriteTimes, _outDataWriteAmounts);
    }
    
    /// mavlink channel to use for this link, as used by mavlink_parse_char. The mavlink channel is only
    /// set into the link when it is added to LinkManager
    uint8_t mavlinkChannel(void) const;

    /// Returns whether this link is high latency or not. High latency links should only perform
    /// minimal communication with vehicle.
    ///     signals: highLatencyChanged
    bool highLatency(void) const { return _highLatency; }

    bool decodedFirstMavlinkPacket(void) const { return _decodedFirstMavlinkPacket; }
    bool setDecodedFirstMavlinkPacket(bool decodedFirstMavlinkPacket) { return _decodedFirstMavlinkPacket = decodedFirstMavlinkPacket; }

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

public slots:

    /**
     * @brief This method allows to write bytes to the interface.
     *
     * If the underlying communication is packet oriented,
     * one write command equals a datagram. In case of serial
     * communication arbitrary byte lengths can be written. The method ensures
     * thread safety regardless of the underlying LinkInterface implementation.
     *
     * @param bytes:  The pointer to the byte array containing the data
     * @param length: The length of the data array
     **/
    void writeBytesSafe(const char *bytes, int length)
    {
        emit _invokeWriteBytes(QByteArray(bytes, length));
    }

private slots:
    virtual void _writeBytes(const QByteArray) = 0;

    void _activeChanged(bool active, int vehicle_id);
    
signals:
    void autoconnectChanged(bool autoconnect);
    void activeChanged(LinkInterface* link, bool active, int vehicle_id);
    void _invokeWriteBytes(QByteArray);
    void highLatencyChanged(bool highLatency);

    /// Signalled when a link suddenly goes away due to it being removed by for example pulling the cable to the connection.
    void connectionRemoved(LinkInterface* link);

    /**
     * @brief New data arrived
     *
     * The new data is contained in the QByteArray data. The data is copied for each
     * receiving protocol. For high-speed links like image transmission this might
     * affect performance, for control links it is however desirable to directly
     * forward the link data.
     *
     * @param link: Link where the data is coming from
     * @param data: The data received
     */
    void bytesReceived(LinkInterface* link, QByteArray data);

    /**
     * @brief New data has been sent
     * *
     * The new data is contained in the QByteArray data.
     * The data is logged into telemetry logging system
     *
     * @param link: Link used
     * @param data: The data sent
     */
    void bytesSent(LinkInterface* link, QByteArray data);

    /**
     * @brief This signal is emitted instantly when the link is connected
     **/
    void connected();

    /**
     * @brief This signal is emitted instantly when the link is disconnected
     **/
    void disconnected();

    /**
     * @brief This signal is emitted if the human readable name of this link changes
     */
    void nameChanged(QString name);

    /** @brief Communication error occurred */
    void communicationError(const QString& title, const QString& error);

    void communicationUpdate(const QString& linkname, const QString& text);

protected:
    // Links are only created by LinkManager so constructor is not public
    LinkInterface(SharedLinkConfigurationPointer& config, bool isPX4Flow = false);

    /// This function logs the send times and amounts of datas for input. Data is used for calculating
    /// the transmission rate.
    ///     @param byteCount Number of bytes received
    ///     @param time Time in ms send occurred
    void _logInputDataRate(quint64 byteCount, qint64 time);
    
    /// This function logs the send times and amounts of datas for output. Data is used for calculating
    /// the transmission rate.
    ///     @param byteCount Number of bytes sent
    ///     @param time Time in ms receive occurred
    void _logOutputDataRate(quint64 byteCount, qint64 time);

    SharedLinkConfigurationPointer _config;
    bool _highLatency;

private:
    /**
     * @brief logDataRateToBuffer Stores transmission times/amounts for statistics
     *
     * This function logs the send times and amounts of datas to the given circular buffers.
     * This data is used for calculating the transmission rate.
     *
     * @param bytesBuffer[out] The buffer to write the bytes value into.
     * @param timeBuffer[out] The buffer to write the time value into
     * @param writeIndex[out] The write index used for this buffer.
     * @param bytes The amount of bytes transmit.
     * @param time The time (in ms) this transmission occurred.
     */
    void _logDataRateToBuffer(quint64 *bytesBuffer, qint64 *timeBuffer, int *writeIndex, quint64 bytes, qint64 time);

    /**
     * @brief getCurrentDataRate Get the current data rate given a data rate buffer.
     *
     * This function attempts to use the times and number of bytes transmit into a current data rate
     * estimation. Since it needs to use timestamps to get the timeperiods over when the data was sent,
     * this is effectively a global data rate over the last _dataRateBufferSize - 1 data points. Also note
     * that data points older than NOW - dataRateCurrentTimespan are ignored.
     *
     * @param index The first valid sample in the data rate buffer. Refers to the oldest time sample.
     * @param dataWriteTimes The time, in ms since epoch, that each data sample took place.
     * @param dataWriteAmounts The amount of data (in bits) that was transferred.
     * @return The bits per second of data transferrence of the interface over the last [-statsCurrentTimespan, 0] timespan.
     */
    qint64 _getCurrentDataRate(int index, const qint64 dataWriteTimes[], const quint64 dataWriteAmounts[]) const;

    /**
     * @brief Connect this interface logically
     *
     * @return True if connection could be established, false otherwise
     **/
    virtual bool _connect(void) = 0;

    virtual void _disconnect(void) = 0;

    /// Sets the mavlink channel to use for this link
    void _setMavlinkChannel(uint8_t channel);
    
    /**
     * @brief startMavlinkMessagesTimer
     *
     * Start/restart the mavlink messages timer for the specific vehicle.
     * If no timer exists an instance is allocated.
     */
    void startMavlinkMessagesTimer(int vehicle_id);

    /**
     * @brief stopMavlinkMessagesTimer
     *
     * Stop and deallocate the mavlink messages timers for all vehicles if any exists.
     */
    void stopMavlinkMessagesTimer();

    bool _mavlinkChannelSet;    ///< true: _mavlinkChannel has been set
    uint8_t _mavlinkChannel;    ///< mavlink channel to use for this link, as used by mavlink_parse_char
    
    static const int _dataRateBufferSize = 20; ///< Specify how many data points to capture for data rate calculations.
    
    static const qint64 _dataRateCurrentTimespan = 500; ///< Set the maximum age of samples to use for data calculations (ms).
    
    // Implement a simple circular buffer for storing when and how much data was received.
    // Used for calculating the incoming data rate. Use with *StatsBuffer() functions.
    int     _inDataIndex;
    quint64 _inDataWriteAmounts[_dataRateBufferSize]; // In bytes
    qint64  _inDataWriteTimes[_dataRateBufferSize]; // in ms
    
    // Implement a simple circular buffer for storing when and how much data was transmit.
    // Used for calculating the outgoing data rate. Use with *StatsBuffer() functions.
    int     _outDataIndex;
    quint64 _outDataWriteAmounts[_dataRateBufferSize]; // In bytes
    qint64  _outDataWriteTimes[_dataRateBufferSize]; // in ms
    
    mutable QMutex _dataRateMutex; // Mutex for accessing the data rate member variables

    bool _enableRateCollection;
    bool _decodedFirstMavlinkPacket;    ///< true: link has correctly decoded it's first mavlink packet
    bool _isPX4Flow;

    QMap<int /* vehicle id */, MavlinkMessagesTimer*> _mavlinkMessagesTimers;
};

typedef QSharedPointer<LinkInterface> SharedLinkInterfacePointer;

