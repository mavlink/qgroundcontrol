#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArrayView>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtPositioning/QGeoCoordinate>

#include "NTRIPSourceTable.h"

// The fuzzer of the NTRIP source table, which a caster sends: the input is the body of its reply. The body arrives in
// pieces, and after each piece the check for the table's last line has to agree with the definition of that line. The
// whole body then becomes the mountpoint list. Its rows are checked against the fields of the table's rows, its text
// for characters that do not show as themselves, and its distance order while the reference position appears, moves
// and goes away and while a name filter narrows it. The seeds (../corpus/source-table) are a caster's table, RTKLIB's
// short rows, rows with odd and missing fields, rows of every format, carrier and authentication, rows with control
// and direction characters in their fields, lines that only resemble the last line, and rows at equal and unknown
// distances.

namespace {

/// Above half the circumference of the sphere QGeoCoordinate measures distances on.
constexpr double SOURCE_TABLE_MAX_DISTANCE_KM = 20100;

void requireTable(bool condition)
{
    if (!condition) {
        std::abort();
    }
}

/// The definition of a complete table: a line of @a body is ENDSOURCETABLE and is whole. The carriage return or line
/// feed after the word shows that it is, and so does the end of a body that has @a ended.
bool hasEndLine(QByteArrayView body, bool ended)
{
    static constexpr QByteArrayView END_LINE("ENDSOURCETABLE");
    for (qsizetype start = 0; start <= body.size();) {
        const qsizetype lineFeed = body.indexOf('\n', start);
        const qsizetype end = lineFeed < 0 ? body.size() : lineFeed;
        const QByteArrayView line = body.sliced(start, end - start);
        if (line.startsWith(END_LINE)) {
            const bool whole =
                line.size() == END_LINE.size() ? lineFeed >= 0 || ended : line.at(END_LINE.size()) == '\r';
            if (whole) {
                return true;
            }
        }
        start = end + 1;
    }
    return false;
}

// The rules below restate the table's own under other names: a unity build compiles NTRIPSourceTable.cc into this
// file's translation unit.

/// Whether the list shows @a ch as itself, by what Unicode says of it: not a control character, not a line or
/// paragraph separator, and not a character that sets the direction of the text after it.
bool showsAsItself(QChar ch)
{
    switch (ch.category()) {
        case QChar::Other_Control:
        case QChar::Separator_Line:
        case QChar::Separator_Paragraph:
            return false;
        default:
            break;
    }
    switch (ch.direction()) {
        case QChar::DirLRE:
        case QChar::DirLRO:
        case QChar::DirRLE:
        case QChar::DirRLO:
        case QChar::DirPDF:
        case QChar::DirLRI:
        case QChar::DirRLI:
        case QChar::DirFSI:
        case QChar::DirPDI:
            return false;
        default:
            return true;
    }
}

bool showsAsItself(const QString& text)
{
    return std::ranges::all_of(text, [](QChar ch) { return showsAsItself(ch); });
}

/// The field @a field of a row as the list shows its text: without the characters that do not show as themselves,
/// and then without the spaces around the rest.
QString shownField(const QString& field)
{
    QString text;
    for (const QChar ch : field) {
        if (showsAsItself(ch)) {
            text.append(ch);
        }
    }
    return text.trimmed();
}

/// The STR rows of @a table that name a mountpoint, each as its fields without the spaces around them. A row is a
/// line, its fields are what its semicolons separate, and the second one is the name, which is requested as it is
/// listed and so has to show as it is.
QList<QStringList> mountpointRows(const QString& table)
{
    QList<QStringList> rows;
    for (const QString& line : table.split(u'\n')) {
        QStringList fields = line.split(u';');
        for (QString& field : fields) {
            field = field.trimmed();
        }
        const QString name = fields.value(1);
        if (fields.constFirst().toUpper() == u"STR" && !name.isEmpty() && showsAsItself(name)) {
            rows.append(fields);
        }
    }
    return rows;
}

/// Whether the format field @a format names a format other than RTCM 3: its letters and digits do not start with
/// RTCM3, and are neither missing nor RTCM without a version.
bool listsOtherFormat(const QString& format)
{
    QString name;
    for (const QChar ch : format) {
        if (ch.isLetterOrNumber()) {
            name.append(ch.toUpper());
        }
    }
    if (!name.startsWith(u"RTCM")) {
        return !name.isEmpty();
    }
    return name.size() > 4 && name.at(4).isDigit() && name.at(4) != u'3';
}

/// The list has the rows of the table in its order, with the fields at their places in an STR row: the messages
/// fifth, the carrier sixth, the navigation systems seventh, the country ninth, the NMEA flag twelfth and the
/// authentication sixteenth, each unknown unless it is one of the format's values. The fields that are text are
/// what shows of them. A row has a problem exactly when it lists another format than RTCM 3 or Digest
/// authentication, and notes exactly when it asks for something or lists its messages.
void checkRows(const NTRIPSourceTableModel& model, const QList<QStringList>& rows)
{
    using Carrier = NTRIPMountpoint::Carrier;
    using Authentication = NTRIPMountpoint::Authentication;
    requireTable(model.mountpoints().size() == rows.size());
    for (qsizetype row = 0; row < rows.size(); ++row) {
        const NTRIPMountpoint& mountpoint = model.mountpoints().at(row);
        const QStringList& fields = rows.at(row);
        requireTable(mountpoint.mountpoint == fields.value(1) && mountpoint.identifier == shownField(fields.value(2)));
        requireTable(mountpoint.format == shownField(fields.value(3)));
        requireTable(mountpoint.messages == shownField(fields.value(4)));
        requireTable(mountpoint.navSystem == shownField(fields.value(6)));
        requireTable(mountpoint.country == shownField(fields.value(8)));
        const QString carrier = fields.value(5);
        requireTable((mountpoint.carrier == Carrier::None) == (carrier == u"0"));
        requireTable((mountpoint.carrier == Carrier::L1) == (carrier == u"1"));
        requireTable((mountpoint.carrier == Carrier::L1L2) == (carrier == u"2"));
        requireTable(mountpoint.needsPosition == (fields.value(11) == u"1"));
        const QString authentication = fields.value(15).toUpper();
        requireTable((mountpoint.authentication == Authentication::None) == (authentication == u"N"));
        requireTable((mountpoint.authentication == Authentication::Basic) == (authentication == u"B"));
        requireTable((mountpoint.authentication == Authentication::Digest) == (authentication == u"D"));

        const QModelIndex index = model.index(static_cast<int>(row), 0);
        const QString problem = index.data(NTRIPSourceTableModel::ProblemRole).toString();
        const bool otherFormat = listsOtherFormat(mountpoint.format);
        requireTable(problem.isEmpty() == (!otherFormat && authentication != u"D"));
        requireTable(!otherFormat || problem.contains(mountpoint.format));
        const QString notes = index.data(NTRIPSourceTableModel::NotesRole).toString();
        const bool asks = mountpoint.needsPosition || authentication == u"N" || authentication == u"B";
        requireTable(notes.isEmpty() == (!asks && mountpoint.messages.isEmpty()));
        requireTable(notes.contains(mountpoint.messages));
    }
}

/// The rows of the model that @a sorted lists, in its order.
QList<int> listedRows(const NTRIPSourceTableSortModel& sorted)
{
    QList<int> rows;
    for (int row = 0; row < sorted.count(); ++row) {
        rows.append(sorted.mapToSource(sorted.index(row, 0)).row());
    }
    return rows;
}

/// With the name filter @a filter the list holds the rows whose name or identifier contains it, whatever the case of
/// either, in the order they have without the filter, which it has again once the filter is gone.
void checkNameFilter(const NTRIPSourceTableModel& model, NTRIPSourceTableSortModel& sorted, const QString& filter)
{
    const QList<int> all = listedRows(sorted);
    const QString text = filter.trimmed();
    QList<int> matching;
    for (const int row : all) {
        const NTRIPMountpoint& mountpoint = model.mountpoints().at(row);
        if (mountpoint.mountpoint.contains(text, Qt::CaseInsensitive) ||
            mountpoint.identifier.contains(text, Qt::CaseInsensitive)) {
            matching.append(row);
        }
    }
    sorted.setNameFilter(filter);
    requireTable(sorted.nameFilter() == filter && listedRows(sorted) == matching);
    sorted.setNameFilter({});
    requireTable(listedRows(sorted) == all);
}

/// Every row names a mountpoint, shows no character that would not show as itself, and has a distance only from a
/// known reference position, and the sorted list holds each row once: known distances ascending, unknown ones last
/// and the caster's order among equal ones.
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
        for (int role = NTRIPSourceTableModel::MountpointRole; role <= NTRIPSourceTableModel::ProblemRole; ++role) {
            const QVariant value = index.data(role);
            requireTable(value.isValid());
            requireTable(value.typeId() != QMetaType::QString || showsAsItself(value.toString()));
        }
        const QString mountpoint = index.data(NTRIPSourceTableModel::MountpointRole).toString();
        requireTable(!mountpoint.isEmpty() && !mountpoint.contains(u';'));
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
        requireTable(complete == hasEndLine(arrived, false));
        if (complete) {
            break;
        }
    }
    requireTable(ntripSourceTableComplete(body, 0, true) == hasEndLine(body, true));

    const QString table = QString::fromUtf8(body);
    const QGeoCoordinate reference(47.4, 8.5);
    NTRIPSourceTableModel model;
    NTRIPSourceTableSortModel sorted(&model);
    model.parseSourceTable(table);
    const int rows = model.count();
    checkRows(model, mountpointRows(table));
    checkList(model, sorted, false);
    for (const QGeoCoordinate& moved : {reference, QGeoCoordinate(-33.9, 151.2), QGeoCoordinate()}) {
        model.updateDistances(moved);
        checkList(model, sorted, moved.isValid());
    }
    model.parseSourceTable(table, reference);
    requireTable(model.count() == rows);
    checkList(model, sorted, true);
    if (rows > 0) {
        // Filters that list some rows and not others where the table allows it: a piece of the first row's name in
        // the other case, a piece of the last row's identifier, and spaces, which are no filter.
        const NTRIPMountpoint& first = model.mountpoints().constFirst();
        const QString name =
            first.mountpoint.toLower() == first.mountpoint ? first.mountpoint.toUpper() : first.mountpoint.toLower();
        const QString& identifier = model.mountpoints().constLast().identifier;
        for (const QString& filter : {name.mid(data[0] % name.size(), 3), identifier.right(2), QStringLiteral("  ")}) {
            checkNameFilter(model, sorted, filter);
        }
    }
    model.clear();
    requireTable(model.count() == 0 && sorted.count() == 0);
    return 0;
}
