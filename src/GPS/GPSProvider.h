/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
