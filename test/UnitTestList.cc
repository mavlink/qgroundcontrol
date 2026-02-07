#include "UnitTestList.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QSet>

Q_STATIC_LOGGING_CATEGORY(UnitTestListLog, "Test.UnitTestList")

// ============================================================================
// Test Execution Functions
// ============================================================================

namespace QGCUnitTest {

int runTests(const QStringList& unitTests, int iterations, const QString& outputFile, TestLabels labelFilter)
{
    // Determine which tests to run
    QStringList testsToRun;
    if (unitTests.isEmpty()) {
        // No specific tests - run all matching the label filter
        testsToRun = registeredTestNames(labelFilter);
    } else {
        // Validate requested test names
        const QStringList invalid = validateTestNames(unitTests);
        if (!invalid.isEmpty()) {
            qCWarning(UnitTestListLog) << "Unknown test(s):" << invalid.join(", ");
            qCWarning(UnitTestListLog) << "Available tests:" << UnitTest::registeredTests().join(", ");
            return -static_cast<int>(invalid.size());
        }
        testsToRun = unitTests;
    }

    if (testsToRun.isEmpty()) {
        qCWarning(UnitTestListLog) << "No tests to run";
        return 0;
    }

    iterations = qMax(1, iterations);
    int result = 0;

    for (int i = 0; i < iterations; ++i) {
        int failures = 0;

        for (const QString& test : testsToRun) {
            failures += UnitTest::run(test, outputFile, labelFilter);
        }

        if (failures == 0) {
            if (iterations > 1) {
                qCDebug(UnitTestListLog).noquote()
                    << QString("ALL TESTS PASSED (iteration %1/%2)").arg(i + 1).arg(iterations);
            } else {
                qCDebug(UnitTestListLog) << "ALL TESTS PASSED";
            }
        } else {
            qCWarning(UnitTestListLog) << failures << "TESTS FAILED!";
            result = -failures;
            break;
        }
    }

    return result;
}

QStringList registeredTestNames()
{
    return UnitTest::registeredTests();
}

QStringList registeredTestNames(TestLabels labelFilter)
{
    return UnitTest::registeredTests(labelFilter);
}

int registeredTestCount()
{
    return UnitTest::testCount();
}

bool isTestRegistered(const QString& testName)
{
    return UnitTest::registeredTests().contains(testName);
}

QStringList validateTestNames(const QStringList& testNames)
{
    const QStringList registered = UnitTest::registeredTests();

    // Use QSet for O(1) lookup instead of O(n) QStringList::contains()
    const QSet<QString> registeredSet(registered.cbegin(), registered.cend());

    QStringList invalid;
    invalid.reserve(testNames.size());

    for (const QString& name : testNames) {
        if (!registeredSet.contains(name)) {
            invalid.append(name);
        }
    }

    return invalid;
}

int handleTestOptions(const QGCCommandLineParser::CommandLineParseResult& args)
{
    // Parse label filter if provided
    TestLabels labelFilter;
    if (args.labelFilter.has_value() && !args.labelFilter->isEmpty()) {
        labelFilter = parseLabels(args.labelFilter.value());
        if (labelFilter == TestLabels()) {
            qCWarning(UnitTestListLog) << "Invalid label filter:" << args.labelFilter.value();
            qCWarning(UnitTestListLog) << "Available labels:" << availableLabelNames().join(", ");
            return -1;
        }
    }

    // Handle --list-tests
    if (args.listTests) {
        const QStringList tests = registeredTestNames(labelFilter);
        const QString filterDesc =
            labelFilter != TestLabels() ? QString(" matching %1").arg(labelsToString(labelFilter)) : QString();

        qCInfo(UnitTestListLog).noquote() << QString("Available unit tests%1: %2").arg(filterDesc).arg(tests.count());

        for (const QString& test : tests) {
            qInfo().noquote() << "  " << test;
        }

        qCInfo(UnitTestListLog).noquote() << QString("\nAvailable labels: %1").arg(availableLabelNames().join(", "));

        return 0;
    }

    // Handle --unittest
    if (args.runningUnitTests) {
        // Count tests that will run
        const QStringList testsToRun = args.unitTests.isEmpty() ? registeredTestNames(labelFilter) : args.unitTests;
        const int testCount = testsToRun.count();

        const QString filterDesc =
            labelFilter != TestLabels() ? QString(" %1").arg(labelsToString(labelFilter)) : QString();

        qCInfo(UnitTestListLog).noquote() << QString("Running %1 unit test(s)%2...").arg(testCount).arg(filterDesc);

        QElapsedTimer timer;
        timer.start();

        const int stressIterations =
            args.stressUnitTests
                ? static_cast<int>(args.stressUnitTestsCount > 0 ? args.stressUnitTestsCount : kStressIterations)
                : 1;

        const int exitCode =
            runTests(args.unitTests, stressIterations, args.unitTestOutput.value_or(QString()), labelFilter);

        const qint64 elapsed = timer.elapsed();
        if (exitCode == 0) {
            qCInfo(UnitTestListLog).noquote() << QString("All %1 test(s) passed in %2 ms").arg(testCount).arg(elapsed);
        } else {
            qCWarning(UnitTestListLog).noquote()
                << QString("%1 test(s) failed (ran in %2 ms)").arg(-exitCode).arg(elapsed);
        }
        return exitCode;
    }

    return 0;
}

}  // namespace QGCUnitTest
