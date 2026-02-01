#pragma once

#include "UnitTest.h"

/// @brief Runner for QML-based tests using Qt Quick Test
///
/// This integrates Qt Quick Test with the QGC unit test framework.
/// QML test files are loaded from the qmltests/ directory in the build folder.
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
