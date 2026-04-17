#pragma once

#include <QtTest/QAbstractItemModelTester>

#include <memory>

#include "UnitTest.h"

/// Test fixture for QAbstractItemModel-derived classes.
///
/// Wraps QAbstractItemModelTester for automatic model invariant checking.
/// The tester runs continuously while attached, asserting on any model
/// contract violation (bad indices, inconsistent row counts, etc.).
///
/// Example:
/// @code
/// class MyModelTest : public ModelTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testBasicInvariants() {
///         MyListModel model;
///         attachModelTester(&model);
///         model.loadData();
///         QCOMPARE(model.rowCount(), 5);
///         // QAbstractItemModelTester asserts automatically on violations
///     }
///
///     void _testWithWarningMode() {
///         MyListModel model;
///         attachModelTester(&model, QAbstractItemModelTester::FailureReportingMode::Warning);
///         // violations logged as warnings instead of fatal
///     }
/// };
/// @endcode
class ModelTest : public UnitTest
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ModelTest)

public:
    explicit ModelTest(QObject* parent = nullptr) : UnitTest(parent)
    {
    }

protected:
    /// Attaches a QAbstractItemModelTester to the given model.
    /// The tester validates model invariants for the lifetime of the test.
    /// @param model The model to validate
    /// @param mode Failure reporting mode (default: Fatal — aborts on violation)
    void attachModelTester(
        QAbstractItemModel* model,
        QAbstractItemModelTester::FailureReportingMode mode = QAbstractItemModelTester::FailureReportingMode::Fatal)
    {
        _tester = std::make_unique<QAbstractItemModelTester>(model, mode, this);
    }

    /// Returns the current model tester (nullptr if none attached)
    QAbstractItemModelTester* modelTester() const
    {
        return _tester.get();
    }

    /// Detaches the current model tester
    void detachModelTester()
    {
        _tester.reset();
    }

protected slots:
    void cleanup() override
    {
        _tester.reset();
        UnitTest::cleanup();
    }

private:
    std::unique_ptr<QAbstractItemModelTester> _tester;
};
