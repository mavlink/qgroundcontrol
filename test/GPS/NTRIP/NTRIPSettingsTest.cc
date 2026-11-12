#include "NTRIPSettingsTest.h"

#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "Fact.h"
#include "NTRIPConfiguration.h"
#include "NTRIPSettings.h"

namespace {
QStringList casterNames(const NTRIPSettings& settings)
{
    QStringList names;
    for (const NTRIPSavedCaster& caster : settings.savedCasters()) {
        names.append(caster.name);
    }
    return names;
}

/// A saved caster entry as the settings store it; @a port and @a more are JSON text.
QString casterJson(const QString& name, const QString& host = QStringLiteral("caster.example.com"),
                   const QString& port = QStringLiteral("2101"), const QString& more = {})
{
    return QStringLiteral(R"({"name":"%1","host":"%2","port":%3,"mountpoint":"MOUNT","username":"user",)"
                          R"("password":"secret","useTls":true,"allowSelfSignedCerts":true%4})")
        .arg(name, host, port, more);
}

/// The pin of a certificate trusted for @a host and @a port, as the session writes it.
QString pinFor(const QString& host, int port, QChar digit = u'a')
{
    NTRIPConnectionConfig caster;
    caster.host = host;
    caster.port = port;
    return caster.endpoint() + QLatin1Char('|') + QString(64, digit);
}

/// A caster without a trusted certificate.
NTRIPSavedCaster savedCaster(const QString& name, const QString& host, int port, const QString& mountpoint = {},
                             const QString& username = {}, const QString& password = {}, bool tls = false)
{
    NTRIPSavedCaster caster;
    caster.name = name;
    caster.host = host;
    caster.port = port;
    caster.mountpoint = mountpoint;
    caster.username = username;
    caster.password = password;
    caster.useTls = tls;
    caster.allowSelfSignedCerts = tls;
    return caster;
}

const NTRIPSavedCaster kNorth =
    savedCaster(QStringLiteral("North network"), QStringLiteral("north.example.com"), 2101, QStringLiteral("NORTH1"),
                QStringLiteral("north-user"), QStringLiteral("north-secret"), true);

void enterCaster(NTRIPSettings& settings, const NTRIPSavedCaster& caster)
{
    settings.ntripServerHostAddress()->setRawValue(caster.host);
    settings.ntripServerPort()->setRawValue(caster.port);
    settings.ntripUsername()->setRawValue(caster.username);
    settings.ntripPassword()->setRawValue(caster.password);
    settings.ntripMountpoint()->setRawValue(caster.mountpoint);
    settings.ntripUseTls()->setRawValue(caster.useTls);
    settings.ntripAllowSelfSignedCerts()->setRawValue(caster.allowSelfSignedCerts);
    settings.ntripPinnedCertificate()->setRawValue(caster.pinnedCertificate);
}

/// The connection settings as a caster named @a name.
NTRIPSavedCaster enteredCaster(NTRIPSettings& settings, const QString& name)
{
    return {.name = name,
            .host = settings.ntripServerHostAddress()->rawValue().toString(),
            .port = settings.ntripServerPort()->rawValue().toInt(),
            .username = settings.ntripUsername()->rawValue().toString(),
            .password = settings.ntripPassword()->rawValue().toString(),
            .mountpoint = settings.ntripMountpoint()->rawValue().toString(),
            .useTls = settings.ntripUseTls()->rawValue().toBool(),
            .allowSelfSignedCerts = settings.ntripAllowSelfSignedCerts()->rawValue().toBool(),
            .pinnedCertificate = settings.ntripPinnedCertificate()->rawValue().toString()};
}
}  // namespace

