#include "ExtractJPEGMetadata.h"

#include <cstring>

#include <QDomDocument>
#include <QDebug>

#include "easyexif.h"
#include "JPEGSegmentParser.h"


namespace {

static const char xmpMarker[] = "http://ns.adobe.com/xap/1.0/";

bool isXmpSegment(const JPEGSegmentParser::Segment& seg)
{
    std::size_t size = static_cast<size_t>(seg.payload_end - seg.payload_start);
    if (size < sizeof(xmpMarker)) {
        return false;
    }
    return std::memcmp(seg.payload_start, xmpMarker, sizeof(xmpMarker)) == 0;
}

void parseXmp(const JPEGSegmentParser::Segment& seg, std::map<QString, QString>* dst)
{
    const char * begin = seg.payload_start + sizeof(xmpMarker);
    std::size_t size = seg.payload_end - begin;

    QDomDocument dom;
    dom.setContent(QByteArray(begin, size));

    QDomElement root = dom.documentElement();
    auto rdfs = root.elementsByTagName("rdf:RDF");
    for (int index = 0; index < rdfs.length(); ++index) {
        const auto& rdf = rdfs.item(index);
        auto descriptions = rdf.childNodes();
        for (int index = 0; index < descriptions.length(); ++index) {
            if (descriptions.item(index).nodeName() != "rdf:Description") {
                continue;
            }
            const auto& description = descriptions.item(index);
            const auto& attrs = description.attributes();
            for (int index = 0; index < attrs.length(); ++index) {
                const auto& item = attrs.item(index);
                const QString& name = item.nodeName();
                const QString& value = item.nodeValue();
                (*dst)[name] = value;
            }

            //parse child fields
            auto fields = description.childNodes();
            for (int index = 0; index < fields.length(); ++index) {
                auto element = fields.item(index).toElement();
                (*dst)[element.tagName()] = element.text();
            }
        }
    }
}

void setUnlessEmpty(
    const std::string& value,
    const QString& key,
    std::map<QString, QString>* dst)
{
    if (!value.empty()) {
        (*dst)[key] = QString::fromStdString(value);
    }
}

void setUnlessEmpty(
    const easyexif::EXIFInfo::Geolocation_t::Coord_t& value,
    const QString& key,
    std::map<QString, QString>* dst)
{
    if (value.direction != '?') {
        (*dst)[key] = QString().sprintf("%.0f° %.0f' %.2f'' %c", value.degrees, value.minutes, value.seconds, value.direction);
    }
}

void parseExif(const QByteArray& data, std::map<QString, QString>* dst)
{
    easyexif::EXIFInfo info;
    info.parseFrom(reinterpret_cast<const unsigned char*>(data.data()),
                   data.size());

    setUnlessEmpty(info.Make, "Exif:Make", dst);
    setUnlessEmpty(info.Model, "Exif:Model", dst);
    if (info.FocalLength) {
        (*dst)["Exif:FocalLength"] = QString::number(info.FocalLength) + " mm";
    }
    setUnlessEmpty(info.GeoLocation.LatComponents, "Exif:GPSLatitude", dst);
    setUnlessEmpty(info.GeoLocation.LonComponents, "Exif:GPSLongitude", dst);
    if (info.GeoLocation.Altitude || info.GeoLocation.AltitudeRef) {
        (*dst)["Exif::GPSAltitude"] = QString::number(info.GeoLocation.Altitude);
        (*dst)["Exif::GPSAltitudeRef"] = info.GeoLocation.AltitudeRef == 0 ? "AboveNull" : "BelowNull";
    }
    setUnlessEmpty(info.DateTime, "Exif:DateTime", dst);
}

}  // namespace

// Note that this open-codes some handling of JPEG file metadata -- chosen
// libraries are LGPL, in order to circle around GPL'd libexiv2.
std::map<QString, QString> extractJPEGMetadata(
    const QByteArray& image_data)
{
    std::map<QString, QString> data;

    parseExif(image_data, &data);

    JPEGSegmentParser parser(image_data.data(), image_data.data() + image_data.size());
    JPEGSegmentParser::Segment seg;
    while (parser.next(seg) == JPEGSegmentParser::ParseResult::Segment) {
        if (seg.kind == 0xe1 && isXmpSegment(seg)) {
            parseXmp(seg, &data);
        }
    }

    return data;
}
