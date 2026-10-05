#include "AtlasKmlManager.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QVariantMap>
#include <QtCore/QXmlStreamReader>
#include <QtPositioning/QGeoCoordinate>

namespace {
constexpr const char *kSettingsGroup = "ATLAS/KmlOverlay";

QString localName(const QXmlStreamReader &xml)
{
    return xml.name().toString();
}

QString cleanedId(const QString &url)
{
    const QString trimmed = url.trimmed();
    return trimmed.startsWith(QLatin1Char('#')) ? trimmed.mid(1) : trimmed;
}

bool finiteAltitude(double value)
{
    return std::isfinite(value);
}
}

AtlasKmlManager::AtlasKmlManager(QObject *parent)
    : QObject(parent)
{
    _loadSettings();
}

void AtlasKmlManager::setVisible(bool visible)
{
    if (_visible == visible) {
        return;
    }

    _visible = visible;
    _saveSettings();
    emit visibleChanged();
}

bool AtlasKmlManager::loadKml(const QUrl &url)
{
    _setError(QString());

    const QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
    if (path.trimmed().isEmpty()) {
        _setError(QStringLiteral("No KML file selected"));
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        _setError(QStringLiteral("Could not open KML: %1").arg(file.errorString()));
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    _styles.clear();
    _styleMapNormal.clear();
    _treeNodes.clear();
    _features.clear();
    _polygonsModel.clear();
    _linesModel.clear();
    _pointsModel.clear();
    _treeModel.clear();

    _folderCount = 0;
    _placemarkCount = 0;

    _minLat = 90.0;
    _maxLat = -90.0;
    _minLon = 180.0;
    _maxLon = -180.0;
    _haveBounds = false;

    _firstPassStyles(data);

    if (!_secondPassDocument(data)) {
        return false;
    }

    if (_features.isEmpty()) {
        _setError(QStringLiteral("No supported KML geometry was found (Polygon, LineString or Point)"));
        return false;
    }

    _sourcePath = QFileInfo(path).absoluteFilePath();
    _fileName = QFileInfo(path).fileName();

    if (_haveBounds) {
        _centerCoordinate = QGeoCoordinate((_minLat + _maxLat) / 2.0, (_minLon + _maxLon) / 2.0);
        _latitudeSpan = std::max(0.0, _maxLat - _minLat);
        _longitudeSpan = std::max(0.0, _maxLon - _minLon);
    } else {
        _centerCoordinate = QGeoCoordinate();
        _latitudeSpan = 0.0;
        _longitudeSpan = 0.0;
    }

    _refreshModels();

    emit featuresChanged();
    emit treeItemsChanged();

    emit eventReceived(
        QStringLiteral("info"),
        QStringLiteral("KML loaded: %1 (%2 folders, %3 placemarks, %4 geometries)")
            .arg(_fileName)
            .arg(_folderCount)
            .arg(_placemarkCount)
            .arg(_features.size()),
        QDateTime::currentMSecsSinceEpoch());

    return true;
}

void AtlasKmlManager::clear()
{
    if (_features.isEmpty() && _sourcePath.isEmpty()) {
        return;
    }

    _styles.clear();
    _styleMapNormal.clear();
    _treeNodes.clear();
    _features.clear();
    _polygonsModel.clear();
    _linesModel.clear();
    _pointsModel.clear();
    _treeModel.clear();

    _fileName.clear();
    _sourcePath.clear();
    _centerCoordinate = QGeoCoordinate();
    _latitudeSpan = 0.0;
    _longitudeSpan = 0.0;
    _folderCount = 0;
    _placemarkCount = 0;

    _setError(QString());

    emit featuresChanged();
    emit treeItemsChanged();

    emit eventReceived(
        QStringLiteral("info"),
        QStringLiteral("KML overlay cleared"),
        QDateTime::currentMSecsSinceEpoch());
}

void AtlasKmlManager::setTreeItemVisible(int id, bool visible)
{
    if (id < 0 || id >= _treeNodes.size()) {
        return;
    }

    _setNodeCheckedRecursive(id, visible);
    _refreshModels();
    emit featuresChanged();
    emit treeItemsChanged();
}

void AtlasKmlManager::setTreeItemExpanded(int id, bool expanded)
{
    if (id < 0 || id >= _treeNodes.size() || !_treeNodes[id].folder) {
        return;
    }

    if (_treeNodes[id].expanded == expanded) {
        return;
    }

    _treeNodes[id].expanded = expanded;
    _refreshModels();
    emit treeItemsChanged();
}

void AtlasKmlManager::setAllItemsVisible(bool visible)
{
    if (_treeNodes.isEmpty()) {
        return;
    }

    _setNodeCheckedRecursive(0, visible);
    _refreshModels();
    emit featuresChanged();
    emit treeItemsChanged();
}

void AtlasKmlManager::_firstPassStyles(const QByteArray &data)
{
    QXmlStreamReader xml(data);

    while (!xml.atEnd()) {
        xml.readNext();

        if (!xml.isStartElement()) {
            continue;
        }

        const QString name = localName(xml);
        if (name == QStringLiteral("Style")) {
            const QString id = xml.attributes().value(QStringLiteral("id")).toString().trimmed();
            if (!id.isEmpty()) {
                _styles.insert(id, _readStyle(xml));
            }
        } else if (name == QStringLiteral("StyleMap")) {
            const QString id = xml.attributes().value(QStringLiteral("id")).toString().trimmed();
            if (!id.isEmpty()) {
                _readStyleMap(xml, id);
            }
        }
    }
}

AtlasKmlManager::StyleData AtlasKmlManager::_readStyle(QXmlStreamReader &xml) const
{
    StyleData style;
    QString section;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = localName(xml);

            if (name == QStringLiteral("LineStyle") ||
                name == QStringLiteral("PolyStyle") ||
                name == QStringLiteral("IconStyle") ||
                name == QStringLiteral("LabelStyle")) {
                section = name;
                continue;
            }

            if (name == QStringLiteral("color")) {
                const QString value = _elementText(xml).trimmed();
                if (section == QStringLiteral("LineStyle")) {
                    style.lineColor = _kmlColorToQt(value);
                    style.hasLineColor = true;
                } else if (section == QStringLiteral("PolyStyle")) {
                    style.polyColor = _kmlColorToQt(value);
                    style.hasPolyColor = true;
                } else if (section == QStringLiteral("LabelStyle")) {
                    style.labelColor = _kmlColorToQt(value);
                    style.hasLabelColor = true;
                }
            } else if (name == QStringLiteral("width") && section == QStringLiteral("LineStyle")) {
                bool ok = false;
                const double value = _elementText(xml).toDouble(&ok);
                if (ok) {
                    style.lineWidth = std::clamp(value, 0.1, 20.0);
                    style.hasLineWidth = true;
                }
            } else if (name == QStringLiteral("fill") && section == QStringLiteral("PolyStyle")) {
                style.polyFill = _boolText(_elementText(xml), true);
                style.hasPolyFill = true;
            } else if (name == QStringLiteral("outline") && section == QStringLiteral("PolyStyle")) {
                style.polyOutline = _boolText(_elementText(xml), true);
                style.hasPolyOutline = true;
            } else if (name == QStringLiteral("scale")) {
                bool ok = false;
                const double value = _elementText(xml).toDouble(&ok);
                if (ok) {
                    if (section == QStringLiteral("IconStyle")) {
                        style.iconScale = std::clamp(value, 0.25, 4.0);
                        style.hasIconScale = true;
                    } else if (section == QStringLiteral("LabelStyle")) {
                        style.labelScale = std::clamp(value, 0.0, 4.0);
                        style.hasLabelScale = true;
                    }
                }
            } else if (name == QStringLiteral("href") && section == QStringLiteral("IconStyle")) {
                style.iconHref = _elementText(xml).trimmed();
                style.hasIconHref = true;
            }
        } else if (xml.isEndElement()) {
            const QString name = localName(xml);
            if (name == QStringLiteral("LineStyle") ||
                name == QStringLiteral("PolyStyle") ||
                name == QStringLiteral("IconStyle") ||
                name == QStringLiteral("LabelStyle")) {
                section.clear();
            } else if (name == QStringLiteral("Style")) {
                break;
            }
        }
    }

    return style;
}

