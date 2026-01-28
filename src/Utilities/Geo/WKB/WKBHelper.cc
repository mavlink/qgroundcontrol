#include "WKBHelper.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDataStream>
#include <QtCore/QBuffer>
#include <cmath>

QGC_LOGGING_CATEGORY(WKBHelperLog, "Utilities.Geo.WKBHelper")

namespace WKBHelper
{

namespace {

class WKBReader {
public:
    WKBReader(const QByteArray &data) : _data(data), _pos(0), _littleEndian(true) {}

    bool readByteOrder() {
        if (_pos >= _data.size()) return false;
        _littleEndian = (_data[_pos++] == wkbNDR);
        return true;
    }

    bool readUInt32(quint32 &value) {
        if (_pos + 4 > _data.size()) return false;
        if (_littleEndian) {
            value = static_cast<quint8>(_data[_pos]) |
                    (static_cast<quint8>(_data[_pos + 1]) << 8) |
                    (static_cast<quint8>(_data[_pos + 2]) << 16) |
                    (static_cast<quint8>(_data[_pos + 3]) << 24);
        } else {
            value = (static_cast<quint8>(_data[_pos]) << 24) |
                    (static_cast<quint8>(_data[_pos + 1]) << 16) |
                    (static_cast<quint8>(_data[_pos + 2]) << 8) |
                    static_cast<quint8>(_data[_pos + 3]);
        }
        _pos += 4;
        return true;
    }

    bool readDouble(double &value) {
        if (_pos + 8 > _data.size()) return false;
        union { quint64 i; double d; } u;
        if (_littleEndian) {
            u.i = static_cast<quint64>(static_cast<quint8>(_data[_pos])) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 1])) << 8) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 2])) << 16) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 3])) << 24) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 4])) << 32) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 5])) << 40) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 6])) << 48) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 7])) << 56);
        } else {
            u.i = (static_cast<quint64>(static_cast<quint8>(_data[_pos])) << 56) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 1])) << 48) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 2])) << 40) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 3])) << 32) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 4])) << 24) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 5])) << 16) |
                  (static_cast<quint64>(static_cast<quint8>(_data[_pos + 6])) << 8) |
                  static_cast<quint64>(static_cast<quint8>(_data[_pos + 7]));
        }
        _pos += 8;
        value = u.d;
        return true;
    }

    bool readCoordinate(QGeoCoordinate &coord, bool hasZ, bool hasM) {
        double x, y, z = std::nan(""), m;
        if (!readDouble(x) || !readDouble(y)) return false;
        if (hasZ && !readDouble(z)) return false;
        if (hasM && !readDouble(m)) return false;

        coord.setLongitude(x);
        coord.setLatitude(y);
        if (!std::isnan(z)) {
            coord.setAltitude(z);
        }
        return true;
    }

    bool readRing(QList<QGeoCoordinate> &coords, bool hasZ, bool hasM) {
        quint32 numPoints;
        if (!readUInt32(numPoints)) return false;

        coords.clear();
        coords.reserve(numPoints);

        for (quint32 i = 0; i < numPoints; i++) {
            QGeoCoordinate coord;
            if (!readCoordinate(coord, hasZ, hasM)) return false;
            coords.append(coord);
        }

        // Remove closing vertex if present
        if (coords.count() >= 2 && coords.first().distanceTo(coords.last()) < 1.0) {
            coords.removeLast();
        }

        return true;
    }

    int pos() const { return _pos; }
    void setPos(int pos) { _pos = pos; }
    int remaining() const { return _data.size() - _pos; }
    QByteArray remainingData() const { return _data.mid(_pos); }

private:
    const QByteArray &_data;
    int _pos;
    bool _littleEndian;
};

class WKBWriter {
public:
    WKBWriter() : _buffer(&_data), _stream(&_buffer) {
        _buffer.open(QIODevice::WriteOnly);
        _stream.setByteOrder(QDataStream::LittleEndian);
        _stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    }

    void writeByteOrder() {
        _stream << static_cast<quint8>(wkbNDR);
    }

