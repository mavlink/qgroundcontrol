/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QString>
#include <QThread>
#include <QByteArray>
#include <QSerialPort>

#include <atomic>

#include "GPSPositionMessage.h"
#include "Drivers/src/gps_helper.h"


/**
 ** class GPSProvider
 * opens a GPS device and handles the protocol
 */
class GPSProvider : public QThread
{
    Q_OBJECT
public:
    GPSProvider(const QString& device, bool enableSatInfo, const std::atomic_bool& requestStop);
    ~GPSProvider();

    /**
     * this is called by the callback method
     */
    void gotRTCMData(uint8_t *data, size_t len);
signals:
    void positionUpdate(GPSPositionMessage message);
    void satelliteInfoUpdate(GPSSatelliteMessage message);
    void RTCMDataUpdate(QByteArray message);

protected:
    void run();

private:
    void publishGPSPosition();
    void publishGPSSatellite();

	/**
	 * callback from the driver for the platform specific stuff
	 */
	static int callbackEntry(GPSCallbackType type, void *data1, int data2, void *user);

	int callback(GPSCallbackType type, void *data1, int data2);

    QString _device;
    const std::atomic_bool& _requestStop;

	struct vehicle_gps_position_s	_reportGpsPos;
	struct satellite_info_s		*_pReportSatInfo = nullptr;

	QSerialPort *_serial = nullptr;
};
