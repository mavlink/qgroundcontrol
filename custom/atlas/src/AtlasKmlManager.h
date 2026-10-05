#pragma once

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/qnumeric.h>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>

class QXmlStreamReader;

class AtlasKmlManager : public QObject
{
    Q_OBJECT

    // Render models
    Q_PROPERTY(QVariantList polygons READ polygons NOTIFY featuresChanged)
    Q_PROPERTY(QVariantList lines READ lines NOTIFY featuresChanged)
    Q_PROPERTY(QVariantList points READ points NOTIFY featuresChanged)

    // Backwards-compatible alias used by older ATLAS QML.
    Q_PROPERTY(QVariantList zones READ polygons NOTIFY featuresChanged)

    // Google-Earth-like tree shown in the KML layer manager.
    Q_PROPERTY(QVariantList treeItems READ treeItems NOTIFY treeItemsChanged)

    Q_PROPERTY(bool loaded READ loaded NOTIFY featuresChanged)
    Q_PROPERTY(int featureCount READ featureCount NOTIFY featuresChanged)
    Q_PROPERTY(int zoneCount READ zoneCount NOTIFY featuresChanged)
    Q_PROPERTY(int polygonCount READ polygonCount NOTIFY featuresChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY featuresChanged)
    Q_PROPERTY(int pointCount READ pointCount NOTIFY featuresChanged)
    Q_PROPERTY(int folderCount READ folderCount NOTIFY treeItemsChanged)
    Q_PROPERTY(int placemarkCount READ placemarkCount NOTIFY treeItemsChanged)

    Q_PROPERTY(QString fileName READ fileName NOTIFY featuresChanged)
    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY featuresChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QGeoCoordinate centerCoordinate READ centerCoordinate NOTIFY featuresChanged)
    Q_PROPERTY(double latitudeSpan READ latitudeSpan NOTIFY featuresChanged)
    Q_PROPERTY(double longitudeSpan READ longitudeSpan NOTIFY featuresChanged)

    // Master layer toggle. Individual folders/placemarks remain in their
    // current state when this switch is toggled.
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
    explicit AtlasKmlManager(QObject *parent = nullptr);

    QVariantList polygons() const { return _polygonsModel; }
    QVariantList lines() const { return _linesModel; }
    QVariantList points() const { return _pointsModel; }
    QVariantList treeItems() const { return _treeModel; }

    bool loaded() const { return !_features.isEmpty(); }
    int featureCount() const { return _features.size(); }
    int zoneCount() const { return _polygonsModel.size(); }
    int polygonCount() const { return _polygonsModel.size(); }
    int lineCount() const { return _linesModel.size(); }
    int pointCount() const { return _pointsModel.size(); }
    int folderCount() const { return _folderCount; }
    int placemarkCount() const { return _placemarkCount; }

    QString fileName() const { return _fileName; }
    QString sourcePath() const { return _sourcePath; }
    QString errorString() const { return _errorString; }
    QGeoCoordinate centerCoordinate() const { return _centerCoordinate; }
    double latitudeSpan() const { return _latitudeSpan; }
    double longitudeSpan() const { return _longitudeSpan; }

    bool visible() const { return _visible; }
    void setVisible(bool visible);

    Q_INVOKABLE bool loadKml(const QUrl &url);
    Q_INVOKABLE void clear();

    // The tree IDs are stable for the currently loaded document only.
    Q_INVOKABLE void setTreeItemVisible(int id, bool visible);
    Q_INVOKABLE void setTreeItemExpanded(int id, bool expanded);
    Q_INVOKABLE void setAllItemsVisible(bool visible);

signals:
    void featuresChanged();
    void treeItemsChanged();
    void errorStringChanged();
    void visibleChanged();
    void eventReceived(const QString &level, const QString &message, qint64 timestampMs);

