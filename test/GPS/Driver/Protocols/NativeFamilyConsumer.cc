#include QGC_GPS_DRIVER_HEADER

int main()
{
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    QGC_GPS_DRIVER_CLASS driver({}, &position, &satellites);
    const auto decoded = driver.decode({});
    return decoded.bytesConsumed != 0 || !decoded.batch.events.empty();
}
