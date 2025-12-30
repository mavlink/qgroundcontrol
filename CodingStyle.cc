/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// This is an example class c++ file which is used to describe the QGroundControl
// coding style. In general almost everything in here has some coding style meaning.
// Not all style choices are explained.

#include "CodingStyle.h"

#include <math.h>

#include <QtCore/QDebug>
#include <QtCore/QFile>

#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

// Note how the Qt headers, System headers, and the QGroundControl headers above are kept in separate groups
// with blank lines between them. Within each group, headers are sorted alphabetically.

// Use QGC_LOGGING_CATEGORY instead of Q_LOGGING_CATEGORY for runtime log configuration support
QGC_LOGGING_CATEGORY(CodingStyleLog, "Example.CodingStyle")

CodingStyle::CodingStyle(QObject* parent)
    : QObject(parent)
{
    // Constructor body - use member initializer list above for initialization
    _commonInit();
}

CodingStyle::~CodingStyle()
{
    // Cleanup code here
    // Qt parent/child ownership handles most cleanup automatically
}

void CodingStyle::_commonInit()
{
    // Common initialization code
    qCDebug(CodingStyleLog) << "CodingStyle initialized";
}

void CodingStyle::setExampleProperty(int value)
{
    if (_exampleProperty != value) {
        _exampleProperty = value;
        emit examplePropertyChanged(value);  // Always emit signals when properties change
    }
}

bool CodingStyle::publicMethod1()
{
    // Implementation here
    return true;
}

void CodingStyle::performAction(const QString& param)
{
    // Defensive coding: validate inputs early
    if (param.isEmpty()) {
        qCWarning(CodingStyleLog) << "performAction called with empty parameter";
        return;
    }

    // Always null-check pointers before dereferencing
    if (!_vehicle) {
        qCWarning(CodingStyleLog) << "No vehicle available";
        return;
    }

    qCDebug(CodingStyleLog) << "performAction:" << param;
}

/// Document non-obvious private methods in the code file.
void CodingStyle::_privateMethod()
{
    // Defensive coding example: validate preconditions and return early on errors
    if (!_vehicle) {
        qCWarning(CodingStyleLog) << "Vehicle not set, cannot proceed";
        return;
    }

    // Always include braces even for single line if/for/while/etc
    if (_exampleProperty == 0) {
        return;
    }

    // Note the brace placement
    if (_privateVariable1 == -1) {
        _privateVariable1 = 42;
    } else {
        // Use defensive checks instead of Q_ASSERT in production code
        // Q_ASSERT is compiled out in release builds
        if (_privateVariable1 != 42) {
            qCWarning(CodingStyleLog) << "Unexpected value:" << _privateVariable1;
        }
    }
}

void CodingStyle::_privateSlot()
{
    // Example: handling Fact value changes
    Fact* fact = qobject_cast<Fact*>(sender());
    if (!fact) {
        qCWarning(CodingStyleLog) << "Invalid sender in _privateSlot";
        return;
    }

    // Use Fact System properly: cookedValue for display, rawValue for storage
    const QVariant cookedValue = fact->cookedValue();
    const QVariant rawValue = fact->rawValue();

    qCDebug(CodingStyleLog) << "Fact changed:" << fact->name()
                            << "cooked:" << cookedValue
                            << "raw:" << rawValue;

    // Example switch statement with proper formatting
    QVariant typedValue;
    switch (fact->type()) {
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            typedValue.setValue(cookedValue.toInt());
            break;
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            typedValue.setValue(cookedValue.toUInt());
            break;
        case FactMetaData::valueTypeFloat: {
            // Use braces for local variable scope in case statements
            const int localScopedVar = 1;
            Q_UNUSED(localScopedVar);  // OK to use Q_UNUSED for intentionally unused variables
            typedValue.setValue(cookedValue.toFloat());
            break;
        }
        case FactMetaData::valueTypeDouble:
            typedValue.setValue(cookedValue.toDouble());
            break;
        default:
            qCWarning(CodingStyleLog) << "Unhandled fact type:" << fact->type();
            break;
    }
}

void CodingStyle::_methodWithManyArguments(
    QObject* parent,
    const QString& caption,
    const QString& dir,
    int options1,
    int /* options2 */,  // Unused arguments: comment out name but keep type
    int options3)
{
    // Do not use Q_UNUSED for method parameters - comment out the parameter name instead
    // This makes it clear the parameter is intentionally unused
    // Implementation here...
}
