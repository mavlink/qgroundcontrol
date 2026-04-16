#pragma once

#include "UnitTest.h"

/// @brief Structural validator for QML test files.
///
/// Despite its name, this class does NOT execute QML tests in-process.
/// Qt Quick Test's quick_test_main() calls exit(), so actual QML test
/// execution must happen in a separate executable via CTest.
///
/// This validator checks that QML test files exist, are readable, contain
/// a TestCase element, and define at least one test_ function.
class QmlTestRunner : public UnitTest
{
    Q_OBJECT

public:
    QmlTestRunner();

private slots:
    void _runQmlTests();

private:
    QString _testDirectory() const;
};
