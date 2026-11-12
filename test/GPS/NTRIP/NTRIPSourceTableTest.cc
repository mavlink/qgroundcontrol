#include "NTRIPSourceTableTest.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "NTRIPSourceTable.h"

Q_DECLARE_METATYPE(NTRIPMountpoint::Carrier)
Q_DECLARE_METATYPE(NTRIPMountpoint::Authentication)

namespace {
/// An STR row with these format, message, carrier, NMEA and authentication fields.
QString sourceTableRow(const QString& format, const QString& messages, const QString& carrier, const QString& nmea,
                       const QString& authentication)
{
    return QStringLiteral("STR;MP;Town;%1;%2;%3;GPS;NET;USA;40;-74;%4;0;gen;none;%5;N;4800;")
        .arg(format, messages, carrier, nmea, authentication);
}

QStringList mountpointNames(const QAbstractItemModel& rows)
{
    QStringList names;
    for (int row = 0; row < rows.rowCount(); ++row) {
        names.append(rows.index(row, 0).data(NTRIPSourceTableModel::MountpointRole).toString());
    }
    return names;
}
}  // namespace

void NTRIPSourceTableTest::_parseSTRLine()
{
    const QString line =
        "STR;MOUNT01;Mount Identifier;RTCM 3.2;"
        "1005(1),1074(1),1084(1),1094(1);2;GPS+GLO+GAL;NET01;USA;40.0000;-74.0000;0;1;QGC Test;none;B;N;4800;misc";
    const auto parsed = NTRIPMountpoint::fromSourceTableLine(line);
    QVERIFY(parsed);
    const NTRIPMountpoint& mp = *parsed;

    QCOMPARE(mp.mountpoint, QStringLiteral("MOUNT01"));
    QCOMPARE(mp.identifier, QStringLiteral("Mount Identifier"));
    QCOMPARE(mp.format, QStringLiteral("RTCM 3.2"));
    QCOMPARE(mp.messages, QStringLiteral("1005(1),1074(1),1084(1),1094(1)"));
    QCOMPARE(mp.carrier, NTRIPMountpoint::Carrier::L1L2);
    QCOMPARE(mp.navSystem, QStringLiteral("GPS+GLO+GAL"));
    QCOMPARE(mp.country, QStringLiteral("USA"));
    QVERIFY(qFuzzyCompare(mp.latitude, 40.0));
    QVERIFY(qFuzzyCompare(mp.longitude, -74.0));
    QVERIFY(!mp.needsPosition);
    QCOMPARE(mp.authentication, NTRIPMountpoint::Authentication::Basic);
    QCOMPARE(mp.bitrate, 4800);
}

void NTRIPSourceTableTest::_rowSaysWhatTheStreamNeeds_data()
{
    using Carrier = NTRIPMountpoint::Carrier;
    using Authentication = NTRIPMountpoint::Authentication;
    QTest::addColumn<QString>("messages");
    QTest::addColumn<QString>("carrierField");
    QTest::addColumn<QString>("nmeaField");
    QTest::addColumn<QString>("authenticationField");
    QTest::addColumn<Carrier>("carrier");
    QTest::addColumn<bool>("needsPosition");
    QTest::addColumn<Authentication>("authentication");
    // What the details line says of the carrier, and the whole line of notes.
    QTest::addColumn<QString>("carrierText");
    QTest::addColumn<QString>("notes");
    QTest::newRow("network-rtk") << QStringLiteral("1004(1),1005(5)") << QStringLiteral("2") << QStringLiteral("1")
                                 << QStringLiteral("B") << Carrier::L1L2 << true << Authentication::Basic
                                 << QStringLiteral("L1+L2 · ")
                                 << QStringLiteral("Needs your position · Needs a login · Messages 1004(1),1005(5)");
    QTest::newRow("open-single-base") << QString() << QStringLiteral("1") << QStringLiteral("0") << QStringLiteral("N")
                                      << Carrier::L1 << false << Authentication::None << QStringLiteral("L1 · ")
                                      << QStringLiteral("No login");
    QTest::newRow("code-only") << QStringLiteral("1(1),3(10)") << QStringLiteral("0") << QStringLiteral("0")
                               << QStringLiteral("B") << Carrier::None << false << Authentication::Basic
                               << QStringLiteral("no carrier phase · ")
                               << QStringLiteral("Needs a login · Messages 1(1),3(10)");
    // A field that is empty or holds something else says nothing, and nothing is shown for it.
    QTest::newRow("not-stated") << QString() << QString() << QString() << QString() << Carrier::Unknown << false
                                << Authentication::Unknown << QString() << QString();
    QTest::newRow("other-values") << QString() << QStringLiteral("3") << QStringLiteral("yes") << QStringLiteral("X")
                                  << Carrier::Unknown << false << Authentication::Unknown << QString() << QString();
    QTest::newRow("spaces-and-case") << QStringLiteral(" 1005(10) ") << QStringLiteral(" 2 ") << QStringLiteral(" 1 ")
                                     << QStringLiteral(" b ") << Carrier::L1L2 << true << Authentication::Basic
                                     << QStringLiteral("L1+L2 · ")
                                     << QStringLiteral("Needs your position · Needs a login · Messages 1005(10)");
}

