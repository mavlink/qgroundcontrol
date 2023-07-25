/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    enum class GPSType {
        u_blox,
        trimble,
        septentrio
    };

    GPSProvider(const QString& device,
                GPSType type,
                bool    enableSatInfo,
                double  surveyInAccMeters,
                int     surveryInDurationSecs,
                bool    useFixedBaseLocation,
                double  fixedBaseLatitude,
                double  fixedBaseLongitude,
                float   fixedBaseAltitudeMeters,
                float   fixedBaseAccuracyMeters,
                const std::atomic_bool& requestStop);
    ~GPSProvider();

    /**
     * this is called by the callback method
     */
    void gotRTCMData(uint8_t *data, size_t len);

signals:
    void positionUpdate(GPSPositionMessage message);
    void satelliteInfoUpdate(GPSSatelliteMessage message);
    void RTCMDataUpdate(QByteArray message);
    void surveyInStatus(float duration, float accuracyMM, double latitude, double longitude, float altitude, bool valid, bool active);

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
    GPSType _type;
    const std::atomic_bool& _requestStop;
    double  _surveyInAccMeters;
    int     _surveryInDurationSecs;
    bool    _useFixedBaseLoction;
    double  _fixedBaseLatitude;
    double  _fixedBaseLongitude;
    float   _fixedBaseAltitudeMeters;
    float   _fixedBaseAccuracyMeters;
    GPSHelper::GPSConfig _gpsConfig{};

	struct sensor_gps_s        _reportGpsPos;
	struct satellite_info_s    *_pReportSatInfo = nullptr;

	QSerialPort *_serial = nullptr;
};