void AtlasKmlManager::_readStyleMap(QXmlStreamReader &xml, const QString &id)
{
    QString currentKey;
    QString normalUrl;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = localName(xml);
            if (name == QStringLiteral("key")) {
                currentKey = _elementText(xml).trimmed();
            } else if (name == QStringLiteral("styleUrl")) {
                const QString url = cleanedId(_elementText(xml));
                if (currentKey == QStringLiteral("normal")) {
                    normalUrl = url;
                }
            }
        } else if (xml.isEndElement() && localName(xml) == QStringLiteral("StyleMap")) {
            break;
        }
    }

    if (!normalUrl.isEmpty()) {
        _styleMapNormal.insert(id, normalUrl);
    }
}

AtlasKmlManager::StyleData AtlasKmlManager::_resolveStyle(const QString &styleUrl) const
{
    QString id = cleanedId(styleUrl);
    for (int i = 0; i < 8 && !id.isEmpty(); ++i) {
        const auto mapIt = _styleMapNormal.constFind(id);
        if (mapIt == _styleMapNormal.constEnd()) {
            break;
        }
        id = cleanedId(mapIt.value());
    }

    const auto styleIt = _styles.constFind(id);
    if (styleIt != _styles.constEnd()) {
        return styleIt.value();
    }

    return StyleData();
}

