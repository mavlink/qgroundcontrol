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

// This is an example class header file which is used to describe the QGroundControl
// coding style. In general almost everything in here has some coding style meaning.
// Not all style choices are explained.

#ifndef CodingStyle_H
#define CodingStyle_H

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>

#include <limits.h>

#include "Fact.h"
#include "UASInterface.h"

// Note how the Qt headers, System, headers and the QGroundControl headers above are kept in seperate groups

// If you are going to use a logging category for a class it should have the same name as the class
// with a suffix of Log.
Q_DECLARE_LOGGING_CATEGORY(CodingStyleLog)

/// Here is the class documentation. Class names are PascalCase. If you override any of the Qt base classes to provide
/// generic base implementations for widespread use prefix the class name with QGC. For example:
///     QGCMessageBox - is a QGC special vesion of Qt MessageBox
///     QGCPalette - is a QGC special version of Qt Palette
/// For normal single use classes do no prefix them name with QGC.

class CodingStyle : public QObject
{
    Q_OBJECT
    
public:
    CodingStyle(QObject* parent = NULL);
    ~CodingStyle();
    
    /// Document public methods which are non-obvious in the header file
    bool publicMethod1(void);
    
signals:
    /// Document signals which are non-obvious in the header file
    void qtSignal(void);
    
public slots:
    // Public slots should only be used if the slot is connected to from another class. Majority of slots
    // should be private.
    void publicSlot(void);
    
    // Don't use protected methods or variables unless the class is specifically meant to be used as a base class.
protected:
    int _protectedVariable;         ///< variable names are camelCase
    
    void _protectedMethod(void);    ///< method names are camelCase
    
private slots:
    void _privateSlot(void);
    
private:
    // Private methods and variable names begin with an "_". Documentation for
    // non-obvious private methods goes in the code file, not the header.
    void _privateMethod(void);
    
    void _methodWithManyArguments(QWidget* parent, const QString& caption, const QString& dir, Options options1, Options options2, Options options3);

    /// Document non-obvious variables in the header file. Long descriptions go here.
    int _privateVariable1;
    
    int _privateVariable2;    ///< Short descriptions go here
    
    static const int _privateStaticVariable;
};

#endif