void NTRIPSourceTableTest::_rowSaysWhatTheStreamNeeds()
{
    QFETCH(QString, messages);
    QFETCH(QString, carrierField);
    QFETCH(QString, nmeaField);
    QFETCH(QString, authenticationField);
    QFETCH(NTRIPMountpoint::Carrier, carrier);
    QFETCH(bool, needsPosition);
    QFETCH(NTRIPMountpoint::Authentication, authentication);
    QFETCH(QString, carrierText);
    QFETCH(QString, notes);
    NTRIPSourceTableModel model;
    model.parseSourceTable(
        sourceTableRow(QStringLiteral("RTCM 3.2"), messages, carrierField, nmeaField, authenticationField));
    QCOMPARE(model.count(), 1);
    const NTRIPMountpoint& mountpoint = model.mountpoints().constFirst();
    QCOMPARE(mountpoint.messages, messages.trimmed());
    QCOMPARE(mountpoint.carrier, carrier);
    QCOMPARE(mountpoint.needsPosition, needsPosition);
    QCOMPARE(mountpoint.authentication, authentication);
    const auto index = model.index(0, 0);
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(),
             QStringLiteral("Town · RTCM 3.2 · GPS · %1USA · 4800 bps").arg(carrierText));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::NotesRole).toString(), notes);
    QVERIFY(model.data(index, NTRIPSourceTableModel::ProblemRole).toString().isEmpty());
}

void NTRIPSourceTableTest::_unusableRowSaysWhy_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("authentication");
    QTest::addColumn<QString>("problem");
    const QString digest = QStringLiteral("Needs Digest authentication, which is not supported.");
    const auto otherFormat = [](const QString& format) {
        return QStringLiteral("Listed as %1: only RTCM 3 corrections can be used.").arg(format);
    };
    for (const char* rtcm3 : {"RTCM 3", "RTCM 3.2", "RTCM3.1", "rtcm3", "RTCM_3.3", "RTCM 3 MSM7"}) {
        QTest::addRow("%s", rtcm3) << QString::fromLatin1(rtcm3) << QStringLiteral("B") << QString();
    }
    // A row that does not name a format, or not its version, may well send RTCM 3.
    QTest::newRow("no-format") << QString() << QStringLiteral("B") << QString();
    QTest::newRow("rtcm-without-version") << QStringLiteral("RTCM") << QStringLiteral("N") << QString();
    // "RTCM2.3" is how the SAPOS caster lists its differential corrections.
    for (const char* other : {"RTCM2.3", "RTCM 2", "CMR+", "RAW", "SPARTN", "u-blox"}) {
        QTest::addRow("%s", other) << QString::fromLatin1(other) << QStringLiteral("B")
                                   << otherFormat(QString::fromLatin1(other));
    }
    QTest::newRow("digest") << QStringLiteral("RTCM 3.2") << QStringLiteral("D") << digest;
    QTest::newRow("digest-lower-case") << QStringLiteral("RTCM 3.2") << QStringLiteral("d") << digest;
    QTest::newRow("other-format-and-digest") << QStringLiteral("CMR") << QStringLiteral("D")
                                             << otherFormat(QStringLiteral("CMR")) + QLatin1Char(' ') + digest;
}

