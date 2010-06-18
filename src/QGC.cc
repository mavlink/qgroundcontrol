#include "QGC.h"

namespace QGC {

quint64 groundTimeUsecs()
{
    QDateTime time = QDateTime::currentDateTime();
    time = time.toUTC();
    /* Return seconds and milliseconds, in milliseconds unit */
    quint64 microseconds = time.toTime_t() * static_cast<quint64>(1000000);
    return static_cast<quint64>(microseconds + (time.time().msec()*1000));
}

}