void NTRIPSettingsTest::_savedCastersLoad_data()
{
    QTest::addColumn<QString>("saved");
    QTest::addColumn<QStringList>("names");
    QTest::addColumn<QString>("warning");
    const QString north = QStringLiteral("North network");
    const QString south = QStringLiteral("South network");
    const QString notArray = QStringLiteral("not a JSON array");
    QTest::newRow("none") << QString() << QStringList() << QString();
    QTest::newRow("casters") << QStringLiteral("[%1,%2]").arg(casterJson(north), casterJson(south))
                             << QStringList{north, south} << QString();
    QTest::newRow("not-json") << QStringLiteral("[{\"name\":") << QStringList() << notArray;
    QTest::newRow("not-an-array") << casterJson(north) << QStringList() << notArray;
    // A caster needs only a name, a host and a port.
    QTest::newRow("caster-alone")
        << QStringLiteral(R"([{"name":"%1","host":"caster.example.com","port":2101}])").arg(north) << QStringList{north}
        << QString();

    // Only whole casters are kept, the first of each name, without the spaces around it.
    const QString host = QStringLiteral("caster.example.com");
    const QStringList entries{
        casterJson(QStringLiteral("  North network ")),
        QStringLiteral("5"),
        QStringLiteral("{}"),
        casterJson(QStringLiteral(" ")),
        casterJson(QString(NTRIPSettings::MaxSavedCasterNameLength + 1, QLatin1Char('n'))),
        casterJson(QStringLiteral("No host"), QString()),
        casterJson(QStringLiteral("Host with a space"), QStringLiteral("caster example.com")),
        casterJson(QStringLiteral("Host with a line break"), QStringLiteral("caster\\r\\nEvil: header")),
        casterJson(QStringLiteral("Long host"), QString(NTRIPSettings::MaxSavedCasterTextLength + 1, QLatin1Char('h'))),
        casterJson(QStringLiteral("Port zero"), host, QStringLiteral("0")),
        casterJson(QStringLiteral("Port too high"), host, QStringLiteral("65536")),
        casterJson(QStringLiteral("Port with a fraction"), host, QStringLiteral("2101.5")),
        casterJson(QStringLiteral("Port as text"), host, QStringLiteral("\"2101\"")),
        casterJson(QStringLiteral("Flag as text"), host, QStringLiteral("2101"), QStringLiteral(",\"useTls\":\"yes\"")),
        casterJson(QStringLiteral("Pin as a number"), host, QStringLiteral("2101"),
                   QStringLiteral(",\"pinnedCertificate\":5")),
        casterJson(north, QStringLiteral("other.example.com")),
        casterJson(south),
    };
    QTest::newRow("invalid-and-repeated") << QStringLiteral("[%1]").arg(entries.join(QLatin1Char(',')))
                                          << QStringList{north, south} << QStringLiteral("Dropped 15 of 17 ");

    QStringList many;
    QStringList manyEntries;
    for (int i = 0; i < NTRIPSettings::MaxSavedCasters + 5; ++i) {
        many.append(QStringLiteral("Caster %1").arg(i));
        manyEntries.append(casterJson(many.last()));
    }
    QTest::newRow("too-many") << QStringLiteral("[%1]").arg(manyEntries.join(QLatin1Char(',')))
                              << many.first(NTRIPSettings::MaxSavedCasters)
                              << QStringLiteral("Dropped 5 of %1 ").arg(many.size());
}

void NTRIPSettingsTest::_savedCastersLoad()
{
    QFETCH(QString, saved);
    QFETCH(QStringList, names);
    QFETCH(QString, warning);
    NTRIPSettings settings;
    QVERIFY(settings.savedCasters().isEmpty());
    // The constructor loads the saved value, which a SettingsFact ignores under unit tests, as a change of it does.
    if (!warning.isEmpty()) {
        expectLogMessage("GPS.Settings.NTRIPSettings", QtWarningMsg, QRegularExpression(warning));
    }
    settings.ntripSavedCasters()->setRawValue(saved);
    if (!warning.isEmpty()) {
        verifyExpectedLogMessage();
    }
    QCOMPARE(casterNames(settings), names);
    if (names.isEmpty()) {
        return;
    }
    // An entry without the other keys is a caster without a mountpoint, a login or TLS.
    const QString host = QStringLiteral("caster.example.com");
    const bool whole = saved.contains(QStringLiteral("\"password\""));
    QCOMPARE(settings.savedCasters().first(), whole
                                                  ? savedCaster(names.first(), host, 2101, QStringLiteral("MOUNT"),
                                                                QStringLiteral("user"), QStringLiteral("secret"), true)
                                                  : savedCaster(names.first(), host, 2101));
}

