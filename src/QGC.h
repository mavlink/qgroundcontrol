#ifndef QGC_H
#define QGC_H

#include <QDateTime>
#include <QColor>

namespace QGC
{
    const QColor colorCyan(55, 154, 195);
    const QColor colorRed(154, 20, 20);
    const QColor colorGreen(20, 200, 20);
    const QColor colorYellow(195, 154, 55);

    /** @brief Get the current ground time in microseconds */
    quint64 groundTimeUsecs();
}

#endif // QGC_H
