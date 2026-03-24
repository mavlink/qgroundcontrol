#pragma once

#include "UnitTest.h"

class LogStoreQueryModelTest : public UnitTest
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
};
