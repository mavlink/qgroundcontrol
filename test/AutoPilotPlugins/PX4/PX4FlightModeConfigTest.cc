#include "PX4FlightModeConfigTest.h"

#include "PX4FlightModeConfig.h"

UT_REGISTER_TEST_LIGHTWEIGHT(PX4FlightModeConfigTest, TestLabel::Unit)

namespace {

using namespace PX4FlightModeConfig;

QStringList describe(const QList<Choice>& choices)
{
    QStringList descriptions;
    for (const Choice& choice : choices) {
        descriptions.append(QStringLiteral("%1=%2").arg(choice.label).arg(choice.value));
    }

    return descriptions;
}

const QStringList kModeStrings{QStringLiteral("Position"), QStringLiteral("External Mode 1"),
                               QStringLiteral("External Mode 2")};
const QVariantList kSlotValues{2, 100, 101};

}  // namespace

void PX4FlightModeConfigTest::_selectionMethod_data()
{
    QTest::addColumn<int>("modeChannel");
    QTest::addColumn<int>("buttonMask");
    QTest::addColumn<SelectionMethod>("expected");

    QTest::newRow("channel") << 5 << 0 << SelectionMethod::Switch;
    QTest::newRow("channelWins") << 5 << 64 << SelectionMethod::Switch;
    QTest::newRow("buttons") << 0 << 64 << SelectionMethod::Buttons;
    QTest::newRow("none") << 0 << 0 << SelectionMethod::None;
}

void PX4FlightModeConfigTest::_selectionMethod()
{
    QFETCH(int, modeChannel);
    QFETCH(int, buttonMask);
    QFETCH(SelectionMethod, expected);

    QCOMPARE(selectionMethod(modeChannel, buttonMask), expected);
}

void PX4FlightModeConfigTest::_buttonChannels()
{
    QCOMPARE(buttonChannels(0), QList<int>());
    QCOMPARE(buttonChannels((1 << 6) | (1 << 8)), (QList<int>{7, 9}));
    // PX4 uses at most six buttons
    QCOMPARE(buttonChannels((1 << kMaxButtonChannels) - 1), (QList<int>{1, 2, 3, 4, 5, 6}));
    QCOMPARE(ignoredButtonChannels((1 << 12) - 1), (QList<int>{7, 8, 9, 10, 11, 12}));
    QCOMPARE(ignoredButtonChannels((1 << 6) | (1 << 8)), QList<int>());
}

void PX4FlightModeConfigTest::_externalModeIndex_data()
{
    QTest::addColumn<QString>("param");
    QTest::addColumn<int>("value");
    QTest::addColumn<int>("expected");

    QTest::newRow("slotBuiltIn") << "COM_FLTMODE1" << 2 << 0;
    QTest::newRow("slotExternal1") << "COM_FLTMODE1" << 100 << 1;
    QTest::newRow("slotExternal8") << "COM_FLTMODE6" << 107 << 8;
    QTest::newRow("slotPastExternal8") << "COM_FLTMODE6" << 108 << 0;
    QTest::newRow("bootBuiltIn") << "COM_FLTMODE_BOOT" << 4 << 0;
    QTest::newRow("bootExternal1") << "COM_FLTMODE_BOOT" << 23 << 1;
    QTest::newRow("bootExternal8") << "COM_FLTMODE_BOOT" << 30 << 8;
}

void PX4FlightModeConfigTest::_externalModeIndex()
{
    QFETCH(QString, param);
    QFETCH(int, value);
    QFETCH(int, expected);

    QCOMPARE(externalModeIndex(param, value), expected);
}

// Only external modes the vehicle has registered are offered, by their reported name. A slot already set to one that is
// not registered stays listed so the combo box is not blank.
void PX4FlightModeConfigTest::_modeChoices_data()
{
    QTest::addColumn<int>("current");
    QTest::addColumn<QStringList>("externalNames");
    QTest::addColumn<QStringList>("expected");

    const QStringList names{QStringLiteral("Survey Assist"), {}, {}, {}, {}, {}, {}, {}};
    QTest::newRow("registeredNamedUnregisteredHidden")
        << 2 << names << QStringList{"Position=2", "Survey Assist=100"};
    QTest::newRow("currentUnregisteredKept")
        << 101 << names << QStringList{"Position=2", "Survey Assist=100", "External Mode 2 (not running)=101"};
    QTest::newRow("noNamesReported") << 2 << QStringList{} << QStringList{"Position=2"};
}

void PX4FlightModeConfigTest::_modeChoices()
{
    QFETCH(int, current);
    QFETCH(QStringList, externalNames);
    QFETCH(QStringList, expected);

    QCOMPARE(describe(modeChoices(QStringLiteral("COM_FLTMODE1"), kModeStrings, kSlotValues, externalNames, current)),
             expected);
}

void PX4FlightModeConfigTest::_modeName()
{
    const QStringList names{QStringLiteral("Survey Assist"), {}, {}, {}, {}, {}, {}, {}};
    const QStringList strings{QStringLiteral("Hold"), QStringLiteral("External 1"), QStringLiteral("External 2")};
    const QVariantList bootValues{4, 23, 24};
    const QString boot = QStringLiteral("COM_FLTMODE_BOOT");

    QCOMPARE(modeName(boot, strings, bootValues, names, 23), QStringLiteral("Survey Assist"));
    QCOMPARE(modeName(boot, strings, bootValues, names, 24), QStringLiteral("External 2 (not running)"));
    QCOMPARE(modeName(boot, strings, bootValues, names, 99), QString());
}
