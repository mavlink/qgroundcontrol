#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

struct LogEntry
{
    Q_GADGET
    QML_ANONYMOUS
    Q_PROPERTY(QDateTime timestamp MEMBER timestamp FINAL)
    Q_PROPERTY(Level level MEMBER level FINAL)
    Q_PROPERTY(QString category MEMBER category FINAL)
    Q_PROPERTY(QString message MEMBER message FINAL)
    Q_PROPERTY(QString file MEMBER file FINAL)
    Q_PROPERTY(QString function MEMBER function FINAL)
    Q_PROPERTY(QString formatted MEMBER formatted FINAL)
    Q_PROPERTY(int line MEMBER line FINAL)

public:
    enum Level
    {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Critical = 3,
        Fatal = 4,
    };
    Q_ENUM(Level)

    LogEntry() = default;
    LogEntry(const LogEntry&) = default;
    LogEntry(LogEntry&&) noexcept = default;
    LogEntry& operator=(const LogEntry&) = default;
    LogEntry& operator=(LogEntry&&) noexcept = default;

    QDateTime timestamp;
    Level level = Debug;
    QString category;
    QString message;
    QString file;
    QString function;
    QString formatted;
    Qt::HANDLE threadId = nullptr;
    int line = 0;

    [[nodiscard]] QString levelLabel() const;
    void buildFormatted();
    [[nodiscard]] static Level fromQtMsgType(QtMsgType type);

    // Shared role definitions for all models displaying LogEntry data
    enum Role
    {
        TimestampRole = Qt::UserRole + 1,
        LevelRole,
        LevelLabelRole,
        CategoryRole,
        MessageRole,
        FormattedRole,
        FileRole,
        FunctionRole,
        LineRole,
        ThreadIdRole,
    };

    enum Column
    {
        TimestampColumn = 0,
        LevelColumn,
        CategoryColumn,
        SourceColumn,
        MessageColumn,
        ColumnCount
    };
    Q_ENUM(Column)

    [[nodiscard]] static QHash<int, QByteArray> roleNames();
    [[nodiscard]] QVariant roleData(int role) const;
    [[nodiscard]] QVariant columnDisplayData(int column) const;
    [[nodiscard]] static QVariant columnHeaderData(int section);
};

// Expose LogEntry enums to QML under the uppercase "LogEntry" name.
// Q_NAMESPACE + QML_NAMED_ELEMENT registers as a namespace (not a value type),
// so the lowercase-name warning does not apply.
namespace LogEntryForeign
{
Q_NAMESPACE
QML_NAMED_ELEMENT(LogEntry)
QML_FOREIGN_NAMESPACE(LogEntry)
}
