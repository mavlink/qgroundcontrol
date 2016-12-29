/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LinkInterface.h"
#include "QGCApplication.h"

/// mavlink channel to use for this link, as used by mavlink_parse_char. The mavlink channel is only
/// set into the link when it is added to LinkManager
uint8_t LinkInterface::mavlinkChannel(void) const
{
    if (!_mavlinkChannelSet) {
        qWarning() << "Call to LinkInterface::mavlinkChannel with _mavlinkChannelSet == false";
    }
    return _mavlinkChannel;
}
// Links are only created by LinkManager so constructor is not public
LinkInterface::LinkInterface(SharedLinkConfigurationPointer& config)
    : QThread(0)
    , _config(config)
    , _mavlinkChannelSet(false)
    , _active(false)
    , _enableRateCollection(false)
    , _decodedFirstMavlinkPacket(false)
{
    _config->setLink(this);

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
///     @param time Time in ms send occurred
void LinkInterface::_logInputDataRate(quint64 byteCount, qint64 time) {
    if(_enableRateCollection)
        _logDataRateToBuffer(_inDataWriteAmounts, _inDataWriteTimes, &_inDataIndex, byteCount, time);
}

/// This function logs the send times and amounts of datas for output. Data is used for calculating
/// the transmission rate.
///     @param byteCount Number of bytes sent
///     @param time Time in ms receive occurred
void LinkInterface::_logOutputDataRate(quint64 byteCount, qint64 time) {
    if(_enableRateCollection)
        _logDataRateToBuffer(_outDataWriteAmounts, _outDataWriteTimes, &_outDataIndex, byteCount, time);
}

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
void LinkInterface::_logDataRateToBuffer(quint64 *bytesBuffer, qint64 *timeBuffer, int *writeIndex, quint64 bytes, qint64 time)
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
qint64 LinkInterface::_getCurrentDataRate(int index, const qint64 dataWriteTimes[], const quint64 dataWriteAmounts[]) const
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

/// Sets the mavlink channel to use for this link
void LinkInterface::_setMavlinkChannel(uint8_t channel)
{
    Q_ASSERT(!_mavlinkChannelSet);
    _mavlinkChannelSet = true;
    _mavlinkChannel = channel;
}
