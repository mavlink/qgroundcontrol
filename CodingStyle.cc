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

// Note how the Qt headers and the QGroundControl headers above are kept separate

#include "CodingStyle.h"
#include "Fact.h"
#include "QGCLoggingCategory.h"

#include <QtCore/File>

#include <math.h>

QGC_LOGGING_CATEGORY(CodingStyleLog, "CodingStyleLog")

CodingStyle::CodingStyle(QObject *parent)
    : QObject(parent)
{

}

CodingStyle::~CodingStyle()
{

}

/// Document non-obvious private methods in the code file.
void CodingStyle::_privateMethod()
{
    // Always include braces even for single line if/for/...
    if (uas != _uasId) {
        return;
    }

    // Note the brace placement
    if (_lastSeenComponent == -1) {
        _lastSeenComponent = component;
    } else {
        Q_ASSERT(component == _lastSeenComponent);  // Asserts are your friend
    }
}

void CodingStyle::_privateMethod2()
{
    const Fact *const fact = qobject_cast<const Fact*>(sender());
    Q_ASSERT(fact);

    QVariant typedValue;
    switch (fact->type()) {
        case FactMetaData::valueTypeInt8:
        case FactMetaData::valueTypeInt16:
        case FactMetaData::valueTypeInt32:
            typedValue.setValue(QVariant(value.toInt()));
            break;
        case FactMetaData::valueTypeUint8:
        case FactMetaData::valueTypeUint16:
        case FactMetaData::valueTypeUint32:
            typedValue.setValue(value.toUInt());
            break;
        case FactMetaData::valueTypeFloat: {
            const int localScopedVar = 1;
            typedValue.setValue(value.toFloat() + localScopedVar);
        }
            break;
        case FactMetaData::valueTypeDouble:
            typedValue.setValue(value.toDouble());
            break;
        default:
            break;
    }
}

void CodingStyle::_methodWithManyArguments(QWidget *parent,
                                           const QString &caption,
                                           const QString &dir,
                                           Options options1,
                                           Options /* options2 */,
                                           Options options3)
{
    // options2 is an unused method argument.
    // Do not use Q_UNUSUED and do not just remove the argument name and leave the type.

    // Implementataion here...
}
