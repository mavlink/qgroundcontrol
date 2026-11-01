#include "APMMotorIdentificationController.h"

#include <cmath>

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtMath>

#include "Fact.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

QGC_LOGGING_CATEGORY(APMMotorIdentificationControllerLog, "AutoPilotPlugins.APMMotorIdentificationController")

APMMotorIdentificationController::APMMotorIdentificationController(QObject* parent)
    : MotorIdentificationController(parent)
{
    int frameClass = -1;
    int frameType = -1;
    if (_vehicle) {
        ParameterManager* const parameterManager = _vehicle->parameterManager();
        static const QString frameClassParam = QStringLiteral("FRAME_CLASS");
        static const QString frameTypeParam = QStringLiteral("FRAME_TYPE");
        if (parameterManager->parameterExists(ParameterManager::defaultComponentId, frameClassParam) &&
            parameterManager->parameterExists(ParameterManager::defaultComponentId, frameTypeParam)) {
            frameClass = parameterManager->getParameter(ParameterManager::defaultComponentId, frameClassParam)
                             ->rawValue()
                             .toInt();
            frameType = parameterManager->getParameter(ParameterManager::defaultComponentId, frameTypeParam)
                            ->rawValue()
                            .toInt();
        }
    }

    // Without the layout the even spacing could put motors in the wrong place, so identification isn't offered
    const std::optional<Layout> layout = loadLayout(frameClass, frameType);
    if (!layout) {
        qCDebug(APMMotorIdentificationControllerLog) << "No layout"
                                                     << "FRAME_CLASS:" << frameClass << "FRAME_TYPE:" << frameType;
        _setLayout({}, {}, false);
        return;
    }

    _motorNumbers = layout->motorNumbers;
    _setLayout(layout->testOrders, layout->angles, true);
}