    void writeUInt32(quint32 value) {
        _stream << value;
    }

    void writeDouble(double value) {
        _stream << value;
    }

    void writeCoordinate(const QGeoCoordinate &coord, bool includeZ) {
        writeDouble(coord.longitude());
        writeDouble(coord.latitude());
        if (includeZ) {
            double z = std::isnan(coord.altitude()) ? 0.0 : coord.altitude();
            writeDouble(z);
        }
    }

    void writeRing(const QList<QGeoCoordinate> &coords, bool includeZ, bool close) {
        int count = coords.count();
        if (close && coords.count() >= 2 && coords.first().distanceTo(coords.last()) >= 1.0) {
            count++;
        }
        writeUInt32(count);

        for (const QGeoCoordinate &coord : coords) {
            writeCoordinate(coord, includeZ);
        }

        if (close && count > coords.count()) {
            writeCoordinate(coords.first(), includeZ);
        }
    }

    QByteArray data() {
        _buffer.close();
        return _data;
    }

private:
    QByteArray _data;
    QBuffer _buffer;
    QDataStream _stream;
};

} // anonymous namespace

int geometryType(const QByteArray &wkb)
{
    if (wkb.size() < 5) return -1;

    WKBReader reader(wkb);
    if (!reader.readByteOrder()) return -1;

    quint32 type;
    if (!reader.readUInt32(type)) return -1;

    return static_cast<int>(type);
}

QString geometryTypeName(int type)
{
    int base = baseType(type);
    QString suffix;
    if (hasZ(type) && hasM(type)) suffix = "ZM";
    else if (hasZ(type)) suffix = "Z";
    else if (hasM(type)) suffix = "M";

    switch (base) {
    case wkbPoint: return "Point" + suffix;
    case wkbLineString: return "LineString" + suffix;
    case wkbPolygon: return "Polygon" + suffix;
    case wkbMultiPoint: return "MultiPoint" + suffix;
    case wkbMultiLineString: return "MultiLineString" + suffix;
    case wkbMultiPolygon: return "MultiPolygon" + suffix;
    case wkbGeometryCollection: return "GeometryCollection" + suffix;
    default: return "Unknown";
    }
}

bool hasZ(int type)
{
    return (type >= 1001 && type <= 1007) || (type >= 3001 && type <= 3007);
}

bool hasM(int type)
{
    return (type >= 2001 && type <= 2007) || (type >= 3001 && type <= 3007);
}

int baseType(int type)
{
    if (type >= 3001) return type - 3000;
    if (type >= 2001) return type - 2000;
    if (type >= 1001) return type - 1000;
    return type;
}

bool parsePoint(const QByteArray &wkb, QGeoCoordinate &coord, QString &errorString)
{
    WKBReader reader(wkb);

    if (!reader.readByteOrder()) {
        errorString = QObject::tr("Invalid WKB: too short");
        return false;
    }

    quint32 type;
    if (!reader.readUInt32(type)) {
        errorString = QObject::tr("Invalid WKB: cannot read type");
        return false;
    }

    int base = baseType(type);
    if (base != wkbPoint) {
        errorString = QObject::tr("Expected Point, got %1").arg(geometryTypeName(type));
        return false;
    }

    if (!reader.readCoordinate(coord, hasZ(type), hasM(type))) {
        errorString = QObject::tr("Invalid WKB: cannot read coordinates");
        return false;
    }

    return true;
}

bool parseLineString(const QByteArray &wkb, QList<QGeoCoordinate> &coords, QString &errorString)
{
    WKBReader reader(wkb);

    if (!reader.readByteOrder()) {
        errorString = QObject::tr("Invalid WKB: too short");
        return false;
    }

    quint32 type;
    if (!reader.readUInt32(type)) {
        errorString = QObject::tr("Invalid WKB: cannot read type");
        return false;
    }

    int base = baseType(type);
    if (base != wkbLineString) {
        errorString = QObject::tr("Expected LineString, got %1").arg(geometryTypeName(type));
        return false;
    }

    if (!reader.readRing(coords, hasZ(type), hasM(type))) {
        errorString = QObject::tr("Invalid WKB: cannot read coordinates");
        return false;
    }

    return true;
}