AtlasKmlManager::StyleData AtlasKmlManager::_mergeStyle(const StyleData &base, const StyleData &overlay)
{
    StyleData result = base;

#define ATLAS_APPLY_STYLE(field, flag) \
    if (overlay.flag) {                \
        result.field = overlay.field;  \
        result.flag = true;            \
    }

    ATLAS_APPLY_STYLE(lineColor, hasLineColor)
    ATLAS_APPLY_STYLE(lineWidth, hasLineWidth)
    ATLAS_APPLY_STYLE(polyColor, hasPolyColor)
    ATLAS_APPLY_STYLE(polyFill, hasPolyFill)
    ATLAS_APPLY_STYLE(polyOutline, hasPolyOutline)
    ATLAS_APPLY_STYLE(iconHref, hasIconHref)
    ATLAS_APPLY_STYLE(iconScale, hasIconScale)
    ATLAS_APPLY_STYLE(labelColor, hasLabelColor)
    ATLAS_APPLY_STYLE(labelScale, hasLabelScale)

#undef ATLAS_APPLY_STYLE

    return result;
}

bool AtlasKmlManager::_secondPassDocument(const QByteArray &data)
{
    QXmlStreamReader xml(data);

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement() && localName(xml) == QStringLiteral("Document")) {
            _parseContainer(xml, -1, 0, true);
            break;
        }
    }

    if (xml.hasError()) {
        _setError(QStringLiteral("KML parse error at line %1: %2")
                      .arg(xml.lineNumber())
                      .arg(xml.errorString()));
        return false;
    }

    if (_treeNodes.isEmpty()) {
        _setError(QStringLiteral("KML has no Document element"));
        return false;
    }

    return true;
}

int AtlasKmlManager::_parseContainer(QXmlStreamReader &xml, int parentId, int depth, bool document)
{
    TreeNode node;
    node.id = _treeNodes.size();
    node.parentId = parentId;
    node.depth = depth;
    node.folder = true;
    node.document = document;
    node.name = document ? QStringLiteral("KML Document") : QStringLiteral("Folder");
    node.checked = true;
    node.expanded = document;
    _treeNodes.append(node);

    const int id = node.id;

    if (parentId >= 0 && parentId < _treeNodes.size()) {
        _treeNodes[parentId].children.append(id);
    }

    if (!document) {
        ++_folderCount;
    }

    const QString closingName = document ? QStringLiteral("Document") : QStringLiteral("Folder");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = localName(xml);

            if (name == QStringLiteral("name")) {
                const QString value = _elementText(xml).trimmed();
                if (!value.isEmpty()) {
                    _treeNodes[id].name = value;
                }
            } else if (name == QStringLiteral("visibility")) {
                _treeNodes[id].checked = _boolText(_elementText(xml), true);
            } else if (name == QStringLiteral("open")) {
                _treeNodes[id].expanded = _boolText(_elementText(xml), document);
            } else if (name == QStringLiteral("Folder")) {
                _parseContainer(xml, id, depth + 1, false);
            } else if (name == QStringLiteral("Placemark")) {
                _parsePlacemark(xml, id, depth + 1);
            } else if (name == QStringLiteral("Style") || name == QStringLiteral("StyleMap")) {
                xml.skipCurrentElement();
            } else {
                // Container metadata such as Schema/ListStyle may contain
                // nested <name> elements. Skip those subtrees so only direct
                // Document/Folder metadata can rename this tree node.
                xml.skipCurrentElement();
            }
        } else if (xml.isEndElement() && localName(xml) == closingName) {
            break;
        }
    }

    return id;
}