void NTRIPSettingsTest::_savedCastersSaveAndRemove()
{
    NTRIPSettings settings;
    QSignalSpy changed(&settings, &NTRIPSettings::savedCastersChanged);
    NTRIPSavedCaster south = savedCaster(QStringLiteral(" South network "), QStringLiteral("192.0.2.7"), 80);
    QVERIFY(settings.saveCaster(kNorth));
    QVERIFY(settings.saveCaster(south));
    south.name = QStringLiteral("South network");
    QCOMPARE(settings.savedCasters(), (QList<NTRIPSavedCaster>{kNorth, south}));
    QCOMPARE(changed.size(), 2);

    // The casters are one stored value, which a later start loads unchanged.
    NTRIPSettings restarted;
    restarted.ntripSavedCasters()->setRawValue(settings.ntripSavedCasters()->rawValue());
    QCOMPARE(restarted.savedCasters(), settings.savedCasters());

    // A saved name keeps its place and takes the new caster.
    NTRIPSavedCaster moved = kNorth;
    moved.mountpoint = QStringLiteral("NORTH2");
    QVERIFY(settings.saveCaster(moved));
    QCOMPARE(settings.savedCasters(), (QList<NTRIPSavedCaster>{moved, south}));

    const auto invalid = [](auto change) {
        NTRIPSavedCaster caster = kNorth;
        change(caster);
        return caster;
    };
    const QString tooLong(NTRIPSettings::MaxSavedCasterTextLength + 1, u'x');
    for (const NTRIPSavedCaster& caster :
         {invalid([](NTRIPSavedCaster& caster) { caster.name = QStringLiteral("  "); }),
          invalid([](NTRIPSavedCaster& caster) {
              caster.name = QString(NTRIPSettings::MaxSavedCasterNameLength + 1, u'n');
          }),
          invalid([](NTRIPSavedCaster& caster) { caster.host.clear(); }),
          invalid([](NTRIPSavedCaster& caster) { caster.host = QStringLiteral("north example.com"); }),
          invalid([](NTRIPSavedCaster& caster) { caster.port = 0; }),
          invalid([](NTRIPSavedCaster& caster) { caster.port = 65536; }),
          invalid([&tooLong](NTRIPSavedCaster& caster) { caster.username = tooLong; }),
          invalid([&tooLong](NTRIPSavedCaster& caster) { caster.password = tooLong; }),
          invalid([&tooLong](NTRIPSavedCaster& caster) { caster.mountpoint = tooLong; })}) {
        QVERIFY(!settings.saveCaster(caster));
    }
    QCOMPARE(settings.savedCasters(), (QList<NTRIPSavedCaster>{moved, south}));

    // A full list refuses another name, and still replaces a saved one.
    for (qsizetype i = settings.savedCasters().size(); i < NTRIPSettings::MaxSavedCasters; ++i) {
        NTRIPSavedCaster caster = kNorth;
        caster.name = QStringLiteral("Caster %1").arg(i);
        QVERIFY(settings.saveCaster(caster));
    }
    NTRIPSavedCaster other = kNorth;
    other.name = QStringLiteral("One too many");
    QVERIFY(!settings.saveCaster(other));
    QVERIFY(settings.saveCaster(kNorth));
    QCOMPARE(settings.savedCasters().size(), NTRIPSettings::MaxSavedCasters);
    QCOMPARE(settings.savedCasters().first(), kNorth);

    changed.clear();
    QVERIFY(settings.removeSavedCaster(kNorth.name));
    QVERIFY(!settings.removeSavedCaster(kNorth.name));
    QCOMPARE(changed.size(), 1);
    QCOMPARE(settings.savedCasters().first(), south);
    QVERIFY(settings.saveCaster(other));
}

void NTRIPSettingsTest::_savedCasterBecomesConnection()
{
    NTRIPSettings settings;
    enterCaster(settings, kNorth);
    QVERIFY(settings.saveConnectionAsCaster(kNorth.name));
    QCOMPARE(settings.savedCasters(), QList<NTRIPSavedCaster>{kNorth});
    // Nothing to save without a host.
    settings.ntripServerHostAddress()->setRawValue(QString());
    QVERIFY(!settings.saveConnectionAsCaster(QStringLiteral("No caster")));

    // The connection settings stay the caster that is connected to; a saved one only replaces it when used.
    const NTRIPSavedCaster south =
        savedCaster(QStringLiteral("South network"), QStringLiteral("192.0.2.7"), 80, QStringLiteral("SOUTH"));
    enterCaster(settings, south);
    QVERIFY(!settings.useSavedCaster(QStringLiteral("Unknown")));
    QCOMPARE(enteredCaster(settings, south.name), south);

    // While the connection is on, the page locks the settings a saved caster would write.
    settings.ntripServerConnectEnabled()->setRawValue(true);
    QVERIFY(!settings.useSavedCaster(kNorth.name));
    QCOMPARE(enteredCaster(settings, south.name), south);

    settings.ntripServerConnectEnabled()->setRawValue(false);
    QVERIFY(settings.useSavedCaster(kNorth.name));
    QCOMPARE(enteredCaster(settings, kNorth.name), kNorth);
    QVERIFY(!settings.ntripServerConnectEnabled()->rawValue().toBool());
}

void NTRIPSettingsTest::_savedCasterKeepsItsOwnCertificate_data()
{
    // The pin trusted when the caster was saved, the pin setting when the caster is used, and the pin setting after.
    QTest::addColumn<bool>("allowSelfSigned");
    QTest::addColumn<QString>("savedPin");
    QTest::addColumn<QString>("trustedPin");
    QTest::addColumn<QString>("pin");
    const QString saved = pinFor(kNorth.host, kNorth.port);
    const QString later = pinFor(kNorth.host, kNorth.port, u'b');
    const QString other = pinFor(QStringLiteral("other.example.com"), kNorth.port, u'c');
    // The same host on another port is another caster.
    const QString otherPort = pinFor(kNorth.host, 2102, u'd');
    QTest::newRow("its-own-pin-returns") << true << saved << other << saved;
    QTest::newRow("another-casters-pin-does-not-stay") << true << QString() << other << QString();
    QTest::newRow("another-ports-pin-does-not-stay") << true << QString() << otherPort << QString();
    QTest::newRow("a-pin-trusted-since-is-kept") << true << saved << later << later;
    QTest::newRow("a-pin-trusted-after-saving-is-kept") << true << QString() << later << later;
    QTest::newRow("none-without-self-signed-certificates") << false << saved << later << QString();
}