bool parsePolygon(const QByteArray &wkb, QList<QGeoCoordinate> &vertices, QString &errorString)
{
    WKBReader reader(wkb);

    if (!reader.readByteOrder()) {
        errorString = QObject::tr("Invalid WKB: too short");
        return false;
    }

    quint32 type;
    if (!reader.readUInt32(type)) {
        errorString = QObject::tr("Invalid WKB: cannot read type");
        return false;
    }

    int base = baseType(type);
    if (base != wkbPolygon) {
        errorString = QObject::tr("Expected Polygon, got %1").arg(geometryTypeName(type));
        return false;
    }

    quint32 numRings;
    if (!reader.readUInt32(numRings)) {
        errorString = QObject::tr("Invalid WKB: cannot read ring count");
        return false;
    }

    if (numRings < 1) {
        errorString = QObject::tr("Invalid WKB: polygon has no rings");
        return false;
    }

    // Read outer ring only
    if (!reader.readRing(vertices, hasZ(type), hasM(type))) {
        errorString = QObject::tr("Invalid WKB: cannot read outer ring");
        return false;
    }

    return true;
}

bool parsePolygonWithHoles(const QByteArray &wkb, QGeoPolygon &polygon, QString &errorString)
{
    WKBReader reader(wkb);

    if (!reader.readByteOrder()) {
        errorString = QObject::tr("Invalid WKB: too short");
        return false;
    }

    quint32 type;
    if (!reader.readUInt32(type)) {
        errorString = QObject::tr("Invalid WKB: cannot read type");
        return false;
    }

    int base = baseType(type);
    if (base != wkbPolygon) {
        errorString = QObject::tr("Expected Polygon, got %1").arg(geometryTypeName(type));
        return false;
    }

    quint32 numRings;
    if (!reader.readUInt32(numRings)) {
        errorString = QObject::tr("Invalid WKB: cannot read ring count");
        return false;
    }

    if (numRings < 1) {
        errorString = QObject::tr("Invalid WKB: polygon has no rings");
        return false;
    }

    // Read outer ring
    QList<QGeoCoordinate> outerRing;
    if (!reader.readRing(outerRing, hasZ(type), hasM(type))) {
        errorString = QObject::tr("Invalid WKB: cannot read outer ring");
        return false;
    }
    polygon.setPerimeter(outerRing);

    // Read holes
    for (quint32 i = 1; i < numRings; i++) {
        QList<QGeoCoordinate> hole;
        if (!reader.readRing(hole, hasZ(type), hasM(type))) {
            errorString = QObject::tr("Invalid WKB: cannot read hole %1").arg(i);
            return false;
        }
        polygon.addHole(hole);
    }

    return true;
}

bool parseMultiPoint(const QByteArray &wkb, QList<QGeoCoordinate> &points, QString &errorString)
{
    WKBReader reader(wkb);

    if (!reader.readByteOrder()) {
        errorString = QObject::tr("Invalid WKB: too short");
        return false;
    }

    quint32 type;
    if (!reader.readUInt32(type)) {
        errorString = QObject::tr("Invalid WKB: cannot read type");
        return false;
    }

    int base = baseType(type);
    if (base != wkbMultiPoint) {
        errorString = QObject::tr("Expected MultiPoint, got %1").arg(geometryTypeName(type));
        return false;
    }

    quint32 numPoints;
    if (!reader.readUInt32(numPoints)) {
        errorString = QObject::tr("Invalid WKB: cannot read point count");
        return false;
    }

    points.clear();
    for (quint32 i = 0; i < numPoints; i++) {
        QGeoCoordinate coord;
        if (!parsePoint(reader.remainingData(), coord, errorString)) {
            return false;
        }
        points.append(coord);
        reader.setPos(reader.pos() + 21 + (hasZ(type) ? 8 : 0) + (hasM(type) ? 8 : 0));
    }

    return true;
}

