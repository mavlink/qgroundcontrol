#pragma once

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(UnitTestsLog)

int runTests(bool stress, QStringView unitTestOptions);
