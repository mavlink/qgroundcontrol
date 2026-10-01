#include "LogViewerControllerTest.h"

#include <QtCore/QVariantMap>

#include "LogViewerController.h"

void LogViewerControllerTest::_fieldOrderNumericIndexTest()
{
    LogViewerController controller;
    controller.setPlottableFields({
        QStringLiteral("esc_status[10].esc_count"),
        QStringLiteral("actuator_motors.control[10]"),
        QStringLiteral("esc_status[2].esc_count"),
        QStringLiteral("actuator_motors.control[2]"),
        QStringLiteral("actuator_motors.control[0]"),
        QStringLiteral("esc_status.esc_count"),
        QStringLiteral("actuator_motors.control[1]"),
    });

    const QStringList expectedFields = {
        QStringLiteral("actuator_motors.control[0]"), QStringLiteral("actuator_motors.control[1]"),
        QStringLiteral("actuator_motors.control[2]"), QStringLiteral("actuator_motors.control[10]"),
        QStringLiteral("esc_status.esc_count"),       QStringLiteral("esc_status[2].esc_count"),
        QStringLiteral("esc_status[10].esc_count"),
    };
    QCOMPARE(controller.plottableFields(), expectedFields);

    controller.toggleGroupExpanded(QStringLiteral("actuator_motors"));

    QStringList rowNames;
    for (const QVariant& row : controller.fieldRows()) {
        const QVariantMap rowMap = row.toMap();
        const bool isGroup = rowMap.value(QStringLiteral("rowType")).toString() == QStringLiteral("group");
        rowNames.append(rowMap.value(isGroup ? QStringLiteral("group") : QStringLiteral("fullName")).toString());
    }

    const QStringList expectedRows = {
        QStringLiteral("actuator_motors"),
        QStringLiteral("actuator_motors.control[0]"),
        QStringLiteral("actuator_motors.control[1]"),
        QStringLiteral("actuator_motors.control[2]"),
        QStringLiteral("actuator_motors.control[10]"),
        QStringLiteral("esc_status"),
        QStringLiteral("esc_status[2]"),
        QStringLiteral("esc_status[10]"),
    };
    QCOMPARE(rowNames, expectedRows);
}

UT_REGISTER_TEST(LogViewerControllerTest, TestLabel::Unit, TestLabel::AnalyzeView)
