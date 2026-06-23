#pragma once

#include "UnitTest.h"

class LogFormatterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _formatAsText();
    void _formatAsJson();
    void _formatAsCsv();
    void _formatAsJsonLines();
    void _csvEscaping();
    void _entryToJsonExportSchema();
    void _entryToJsonRemoteSchema();
    void _formatDispatch();
};