void NTRIPSourceTableTest::_unusableRowSaysWhy()
{
    QFETCH(QString, format);
    QFETCH(QString, authentication);
    QFETCH(QString, problem);
    NTRIPSourceTableModel model;
    const NTRIPSourceTableSortModel sorted(&model);
    model.parseSourceTable(
        sourceTableRow(format, QStringLiteral("1005(10)"), QStringLiteral("2"), QStringLiteral("0"), authentication));
    // The row is listed either way: the caster's table may be wrong, and the name can be typed as well.
    QCOMPARE(sorted.count(), 1);
    QCOMPARE(sorted.index(0, 0).data(NTRIPSourceTableModel::ProblemRole).toString(), problem);
}

void NTRIPSourceTableTest::_nameFilterNarrowsSortedRows()
{
    const QString table = QStringLiteral(
        "STR;FAR_MSM7;Berlin;RTCM 3.2;;2;GPS;NET;DEU;52.5;13.4\r\n"
        "STR;NEAR_MSM4;New York;RTCM 3.2;;2;GPS;NET;USA;40.7;-74.0\r\n"
        "STR;NOWHERE\r\n"
        "STR;MID_msm7;Newark;RTCM 3.2;;2;GPS;NET;USA;40.7;-74.2\r\n"
        "ENDSOURCETABLE\r\n");
    NTRIPSourceTableModel model;
    NTRIPSourceTableSortModel sorted(&model);
    QAbstractItemModelTester tester(&sorted, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy counts(&sorted, &NTRIPSourceTableSortModel::countChanged);
    model.parseSourceTable(table, QGeoCoordinate(40.7, -74.0));
    const QStringList all{QStringLiteral("NEAR_MSM4"), QStringLiteral("MID_msm7"), QStringLiteral("FAR_MSM7"),
                          QStringLiteral("NOWHERE")};
    QCOMPARE(mountpointNames(sorted), all);
    QCOMPARE(counts.size(), 1);

    // Whatever the case of the name or of the filter, and in distance order as before.
    sorted.setNameFilter(QStringLiteral("Msm7"));
    QCOMPARE(sorted.nameFilter(), QStringLiteral("Msm7"));
    QCOMPARE(mountpointNames(sorted), (QStringList{QStringLiteral("MID_msm7"), QStringLiteral("FAR_MSM7")}));
    QCOMPARE(sorted.count(), 2);
    QCOMPARE(counts.size(), 2);
    // The caster's description of a mountpoint, usually its town, counts as its name.
    sorted.setNameFilter(QStringLiteral(" new "));
    QCOMPARE(mountpointNames(sorted), (QStringList{QStringLiteral("NEAR_MSM4"), QStringLiteral("MID_msm7")}));
    QCOMPARE(counts.size(), 2);
    sorted.setNameFilter(QStringLiteral("no such name"));
    QCOMPARE(sorted.count(), 0);
    QCOMPARE(counts.size(), 3);

    // The filter stays for the distances of another position and for the next table.
    sorted.setNameFilter(QStringLiteral("msm"));
    model.updateDistances(QGeoCoordinate(52.5, 13.4));
    QCOMPARE(mountpointNames(sorted),
             (QStringList{QStringLiteral("FAR_MSM7"), QStringLiteral("NEAR_MSM4"), QStringLiteral("MID_msm7")}));
    model.parseSourceTable(QStringLiteral("STR;OTHER;Town\r\nSTR;NEXT_MSM5;Town\r\nENDSOURCETABLE\r\n"));
    QCOMPARE(mountpointNames(sorted), QStringList{QStringLiteral("NEXT_MSM5")});

    // A filter of spaces alone is none.
    sorted.setNameFilter(QStringLiteral("  "));
    QCOMPARE(mountpointNames(sorted), (QStringList{QStringLiteral("OTHER"), QStringLiteral("NEXT_MSM5")}));
}

void NTRIPSourceTableTest::_parseFullTable()
{
    const QString table =
        "SOURCETABLE 200 OK\r\n"
        "STR;MP1;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.0;-74.0;0;1;gen;none;B;N;4800;misc\r\n"
        "STR;MP2;Id2;RTCM 3.3;details;0;GLO;NET;DEU;52.0;13.0;1;0;gen;none;N;Y;9600;misc\r\n"
        "CAS;caster.example.com;2101;desc;op;0;USA;0;0;0;0;0;0;misc\r\n"
        "NET;NET;operator;B;N;https://example.com;https://example.com/streams;contact@example.com;none\r\n"
        "ENDSOURCETABLE\r\n";

    NTRIPSourceTableModel model;
    model.parseSourceTable(table);

    // Only STR records are mountpoints.
    QCOMPARE(model.count(), 2);
    QCOMPARE(model.rowCount(), 2);
}

void NTRIPSourceTableTest::_shortRow_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<bool>("listed");
    QTest::addColumn<QString>("details");
    // RTKLIB's caster sends "STR;<mountpoint>;<text>", where the text is whatever its operator gave it.
    QTest::newRow("rtklib-text") << QStringLiteral("STR;BASE;Rooftop antenna") << true
                                 << QStringLiteral("Rooftop antenna");
    QTest::newRow("rtklib-no-text") << QStringLiteral("STR;BASE;") << true << QString();
    QTest::newRow("name-only") << QStringLiteral("STR;BASE") << true << QString();
    QTest::newRow("through-format") << QStringLiteral("STR;BASE;Rooftop antenna;RTCM 3.2;1005(10),1077(1)") << true
                                    << QStringLiteral("Rooftop antenna · RTCM 3.2");
    QTest::newRow("through-country") << QStringLiteral("STR;BASE;;RTCM 3.2;;2;GPS+GLO;NET;DEU") << true
                                     << QStringLiteral("RTCM 3.2 · GPS+GLO · L1+L2 · DEU");
    // A description that only repeats the name adds nothing.
    QTest::newRow("named-twice") << QStringLiteral("STR;BASE;base;RTCM 3.2") << true << QStringLiteral("RTCM 3.2");
    // Without a name there is nothing to connect to.
    QTest::newRow("no-name") << QStringLiteral("STR;;Rooftop antenna;RTCM 3.2") << false << QString();
    QTest::newRow("blank-name") << QStringLiteral("STR; ;Rooftop antenna") << false << QString();
    QTest::newRow("type-only") << QStringLiteral("STR") << false << QString();
}

