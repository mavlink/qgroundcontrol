#include "VehicleFactGroupTest.h"

#include <QtGui/QVector3D>

#include "MAVLinkLib.h"
#include "Vehicle.h"
#include "VehicleFactGroup.h"

UT_REGISTER_TEST(VehicleFactGroupTest, TestLabel::Integration, TestLabel::Vehicle)

void VehicleFactGroupTest::_attitudeQuaternionReprOffsetRates_data()
{
    QTest::addColumn<QVector3D>("bodyRates");
    QTest::addColumn<QVector3D>("displayRates");

    // PX4 tailsitter in fixed-wing mode: repr_offset_q is +90 deg about pitch, so display X is body -Z
    QTest::newRow("yaw") << QVector3D(0.0f, 0.0f, 0.5f) << QVector3D(-0.5f, 0.0f, 0.0f);
    QTest::newRow("roll") << QVector3D(0.5f, 0.0f, 0.0f) << QVector3D(0.0f, 0.0f, 0.5f);
    QTest::newRow("pitch") << QVector3D(0.0f, 0.3f, 0.0f) << QVector3D(0.0f, 0.3f, 0.0f);
}

void VehicleFactGroupTest::_attitudeQuaternionReprOffsetRates()
{
    QFETCH(QVector3D, bodyRates);
    QFETCH(QVector3D, displayRates);

    const float reprOffset[4] = {static_cast<float>(M_SQRT1_2), 0.0f, static_cast<float>(M_SQRT1_2), 0.0f};
    mavlink_message_t message{};
    (void) mavlink_msg_attitude_quaternion_pack(vehicle()->id(), vehicle()->compId(), &message, 0, 1.0f, 0.0f, 0.0f,
                                                0.0f, bodyRates.x(), bodyRates.y(), bodyRates.z(), reprOffset);

    VehicleFactGroup factGroup;
    factGroup.handleMessage(vehicle(), message);

    QVERIFY(qAbs(factGroup.rollRate()->rawValue().toDouble() - qRadiansToDegrees(displayRates.x())) < 1e-3);
    QVERIFY(qAbs(factGroup.pitchRate()->rawValue().toDouble() - qRadiansToDegrees(displayRates.y())) < 1e-3);
    QVERIFY(qAbs(factGroup.yawRate()->rawValue().toDouble() - qRadiansToDegrees(displayRates.z())) < 1e-3);
}
