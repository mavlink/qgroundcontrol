#pragma once

#include "LogStoreTestBase.h"

class LogStoreTest : public LogStoreTestBase
{
    Q_OBJECT

private slots:
    void _openAndClose();
    void _appendAndQuery();
    void _queryFilters();
    void _sessions();
    void _deleteSession();
    void _exportSession();
    void _entryCount();
    void _exportSessionEmpty();
};
