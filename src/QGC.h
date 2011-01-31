#ifndef QGC_H
#define QGC_H

#include <QDateTime>
#include <QColor>
#include <QThread>

#include "configuration.h"

namespace QGC
{
    const QColor colorCyan(55, 154, 195);
    const QColor colorRed(154, 20, 20);
    const QColor colorGreen(20, 200, 20);
    const QColor colorYellow(255, 255, 0);
    const QColor colorDarkYellow(180, 180, 0);
    const QColor colorBackground("#050508");

    /** @brief Get the current ground time in microseconds */
    quint64 groundTimeUsecs();
    /** @brief Returns the angle limited to -pi - pi */
    double limitAngleToPMPI(double angle);
    int applicationVersion();

    const static int MAX_FLIGHT_TIME = 60 * 60 * 24 * 21;

    class SLEEP : public QThread
    {
    public:
        /**
         * @brief Set a thread to sleep for seconds
         * @param s time in seconds to sleep
         **/
        static void sleep(unsigned long s)
        {
            QThread::sleep(s);
        }
        /**
         * @brief Set a thread to sleep for milliseconds
         * @param ms time in milliseconds to sleep
         **/
        static void msleep(unsigned long ms)
        {
            QThread::msleep(ms);
        }
        /**
         * @brief Set a thread to sleep for microseconds
         * @param us time in microseconds to sleep
         **/
        static void usleep(unsigned long us)
        {
            QThread::usleep(us);
        }
    };

}

#define QGC_EVENTLOOP_DEBUG 0

#endif // QGC_H