private:
    struct StyleData {
        QString lineColor = QStringLiteral("#FFFFFFFF");
        double lineWidth = 1.0;
        QString polyColor = QStringLiteral("#FFFFFFFF");
        bool polyFill = true;
        bool polyOutline = true;
        QString iconHref;
        double iconScale = 1.0;
        QString labelColor = QStringLiteral("#FFFFFFFF");
        double labelScale = 1.0;

        bool hasLineColor = false;
        bool hasLineWidth = false;
        bool hasPolyColor = false;
        bool hasPolyFill = false;
        bool hasPolyOutline = false;
        bool hasIconHref = false;
        bool hasIconScale = false;
        bool hasLabelColor = false;
        bool hasLabelScale = false;
    };

    struct TreeNode {
        int id = -1;
        int parentId = -1;
        int depth = 0;
        bool folder = false;
        bool document = false;
        QString name;
        bool checked = true;
        bool expanded = false;
        QList<int> children;
        QList<int> featureIndices;
    };

    struct Feature {
        int treeId = -1;
        QString kind;                 // polygon, line, point
        QString name;
        QString description;
        QVariantList coordinates;
        QGeoCoordinate point;
        int altitudeMode = 0;         // 0 clamp, 1 relativeToGround, 2 absolute
        QString altitudeModeName;
        bool extrude = false;
        double altitudeMin = qQNaN();
        double altitudeMax = qQNaN();
        StyleData style;
    };

    void _setError(const QString &message);
    void _loadSettings();
    void _saveSettings() const;

    void _firstPassStyles(const QByteArray &data);
    StyleData _readStyle(QXmlStreamReader &xml) const;
    void _readStyleMap(QXmlStreamReader &xml, const QString &id);
    StyleData _resolveStyle(const QString &styleUrl) const;
    static StyleData _mergeStyle(const StyleData &base, const StyleData &overlay);

    bool _secondPassDocument(const QByteArray &data);
    int _parseContainer(QXmlStreamReader &xml, int parentId, int depth, bool document);
    int _parsePlacemark(QXmlStreamReader &xml, int parentId, int depth);
    Feature _parsePolygon(QXmlStreamReader &xml, int treeId);
    Feature _parseLineString(QXmlStreamReader &xml, int treeId);
    Feature _parsePoint(QXmlStreamReader &xml, int treeId);

    static QVariantList _parseCoordinates(const QString &text, double *minAltitude, double *maxAltitude);
    static int _altitudeModeCode(const QString &mode);
    static QString _kmlColorToQt(const QString &kmlColor);
    static bool _boolText(const QString &text, bool defaultValue);
    static QString _elementText(QXmlStreamReader &xml);

    void _applyStyleToFeature(Feature &feature, const StyleData &style);
    void _updateBounds(const QVariantList &coordinates);
    void _updateBounds(const QGeoCoordinate &coordinate);
    void _refreshModels();

    bool _treeNodeEffectiveChecked(int id) const;
    bool _treeNodeRowVisible(int id) const;
    int _treeNodeRecursiveFeatureCount(int id) const;
    QString _treeNodeGeometrySummary(const TreeNode &node) const;
    QString _treeNodeAltitudeSummary(const TreeNode &node) const;
    QString _treeNodePreviewColor(const TreeNode &node) const;
    void _setNodeCheckedRecursive(int id, bool checked);

    QVariantMap _featureToVariant(const Feature &feature, int featureIndex, bool visible) const;

    QList<TreeNode> _treeNodes;
    QList<Feature> _features;

    QVariantList _polygonsModel;
    QVariantList _linesModel;
    QVariantList _pointsModel;
    QVariantList _treeModel;

    QHash<QString, StyleData> _styles;
    QHash<QString, QString> _styleMapNormal;

    QString _fileName;
    QString _sourcePath;
    QString _errorString;

    QGeoCoordinate _centerCoordinate;
    double _latitudeSpan = 0.0;
    double _longitudeSpan = 0.0;
    double _minLat = 90.0;
    double _maxLat = -90.0;
    double _minLon = 180.0;
    double _maxLon = -180.0;
    bool _haveBounds = false;

    bool _visible = true;
    int _folderCount = 0;
    int _placemarkCount = 0;
};
