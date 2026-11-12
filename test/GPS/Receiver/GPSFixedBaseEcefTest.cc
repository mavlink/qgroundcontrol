#include "GPSFixedBaseEcefTest.h"

#include <cmath>

#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "GPSFixedBaseEcef.h"
#include "RTKSettings.h"

namespace {

void savePosition(RTKSettings& settings, double latitude, double longitude, double altitude)
{
    settings.fixedBasePositionLatitude()->setRawValue(latitude);
    settings.fixedBasePositionLongitude()->setRawValue(longitude);
    settings.fixedBasePositionAltitude()->setRawValue(altitude);
}

/// Enters @a text in @a fact as its text field does.
void enter(Fact* fact, const char* text)
{
    fact->setCookedValue(QString::fromLatin1(text));
}

}  // namespace

void GPSFixedBaseEcefTest::_followsSavedPosition()
{
    RTKSettings settings;
    savePosition(settings, 45, 45, 0);
    GPSFixedBaseEcef ecef;
    ecef.setSettings(&settings);
    QCOMPARE(ecef.x()->cookedValueString(), QStringLiteral("3194419.1451"));
    QCOMPARE(ecef.y()->cookedValueString(), QStringLiteral("3194419.1451"));
    QCOMPARE(ecef.z()->cookedValueString(), QStringLiteral("4487348.4089"));
    // Metres, whatever the application's distance unit is, which the label says.
    QVERIFY(ecef.x()->cookedUnits().isEmpty());
    QCOMPARE(ecef.x()->label(), QStringLiteral("Base Position X (ECEF, m)"));

    // A saved site that is used, or the other form of the position, changes the saved position.
    savePosition(settings, 0, 180, -30);
    QCOMPARE(ecef.x()->cookedValueString(), QStringLiteral("-6378107.0000"));
    QCOMPARE(ecef.y()->cookedValueString(), QStringLiteral("0.0000"));
    QCOMPARE(ecef.z()->cookedValueString(), QStringLiteral("0.0000"));
    QVERIFY(!ecef.unsaved());
}

void GPSFixedBaseEcefTest::_editSavesPosition()
{
    RTKSettings settings;
    savePosition(settings, 45, 45, 0);
    GPSFixedBaseEcef ecef;
    ecef.setSettings(&settings);
    GPSFixedBaseEcef other;
    other.setSettings(&settings);

    // 46 degrees north, 7 degrees east and 1500 m, to the 0.1 mm a data sheet gives.
    enter(ecef.x(), "4406271.7614");
    enter(ecef.y(), "541022.1434");
    enter(ecef.z(), "4566326.5505");
    QVERIFY(!ecef.unsaved());
    QVERIFY(std::abs(settings.fixedBasePositionLatitude()->rawValue().toDouble() - 46) < 1e-9);
    QVERIFY(std::abs(settings.fixedBasePositionLongitude()->rawValue().toDouble() - 7) < 1e-9);
    QVERIFY(std::abs(settings.fixedBasePositionAltitude()->rawValue().toDouble() - 1500) < 1e-4);
    // The coordinates stay as entered, and another view of the position follows.
    QCOMPARE(ecef.x()->rawValue().toDouble(), 4406271.7614);
    QCOMPARE(ecef.y()->rawValue().toDouble(), 541022.1434);
    QCOMPARE(ecef.z()->rawValue().toDouble(), 4566326.5505);
    QCOMPARE(other.x()->cookedValueString(), QStringLiteral("4406271.7614"));
    QCOMPARE(other.y()->cookedValueString(), QStringLiteral("541022.1434"));
    QCOMPARE(other.z()->cookedValueString(), QStringLiteral("4566326.5505"));

    // One coordinate edited moves the position by that much along its axis.
    enter(ecef.z(), "4566327.5505");
    QCOMPARE(other.x()->cookedValueString(), QStringLiteral("4406271.7614"));
    QCOMPARE(other.z()->cookedValueString(), QStringLiteral("4566327.5505"));
}

void GPSFixedBaseEcefTest::_showingChangesNothing()
{
    RTKSettings settings;
    {
        // 2400 m up, where a height saved in single precision is good to 0.12 mm only.
        GPSFixedBaseEcef entered;
        entered.setSettings(&settings);
        enter(entered.x(), "4406892.5490");
        enter(entered.y(), "541098.3666");
        enter(entered.z(), "4566974.2225");
    }
    QSignalSpy latitude(settings.fixedBasePositionLatitude(), &Fact::rawValueChanged);
    QSignalSpy longitude(settings.fixedBasePositionLongitude(), &Fact::rawValueChanged);
    QSignalSpy altitude(settings.fixedBasePositionAltitude(), &Fact::rawValueChanged);
    // Shown again, as when the form is opened the next time, the coordinates read as they were entered.
    GPSFixedBaseEcef shown;
    shown.setSettings(&settings);
    QCOMPARE(shown.x()->cookedValueString(), QStringLiteral("4406892.5490"));
    QCOMPARE(shown.y()->cookedValueString(), QStringLiteral("541098.3666"));
    QCOMPARE(shown.z()->cookedValueString(), QStringLiteral("4566974.2225"));
    QVERIFY(latitude.isEmpty() && longitude.isEmpty() && altitude.isEmpty());
}

void GPSFixedBaseEcefTest::_savesOnlyNearEarthSurface()
{
    RTKSettings settings;
    savePosition(settings, 0, 0, 0);
    GPSFixedBaseEcef ecef;
    ecef.setSettings(&settings);
    QSignalSpy unsaved(&ecef, &GPSFixedBaseEcef::unsavedChanged);

    // A position typed one coordinate after the other passes through points far from the surface, which save nothing.
    enter(ecef.x(), "0");
    QVERIFY(ecef.unsaved());
    QCOMPARE(unsaved.size(), 1);
    QCOMPARE(settings.fixedBasePositionLatitude()->rawValue().toDouble(), 0.);
    QCOMPARE(settings.fixedBasePositionAltitude()->rawValue().toDouble(), 0.);
    enter(ecef.z(), "6356752.3142");
    QVERIFY(!ecef.unsaved());
    QCOMPARE(unsaved.size(), 2);
    QVERIFY(std::abs(settings.fixedBasePositionLatitude()->rawValue().toDouble() - 90) < 1e-9);
    QVERIFY(std::abs(settings.fixedBasePositionAltitude()->rawValue().toDouble()) < 1e-4);

    // A value beyond any point near the surface is not taken at all.
    QVERIFY(!ecef.x()->validate(QStringLiteral("70000000"), false).isEmpty());

    // The saved position replaces coordinates that were left unsaved.
    enter(ecef.z(), "0");
    QVERIFY(ecef.unsaved());
    savePosition(settings, 0, 90, 100);
    QVERIFY(!ecef.unsaved());
    QCOMPARE(ecef.y()->cookedValueString(), QStringLiteral("6378237.0000"));
}

UT_REGISTER_TEST(GPSFixedBaseEcefTest, TestLabel::Unit)
