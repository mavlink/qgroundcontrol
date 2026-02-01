#include "QGCLogEntry.h"

QString QGCLogEntry::toString() const
{
    QString result = timestamp.toString(Qt::ISODateWithMs);

    if (level != Debug) {
        result += ' ' + levelLabel(level) + ':';
    }

    result += ' ' + message;

    if (!category.isEmpty()) {
        result += " - " + category;
    }

    if (!function.isEmpty()) {
        if (!file.isEmpty()) {
            result += QStringLiteral(" - (%1:%2 %3)").arg(file).arg(line).arg(function);
        } else {
            result += QStringLiteral(" - (%1:%2)").arg(function).arg(line);
        }
    }

    return result;
}

QGCLogEntry::Level QGCLogEntry::fromQtMsgType(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return Debug;
    case QtInfoMsg:
        return Info;
    case QtWarningMsg:
        return Warning;
    case QtCriticalMsg:
        return Critical;
    case QtFatalMsg:
        return Fatal;
    }
    return Debug;
}

QString QGCLogEntry::levelLabel(Level level)
{
    switch (level) {
    case Debug:
        return QStringLiteral("D");
    case Info:
        return QStringLiteral("I");
    case Warning:
        return QStringLiteral("W");
    case Critical:
        return QStringLiteral("C");
    case Fatal:
        return QStringLiteral("F");
    }
    return QString();
}
