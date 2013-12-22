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

/**
* The link interface defines the interface for all links used to communicate
* with the groundstation application.
*
**/
class LinkInterface : public QThread
{
    Q_OBJECT
public:
    LinkInterface(QObject* parent = 0) : QThread(parent) {}
    virtual ~LinkInterface() { emit this->deleteLink(this); }

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
    virtual qint64 getCurrentInDataRate() const = 0;

    /**
     * @Brief Get the current outgoing data rate.
     *
     * This should be over a short timespan, something like 100ms. A precise value isn't necessary,
     * and this can be filtered, but should be a reasonable estimate of current data rate.
     *
     * @return The data rate of the interface in bits per second, 0 if unknown
     **/
    virtual qint64 getCurrentOutDataRate() const = 0;

    /**
     * @brief Connect this interface logically
     *
     * @return True if connection could be established, false otherwise
     **/
    virtual bool connect() = 0;

    /**
     * @brief Disconnect this interface logically
     *
     * @return True if connection could be terminated, false otherwise
     **/
    virtual bool disconnect() = 0;

    /**
     * @brief Get the current number of bytes in buffer.
     *
     * @return The number of bytes ready to read
     **/
    virtual qint64 bytesAvailable() = 0;

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

	/** @brief destroying element */
	void deleteLink(LinkInterface* const link);

protected:
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

};

/* Declare C++ interface as Qt interface */
//Q_DECLARE_INTERFACE(LinkInterface, "org.openground.comm.links.LinkInterface/1.0")

#endif // _LINKINTERFACE_H_