void NTRIPSourceTableTest::_shortRow()
{
    QFETCH(QString, line);
    QFETCH(bool, listed);
    QFETCH(QString, details);
    NTRIPSourceTableModel model;
    model.parseSourceTable(line + QStringLiteral("\r\nENDSOURCETABLE\r\n"), QGeoCoordinate(40, -74));
    QCOMPARE(model.count(), listed ? 1 : 0);
    if (listed) {
        const auto index = model.index(0, 0);
        QCOMPARE(model.data(index, NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("BASE"));
        QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(), details);
        // A row without coordinates has no distance.
        QCOMPARE(model.data(index, NTRIPSourceTableModel::DistanceKmRole).toDouble(), -1.0);
    }
}

void NTRIPSourceTableTest::_rowShowsOnlyWhatPrints_data()
{
    QTest::addColumn<QChar>("character");
    QTest::addColumn<bool>("shown");
    QTest::newRow("escape") << QChar(0x1b) << false;
    QTest::newRow("tab") << QChar(u'\t') << false;
    QTest::newRow("carriage-return") << QChar(u'\r') << false;
    QTest::newRow("delete") << QChar(0x7f) << false;
    QTest::newRow("next-line") << QChar(0x85) << false;
    QTest::newRow("line-separator") << QChar(0x2028) << false;
    QTest::newRow("paragraph-separator") << QChar(0x2029) << false;
    // What sets the direction of the text after it would reorder the rest of the row.
    QTest::newRow("left-to-right-embedding") << QChar(0x202a) << false;
    QTest::newRow("pop-directional-formatting") << QChar(0x202c) << false;
    QTest::newRow("right-to-left-override") << QChar(0x202e) << false;
    QTest::newRow("right-to-left-isolate") << QChar(0x2067) << false;
    QTest::newRow("pop-directional-isolate") << QChar(0x2069) << false;
    // Text in a script that is written from right to left stays, with the marks that belong to it.
    QTest::newRow("hebrew-letter") << QChar(0x05d0) << true;
    QTest::newRow("right-to-left-mark") << QChar(0x200f) << true;
    QTest::newRow("zero-width-non-joiner") << QChar(0x200c) << true;
}

