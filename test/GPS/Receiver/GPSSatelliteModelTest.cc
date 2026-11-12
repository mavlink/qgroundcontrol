#include "GPSSatelliteModelTest.h"

#include <vector>

#include <QtCore/QList>
#include <QtTest/QAbstractItemModelTester>
#include <QtTest/QSignalSpy>

#include "GPSReceiverReports.h"
#include "GPSSatelliteModel.h"

namespace {

using Satellites = std::vector<GPSSatellite>;
using Use = GPSSatellite::Use;

GPSSatellite satellite(GPSConstellation constellation, int id, int signal = 40, Use use = Use::Used)
{
    return {.constellation = constellation,
            .id = static_cast<uint16_t>(id),
            .azimuthDegrees = static_cast<uint16_t>(id * 10),
            .elevationDegrees = static_cast<int8_t>(id),
            .directionKnown = true,
            .signalDbHz = static_cast<uint8_t>(signal),
            .use = use};
}

GPSSatellite gps(int id, int signal = 40, Use use = Use::Used)
{
    return satellite(GPSConstellation::GPS, id, signal, use);
}

GPSSatellite galileo(int id, int signal = 40, Use use = Use::Used)
{
    return satellite(GPSConstellation::Galileo, id, signal, use);
}

/// Each row as "<constellation name> <number>", with a leading "|" where it starts its constellation.
QStringList rows(const GPSSatelliteModel& model)
{
    QStringList result;
    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row);
        result.append(
            QStringLiteral("%1%2 %3")
                .arg(index.data(GPSSatelliteModel::StartsConstellationRole).toBool() ? QStringLiteral("|") : QString(),
                     index.data(GPSSatelliteModel::ConstellationNameRole).toString())
                .arg(index.data(GPSSatelliteModel::SatelliteIdRole).toInt()));
    }
    return result;
}

}  // namespace

void GPSSatelliteModelTest::_rowsChangeInPlace()
{
    GPSSatelliteModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
    QSignalSpy counts(&model, &GPSSatelliteModel::countChanged);
    const auto changedRoles = [&changed](qsizetype emission) { return changed.at(emission).at(2).value<QList<int>>(); };
    const QList<int> valueRoles{GPSSatelliteModel::ElevationRole, GPSSatelliteModel::AzimuthRole,
                                GPSSatelliteModel::DirectionKnownRole, GPSSatelliteModel::SignalStrengthRole,
                                GPSSatelliteModel::UsedRole};

    // A row for each satellite, in the report's order; the first of each constellation starts it.
    Satellites sky{gps(2, 44), gps(5, 41, Use::Unused), galileo(4, 43, Use::Unknown)};
    sky[1].directionKnown = false;
    model.setSatellites(sky);
    QCOMPARE(rows(model), QStringList({"|GPS 2", "GPS 5", "|Galileo 4"}));
    QCOMPARE(model.count(), 3);
    QCOMPARE(counts.size(), 1);
    const QModelIndex first = model.index(0);
    QCOMPARE(first.data(GPSSatelliteModel::ConstellationRole).toInt(), int(GPSSatelliteModel::Constellation::GPS));
    QCOMPARE(first.data(GPSSatelliteModel::ElevationRole).toInt(), 2);
    QCOMPARE(first.data(GPSSatelliteModel::AzimuthRole).toInt(), 20);
    QCOMPARE(first.data(GPSSatelliteModel::DirectionKnownRole).toBool(), true);
    QCOMPARE(first.data(GPSSatelliteModel::SignalStrengthRole).toInt(), 44);
    QCOMPARE(first.data(GPSSatelliteModel::UsedRole).toBool(), true);
    QCOMPARE(model.index(1).data(GPSSatelliteModel::DirectionKnownRole).toBool(), false);
    // Neither a satellite the receiver does not use nor one it does not say that of is used.
    QCOMPARE(model.index(1).data(GPSSatelliteModel::UsedRole).toBool(), false);
    QCOMPARE(model.index(2).data(GPSSatelliteModel::UsedRole).toBool(), false);
    QCOMPARE(inserted.size(), 3);
    QVERIFY(changed.isEmpty());

    // The same satellites again notify nothing.
    model.setSatellites(sky);
    QCOMPARE(inserted.size(), 3);
    QVERIFY(changed.isEmpty() && removed.isEmpty());
    QCOMPARE(counts.size(), 1);

    // A new epoch changes the values of the rows that differ, and only those.
    sky[1] = gps(5, 38);
    model.setSatellites(sky);
    QCOMPARE(changed.size(), 1);
    QCOMPARE(changed.last().at(0).toModelIndex(), model.index(1));
    QCOMPARE(changed.last().at(1).toModelIndex(), model.index(1));
    QCOMPARE(changedRoles(0), valueRoles);
    QCOMPARE(model.index(1).data(GPSSatelliteModel::SignalStrengthRole).toInt(), 38);
    QCOMPARE(model.index(1).data(GPSSatelliteModel::UsedRole).toBool(), true);

    // A satellite that sets removes its row, and the next one of its constellation starts it.
    sky.erase(sky.begin());
    model.setSatellites(sky);
    QCOMPARE(rows(model), QStringList({"|GPS 5", "|Galileo 4"}));
    QCOMPARE(removed.size(), 1);
    QCOMPARE(removed.last().at(1).toInt(), 0);
    QCOMPARE(changed.size(), 2);
    QCOMPARE(changed.last().at(0).toModelIndex(), model.index(0));
    QCOMPARE(changedRoles(1), QList<int>{GPSSatelliteModel::StartsConstellationRole});
    QCOMPARE(counts.size(), 2);

    // Satellites that rise insert their rows where the report has them; one that rises before the first of its
    // constellation takes the start from it.
    sky = {gps(3), gps(5, 38), gps(9), galileo(4, 43, Use::Unknown), galileo(11)};
    model.setSatellites(sky);
    QCOMPARE(rows(model), QStringList({"|GPS 3", "GPS 5", "GPS 9", "|Galileo 4", "Galileo 11"}));
    QCOMPARE(inserted.size(), 6);
    QCOMPARE(inserted.at(3).at(1).toInt(), 0);
    QCOMPARE(inserted.at(4).at(1).toInt(), 2);
    QCOMPARE(inserted.at(5).at(1).toInt(), 4);
    QCOMPARE(changed.size(), 3);
    QCOMPARE(changed.last().at(0).toModelIndex(), model.index(1));
    QCOMPARE(changedRoles(2), QList<int>{GPSSatelliteModel::StartsConstellationRole});
    QCOMPARE(removed.size(), 1);
    QCOMPARE(counts.size(), 3);

    // A receiver that stops reporting satellites empties the model.
    model.setSatellites({});
    QCOMPARE(model.count(), 0);
    QCOMPARE(removed.size(), 6);
    QCOMPARE(counts.size(), 4);
    // A view keeps its delegates: the rows are never reset.
    QVERIFY(resets.isEmpty());
}