int AtlasKmlManager::_parsePlacemark(QXmlStreamReader &xml, int parentId, int depth)
{
    TreeNode node;
    node.id = _treeNodes.size();
    node.parentId = parentId;
    node.depth = depth;
    node.folder = false;
    node.document = false;
    node.name = QStringLiteral("Placemark");
    node.checked = true;
    node.expanded = false;
    _treeNodes.append(node);

    const int id = node.id;

    if (parentId >= 0 && parentId < _treeNodes.size()) {
        _treeNodes[parentId].children.append(id);
    }

    ++_placemarkCount;

    QString styleUrl;
    StyleData inlineStyle;
    bool haveInlineStyle = false;
    QString description;
    QList<int> featureIndices;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = localName(xml);

            if (name == QStringLiteral("name")) {
                const QString value = _elementText(xml).trimmed();
                if (!value.isEmpty()) {
                    _treeNodes[id].name = value;
                }
            } else if (name == QStringLiteral("description")) {
                description = _elementText(xml).trimmed();
            } else if (name == QStringLiteral("visibility")) {
                _treeNodes[id].checked = _boolText(_elementText(xml), true);
            } else if (name == QStringLiteral("styleUrl")) {
                styleUrl = _elementText(xml).trimmed();
            } else if (name == QStringLiteral("Style")) {
                inlineStyle = _readStyle(xml);
                haveInlineStyle = true;
            } else if (name == QStringLiteral("Polygon")) {
                Feature feature = _parsePolygon(xml, id);
                feature.name = _treeNodes[id].name;
                feature.description = description;
                featureIndices.append(_features.size());
                _features.append(std::move(feature));
            } else if (name == QStringLiteral("LineString")) {
                Feature feature = _parseLineString(xml, id);
                feature.name = _treeNodes[id].name;
                feature.description = description;
                featureIndices.append(_features.size());
                _features.append(std::move(feature));
            } else if (name == QStringLiteral("Point")) {
                Feature feature = _parsePoint(xml, id);
                feature.name = _treeNodes[id].name;
                feature.description = description;
                featureIndices.append(_features.size());
                _features.append(std::move(feature));
            } else if (name != QStringLiteral("MultiGeometry")) {
                // Skip LookAt, ExtendedData and other metadata subtrees. A
                // MultiGeometry is deliberately not skipped because its
                // Polygon/LineString/Point children are parsed by this loop.
                xml.skipCurrentElement();
            }
        } else if (xml.isEndElement() && localName(xml) == QStringLiteral("Placemark")) {
            break;
        }
    }

    StyleData style = _resolveStyle(styleUrl);
    if (haveInlineStyle) {
        style = _mergeStyle(style, inlineStyle);
    }

    for (const int featureIndex : std::as_const(featureIndices)) {
        if (featureIndex < 0 || featureIndex >= _features.size()) {
            continue;
        }

        Feature &feature = _features[featureIndex];
        feature.name = _treeNodes[id].name;
        feature.description = description;
        _applyStyleToFeature(feature, style);
        _treeNodes[id].featureIndices.append(featureIndex);

        if (feature.kind == QStringLiteral("point")) {
            _updateBounds(feature.point);
        } else {
            _updateBounds(feature.coordinates);
        }
    }

    return id;
}

