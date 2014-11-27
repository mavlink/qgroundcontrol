/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

PIXHAWK is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

PIXHAWK is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

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

class LinkManager;

/**
* The link interface defines the interface for all links used to communicate
* with the groundstation application.
*
**/
class LinkInterface : public QThread
{
    Q_OBJECT
    
    // Only LinkManager is allowed to _connect or _disconnect a link
    friend class LinkManager;
    
public:
    LinkInterface() :
        QThread(0),
        _ownedByLinkManager(false),
        _deletedByLinkManager(false)
    {
        // Initialize everything for the data rate calculation buffers.
        inDataIndex = 0;
        outDataIndex = 0;

        // Initialize our data rate buffers manually, cause C++<03 is dumb.
        for (int i = 0; i < dataRateBufferSize; ++i)
        {
            inDataWriteAmounts[i] = 0;
            inDataWriteTimes[i] = 0;
            outDataWriteAmounts[i] = 0;
            outDataWriteTimes[i] = 0;
        }

        qRegisterMetaType<LinkInterface*>("LinkInterface*");
    }

    virtual ~LinkInterface() {
        // LinkManager take ownership of Links once they are added to it. Once added to LinkManager
        // user LinkManager::deleteLink to remove if necessary/
        Q_ASSERT(!_ownedByLinkManager || _deletedByLinkManager);
    }

    /* Connection management */

    /**
     * @brief Get the ID of this link
     *
     * The ID is an unsigned integer, starting at 0
     * @return ID of this link
     **/
    virtual int getId() const = 0;

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

    /**
     * @Brief Get the current incoming data rate.
     *
     * This should be over a short timespan, something like 100ms. A precise value isn't necessary,
     * and this can be filtered, but should be a reasonable estimate of current data rate.
     *
     * @return The data rate of the interface in bits per second, 0 if unknown
     **/
    qint64 getCurrentInDataRate() const
    {
        return getCurrentDataRate(inDataIndex, inDataWriteTimes, inDataWriteAmounts);
    }

    /**
     * @Brief Get the current outgoing data rate.
     *
     * This should be over a short timespan, something like 100ms. A precise value isn't necessary,
     * and this can be filtered, but should be a reasonable estimate of current data rate.
     *
     * @return The data rate of the interface in bits per second, 0 if unknown
     **/
    qint64 getCurrentOutDataRate() const
    {
        return getCurrentDataRate(outDataIndex, outDataWriteTimes, outDataWriteAmounts);
    }
    
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
     * communication arbitrary byte lengths can be written
     *
     * @param bytes The pointer to the byte array containing the data
     * @param length The length of the data array
     **/
    virtual void writeBytes(const char *bytes, qint64 length) = 0;

signals:

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
     * @brief This signal is emitted instantly when the link status changes
     **/
    void connected(bool connected);

    /**
     * @brief This signal is emitted if the human readable name of this link changes
     */
    void nameChanged(QString name);

    /** @brief Communication error occured */
    void communicationError(const QString& linkname, const QString& error);

    void communicationUpdate(const QString& linkname, const QString& text);

protected:

    static const int dataRateBufferSize = 20; ///< Specify how many data points to capture for data rate calculations.

    static const qint64 dataRateCurrentTimespan = 500; ///< Set the maximum age of samples to use for data calculations (ms).

    // Implement a simple circular buffer for storing when and how much data was received.
    // Used for calculating the incoming data rate. Use with *StatsBuffer() functions.
    int inDataIndex;
    quint64 inDataWriteAmounts[dataRateBufferSize]; // In bytes
    qint64 inDataWriteTimes[dataRateBufferSize]; // in ms

    // Implement a simple circular buffer for storing when and how much data was transmit.
    // Used for calculating the outgoing data rate. Use with *StatsBuffer() functions.
    int outDataIndex;
    quint64 outDataWriteAmounts[dataRateBufferSize]; // In bytes
    qint64 outDataWriteTimes[dataRateBufferSize]; // in ms

    mutable QMutex dataRateMutex; // Mutex for accessing the data rate member variables

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
    static void logDataRateToBuffer(quint64 *bytesBuffer, qint64 *timeBuffer, int *writeIndex, quint64 bytes, qint64 time)
    {
        int i = *writeIndex;

        // Now write into the buffer, if there's no room, we just overwrite the first data point.
        bytesBuffer[i] = bytes;
        timeBuffer[i] = time;

        // Increment and wrap the write index
        ++i;
        if (i == dataRateBufferSize)
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
     * this is effectively a global data rate over the last dataRateBufferSize - 1 data points. Also note
     * that data points older than NOW - dataRateCurrentTimespan are ignored.
     *
     * @param index The first valid sample in the data rate buffer. Refers to the oldest time sample.
     * @param dataWriteTimes The time, in ms since epoch, that each data sample took place.
     * @param dataWriteAmounts The amount of data (in bits) that was transferred.
     * @return The bits per second of data transferrence of the interface over the last [-statsCurrentTimespan, 0] timespan.
     */
    qint64 getCurrentDataRate(int index, const qint64 dataWriteTimes[], const quint64 dataWriteAmounts[]) const
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();

        // Limit the time we calculate to the recent past
        const qint64 cutoff = now - dataRateCurrentTimespan;

        // Grab the mutex for working with the stats variables
        QMutexLocker dataRateLocker(&dataRateMutex);

        // Now iterate through the buffer of all received data packets adding up all values
        // within now and our cutof.
        qint64 totalBytes = 0;
        qint64 totalTime = 0;
        qint64 lastTime = 0;
        int size = dataRateBufferSize;
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
            if (++index == dataRateBufferSize)
            {
                index = 0;
            }
        }

        // Return the final calculated value in bits / s, converted from bytes/ms.
        qint64 dataRate = (totalTime != 0)?(qint64)((float)totalBytes * 8.0f / ((float)totalTime / 1000.0f)):0;

        // Finally return our calculated data rate.
        return dataRate;
    }

    static int getNextLinkId() {
        static int nextId = 1;
        return nextId++;
    }

protected slots:

    /**
     * @brief Read a number of bytes from the interface.
     *
     * @param bytes The pointer to write the bytes to
     * @param maxLength The maximum length which can be written
     **/
    virtual void readBytes() = 0;

private:
    /**
     * @brief Connect this interface logically
     *
     * @return True if connection could be established, false otherwise
     **/
    virtual bool _connect(void) = 0;
    
    /**
     * @brief Disconnect this interface logically
     *
     * @return True if connection could be terminated, false otherwise
     **/
    virtual bool _disconnect(void) = 0;
    
    bool _ownedByLinkManager;   ///< true: This link has been added to LinkManager, false: Link not added to LinkManager
    bool _deletedByLinkManager; ///< true: Link being deleted from LinkManager, false: error, Links should only be deleted from LinkManager
    
    friend class LinkManager;
};

#endif // _LINKINTERFACE_H_