bool parseMultiLineString(const QByteArray &wkb, QList<QList<QGeoCoordinate>> &polylines, QString &errorString)
{
    WKBReader reader(wkb);

    if (!reader.readByteOrder()) {
        errorString = QObject::tr("Invalid WKB: too short");
        return false;
    }

    quint32 type;
    if (!reader.readUInt32(type)) {
        errorString = QObject::tr("Invalid WKB: cannot read type");
        return false;
    }

    int base = baseType(type);
    if (base != wkbMultiLineString) {
        errorString = QObject::tr("Expected MultiLineString, got %1").arg(geometryTypeName(type));
        return false;
    }

    quint32 numLineStrings;
    if (!reader.readUInt32(numLineStrings)) {
        errorString = QObject::tr("Invalid WKB: cannot read linestring count");
        return false;
    }

    polylines.clear();
    for (quint32 i = 0; i < numLineStrings; i++) {
        QList<QGeoCoordinate> coords;
        if (!parseLineString(reader.remainingData(), coords, errorString)) {
            return false;
        }
        polylines.append(coords);
        // Advance past the parsed linestring - approximation
        reader.setPos(reader.pos() + 9 + coords.count() * (16 + (hasZ(type) ? 8 : 0) + (hasM(type) ? 8 : 0)));
    }

    return true;
}

bool parseMultiPolygon(const QByteArray &wkb, QList<QList<QGeoCoordinate>> &polygons, QString &errorString)
{
    WKBReader reader(wkb);

    if (!reader.readByteOrder()) {
        errorString = QObject::tr("Invalid WKB: too short");
        return false;
    }

    quint32 type;
    if (!reader.readUInt32(type)) {
        errorString = QObject::tr("Invalid WKB: cannot read type");
        return false;
    }

    int base = baseType(type);
    if (base != wkbMultiPolygon) {
        errorString = QObject::tr("Expected MultiPolygon, got %1").arg(geometryTypeName(type));
        return false;
    }

    quint32 numPolygons;
    if (!reader.readUInt32(numPolygons)) {
        errorString = QObject::tr("Invalid WKB: cannot read polygon count");
        return false;
    }

    polygons.clear();
    for (quint32 i = 0; i < numPolygons; i++) {
        QList<QGeoCoordinate> vertices;
        if (!parsePolygon(reader.remainingData(), vertices, errorString)) {
            return false;
        }
        polygons.append(vertices);
        // Advance - approximation (should track actual bytes read)
        reader.setPos(reader.pos() + 13 + (vertices.count() + 1) * (16 + (hasZ(type) ? 8 : 0)));
    }

    return true;
}

