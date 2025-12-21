/****************************************************************************
 *
 * (c) 2009-2025 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

// This is an example class header file which is used to describe the QGroundControl
// coding style. In general almost everything in here has some coding style meaning.
// Not all style choices are explained.
//
// QGroundControl requires C++20. Use modern C++ features where appropriate.

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include <limits.h>
#include <span>
#include <string_view>

#include "Fact.h"
#include "Vehicle.h"

// Note how the Qt headers, System headers and the QGroundControl headers above are kept in separate groups
// with blank lines between them. Within each group, headers are sorted alphabetically.
// Use full paths for Qt headers (e.g., QtCore/QObject not QObject).

// Forward declarations for classes that are only used as pointers/references
class Vehicle;

// If you are going to use a logging category for a class it should have the same name as the class
// with a suffix of Log.
Q_DECLARE_LOGGING_CATEGORY(CodingStyleLog)

/// Here is the class documentation. Class names are PascalCase. If you override any of the Qt base classes to provide
/// generic base implementations for widespread use prefix the class name with QGC. For example:
/// For normal single use classes do not prefix the name with QGC.
///
/// Qt6 QML Integration: Use QML_ELEMENT for QML-creatable types, QML_SINGLETON for singletons,
/// and QML_UNCREATABLE("reason") for C++-only instantiation.

class CodingStyle : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Vehicle.h")  // Use Q_MOC_INCLUDE for forward-declared types used in Q_PROPERTY

    /// Q_PROPERTY definitions for QML binding
    /// Format: Q_PROPERTY(Type name READ getter WRITE setter NOTIFY signal)
    /// Use CONSTANT for properties that never change
    /// Use tab spacing to align property elements vertically. Don't take to an extreme, goal is to improve readability.
    Q_PROPERTY(int      exampleProperty     READ exampleProperty WRITE setExampleProperty NOTIFY examplePropertyChanged)
    Q_PROPERTY(Vehicle* vehicle             READ vehicle            CONSTANT)
    Q_PROPERTY(bool     readOnlyProperty    READ readOnlyProperty   NOTIFY readOnlyPropertyChanged)

public:
    explicit CodingStyle(QObject* parent = nullptr);  // Use nullptr, not NULL
    ~CodingStyle() override;                          // Use override keyword for virtual destructors

    // Enums exposed to QML must use Q_ENUM
    enum class ExampleEnum
    {
        EnumValue1,
        EnumValue2,
        EnumValue3,
    };
    Q_ENUM(ExampleEnum)

    /// Document public methods which are non-obvious in the header file
    /// Use Q_INVOKABLE for methods callable from QML
    /// Use [[nodiscard]] for functions whose return value should not be ignored
    [[nodiscard]] Q_INVOKABLE bool publicMethod1();
    Q_INVOKABLE void performAction(const QString& param);

    // C++20: Use std::string_view for read-only string parameters when Qt types aren't needed
    [[nodiscard]] bool validateInput(std::string_view input) const;

    // C++20: Use std::span for array-like parameters instead of pointer + size
    void processData(std::span<const int> data);

    // Public getters/setters - use [[nodiscard]] for getters
    [[nodiscard]] int exampleProperty() const
    {
        return _exampleProperty;
    }
    void setExampleProperty(int value);
    [[nodiscard]] Vehicle* vehicle() const
    {
        return _vehicle;
    }
    [[nodiscard]] bool readOnlyProperty() const
    {
        return _readOnlyProperty;
    }

   signals:
    /// Document signals which are non-obvious in the header file
    /// Signals should be emitted when properties change to maintain QML bindings
    void examplePropertyChanged(int newValue);
    void readOnlyPropertyChanged();
    void qtSignal();

public slots:
    // Public slots should only be used if the slot is connected to from another class.
    // Most slots should be private. Prefer signals/slots over direct method calls for loose coupling.
    void publicSlot();

    // Don't use protected methods or variables unless the class is specifically meant to be used as a base class.
protected:
    int _protectedVariable = 0;  ///< variable names are camelCase

    void _protectedMethod();     ///< method names are camelCase

private slots:
    void _privateSlot();

private:
    // Private methods and variable names begin with an "_". Documentation for
    // non-obvious private methods goes in the header file.
    void _privateMethod();
    void _commonInit();

    // For methods with many arguments, align parameters vertically
    void _methodWithManyArguments(QObject* parent, const QString& caption, const QString& dir, int options1,
                                  int options2, int options3);

    /// Document non-obvious variables in the header file. Long descriptions go here.
    int _exampleProperty = 0;
    bool _readOnlyProperty = false;
    Vehicle* _vehicle = nullptr;

    int _privateVariable1 = 2;  ///< Short descriptions go here
    int _privateVariable2 = 3;

    static constexpr int _privateStaticVariable = 42;  // Use constexpr for compile-time constants
};
