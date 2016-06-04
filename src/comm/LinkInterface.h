/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
* @file
*   @brief Brief Description
*
*   @author Lorenz Meier <mavteam@student.ethz.ch>
*
*/

#ifndef _LINKINTERFACE_H_
#define _LINKINTERFACE_H_

#include <QThread>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaType>
#include <QSharedPointer>

#include "QGCMAVLink.h"

class LinkManager;
class LinkConfiguration;

/**
* The link interface defines the interface for all links used to communicate
* with the groundstation application.
*
**/
class LinkInterface : public QThread
{
    Q_OBJECT

    // Only LinkManager is allowed to create/delete or _connect/_disconnect a link
    friend class LinkManager;

public:
    Q_PROPERTY(bool active      READ active         WRITE setActive         NOTIFY activeChanged)

    // Property accessors
    bool active(void)                       { return _active; }
    void setActive(bool active)             { _active = active; emit activeChanged(active); }

    /**
     * @brief Get link configuration
     * @return A pointer to the instance of LinkConfiguration
     **/
    virtual LinkConfiguration* getLinkConfiguration() = 0;

    /* Connection management */

    /**
     * @brief Get the human readable name of this link
     */
    virtual QString getName() const = 0;

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
     * @Enable/Disable data rate collection
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
    uint8_t getMavlinkChannel(void) const { Q_ASSERT(_mavlinkChannelSet); return _mavlinkChannel; }

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
     * @param bytes The pointer to the byte array containing the data
     * @param length The length of the data array
     **/
    void writeBytesSafe(const char *bytes, int length)
    {
        emit _invokeWriteBytes(QByteArray(bytes, length));
    }

private slots:
    virtual void _writeBytes(const QByteArray) = 0;
    
signals:
    void autoconnectChanged(bool autoconnect);
    void activeChanged(bool active);
    void _invokeWriteBytes(QByteArray);

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
     * @param data the new bytes
     */
    void bytesReceived(LinkInterface* link, QByteArray data);

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

    /** @brief Communication error occured */
    void communicationError(const QString& title, const QString& error);

    void communicationUpdate(const QString& linkname, const QString& text);

protected:
    // Links are only created by LinkManager so constructor is not public
    LinkInterface() :
        QThread(0)
        , _mavlinkChannelSet(false)
        , _active(false)
        , _enableRateCollection(false)
    {
        // Initialize everything for the data rate calculation buffers.
        _inDataIndex  = 0;
        _outDataIndex = 0;
        
        // Initialize our data rate buffers.
        memset(_inDataWriteAmounts, 0, sizeof(_inDataWriteAmounts));
        memset(_inDataWriteTimes,   0, sizeof(_inDataWriteTimes));
        memset(_outDataWriteAmounts,0, sizeof(_outDataWriteAmounts));
        memset(_outDataWriteTimes,  0, sizeof(_outDataWriteTimes));
        
        QObject::connect(this, &LinkInterface::_invokeWriteBytes, this, &LinkInterface::_writeBytes);
        qRegisterMetaType<LinkInterface*>("LinkInterface*");
    }

    /// This function logs the send times and amounts of datas for input. Data is used for calculating
    /// the transmission rate.
    ///     @param byteCount Number of bytes received
    ///     @param time Time in ms send occured
    void _logInputDataRate(quint64 byteCount, qint64 time) {
        if(_enableRateCollection)
            _logDataRateToBuffer(_inDataWriteAmounts, _inDataWriteTimes, &_inDataIndex, byteCount, time);
    }
    
    /// This function logs the send times and amounts of datas for output. Data is used for calculating
    /// the transmission rate.
    ///     @param byteCount Number of bytes sent
    ///     @param time Time in ms receive occured
    void _logOutputDataRate(quint64 byteCount, qint64 time) {
        if(_enableRateCollection)
            _logDataRateToBuffer(_outDataWriteAmounts, _outDataWriteTimes, &_outDataIndex, byteCount, time);
    }
    
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
    void _logDataRateToBuffer(quint64 *bytesBuffer, qint64 *timeBuffer, int *writeIndex, quint64 bytes, qint64 time)
    {
        QMutexLocker dataRateLocker(&_dataRateMutex);
        
        int i = *writeIndex;

        // Now write into the buffer, if there's no room, we just overwrite the first data point.
        bytesBuffer[i] = bytes;
        timeBuffer[i] = time;

        // Increment and wrap the write index
        ++i;
        if (i == _dataRateBufferSize)
        {
            i = 0;
        }
        *writeIndex = i;
    }

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
    qint64 _getCurrentDataRate(int index, const qint64 dataWriteTimes[], const quint64 dataWriteAmounts[]) const
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();

        // Limit the time we calculate to the recent past
        const qint64 cutoff = now - _dataRateCurrentTimespan;

        // Grab the mutex for working with the stats variables
        QMutexLocker dataRateLocker(&_dataRateMutex);

        // Now iterate through the buffer of all received data packets adding up all values
        // within now and our cutof.
        qint64 totalBytes = 0;
        qint64 totalTime = 0;
        qint64 lastTime = 0;
        int size = _dataRateBufferSize;
        while (size-- > 0)
        {
            // If this data is within our cutoff time, include it in our calculations.
            // This also accounts for when the buffer is empty and filled with 0-times.
            if (dataWriteTimes[index] > cutoff && lastTime > 0) {
                // Track the total time, using the previous time as our timeperiod.
                totalTime += dataWriteTimes[index] - lastTime;
                totalBytes += dataWriteAmounts[index];
            }

            // Track the last time sample for doing timespan calculations
            lastTime = dataWriteTimes[index];

            // Increment and wrap the index if necessary.
            if (++index == _dataRateBufferSize)
            {
                index = 0;
            }
        }

        // Return the final calculated value in bits / s, converted from bytes/ms.
        qint64 dataRate = (totalTime != 0)?(qint64)((float)totalBytes * 8.0f / ((float)totalTime / 1000.0f)):0;

        // Finally return our calculated data rate.
        return dataRate;
    }

    /**
     * @brief Connect this interface logically
     *
     * @return True if connection could be established, false otherwise
     **/
    virtual bool _connect(void) = 0;

    virtual void _disconnect(void) = 0;
    
    /// Sets the mavlink channel to use for this link
    void _setMavlinkChannel(uint8_t channel) { Q_ASSERT(!_mavlinkChannelSet); _mavlinkChannelSet = true; _mavlinkChannel = channel; }
    
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

    bool _active;       ///< true: link is actively receiving mavlink messages
    bool _enableRateCollection;
};

typedef QSharedPointer<LinkInterface> SharedLinkInterface;

#endif // _LINKINTERFACE_H_
