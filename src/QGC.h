#ifndef QGC_H
#define QGC_H

#include <QDateTime>
#include <QColor>

namespace QGC
{
    const QColor ColorCyan(55, 154, 195);

    /** @brief Get the current ground time in microseconds */
    quint64 groundTimeUsecs();
}

#endif // QGC_H
