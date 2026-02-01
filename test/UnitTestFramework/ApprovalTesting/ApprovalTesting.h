#pragma once

/// @file ApprovalTesting.h
/// @brief ApprovalTests snapshot testing support for QGroundControl unit tests
///
/// ApprovalTests provides "golden master" testing where you verify output
/// against a previously approved result, rather than writing manual assertions.
///
/// @see https://github.com/approvals/ApprovalTests.cpp
///
/// ## How It Works
///
/// 1. First run: Creates `TestClass._testMethod.received.txt` with actual output
/// 2. You review and rename to `.approved.txt` (or use a diff tool)
/// 3. Subsequent runs: Compares actual vs approved, fails if different
/// 4. When output changes intentionally: Review and re-approve
///
/// ## Basic Usage
///
/// ```cpp
/// #include "ApprovalTesting.h"
///
/// void MyTest::_testJsonOutput()
/// {
///     MissionItem item(...);
///     QString json = item.toJson();
///
///     // Verifies against MyTest._testJsonOutput.approved.txt
///     Approvals::verify(json.toStdString());
/// }
/// ```
///
/// ## Verifying Different Types
///
/// ```cpp
/// // String
/// Approvals::verify("output text");
///
/// // JSON (pretty printed)
/// Approvals::verify(jsonString, Options().fileOptions().withFileExtension(".json"));
///
/// // Container of items
/// std::vector<int> items = {1, 2, 3};
/// Approvals::verifyAll("items", items);
///
/// // Custom object (needs toString or <<)
/// Approvals::verify(myObject);
/// ```
///
/// ## File Naming
///
/// Approved files are named: `ClassName._methodName.approved.txt`
/// Received files are named: `ClassName._methodName.received.txt`
///
/// Store `.approved.*` files in git, ignore `.received.*` files.
///
/// ## Approving Results
///
/// Option 1: Manually rename `.received.txt` to `.approved.txt`
/// Option 2: Use a diff tool (configure via environment)
/// Option 3: Use ApprovalTests reporters
///
/// ```cpp
/// // Use specific reporter
/// Approvals::verify(output, Options().withReporter(Reporters::DiffReporter()));
/// ```
///
/// ## Best Practices
///
/// 1. Add `*.received.*` to `.gitignore`
/// 2. Commit `.approved.*` files to version control
/// 3. Use for complex output that's hard to assert manually
/// 4. Review changes carefully before approving
/// 5. Use `.json`, `.xml`, `.html` extensions for syntax highlighting
///
/// ## Good Use Cases
///
/// - Mission plan JSON serialization
/// - Parameter file export
/// - MAVLink message formatting
/// - Log parsing output
/// - Complex data structure dumps
/// - Any output where "does it look right?" is easier than assertions

// Configure ApprovalTests for Qt Test
#define APPROVALS_QT
#include <ApprovalTests.hpp>

// Convenience aliases
using Approvals = ApprovalTests::Approvals;
using Options = ApprovalTests::Options;

/// Namespace for QGC-specific approval testing utilities
namespace qgc::approvals {

/// Verify a QString
inline void verify(const QString& str)
{
    Approvals::verify(str.toStdString());
}

/// Verify a QString as JSON (with .json extension for syntax highlighting)
inline void verifyJson(const QString& json)
{
    Approvals::verify(
        json.toStdString(),
        Options().fileOptions().withFileExtension(".json"));
}

/// Verify a QString as XML
inline void verifyXml(const QString& xml)
{
    Approvals::verify(
        xml.toStdString(),
        Options().fileOptions().withFileExtension(".xml"));
}

/// Verify a QByteArray
inline void verify(const QByteArray& data)
{
    Approvals::verify(data.toStdString());
}

/// Verify a QJsonDocument (pretty printed)
inline void verify(const QJsonDocument& doc)
{
    Approvals::verify(
        doc.toJson(QJsonDocument::Indented).toStdString(),
        Options().fileOptions().withFileExtension(".json"));
}

/// Verify a QJsonObject (pretty printed)
inline void verify(const QJsonObject& obj)
{
    verify(QJsonDocument(obj));
}

/// Verify a QJsonArray (pretty printed)
inline void verify(const QJsonArray& arr)
{
    verify(QJsonDocument(arr));
}

} // namespace qgc::approvals
