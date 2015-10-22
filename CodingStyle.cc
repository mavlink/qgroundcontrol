/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

// This is an example class c++ file which is used to describe the QGroundControl
// coding style. In general almost everything in here has some coding style meaning.
// Not all style choices are explained.

#include "CodingStyle.h"
#include "QGCApplication.h"

#include <QFile>
#include <QDebug>

#include <math.h>

// Note how the Qt headers and the QGroundControl headers above are kept seperate

Q_LOGGING_CATEGORY(CodingStyleLog, "CodingStyleLog")

const int CodingStyle::_privateStaticVariable = 0;

CodingStyle::CodingStyle(QObject* parent) :
    QObject(parent),
    _protectedVariable(1),
    _privateVariable1(2),
    _privateVariable2(3)
{

}

/// Document non-obvious private methods in the code file.
void CodingStyle::_privateMethod(void)
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

void CodingStyle::_privateMethod2(void)
{
    Fact* fact = qobject_cast<Fact*>(sender());
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
            int localScopedVar = 1;
            typedValue.setValue(value.toFloat());
        }
            break;
            
        case FactMetaData::valueTypeDouble:
            typedValue.setValue(value.toDouble());
            break;
    }
}

void CodingStyle::_methodWithManyArguments(QWidget*         parent,
                                           const QString&   caption,
                                           const QString&   dir,
                                           Options          options1,
                                           Options          options2,
                                           Options          options3)
{
    // Implementataion here...
}