void GPSSatelliteModelTest::_reportReplacesRows_data()
{
    QTest::addColumn<Satellites>("before");
    QTest::addColumn<Satellites>("after");
    QTest::newRow("other-constellation") << Satellites{gps(1), gps(2)} << Satellites{galileo(1), galileo(2)};
    QTest::newRow("interleaved") << Satellites{gps(1), gps(3), galileo(2), galileo(4)}
                                 << Satellites{gps(2), gps(3), gps(4), galileo(1), galileo(4)};
    // The model does not depend on the order it expects of a report.
    QTest::newRow("unordered") << Satellites{galileo(4), gps(3), gps(1)} << Satellites{gps(3), galileo(4), gps(1)};
    QTest::newRow("repeated") << Satellites{gps(1)} << Satellites{gps(1), gps(1, 20)};
}

void GPSSatelliteModelTest::_reportReplacesRows()
{
    QFETCH(Satellites, before);
    QFETCH(Satellites, after);
    GPSSatelliteModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    model.setSatellites(before);
    model.setSatellites(after);
    QCOMPARE(model.count(), int(after.size()));
    for (int row = 0; row < model.count(); ++row) {
        const QModelIndex index = model.index(row);
        const GPSSatellite& expected = after[static_cast<size_t>(row)];
        QCOMPARE(index.data(GPSSatelliteModel::ConstellationRole).toInt(), int(expected.constellation));
        QCOMPARE(index.data(GPSSatelliteModel::SatelliteIdRole).toInt(), int(expected.id));
        QCOMPARE(index.data(GPSSatelliteModel::SignalStrengthRole).toInt(), int(expected.signalDbHz));
        QCOMPARE(index.data(GPSSatelliteModel::StartsConstellationRole).toBool(),
                 row == 0 || after[static_cast<size_t>(row) - 1].constellation != expected.constellation);
    }
}

UT_REGISTER_TEST(GPSSatelliteModelTest, TestLabel::Unit)
