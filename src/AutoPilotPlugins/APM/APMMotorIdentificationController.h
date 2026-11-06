#pragma once

#include <optional>

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtQmlIntegration/QtQmlIntegration>

#include "MotorIdentificationController.h"

Q_DECLARE_LOGGING_CATEGORY(APMMotorIdentificationControllerLog)

/// ArduPilot motor identification. Uses the layout for FRAME_CLASS/FRAME_TYPE so each position is where ArduPilot
/// expects a motor, and fixes a wrong motor order by reassigning SERVOn_FUNCTION.
class APMMotorIdentificationController : public MotorIdentificationController
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit APMMotorIdentificationController(QObject* parent = nullptr);

    struct Layout
    {
        QList<int> motorNumbers;  ///< MotorN output function number of each motor, in motor test order
        QList<int> testOrders;    ///< Motor test number of each motor
        QList<double> angles;     ///< Angle of each motor, clockwise from the nose
    };

    /// Reassigns SERVOn_FUNCTION so each identified motor is driven as the motor ArduPilot expects at its position
    void remapMotors() override;

    /// One line for each SERVOn_FUNCTION which remapMotors() changes
    QStringList remapChanges() const override;

    /// @return Layout from ArduPilot's motor matrix, nullopt if the frame is unknown or has stacked motors
    [[nodiscard]] static std::optional<Layout> loadLayout(int frameClass, int frameType);

    /// @param motorNumbers MotorN number of each motor, in motor test order
    /// @param selectedPositions Position the user selected for each motor
    /// @param servoFunctions Current SERVOn_FUNCTION value keyed by n
    /// @return New SERVOn_FUNCTION value keyed by n, only for outputs which change
    [[nodiscard]] static QMap<int, int> remapServoFunctions(const QList<int>& motorNumbers,
                                                            const QList<int>& selectedPositions,
                                                            const QMap<int, int>& servoFunctions);

    /// @return SERVOn_FUNCTION value for MotorN, -1 if out of range
    [[nodiscard]] static int motorFunction(int motorNumber);

private:
    QMap<int, int> _servoFunctions() const;
    static QString _servoFunctionParam(int servo);

    static constexpr int _maxServoOutputs = 32;

    QList<int> _motorNumbers;
};
