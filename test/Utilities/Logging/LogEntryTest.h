#pragma once

#include "UnitTest.h"

class LogEntryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _levelLabel();
    void _fromQtMsgType();
    void _buildFormatted();
    void _moveSemantics();
    void _defaultValues();
    void _roleNames();
    void _roleData();
    void _columnDisplayData();
    void _columnHeaderData();
};