AtlasKmlManager::Feature AtlasKmlManager::_parsePolygon(QXmlStreamReader &xml, int treeId)
{
    Feature feature;
    feature.treeId = treeId;
    feature.kind = QStringLiteral("polygon");
    feature.altitudeMode = 0;
    feature.altitudeModeName = QStringLiteral("clampToGround");

    bool inOuterBoundary = false;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = localName(xml);

            if (name == QStringLiteral("outerBoundaryIs")) {
                inOuterBoundary = true;
            } else if (name == QStringLiteral("innerBoundaryIs")) {
                inOuterBoundary = false;
            } else if (name == QStringLiteral("altitudeMode")) {
                feature.altitudeModeName = _elementText(xml).trimmed();
                feature.altitudeMode = _altitudeModeCode(feature.altitudeModeName);
            } else if (name == QStringLiteral("extrude")) {
                feature.extrude = _boolText(_elementText(xml), false);
            } else if (name == QStringLiteral("coordinates") && inOuterBoundary) {
                feature.coordinates = _parseCoordinates(
                    _elementText(xml), &feature.altitudeMin, &feature.altitudeMax);
            }
        } else if (xml.isEndElement()) {
            const QString name = localName(xml);
            if (name == QStringLiteral("outerBoundaryIs") || name == QStringLiteral("innerBoundaryIs")) {
                inOuterBoundary = false;
            } else if (name == QStringLiteral("Polygon")) {
                break;
            }
        }
    }

    return feature;
}

AtlasKmlManager::Feature AtlasKmlManager::_parseLineString(QXmlStreamReader &xml, int treeId)
{
    Feature feature;
    feature.treeId = treeId;
    feature.kind = QStringLiteral("line");
    feature.altitudeMode = 0;
    feature.altitudeModeName = QStringLiteral("clampToGround");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = localName(xml);

            if (name == QStringLiteral("altitudeMode")) {
                feature.altitudeModeName = _elementText(xml).trimmed();
                feature.altitudeMode = _altitudeModeCode(feature.altitudeModeName);
            } else if (name == QStringLiteral("extrude")) {
                feature.extrude = _boolText(_elementText(xml), false);
            } else if (name == QStringLiteral("coordinates")) {
                feature.coordinates = _parseCoordinates(
                    _elementText(xml), &feature.altitudeMin, &feature.altitudeMax);
            }
        } else if (xml.isEndElement() && localName(xml) == QStringLiteral("LineString")) {
            break;
        }
    }

    return feature;
}

AtlasKmlManager::Feature AtlasKmlManager::_parsePoint(QXmlStreamReader &xml, int treeId)
{
    Feature feature;
    feature.treeId = treeId;
    feature.kind = QStringLiteral("point");
    feature.altitudeMode = 0;
    feature.altitudeModeName = QStringLiteral("clampToGround");

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = localName(xml);

            if (name == QStringLiteral("altitudeMode")) {
                feature.altitudeModeName = _elementText(xml).trimmed();
                feature.altitudeMode = _altitudeModeCode(feature.altitudeModeName);
            } else if (name == QStringLiteral("extrude")) {
                feature.extrude = _boolText(_elementText(xml), false);
            } else if (name == QStringLiteral("coordinates")) {
                double minAltitude = qQNaN();
                double maxAltitude = qQNaN();
                const QVariantList coordinates = _parseCoordinates(
                    _elementText(xml), &minAltitude, &maxAltitude);
                if (!coordinates.isEmpty()) {
                    feature.point = coordinates.first().value<QGeoCoordinate>();
                    feature.altitudeMin = minAltitude;
                    feature.altitudeMax = maxAltitude;
                }
            }
        } else if (xml.isEndElement() && localName(xml) == QStringLiteral("Point")) {
            break;
        }
    }

    return feature;
}

