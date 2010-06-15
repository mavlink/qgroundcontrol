#ifndef QGC_H
#define QGC_H

#include <QDateTime>
#include <QColor>

namespace QGC
{
    const QColor ColorCyan(55, 154, 195);

    quint64 groundTimeUsecs()
    {
        QDateTime time = QDateTime::currentDateTime();
        time = time.toUTC();
        /* Return seconds and milliseconds, in milliseconds unit */
        quint64 microseconds = time.toTime_t() * static_cast<quint64>(1000000);
        return static_cast<quint64>(microseconds + (time.time().msec()*1000));
    }
}

#endif // QGC_H
