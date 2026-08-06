#pragma once

#include "UnitTest.h"

/// Audits every QGC-internal JSON file compiled into resources: each file must parse
/// through its real loader without warnings. Files are discovered at runtime by their
/// "fileType" header, so new files are covered automatically - a file the app can load
/// is in resources (and found here), a file not in resources can't be loaded by the app
/// either. Turns silent parse-warning fallbacks (e.g. strict key validation) into CI failures.
class JsonResourceAuditTest : public UnitTest
{
    Q_OBJECT

public:
    JsonResourceAuditTest() = default;

private slots:
    void _allResourceJsonParsesClean_test();

private:
    void _verifyNoWarnings(const QString& jsonPath, const QString& category);
};