QVariantList AtlasKmlManager::_parseCoordinates(const QString &text, double *minAltitude, double *maxAltitude)
{
    QVariantList coordinates;
    double localMinAltitude = std::numeric_limits<double>::infinity();
    double localMaxAltitude = -std::numeric_limits<double>::infinity();
    bool haveAltitude = false;

    const QStringList tokens = text.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        const QStringList fields = token.split(QLatin1Char(','));
        if (fields.size() < 2) {
            continue;
        }

        bool lonOk = false;
        bool latOk = false;
        const double lon = fields.at(0).toDouble(&lonOk);
        const double lat = fields.at(1).toDouble(&latOk);

        if (!lonOk || !latOk || lon < -180.0 || lon > 180.0 || lat < -90.0 || lat > 90.0) {
            continue;
        }

        double altitude = qQNaN();
        if (fields.size() >= 3) {
            bool altitudeOk = false;
            const double value = fields.at(2).toDouble(&altitudeOk);
            if (altitudeOk && finiteAltitude(value)) {
                altitude = value;
                localMinAltitude = std::min(localMinAltitude, value);
                localMaxAltitude = std::max(localMaxAltitude, value);
                haveAltitude = true;
            }
        }

        const QGeoCoordinate coordinate = finiteAltitude(altitude)
                                              ? QGeoCoordinate(lat, lon, altitude)
                                              : QGeoCoordinate(lat, lon);
        coordinates.append(QVariant::fromValue(coordinate));
    }

    // Explicitly closed rings/paths are common in KML. Polygon/line visuals
    // do not need a duplicate last vertex.
    if (coordinates.size() >= 2) {
        const QGeoCoordinate first = coordinates.first().value<QGeoCoordinate>();
        const QGeoCoordinate last = coordinates.last().value<QGeoCoordinate>();
        if (first.isValid() && last.isValid() &&
            first.distanceTo(last) < 0.01 &&
            ((!finiteAltitude(first.altitude()) && !finiteAltitude(last.altitude())) ||
             qAbs(first.altitude() - last.altitude()) < 0.01)) {
            coordinates.removeLast();
        }
    }

    if (minAltitude) {
        *minAltitude = haveAltitude ? localMinAltitude : qQNaN();
    }
    if (maxAltitude) {
        *maxAltitude = haveAltitude ? localMaxAltitude : qQNaN();
    }

    return coordinates;
}

int AtlasKmlManager::_altitudeModeCode(const QString &mode)
{
    const QString normalized = mode.trimmed().toLower();

    if (normalized == QStringLiteral("absolute")) {
        return 2;
    }
    if (normalized == QStringLiteral("relativetoground") ||
        normalized == QStringLiteral("relativetoseafloor")) {
        return 1;
    }

    return 0;
}

QString AtlasKmlManager::_kmlColorToQt(const QString &kmlColor)
{
    const QString value = kmlColor.trimmed();

    // KML colors are aabbggrr. Qt/QML accepts #AARRGGBB.
    if (value.size() == 8) {
        return QStringLiteral("#%1%2%3%4")
            .arg(value.mid(0, 2).toUpper(),
                 value.mid(6, 2).toUpper(),
                 value.mid(4, 2).toUpper(),
                 value.mid(2, 2).toUpper());
    }

    if (value.size() == 6) {
        // Be permissive with non-standard bbGGRR values.
        return QStringLiteral("#FF%1%2%3")
            .arg(value.mid(4, 2).toUpper(),
                 value.mid(2, 2).toUpper(),
                 value.mid(0, 2).toUpper());
    }

    return QStringLiteral("#FFFFFFFF");
}

bool AtlasKmlManager::_boolText(const QString &text, bool defaultValue)
{
    const QString value = text.trimmed().toLower();
    if (value == QStringLiteral("1") || value == QStringLiteral("true")) {
        return true;
    }
    if (value == QStringLiteral("0") || value == QStringLiteral("false")) {
        return false;
    }
    return defaultValue;
}

QString AtlasKmlManager::_elementText(QXmlStreamReader &xml)
{
    return xml.readElementText(QXmlStreamReader::IncludeChildElements);
}

void AtlasKmlManager::_applyStyleToFeature(Feature &feature, const StyleData &style)
{
    feature.style = style;
}

void AtlasKmlManager::_updateBounds(const QVariantList &coordinates)
{
    for (const QVariant &coordinateValue : coordinates) {
        _updateBounds(coordinateValue.value<QGeoCoordinate>());
    }
}

void AtlasKmlManager::_updateBounds(const QGeoCoordinate &coordinate)
{
    if (!coordinate.isValid()) {
        return;
    }

    _minLat = std::min(_minLat, coordinate.latitude());
    _maxLat = std::max(_maxLat, coordinate.latitude());
    _minLon = std::min(_minLon, coordinate.longitude());
    _maxLon = std::max(_maxLon, coordinate.longitude());
    _haveBounds = true;
}

bool AtlasKmlManager::_treeNodeEffectiveChecked(int id) const
{
    int current = id;
    while (current >= 0 && current < _treeNodes.size()) {
        if (!_treeNodes[current].checked) {
            return false;
        }
        current = _treeNodes[current].parentId;
    }
    return true;
}