void NTRIPSourceTableTest::_rowShowsOnlyWhatPrints()
{
    QFETCH(QChar, character);
    QFETCH(bool, shown);
    // The character inside each field that is shown as text, and before and after two of them.
    const auto with = [character](const QString& text) { return text.first(2) + character + text.sliced(2); };
    const auto shows = [character, shown](const QString& text) {
        return shown ? text.first(2) + character + text.sliced(2) : text;
    };
    const QString row = QStringLiteral("STR;%1;%2;%3;%4;2;%5;NET;%6;40;-74;0;0;gen;none;B;N;4800\r\n");
    const QString town = QStringLiteral("Town");
    const QString format = QStringLiteral("CMR+");
    const QString messages = QStringLiteral("1005(10)");
    const QString systems = QStringLiteral("GPS+GLO");
    const QString country = QStringLiteral("DEU");
    NTRIPSourceTableModel model;
    model.parseSourceTable(row.arg(QStringLiteral("NAME"), character + with(town) + character, with(format),
                                   with(messages), with(systems), character + with(country) + character));
    QCOMPARE(model.count(), 1);
    const NTRIPMountpoint& mountpoint = model.mountpoints().constFirst();
    const QString around = shown ? QString(character) : QString();
    QCOMPARE(mountpoint.identifier, around + shows(town) + around);
    QCOMPARE(mountpoint.format, shows(format));
    QCOMPARE(mountpoint.messages, shows(messages));
    QCOMPARE(mountpoint.navSystem, shows(systems));
    QCOMPARE(mountpoint.country, around + shows(country) + around);
    const auto index = model.index(0, 0);
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(),
             QStringLiteral("%1 · %2 · %3 · L1+L2 · %4 · 4800 bps")
                 .arg(mountpoint.identifier, shows(format), shows(systems), mountpoint.country));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::NotesRole).toString(),
             QStringLiteral("Needs a login · Messages %1").arg(shows(messages)));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::ProblemRole).toString(),
             QStringLiteral("Listed as %1: only RTCM 3 corrections can be used.").arg(shows(format)));

    // A name is requested as it is listed, so one that would not show as it is, is not listed.
    model.parseSourceTable(row.arg(with(QStringLiteral("NAME")), town, format, messages, systems, country));
    QCOMPARE(model.count(), shown ? 1 : 0);
}