std::optional<APMMotorIdentificationController::Layout> APMMotorIdentificationController::loadLayout(int frameClass,
                                                                                                     int frameType)
{
    QFile file(QStringLiteral(":/json/APMMotorLayouts.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(APMMotorIdentificationControllerLog) << "Unable to open motor layouts:" << file.errorString();
        return std::nullopt;
    }

    const QJsonArray layouts =
        QJsonDocument::fromJson(file.readAll()).object().value(QStringLiteral("layouts")).toArray();
    for (const QJsonValue& layoutValue : layouts) {
        const QJsonObject layoutObject = layoutValue.toObject();
        if ((layoutObject.value(QStringLiteral("class")).toInt() != frameClass) ||
            (layoutObject.value(QStringLiteral("type")).toInt() != frameType)) {
            continue;
        }

        Layout layout;
        for (const QJsonValue& motorValue : layoutObject.value(QStringLiteral("motors")).toArray()) {
            const QJsonObject motorObject = motorValue.toObject();
            const double roll = motorObject.value(QStringLiteral("roll")).toDouble();
            const double pitch = motorObject.value(QStringLiteral("pitch")).toDouble();
            // A tricopter's tail servo is in the motor test order but has no roll or pitch factor
            if ((roll == 0.0) && (pitch == 0.0)) {
                continue;
            }

            // ArduPilot sets roll to -sin(angle) and pitch to cos(angle)
            double angle = qRadiansToDegrees(std::atan2(-roll, pitch));
            if (angle < 0.0) {
                angle += 360.0;
            }
            for (const double otherAngle : layout.angles) {
                if (std::abs(angle - otherAngle) < 1.0) {
                    qCDebug(APMMotorIdentificationControllerLog)
                        << "Stacked motors in frame" << frameClass << frameType;
                    return std::nullopt;
                }
            }

            layout.motorNumbers.append(motorObject.value(QStringLiteral("number")).toInt());
            layout.testOrders.append(motorObject.value(QStringLiteral("testOrder")).toInt());
            layout.angles.append(angle);
        }
        return layout;
    }

    return std::nullopt;
}

int APMMotorIdentificationController::motorFunction(int motorNumber)
{
    // ArduPilot SRV_Channel::Function: Motor1-8 are 33-40, Motor9-12 are 82-85
    if ((motorNumber >= 1) && (motorNumber <= 8)) {
        return 32 + motorNumber;
    }
    if ((motorNumber >= 9) && (motorNumber <= 12)) {
        return 73 + motorNumber;
    }
    return -1;
}

QMap<int, int> APMMotorIdentificationController::remapServoFunctions(const QList<int>& motorNumbers,
                                                                     const QList<int>& selectedPositions,
                                                                     const QMap<int, int>& servoFunctions)
{
    // The motor driven by an output spun at the selected position, so the output takes the function of the motor
    // which belongs there
    QMap<int, int> functionMap;
    for (int motor = 0; motor < selectedPositions.count(); motor++) {
        const int position = selectedPositions[motor];
        if ((motor >= motorNumbers.count()) || (position < 0) || (position >= motorNumbers.count())) {
            return {};
        }
        functionMap[motorFunction(motorNumbers[motor])] = motorFunction(motorNumbers[position]);
    }

    QMap<int, int> newFunctions;
    for (auto it = servoFunctions.constBegin(); it != servoFunctions.constEnd(); ++it) {
        const int newFunction = functionMap.value(it.value(), it.value());
        if (newFunction != it.value()) {
            newFunctions[it.key()] = newFunction;
        }
    }
    return newFunctions;
}

QStringList APMMotorIdentificationController::remapChanges() const
{
    QStringList changes;
    if (!complete() || orderCorrect()) {
        return changes;
    }

    const auto motorName = [](int function) {
        const int motorNumber = (function >= motorFunction(9)) ? function - 73 : function - 32;
        return tr("Motor %1").arg(motorNumber);
    };

    const QMap<int, int> servoFunctions = _servoFunctions();
    const QMap<int, int> newFunctions = remapServoFunctions(_motorNumbers, _selections(), servoFunctions);
    for (auto it = newFunctions.constBegin(); it != newFunctions.constEnd(); ++it) {
        changes.append(
            tr("%1: %2 to %3")
                .arg(_servoFunctionParam(it.key()), motorName(servoFunctions[it.key()]), motorName(it.value())));
    }
    return changes;
}

void APMMotorIdentificationController::remapMotors()
{
    if (!complete() || !_vehicle) {
        qCWarning(APMMotorIdentificationControllerLog) << "Identification not complete";
        return;
    }

    const QMap<int, int> newFunctions = remapServoFunctions(_motorNumbers, _selections(), _servoFunctions());
    for (auto it = newFunctions.constBegin(); it != newFunctions.constEnd(); ++it) {
        qCDebug(APMMotorIdentificationControllerLog) << _servoFunctionParam(it.key()) << "->" << it.value();
        _vehicle->parameterManager()
            ->getParameter(ParameterManager::defaultComponentId, _servoFunctionParam(it.key()))
            ->setRawValue(it.value());
    }

    cancel();
}

QMap<int, int> APMMotorIdentificationController::_servoFunctions() const
{
    QMap<int, int> servoFunctions;
    if (!_vehicle) {
        return servoFunctions;
    }

    ParameterManager* const parameterManager = _vehicle->parameterManager();
    for (int servo = 1; servo <= _maxServoOutputs; servo++) {
        const QString param = _servoFunctionParam(servo);
        if (parameterManager->parameterExists(ParameterManager::defaultComponentId, param)) {
            servoFunctions[servo] =
                parameterManager->getParameter(ParameterManager::defaultComponentId, param)->rawValue().toInt();
        }
    }
    return servoFunctions;
}

QString APMMotorIdentificationController::_servoFunctionParam(int servo)
{
    return QStringLiteral("SERVO%1_FUNCTION").arg(servo);
}
