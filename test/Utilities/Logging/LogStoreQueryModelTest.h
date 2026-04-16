#pragma once

#include "LogStoreTestBase.h"

class LogStoreQueryModelTest : public LogStoreTestBase
{
    Q_OBJECT

private slots:
    void _emptyModel();
    void _refreshPopulates();
    void _sessionFilter();
    void _levelFilter();
    void _categoryFilter();
    void _textFilter();
    void _availableSessions();
    void _loadMore();

private:
    static void populateStore(LogStore* store, int count,
                              const QString& prefix = QStringLiteral("msg"),
                              LogEntry::Level level = LogEntry::Info,
                              const QString& category = QStringLiteral("Test"));
};