void NTRIPSourceTableTest::_updateDistancesAll()
{
    const QString table =
        "STR;NYC;Id1;RTCM 3.2;details;2;GPS;NET;USA;40.7128;-74.0060;0;1;gen;none;B;N;4800;misc\r\n"
        "STR;LON;Id2;RTCM 3.3;details;0;GPS;NET;GBR;51.5074;-0.1278;1;0;gen;none;N;Y;9600;misc\r\n"
        "ENDSOURCETABLE\r\n";

    NTRIPSourceTableModel model;
    NTRIPSourceTableSortModel sorted(&model);
    QAbstractItemModelTester tester(&sorted, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.parseSourceTable(table);
    QCOMPARE(sorted.count(), 2);
    QSignalSpy changes(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy resets(&sorted, &QAbstractItemModel::modelReset);

    model.updateDistances(QGeoCoordinate(51.5074, -0.1278));
    QCOMPARE(changes.size(), 1);
    const auto change = changes.takeFirst();
    QCOMPARE(qvariant_cast<QModelIndex>(change[0]), model.index(0, 0));
    QCOMPARE(qvariant_cast<QModelIndex>(change[1]), model.index(1, 0));
    QCOMPARE(qvariant_cast<QList<int>>(change[2]), QList<int>{NTRIPSourceTableModel::DistanceKmRole});
    // The source keeps caster order; the sorted view puts the nearest mountpoint first.
    QCOMPARE(mountpointNames(model), (QStringList{QStringLiteral("NYC"), QStringLiteral("LON")}));
    QCOMPARE(mountpointNames(sorted), (QStringList{QStringLiteral("LON"), QStringLiteral("NYC")}));
    QVERIFY(sorted.index(0, 0).data(NTRIPSourceTableModel::DistanceKmRole).toDouble() < 1.0);
    QVERIFY(sorted.index(1, 0).data(NTRIPSourceTableModel::DistanceKmRole).toDouble() > 5000.0);

    model.updateDistances(QGeoCoordinate(40.7128, -74.0060));
    QCOMPARE(mountpointNames(sorted), (QStringList{QStringLiteral("NYC"), QStringLiteral("LON")}));
    model.updateDistances(QGeoCoordinate(40.7128, -74.0060));
    QCOMPARE(changes.size(), 1);
    QVERIFY(resets.isEmpty());
}

void NTRIPSourceTableTest::_singleMountpointDistanceNotification()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable(QStringLiteral("STR;MP1;Id;RTCM 3.2;details;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800\n"));
    QSignalSpy changes(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
    for (const auto& coordinate : {QGeoCoordinate(40, -74), QGeoCoordinate(52, 13), QGeoCoordinate()}) {
        model.updateDistances(coordinate);
        QCOMPARE(changes.size(), 1);
        const auto change = changes.takeFirst();
        QCOMPARE(qvariant_cast<QModelIndex>(change[0]), model.index(0, 0));
        QCOMPARE(qvariant_cast<QModelIndex>(change[1]), model.index(0, 0));
        QCOMPARE(qvariant_cast<QList<int>>(change[2]), QList<int>{NTRIPSourceTableModel::DistanceKmRole});
        const double distance = model.data(model.index(0, 0), NTRIPSourceTableModel::DistanceKmRole).toDouble();
        QCOMPARE(distance, coordinate.isValid() ? coordinate.distanceTo(QGeoCoordinate(40, -74)) / 1000 : -1.0);
    }
    QVERIFY(resets.isEmpty());
}

void NTRIPSourceTableTest::_emptyTable()
{
    NTRIPSourceTableModel model;
    model.parseSourceTable("");
    QCOMPARE(model.count(), 0);

    model.parseSourceTable("ENDSOURCETABLE\r\n");
    QCOMPARE(model.count(), 0);
}

void NTRIPSourceTableTest::_tableTerminator_data()
{
    QTest::addColumn<QByteArray>("body");
    // The bytes after which the table is complete while more of the body may follow; -1 when it never is.
    QTest::addColumn<int>("completeAfter");
    // Whether the table is complete once the body has ended.
    QTest::addColumn<bool>("completeAtEnd");
    const QByteArray row = "STR;MP;Id;RTCM 3.2;;2;GPS;NET;USA;40;-74;0;1;gen;none;B;N;4800";
    const int rowSize = static_cast<int>(row.size());
    // The first byte of its line ending shows that the terminator line is whole.
    QTest::newRow("crlf") << row + "\r\nENDSOURCETABLE\r\n" << rowSize + 17 << true;
    QTest::newRow("lf") << row + "\nENDSOURCETABLE\n" << rowSize + 16 << true;
    QTest::newRow("empty-table") << QByteArray("ENDSOURCETABLE\r\n") << 15 << true;
    // Without a line ending only the end of the body shows it.
    QTest::newRow("no-final-newline") << row + "\r\nENDSOURCETABLE" << -1 << true;
    QTest::newRow("partial") << row + "\r\nENDSOURCE" << -1 << false;
    QTest::newRow("inside-row") << QByteArray("STR;ENDSOURCETABLE;Id\r\n") << -1 << false;
    QTest::newRow("prefix-of-line") << row + "\r\nENDSOURCETABLES\r\n" << -1 << false;
    const QByteArray longerLine = row + "\r\nENDSOURCETABLES\r\n" + row;
    QTest::newRow("terminator-after-prefix-of-line") << longerLine + "\r\nENDSOURCETABLE\r\n"
                                                     << static_cast<int>(longerLine.size()) + 17 << true;
}

void NTRIPSourceTableTest::_tableTerminator()
{
    QFETCH(QByteArray, body);
    QFETCH(int, completeAfter);
    QFETCH(bool, completeAtEnd);
    // However the body is split, a piece completes the table only once it holds the bytes that complete the whole.
    for (qsizetype split = 0; split <= body.size(); ++split) {
        const bool complete = ntripSourceTableComplete(body.first(split));
        QCOMPARE(complete, completeAfter >= 0 && split >= completeAfter);
        // After a check of an incomplete piece, checking only what the rest could complete gives the same answer.
        if (!complete) {
            QCOMPARE(ntripSourceTableComplete(body, split), completeAfter >= 0);
        }
    }
    QCOMPARE(ntripSourceTableComplete(body, 0, true), completeAtEnd);
}

void NTRIPSourceTableTest::_rolesAreReadOnlyProperties()
{
    NTRIPSourceTableModel model;
    const auto roles = model.roleNames();

    const struct
    {
        int role;
        const char* name;
    } expected[] = {
        {NTRIPSourceTableModel::MountpointRole, "mountpoint"}, {NTRIPSourceTableModel::FormatRole, "format"},
        {NTRIPSourceTableModel::NavSystemRole, "navSystem"},   {NTRIPSourceTableModel::CountryRole, "country"},
        {NTRIPSourceTableModel::BitrateRole, "bitrate"},       {NTRIPSourceTableModel::DistanceKmRole, "distanceKm"},
        {NTRIPSourceTableModel::DetailsRole, "details"},       {NTRIPSourceTableModel::NotesRole, "notes"},
        {NTRIPSourceTableModel::ProblemRole, "problem"},
    };

    for (const auto& [role, name] : expected) {
        QCOMPARE(roles.value(role), QByteArray(name));
    }
    model.parseSourceTable(QStringLiteral("STR;MP1;Id;RTCM 3.2;;2;GPS;Net;DEU;52.00;13.00;1;0;Gen;none;B;N;9600;"));
    QCOMPARE(model.rowCount(), 1);
    const auto index = model.index(0, 0);
    QCOMPARE(model.data(index, NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("MP1"));
    const QString details = QStringLiteral("Id · RTCM 3.2 · GPS · L1+L2 · DEU · 9600 bps");
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(), details);
    QCOMPARE(model.data(index, NTRIPSourceTableModel::NotesRole).toString(),
             QStringLiteral("Needs your position · Needs a login"));
    // The distance is a role of its own, which the list shows in the application's unit.
    model.updateDistances(QGeoCoordinate(52, 13));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DistanceKmRole).toDouble(), 0.0);
    QCOMPARE(model.data(index, NTRIPSourceTableModel::DetailsRole).toString(), details);
    QVERIFY(!(model.flags(index) & Qt::ItemIsEditable));
    QVERIFY(!model.setData(index, QStringLiteral("changed"), NTRIPSourceTableModel::MountpointRole));
    QCOMPARE(model.data(index, NTRIPSourceTableModel::MountpointRole).toString(), QStringLiteral("MP1"));
}