bool AtlasKmlManager::_treeNodeRowVisible(int id) const
{
    if (id < 0 || id >= _treeNodes.size()) {
        return false;
    }

    int parent = _treeNodes[id].parentId;
    while (parent >= 0 && parent < _treeNodes.size()) {
        if (!_treeNodes[parent].expanded) {
            return false;
        }
        parent = _treeNodes[parent].parentId;
    }
    return true;
}

int AtlasKmlManager::_treeNodeRecursiveFeatureCount(int id) const
{
    if (id < 0 || id >= _treeNodes.size()) {
        return 0;
    }

    int count = _treeNodes[id].featureIndices.size();
    for (const int child : _treeNodes[id].children) {
        count += _treeNodeRecursiveFeatureCount(child);
    }
    return count;
}

QString AtlasKmlManager::_treeNodeGeometrySummary(const TreeNode &node) const
{
    if (node.folder) {
        const int count = _treeNodeRecursiveFeatureCount(node.id);
        return count == 1 ? QStringLiteral("1 geometry")
                          : QStringLiteral("%1 geometries").arg(count);
    }

    int polygons = 0;
    int lines = 0;
    int points = 0;
    for (const int featureIndex : node.featureIndices) {
        if (featureIndex < 0 || featureIndex >= _features.size()) {
            continue;
        }
        const QString kind = _features[featureIndex].kind;
        if (kind == QStringLiteral("polygon")) ++polygons;
        else if (kind == QStringLiteral("line")) ++lines;
        else if (kind == QStringLiteral("point")) ++points;
    }

    QStringList parts;
    if (polygons) parts << (polygons == 1 ? QStringLiteral("Polygon") : QStringLiteral("%1 polygons").arg(polygons));
    if (lines) parts << (lines == 1 ? QStringLiteral("Line") : QStringLiteral("%1 lines").arg(lines));
    if (points) parts << (points == 1 ? QStringLiteral("Point") : QStringLiteral("%1 points").arg(points));
    return parts.join(QStringLiteral(" + "));
}

QString AtlasKmlManager::_treeNodeAltitudeSummary(const TreeNode &node) const
{
    if (node.folder || node.featureIndices.isEmpty()) {
        return QString();
    }

    const Feature &feature = _features[node.featureIndices.first()];
    if (!finiteAltitude(feature.altitudeMax)) {
        return feature.altitudeMode == 0 ? QStringLiteral("ground") : QString();
    }

    const double altitude = feature.altitudeMax;
    QString suffix;
    if (feature.altitudeMode == 2) {
        suffix = QStringLiteral(" m AMSL");
    } else if (feature.altitudeMode == 1) {
        suffix = QStringLiteral(" m AGL");
    } else {
        return QStringLiteral("ground");
    }

    if (finiteAltitude(feature.altitudeMin) && qAbs(feature.altitudeMin - feature.altitudeMax) > 0.01) {
        return QStringLiteral("%1–%2%3")
            .arg(feature.altitudeMin, 0, 'f', 0)
            .arg(feature.altitudeMax, 0, 'f', 0)
            .arg(suffix);
    }

    return QStringLiteral("%1%2").arg(altitude, 0, 'f', 0).arg(suffix);
}

QString AtlasKmlManager::_treeNodePreviewColor(const TreeNode &node) const
{
    if (node.featureIndices.isEmpty()) {
        return QStringLiteral("#667784");
    }

    const Feature &feature = _features[node.featureIndices.first()];
    return feature.style.lineColor;
}

void AtlasKmlManager::_setNodeCheckedRecursive(int id, bool checked)
{
    if (id < 0 || id >= _treeNodes.size()) {
        return;
    }

    _treeNodes[id].checked = checked;
    for (const int child : _treeNodes[id].children) {
        _setNodeCheckedRecursive(child, checked);
    }
}

