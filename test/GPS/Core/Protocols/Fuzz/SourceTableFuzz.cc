#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArrayView>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

#include "NTRIPSourceTable.h"

// The fuzzer of the NTRIP source table, which a caster sends: the input is the body of its reply. The body arrives in
// pieces, and after each piece the check for the table's last line has to agree with the definition of that line. The
// whole body then becomes the mountpoint list, whose rows and distance order are checked while the reference position
// appears, moves and goes away. The seeds (../corpus/source-table) are a caster's table, RTKLIB's short rows, rows
// with odd and missing fields, lines that only resemble the last line, and rows at equal and unknown distances.

namespace {

/// Above half the circumference of the sphere QGeoCoordinate measures distances on.
constexpr double SOURCE_TABLE_MAX_DISTANCE_KM = 20100;

void requireTable(bool condition)
{
    if (!condition) {
        std::abort();
    }
}

/// The definition of a complete table: a line of @a body is ENDSOURCETABLE, with or without a carriage return after it.
bool hasEndLine(QByteArrayView body)
{
    static constexpr QByteArrayView END_LINE("ENDSOURCETABLE");
    for (qsizetype start = 0; start <= body.size();) {
        qsizetype end = body.indexOf('\n', start);
        if (end < 0) {
            end = body.size();
        }
        const QByteArrayView line = body.sliced(start, end - start);
        if (line.startsWith(END_LINE) && (line.size() == END_LINE.size() || line.at(END_LINE.size()) == '\r')) {
            return true;
        }
        start = end + 1;
    }
    return false;
}

/// Every row names a mountpoint and has a distance only from a known reference position, and the sorted list holds
/// each row once: known distances ascending, unknown ones last and the caster's order among equal ones.
void checkList(const NTRIPSourceTableModel& model, const NTRIPSourceTableSortModel& sorted, bool referenceKnown)
{
    requireTable(model.count() == model.rowCount() && sorted.count() == model.count());
    std::vector<bool> listed(static_cast<size_t>(model.count()));
    int previousRow = -1;
    double previousKm = 0;
    for (int row = 0; row < sorted.count(); ++row) {
        const QModelIndex index = sorted.index(row, 0);
        const int sourceRow = sorted.mapToSource(index).row();
        requireTable(sourceRow >= 0 && sourceRow < model.count() && !listed[static_cast<size_t>(sourceRow)]);
        listed[static_cast<size_t>(sourceRow)] = true;
        for (int role = NTRIPSourceTableModel::MountpointRole; role <= NTRIPSourceTableModel::DetailsRole; ++role) {
            requireTable(index.data(role).isValid());
        }
        const QString mountpoint = index.data(NTRIPSourceTableModel::MountpointRole).toString();
        requireTable(!mountpoint.isEmpty() && !mountpoint.contains(u';') && !mountpoint.contains(u'\n'));
        const double km = index.data(NTRIPSourceTableModel::DistanceKmRole).toDouble();
        const bool known = km >= 0;
        requireTable(known ? referenceKnown && km <= SOURCE_TABLE_MAX_DISTANCE_KM : km == -1.0);
        if (row > 0) {
            const bool previousKnown = previousKm >= 0;
            requireTable(previousKnown || !known);
            requireTable(!known || previousKm <= km);
            if (known == previousKnown && (!known || km == previousKm)) {
                requireTable(previousRow < sourceRow);
            }
        }
        previousRow = sourceRow;
        previousKm = km;
    }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    const QByteArrayView body(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));

    // The pieces are up to 16 bytes long, as the first byte has it, and longer ones for a long body.
    const qsizetype stride = body.isEmpty() ? 1 : std::max<qsizetype>(1 + data[0] % 16, body.size() / 128);
    for (qsizetype checked = 0, received = 0; received < body.size(); checked = received) {
        received = std::min(received + stride, body.size());
        const QByteArrayView arrived = body.first(received);
        const bool complete = ntripSourceTableComplete(arrived, checked);
        requireTable(complete == hasEndLine(arrived));
        if (complete) {
            break;
        }
    }
    requireTable(ntripSourceTableComplete(body) == hasEndLine(body));

    const QString table = QString::fromUtf8(body);
    const QGeoCoordinate reference(47.4, 8.5);
    NTRIPSourceTableModel model;
    const NTRIPSourceTableSortModel sorted(&model);
    model.parseSourceTable(table);
    const int rows = model.count();
    requireTable(rows <= table.count(u'\n') + 1);
    checkList(model, sorted, false);
    for (const QGeoCoordinate& moved : {reference, QGeoCoordinate(-33.9, 151.2), QGeoCoordinate()}) {
        model.updateDistances(moved);
        checkList(model, sorted, moved.isValid());
    }
    model.parseSourceTable(table, reference);
    requireTable(model.count() == rows);
    checkList(model, sorted, true);
    model.clear();
    requireTable(model.count() == 0 && sorted.count() == 0);
    return 0;
}