bool parseGeometry(const QByteArray &wkb,
                   QList<QGeoCoordinate> &points,
                   QList<QList<QGeoCoordinate>> &polylines,
                   QList<QList<QGeoCoordinate>> &polygons,
                   QString &errorString)
{
    int type = geometryType(wkb);
    if (type < 0) {
        errorString = QObject::tr("Invalid WKB data");
        return false;
    }

    int base = baseType(type);

    switch (base) {
    case wkbPoint: {
        QGeoCoordinate coord;
        if (!parsePoint(wkb, coord, errorString)) return false;
        points.append(coord);
        return true;
    }
    case wkbLineString: {
        QList<QGeoCoordinate> coords;
        if (!parseLineString(wkb, coords, errorString)) return false;
        polylines.append(coords);
        return true;
    }
    case wkbPolygon: {
        QList<QGeoCoordinate> vertices;
        if (!parsePolygon(wkb, vertices, errorString)) return false;
        polygons.append(vertices);
        return true;
    }
    case wkbMultiPoint: {
        return parseMultiPoint(wkb, points, errorString);
    }
    case wkbMultiLineString: {
        return parseMultiLineString(wkb, polylines, errorString);
    }
    case wkbMultiPolygon: {
        return parseMultiPolygon(wkb, polygons, errorString);
    }
    case wkbGeometryCollection: {
        // Parse geometry collection header
        WKBReader reader(wkb);
        if (!reader.readByteOrder()) {
            errorString = QObject::tr("Invalid WKB: cannot read byte order");
            return false;
        }

        quint32 wkbType;
        if (!reader.readUInt32(wkbType)) {
            errorString = QObject::tr("Invalid WKB: cannot read type");
            return false;
        }

        quint32 numGeometries;
        if (!reader.readUInt32(numGeometries)) {
            errorString = QObject::tr("Invalid WKB: cannot read geometry count");
            return false;
        }

        // Recursively parse each geometry in the collection
        for (quint32 i = 0; i < numGeometries; i++) {
            QByteArray subWkb = reader.remainingData();
            if (subWkb.isEmpty()) {
                errorString = QObject::tr("Invalid WKB: truncated geometry collection at index %1").arg(i);
                return false;
            }

            // Get size of this sub-geometry by parsing it
            int subType = geometryType(subWkb);
            if (subType < 0) {
                errorString = QObject::tr("Invalid WKB: invalid geometry at index %1").arg(i);
                return false;
            }

            // Parse the sub-geometry recursively
            if (!parseGeometry(subWkb, points, polylines, polygons, errorString)) {
                return false;
            }

            // Estimate size consumed (header + data)
            // This is approximate - ideally we'd track actual bytes read
            int subBase = baseType(subType);
            bool subHasZ = hasZ(subType);
            int coordSize = subHasZ ? 24 : 16;  // 2 or 3 doubles
            int headerSize = 5;  // byte order + type

            int bytesConsumed = headerSize;
            switch (subBase) {
            case wkbPoint:
                bytesConsumed += coordSize;
                break;
            case wkbLineString:
            case wkbPolygon:
                // Approximate - skip past this geometry
                bytesConsumed += 4;  // point count
                break;
            default:
                // For multi-geometries, just skip a minimum
                bytesConsumed += 4;
                break;
            }

            reader.setPos(reader.pos() + bytesConsumed);
        }

        return true;
    }
    default:
        errorString = QObject::tr("Unknown geometry type: %1").arg(type);
        return false;
    }
}

QByteArray toWKBPoint(const QGeoCoordinate &coord, bool includeZ)
{
    WKBWriter writer;
    writer.writeByteOrder();
    writer.writeUInt32(includeZ ? wkbPointZ : wkbPoint);
    writer.writeCoordinate(coord, includeZ);
    return writer.data();
}

QByteArray toWKBLineString(const QList<QGeoCoordinate> &coords, bool includeZ)
{
    WKBWriter writer;
    writer.writeByteOrder();
    writer.writeUInt32(includeZ ? wkbLineStringZ : wkbLineString);
    writer.writeRing(coords, includeZ, false);
    return writer.data();
}

QByteArray toWKBPolygon(const QList<QGeoCoordinate> &vertices, bool includeZ)
{
    WKBWriter writer;
    writer.writeByteOrder();
    writer.writeUInt32(includeZ ? wkbPolygonZ : wkbPolygon);
    writer.writeUInt32(1); // One ring
    writer.writeRing(vertices, includeZ, true);
    return writer.data();
}

QByteArray toWKBMultiPoint(const QList<QGeoCoordinate> &points, bool includeZ)
{
    WKBWriter writer;
    writer.writeByteOrder();
    writer.writeUInt32(includeZ ? wkbMultiPointZ : wkbMultiPoint);
    writer.writeUInt32(points.count());

    for (const QGeoCoordinate &point : points) {
        QByteArray pointWkb = toWKBPoint(point, includeZ);
        // Write raw bytes (already has header)
        writer.writeByteOrder();
        writer.writeUInt32(includeZ ? wkbPointZ : wkbPoint);
        writer.writeCoordinate(point, includeZ);
    }

    return writer.data();
}

