#pragma once

/// @file QmlTesting.h
/// @brief Qt Quick Test support for QGroundControl QML component testing
///
/// Qt Quick Test allows testing QML components using QML-based test cases.
/// Tests are written in QML using the TestCase type from QtTest.
///
/// @see https://doc.qt.io/qt-6/qtquicktest-index.html
///
/// ## How It Works
///
/// 1. QML test files (tst_*.qml) are placed in test/UnitTestFramework/QmlTesting/tests/
/// 2. Each file contains TestCase elements with test functions
/// 3. QmlTestRunner loads and executes these tests
/// 4. Results integrate with CTest
///
/// ## Basic Test Structure
///
/// ```qml
/// import QtQuick
/// import QtTest
///
/// TestCase {
///     name: "MyComponentTest"
///
///     // Component under test
///     MyComponent {
///         id: component
///     }
///
///     function test_initialState() {
///         compare(component.value, 0)
///         verify(component.enabled)
///     }
///
///     function test_userInteraction() {
///         mouseClick(component)
///         compare(component.clicked, true)
///     }
/// }
/// ```
///
/// ## Available Test Functions
///
/// - compare(actual, expected) - Compare values
/// - verify(condition) - Assert condition is true
/// - tryCompare(obj, prop, value, timeout) - Wait for property
/// - tryVerify(function, timeout) - Wait for condition
/// - mouseClick(item, x, y) - Simulate mouse click
/// - keyClick(key) - Simulate key press
/// - keyPress/keyRelease - Key events
/// - wait(ms) - Wait milliseconds
/// - waitForRendering(item) - Wait for item to render
///
/// ## Testing Signals
///
/// ```qml
/// SignalSpy {
///     id: spy
///     target: component
///     signalName: "valueChanged"
/// }
///
/// function test_signal() {
///     component.value = 42
///     compare(spy.count, 1)
/// }
/// ```
///
/// ## Data-Driven Tests
///
/// ```qml
/// function test_values_data() {
///     return [
///         { tag: "zero", input: 0, expected: "0" },
///         { tag: "positive", input: 42, expected: "42" },
///         { tag: "negative", input: -1, expected: "-1" },
///     ]
/// }
///
/// function test_values(data) {
///     component.value = data.input
///     compare(component.text, data.expected)
/// }
/// ```
///
/// ## Running QML Tests
///
/// Via CTest:
///   ctest -R QmlTest
///
/// Via command line:
///   ./QGroundControl --unittest QmlTestRunner
///
/// ## File Naming
///
/// QML test files must be named: tst_*.qml
/// Example: tst_FactTextField.qml, tst_QGCButton.qml
///
/// ## Best Practices
///
/// 1. Test one component per file
/// 2. Use SignalSpy for async operations
/// 3. Use tryCompare/tryVerify for animations
/// 4. Keep tests independent (no shared state)
/// 5. Mock external dependencies when possible

#include <QtQuickTest/QtQuickTest>
