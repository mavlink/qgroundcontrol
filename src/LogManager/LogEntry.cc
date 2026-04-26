#include "LogEntry.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QVariant>

QString LogEntry::levelLabel() const
{
    static const QString labels[] = {
        QStringLiteral("D"),
        QStringLiteral("I"),
        QStringLiteral("W"),
        QStringLiteral("C"),
        QStringLiteral("F"),
    };
    const int idx = static_cast<int>(level);
    if (idx >= 0 && idx < static_cast<int>(std::size(labels))) {
        return labels[idx];
    }
    return QStringLiteral("?");
}

void LogEntry::buildFormatted()
{
    formatted =
        QStringLiteral("%1 [%2] %3: %4").arg(timestamp.toString(Qt::ISODateWithMs), levelLabel(), category, message);
}

LogEntry::Level LogEntry::fromQtMsgType(QtMsgType type)
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

QHash<int, QByteArray> LogEntry::roleNames()
{
    static const QHash<int, QByteArray> roles{
        {Qt::DisplayRole, "display"},
        {TimestampRole, "timestamp"},
        {LevelRole, "level"},
        {LevelLabelRole, "levelLabel"},
        {CategoryRole, "category"},
        {MessageRole, "message"},
        {FormattedRole, "formatted"},
        {FileRole, "file"},
        {FunctionRole, "function"},
        {LineRole, "line"},
        {ThreadIdRole, "threadId"},
    };
    return roles;
}

QVariant LogEntry::roleData(int role) const
{
    switch (role) {
        case FormattedRole:
            return formatted;
        case TimestampRole:
            return timestamp;
        case LevelRole:
            return static_cast<int>(level);
        case LevelLabelRole:
            return levelLabel();
        case CategoryRole:
            return category;
        case MessageRole:
            return message;
        case FileRole:
            return file;
        case FunctionRole:
            return function;
        case LineRole:
            return line;
        case ThreadIdRole:
            return QString::number(reinterpret_cast<quintptr>(threadId), 16);
    }
    return {};
}

QVariant LogEntry::columnDisplayData(int column) const
{
    switch (static_cast<Column>(column)) {
        case TimestampColumn:
            return timestamp.toString(QStringLiteral("hh:mm:ss.zzz"));
        case LevelColumn:
            return levelLabel();
        case CategoryColumn:
            return category;
        case MessageColumn:
            return message;
        case SourceColumn:
            if (file.isEmpty()) {
                return QString();
            }
            return line > 0 ? QStringLiteral("%1:%2").arg(file).arg(line) : file;
        case ColumnCount:
            break;
    }
    return {};
}

QVariant LogEntry::columnHeaderData(int section)
{
    switch (static_cast<Column>(section)) {
        case TimestampColumn:
            return QStringLiteral("Time");
        case LevelColumn:
            return QStringLiteral("Level");
        case CategoryColumn:
            return QStringLiteral("Category");
        case MessageColumn:
            return QStringLiteral("Message");
        case SourceColumn:
            return QStringLiteral("Source");
        case ColumnCount:
            break;
    }
    return {};
}