QByteArray toWKBMultiLineString(const QList<QList<QGeoCoordinate>> &polylines, bool includeZ)
{
    WKBWriter writer;
    writer.writeByteOrder();
    writer.writeUInt32(includeZ ? wkbMultiLineStringZ : wkbMultiLineString);
    writer.writeUInt32(polylines.count());

    for (const QList<QGeoCoordinate> &coords : polylines) {
        writer.writeByteOrder();
        writer.writeUInt32(includeZ ? wkbLineStringZ : wkbLineString);
        writer.writeRing(coords, includeZ, false);
    }

    return writer.data();
}

QByteArray toWKBMultiPolygon(const QList<QList<QGeoCoordinate>> &polygons, bool includeZ)
{
    WKBWriter writer;
    writer.writeByteOrder();
    writer.writeUInt32(includeZ ? wkbMultiPolygonZ : wkbMultiPolygon);
    writer.writeUInt32(polygons.count());

    for (const QList<QGeoCoordinate> &vertices : polygons) {
        writer.writeByteOrder();
        writer.writeUInt32(includeZ ? wkbPolygonZ : wkbPolygon);
        writer.writeUInt32(1); // One ring
        writer.writeRing(vertices, includeZ, true);
    }

    return writer.data();
}

bool parseGeoPackageHeader(const QByteArray &gpb, int &srid, int &wkbOffset, QString &errorString)
{
    // GeoPackage Binary header:
    // 2 bytes: magic "GP"
    // 1 byte: version
    // 1 byte: flags
    // 4 bytes: SRID
    // [envelope if present]
    // WKB geometry

    if (gpb.size() < 8) {
        errorString = QObject::tr("GeoPackage binary too short");
        return false;
    }

    // Check magic
    if (gpb[0] != 'G' || gpb[1] != 'P') {
        errorString = QObject::tr("Invalid GeoPackage magic bytes");
        return false;
    }

    quint8 flags = gpb[3];
    bool littleEndian = (flags & 0x01) != 0;
    int envelopeType = (flags >> 1) & 0x07;

    // Read SRID
    if (littleEndian) {
        srid = static_cast<quint8>(gpb[4]) |
               (static_cast<quint8>(gpb[5]) << 8) |
               (static_cast<quint8>(gpb[6]) << 16) |
               (static_cast<quint8>(gpb[7]) << 24);
    } else {
        srid = (static_cast<quint8>(gpb[4]) << 24) |
               (static_cast<quint8>(gpb[5]) << 16) |
               (static_cast<quint8>(gpb[6]) << 8) |
               static_cast<quint8>(gpb[7]);
    }

    // Calculate envelope size
    int envelopeSize = 0;
    switch (envelopeType) {
    case 0: envelopeSize = 0; break;      // No envelope
    case 1: envelopeSize = 32; break;     // XY envelope
    case 2: envelopeSize = 48; break;     // XYZ envelope
    case 3: envelopeSize = 48; break;     // XYM envelope
    case 4: envelopeSize = 64; break;     // XYZM envelope
    default:
        errorString = QObject::tr("Unknown envelope type: %1").arg(envelopeType);
        return false;
    }

    wkbOffset = 8 + envelopeSize;

    if (wkbOffset > gpb.size()) {
        errorString = QObject::tr("GeoPackage binary truncated");
        return false;
    }

    return true;
}

QByteArray toGeoPackageBinary(const QByteArray &wkb, int srid)
{
    QByteArray gpb;
    gpb.reserve(8 + wkb.size());

    // Magic
    gpb.append('G');
    gpb.append('P');

    // Version
    gpb.append(static_cast<char>(0));

    // Flags: little endian, no envelope
    gpb.append(static_cast<char>(0x01));

    // SRID (little endian)
    gpb.append(static_cast<char>(srid & 0xFF));
    gpb.append(static_cast<char>((srid >> 8) & 0xFF));
    gpb.append(static_cast<char>((srid >> 16) & 0xFF));
    gpb.append(static_cast<char>((srid >> 24) & 0xFF));

    // WKB
    gpb.append(wkb);

    return gpb;
}

} // namespace WKBHelper