void NTRIPSettingsTest::_savedCasterKeepsItsOwnCertificate()
{
    QFETCH(bool, allowSelfSigned);
    QFETCH(QString, savedPin);
    QFETCH(QString, trustedPin);
    QFETCH(QString, pin);
    NTRIPSettings settings;
    NTRIPSavedCaster north = kNorth;
    north.allowSelfSignedCerts = allowSelfSigned;
    // Saving takes the trusted certificate with the caster only when it is this caster's.
    north.pinnedCertificate = savedPin.isEmpty() ? pinFor(QStringLiteral("other.example.com"), 2101) : savedPin;
    enterCaster(settings, north);
    QVERIFY(settings.saveConnectionAsCaster(north.name));
    QCOMPARE(settings.savedCasters().first().pinnedCertificate, allowSelfSigned ? savedPin : QString());

    // Another caster was entered since, or this one's certificate was trusted anew.
    settings.ntripServerHostAddress()->setRawValue(QStringLiteral("other.example.com"));
    settings.ntripPinnedCertificate()->setRawValue(trustedPin);
    QVERIFY(settings.useSavedCaster(north.name));
    QCOMPARE(settings.ntripPinnedCertificate()->rawValue().toString(), pin);
    QCOMPARE(settings.ntripServerHostAddress()->rawValue().toString(), north.host);
}

void NTRIPSettingsTest::_pinNamesAnIPv6CasterInBrackets_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    // The caster a stored pin names, and how the settings hold that pin.
    QTest::addColumn<QString>("stored");
    QTest::addColumn<QString>("held");
    // The pin is the one of the caster at host and port.
    QTest::addColumn<bool>("own");
    const QString name = QStringLiteral("caster.example.com");
    const QString literal = QStringLiteral("fe80::1");
    QTest::newRow("name") << name << 2101 << QStringLiteral("caster.example.com:2101")
                          << QStringLiteral("caster.example.com:2101") << true;
    QTest::newRow("ipv6") << literal << 2101 << QStringLiteral("[fe80::1]:2101") << QStringLiteral("[fe80::1]:2101")
                          << true;
    // A literal was stored without brackets once. Its port follows its last colon, so it names one caster only.
    QTest::newRow("ipv6-stored-without-brackets")
        << literal << 2101 << QStringLiteral("fe80::1:2101") << QStringLiteral("[fe80::1]:2101") << true;
    QTest::newRow("zone-stored-without-brackets")
        << QStringLiteral("fe80::1%2") << 2101 << QStringLiteral("fe80::1%2:2101") << QStringLiteral("[fe80::1%2]:2101")
        << true;
    // The pin of fe80::1:2101 at port 443 is not the one of fe80::1, at either port.
    QTest::newRow("another-host-that-starts-the-same")
        << literal << 2101 << QStringLiteral("fe80::1:2101:443") << QStringLiteral("[fe80::1:2101]:443") << false;
    QTest::newRow("another-host-at-its-port")
        << literal << 443 << QStringLiteral("fe80::1:2101:443") << QStringLiteral("[fe80::1:2101]:443") << false;
}

void NTRIPSettingsTest::_pinNamesAnIPv6CasterInBrackets()
{
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, stored);
    QFETCH(QString, held);
    QFETCH(bool, own);
    const QString digest = QLatin1Char('|') + QString(64, u'a');
    NTRIPConnectionConfig connection;
    connection.host = host;
    connection.port = port;
    // The settings name a caster as its connection does.
    QCOMPARE(held == connection.endpoint(), own);

    NTRIPSettings settings;
    settings.ntripPinnedCertificate()->setRawValue(stored + digest);
    QCOMPARE(settings.ntripPinnedCertificate()->rawValue().toString(), held + digest);

    // A saved caster keeps the pin that names it, and no other.
    settings.ntripSavedCasters()->setRawValue(
        QStringLiteral("[%1]").arg(casterJson(kNorth.name, host, QString::number(port),
                                              QStringLiteral(",\"pinnedCertificate\":\"%1\"").arg(stored + digest))));
    QCOMPARE(settings.savedCasters().size(), 1);
    QCOMPARE(settings.savedCasters().first().pinnedCertificate, own ? held + digest : QString());
}

UT_REGISTER_TEST(NTRIPSettingsTest, TestLabel::Unit)
