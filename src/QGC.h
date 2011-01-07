#ifndef QGC_H
#define QGC_H

#include <QDateTime>
#include <QColor>

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
    int applicationVersion();

    const QString APPNAME = "QGROUNDCONTROL";
    const QString COMPANYNAME = "OPENMAV";
    const int APPLICATIONVERSION = 80; // 0.8.0
}

#define QGC_EVENTLOOP_DEBUG 0

#endif // QGC_H
