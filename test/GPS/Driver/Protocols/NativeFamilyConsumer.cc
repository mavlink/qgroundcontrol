#include QGC_GPS_DRIVER_HEADER

#include <type_traits>

static_assert(!std::is_copy_constructible_v<QGC_GPS_DRIVER_CLASS>);
static_assert(!std::is_copy_assignable_v<QGC_GPS_DRIVER_CLASS>);
static_assert(!std::is_move_constructible_v<QGC_GPS_DRIVER_CLASS>);
static_assert(!std::is_move_assignable_v<QGC_GPS_DRIVER_CLASS>);

int main()
{
    GPSNativePositionReport position;
    GPSNativeSatelliteReport satellites;
    QGC_GPS_DRIVER_CLASS driver({}, &position, &satellites);
    const auto decoded = driver.decode({});
    return decoded.bytesConsumed != 0 || !decoded.batch.events.empty();
}