QVariantMap AtlasKmlManager::_featureToVariant(const Feature &feature, int featureIndex, bool visible) const
{
    QVariantMap map;
    map.insert(QStringLiteral("featureIndex"), featureIndex);
    map.insert(QStringLiteral("treeId"), feature.treeId);
    map.insert(QStringLiteral("kind"), feature.kind);
    map.insert(QStringLiteral("name"), feature.name);
    map.insert(QStringLiteral("description"), feature.description);
    map.insert(QStringLiteral("coordinates"), feature.coordinates);
    map.insert(QStringLiteral("coordinate"), QVariant::fromValue(feature.point));
    map.insert(QStringLiteral("altitudeModeCode"), feature.altitudeMode);
    map.insert(QStringLiteral("altitudeMode"), feature.altitudeModeName);
    map.insert(QStringLiteral("extrude"), feature.extrude);
    map.insert(QStringLiteral("visible"), visible);

    map.insert(QStringLiteral("lineColor"), feature.style.lineColor);
    map.insert(QStringLiteral("lineWidth"), feature.style.lineWidth);
    map.insert(QStringLiteral("fillColor"), feature.style.polyColor);
    map.insert(QStringLiteral("fillEnabled"), feature.style.polyFill);
    map.insert(QStringLiteral("outlineEnabled"), feature.style.polyOutline);
    map.insert(QStringLiteral("iconHref"), feature.style.iconHref);
    map.insert(QStringLiteral("iconScale"), feature.style.iconScale);
    map.insert(QStringLiteral("labelColor"), feature.style.labelColor);
    map.insert(QStringLiteral("labelScale"), feature.style.labelScale);

    map.insert(QStringLiteral("altitudeMin"), feature.altitudeMin);
    map.insert(QStringLiteral("altitudeMax"), feature.altitudeMax);

    return map;
}

void AtlasKmlManager::_refreshModels()
{
    _polygonsModel.clear();
    _linesModel.clear();
    _pointsModel.clear();
    _treeModel.clear();

    for (int i = 0; i < _features.size(); ++i) {
        const Feature &feature = _features[i];
        const bool featureVisible = _treeNodeEffectiveChecked(feature.treeId);
        const QVariantMap variant = _featureToVariant(feature, i, featureVisible);

        if (feature.kind == QStringLiteral("polygon")) {
            if (feature.coordinates.size() >= 3) {
                _polygonsModel.append(variant);
            }
        } else if (feature.kind == QStringLiteral("line")) {
            if (feature.coordinates.size() >= 2) {
                _linesModel.append(variant);
            }
        } else if (feature.kind == QStringLiteral("point")) {
            if (feature.point.isValid()) {
                _pointsModel.append(variant);
            }
        }
    }

    for (const TreeNode &node : std::as_const(_treeNodes)) {
        QVariantMap row;
        row.insert(QStringLiteral("id"), node.id);
        row.insert(QStringLiteral("parentId"), node.parentId);
        row.insert(QStringLiteral("depth"), node.depth);
        row.insert(QStringLiteral("type"), node.folder ? QStringLiteral("folder") : QStringLiteral("placemark"));
        row.insert(QStringLiteral("document"), node.document);
        row.insert(QStringLiteral("name"), node.name);
        row.insert(QStringLiteral("checked"), node.checked);
        row.insert(QStringLiteral("effectiveChecked"), _treeNodeEffectiveChecked(node.id));
        row.insert(QStringLiteral("expanded"), node.expanded);
        row.insert(QStringLiteral("rowVisible"), _treeNodeRowVisible(node.id));
        row.insert(QStringLiteral("childCount"), node.children.size());
        row.insert(QStringLiteral("featureCount"), _treeNodeRecursiveFeatureCount(node.id));
        row.insert(QStringLiteral("summary"), _treeNodeGeometrySummary(node));
        row.insert(QStringLiteral("altitude"), _treeNodeAltitudeSummary(node));
        row.insert(QStringLiteral("previewColor"), _treeNodePreviewColor(node));
        _treeModel.append(row);
    }
}

void AtlasKmlManager::_setError(const QString &message)
{
    if (_errorString == message) {
        return;
    }

    _errorString = message;
    emit errorStringChanged();

    if (!message.isEmpty()) {
        emit eventReceived(
            QStringLiteral("warning"),
            message,
            QDateTime::currentMSecsSinceEpoch());
    }
}

void AtlasKmlManager::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    _visible = settings.value(QStringLiteral("visible"), true).toBool();
    settings.endGroup();
}

void AtlasKmlManager::_saveSettings() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kSettingsGroup));
    settings.setValue(QStringLiteral("visible"), _visible);
    settings.endGroup();
}
