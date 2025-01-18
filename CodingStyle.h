/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// This is an example class header file which is used to describe the QGroundControl
// coding style. In general almost everything in here has some coding style meaning.
// Not all style choices are explained.

#pragma once

/// Note how the Qt headers, System, headers and the QGroundControl headers above are kept in separate groups
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QXmlStreamReader>

#include <limits.h>

/// If you are going to use a logging category for a class it should have the same name as the class
/// with a suffix of Log.
Q_DECLARE_LOGGING_CATEGORY(CodingStyleLog)

/// Here is the class documentation. Class names are PascalCase. If you override any of the Qt base classes to provide
/// generic base implementations for widespread use prefix the class name with QGC. For example:
///     QGCMessageBox - is a QGC special vesion of Qt MessageBox
///     QGCPalette - is a QGC special version of Qt Palette
/// For normal single use classes do not prefix them name with QGC.

class CodingStyle : public QObject
{
    Q_OBJECT

public:
    explicit CodingStyle(QObject *parent = nullptr);
    ~CodingStyle();

    /// Document public methods which are non-obvious in the header file
    bool publicMethod1();

signals:
    /// Document signals which are non-obvious in the header file
    void qtSignal();

public slots:
    /// Public slots should only be used if the slot is connected to from another class. Majority of slots
    /// should be private.
    void publicSlot();

protected:
    /// Don't use protected methods or variables unless the class is specifically meant to be used as a base class.
    void _protectedMethod();    ///< method names are camelCase

    int _protectedVariable = 1; ///< variable names are camelCase

private slots:
    void _privateSlot();

private:
    /// Private methods and variable names begin with an "_". Documentation for
    /// non-obvious private methods goes in the code file, not the header.
    void _privateMethod();

    void _methodWithManyArguments(QWidget *parent, const QString &caption, const QString &dir, Options options1, Options options2, Options options3);

    /// Document non-obvious variables in the header file. Long descriptions go here.
    int _privateVariable1 = 2;
    int _privateVariable2 = 3; ///< Short descriptions go here

    static constexpr int _privateStaticVariable = 0;
};