void NTRIPSourceTableTest::_coordinateValidity_data()
{
    QTest::addColumn<QString>("latitude");
    QTest::addColumn<QString>("longitude");
    QTest::addColumn<bool>("known");
    QTest::newRow("empty-latitude") << QString() << QStringLiteral("-74") << false;
    QTest::newRow("invalid-latitude") << QStringLiteral("bad") << QStringLiteral("-74") << false;
    QTest::newRow("out-of-range-latitude") << QStringLiteral("91") << QStringLiteral("-74") << false;
    QTest::newRow("nonfinite-longitude") << QStringLiteral("40") << QStringLiteral("nan") << false;
    QTest::newRow("out-of-range-longitude") << QStringLiteral("40") << QStringLiteral("181") << false;
    QTest::newRow("equator") << QStringLiteral("0") << QStringLiteral("-74") << true;
    QTest::newRow("prime-meridian") << QStringLiteral("40") << QStringLiteral("0") << true;
    QTest::newRow("unspecified-origin") << QStringLiteral("0") << QStringLiteral("0") << false;
}

void NTRIPSourceTableTest::_coordinateValidity()
{
    QFETCH(QString, latitude);
    QFETCH(QString, longitude);
    QFETCH(bool, known);
    auto mountpoint = NTRIPMountpoint::fromSourceTableLine(
        QStringLiteral("STR;TEST;Id;RTCM 3.2;;2;GPS;NET;USA;%1;%2;0;1;gen;none;B;N;4800").arg(latitude, longitude));
    QVERIFY(mountpoint);
    mountpoint->updateDistance(QGeoCoordinate(40, -74));
    QCOMPARE(mountpoint->distanceKm >= 0, known);
}

UT_REGISTER_TEST_LIGHTWEIGHT(NTRIPSourceTableTest, TestLabel::Unit)
