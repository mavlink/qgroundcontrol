/****************************************************************************
 *
 *   (c) 2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QtTest/QtTest>
#include <QDebug>

#include "MapGridMGRS.h"

#define EPSILON 0.0001

class MapGridTests : public QObject {
    Q_OBJECT

public:
    QVariant values;

signals:
    void valuesChanged();

private slots:
    void updateValues(const QVariant& newValues);

    void testMGRSZoneBoundaries();
    void testMapGridMGRS();
};

void
MapGridTests::updateValues(const QVariant& newValues)
{
    values = newValues;
}

bool compare(double d1, double d2)
{
    return std::abs(d1 - d2) < EPSILON;
}

bool compare(const QGeoCoordinate& c1, const QGeoCoordinate& c2)
{
    return compare(c1.latitude(), c2.latitude()) && compare(c1.longitude(), c2.longitude());
}

bool compare(const QList<QString>& list1, const QList<QString>& list2)
{
    if (list1.count() != list2.count()) {
        return false;
    }
    for (int i = 0; i < list1.count(); i++) {
        if (!list2.contains(list1[i])) {
            return false;
        }
    }
    return true;
}

bool compare(const QGeoPath& path1, const QGeoPath& path2)
{
    if (path1.size() != path2.size()) {
        return false;
    }

    for (int i = 0; i < path1.size(); i++) {
        bool found = false;
        for (int j = 0; j<path2.size(); j++) {
            if (compare(path1.coordinateAt(i), path2.coordinateAt(j))) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }


    return true;
}

bool compare(const QList<QGeoPath>& list1, const QList<QGeoPath>& list2)
{
    if (list1.count() != list2.count()) {
        return false;
    }
    for (int i = 0; i<list1.count(); i++) {
        bool found = false;
        for (int j = 0; j<list2.count(); j++) {
            if (compare(list1[i], list2[i])) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

QList<QString> extractLabelTexts(const QVariant val)
{
    QList<QString> labels;

    QJsonValue jsonVal = QJsonValue::fromVariant(val);
    QJsonArray jsonLabels = jsonVal["labels"].toArray();

    for (int i = 0; i < jsonLabels.count(); i++) {
        labels.push_back(jsonLabels.at(i)["text"].toString());
    }

    return labels;
}

QList<QGeoPath> extractPaths(const QVariant val)
{
    QList<QGeoPath> paths;

    QJsonValue jsonVal = QJsonValue::fromVariant(val);
    QJsonArray jsonPaths = jsonVal["lines"].toArray();

    for (int i = 0; i < jsonPaths.count(); i++) {
        QGeoPath path;
        QJsonArray pts = jsonPaths.at(i)["points"].toArray();
        if (pts.count() == 0) {
            continue;
        }
        for (int j = 0; j < pts.count(); j++) {
            double lat = pts.at(j)["lat"].toDouble();
            double lng = pts.at(j)["lng"].toDouble();
            path.addCoordinate(QGeoCoordinate(lat, lng));
        }
        paths.push_back(path);
    }

    return paths;
}

// Tests boundaries of created MGRS zones. Where leftOverlap or rightOverlap is true also _fixEdge method results are tested
void MapGridTests::testMGRSZoneBoundaries()
{
    MGRSZone z0("3X2Y");
    QVERIFY(!z0.valid);

    MGRSZone z1("32UPU");
    QVERIFY(z1.valid);
    QVERIFY(!z1.leftOverlap);
    QVERIFY(!z1.rightOverlap);
    QVERIFY(compare(z1.bottomLeft, QGeoCoordinate(47.845565672, 10.336597771)));
    QVERIFY(compare(z1.bottomRight, QGeoCoordinate(47.822240269, 11.672048336)));
    QVERIFY(compare(z1.topLeft, QGeoCoordinate(48.744979534, 10.360285924)));
    QVERIFY(compare(z1.topRight, QGeoCoordinate(48.720910536, 11.719351462)));

    MGRSZone z2("32UQU");
    QVERIFY(z2.valid);
    QVERIFY(!z2.leftOverlap);
    QVERIFY(z2.rightOverlap);
    QVERIFY(compare(z2.bottomLeft, QGeoCoordinate(47.822239958, 11.672061682)));
    QVERIFY(compare(z2.bottomRight, QGeoCoordinate(47.814132066, 11.999990732)));
    QVERIFY(compare(z2.topLeft, QGeoCoordinate(48.720910216, 11.719365043)));
    QVERIFY(compare(z2.topRight, QGeoCoordinate(48.713939877, 11.999986648)));

    MGRSZone z3("32URU");
    QVERIFY(z3.valid);
    QVERIFY(z3.leftOverlap);
    QVERIFY(z3.rightOverlap);
    QVERIFY(compare(z3.bottomLeft, QGeoCoordinate(47.783418534, 13.005277239)));
    QVERIFY(compare(z3.bottomRight, QGeoCoordinate(47.729183703, 14.335121901)));
    QVERIFY(compare(z3.topLeft, QGeoCoordinate(48.680852741, 13.076050836)));
    QVERIFY(compare(z3.topRight, QGeoCoordinate(48.624894327, 14.429149724)));
}

void generateReferenceTestData(double zoomLevel, const QGeoCoordinate& topLeft, const QGeoCoordinate& bottomRight, const QVariant& qvals)
{
    QString out;
    QTextStream stream(&out);

    stream << "geometryChanged(" << zoomLevel
           << ", QGeoCoordinate(" << topLeft.latitude() << ", " << topLeft.longitude() << ")"
           << ", QGeoCoordinate(" << bottomRight.latitude() << ", " << bottomRight.longitude() << "));";
    qDebug() << out;
    out = "";

    QList<QString> lbls = extractLabelTexts(qvals);
    stream << "QList<QString> labels({";
    for (int i = 0; i < lbls.count(); i++) {
        if (i > 0) {
            stream << ", ";
        }
        stream << "\"" << lbls[i] << "\"";
    }
    stream << "});";
    qDebug() << out;
    out = "";

    QList<QGeoPath> pths = extractPaths(qvals);
    stream << "QList<QGeoPath> paths({";
    for (int i = 0; i < pths.count(); i++) {
        if (i > 0) {
            stream << ", ";
        }
        stream << "QGeoPath({";
        for (int j = 0; j < pths[i].size(); j++) {
            if (j > 0) {
                stream << ", ";
            }
            QGeoCoordinate c = pths[i].coordinateAt(j);
            stream << "QGeoCoordinate(" << c.latitude() << ", " << c.longitude() << ")";
        }
        stream << "})";
    }
    stream << "});";

    qDebug() << out;
}

void MapGridTests::testMapGridMGRS()
{
    MapGridMGRS mapGrid;
    QList<QString> labels;
    QList<QGeoPath> paths;

    connect(&mapGrid, &MapGridMGRS::updateValues, this, &MapGridTests::updateValues);

    mapGrid.geometryChanged(8.24, QGeoCoordinate(48.8634, 3.78194), QGeoCoordinate(44.7184, 15.3777));
    QList<QString> refLabels3({"YQ", "QV", "PV", "QU", "GR", "BP", "CP", "DP", "GP", "FP", "MU", "MT", "VP", "VN", "YP", "XP", "BQ", "YK", "BR", "YL", "TK", "QQ", "LP", "BJ", "CJ", "CK", "BK", "BL", "BM", "CM", "CL", "DL", "DK", "DJ", "EJ", "EK", "EL", "FL", "FK", "FJ", "GJ", "KP", "GK", "KQ", "GL", "KR", "LR", "LQ", "MQ", "MP", "NP", "NQ", "PQ", "PP", "QP", "TJ", "UJ", "UK", "QR", "TL", "UL", "VL", "VK", "VJ", "WJ", "WK", "WL", "XL", "XK", "XJ", "YJ", "BP", "CP", "CQ", "CR", "CS", "YM", "BS", "YN", "XN", "XM", "WM", "VM", "UM", "TM", "QS", "PS", "PR", "NR", "MR", "MS", "LS", "GM", "KS", "GN", "FN", "FM", "EM", "DM", "DN", "BN", "CN", "CP", "BQ", "CQ", "BR", "CR", "DR", "DQ", "DP", "EP", "EN", "EP", "EQ", "ER", "FR", "GQ", "FQ", "FP", "KU", "LU", "KT", "LT", "LU", "KV", "LV", "KA", "LA", "MA", "MV", "MU", "NU", "PU", "QT", "PT", "PU", "TP", "UP", "TN", "UN", "UP", "TQ", "UQ", "VQ", "VP", "WP", "WN", "WP", "WQ", "XQ", "XP", "BU", "CU", "BT", "CT", "CU", "BV", "CV", "BA", "CA", "YR", "XR", "WR", "VR", "TR", "UR", "QA", "PA", "NA", "NV", "NU", "NT", "NS"});
    labels = extractLabelTexts(values);
    QVERIFY(compare(labels, refLabels3));

    QList<QGeoPath> refPaths3({QGeoPath({QGeoCoordinate(48, -60), QGeoCoordinate(48, 0)}), QGeoPath({QGeoCoordinate(48, 0), QGeoCoordinate(48, 60)}), QGeoPath({QGeoCoordinate(-80, 0), QGeoCoordinate(84, 0)}), QGeoPath({QGeoCoordinate(-80, 6), QGeoCoordinate(56, 6)}), QGeoPath({QGeoCoordinate(-80, 12), QGeoCoordinate(72, 12)}), QGeoPath({QGeoCoordinate(-80, 18), QGeoCoordinate(72, 18)}), QGeoPath({QGeoCoordinate(49.6194, 17.7691), QGeoCoordinate(48.7209, 17.7194), QGeoCoordinate(48.7139, 18)}), QGeoPath({QGeoCoordinate(49.6194, 11.7691), QGeoCoordinate(48.7209, 11.7194), QGeoCoordinate(48.7139, 12)}), QGeoPath({QGeoCoordinate(49.6443, 10.3852), QGeoCoordinate(48.745, 10.3603), QGeoCoordinate(48.7209, 11.7194)}), QGeoPath({QGeoCoordinate(48.7209, 11.7194), QGeoCoordinate(47.8222, 11.6721), QGeoCoordinate(47.8141, 12)}), QGeoPath({QGeoCoordinate(50.5177, 5.82132), QGeoCoordinate(49.6194, 5.76906), QGeoCoordinate(49.6137, 5.99999)}), QGeoPath({QGeoCoordinate(47.8141, 9.26808e-06), QGeoCoordinate(47.8222, 0.327938)}), QGeoPath({QGeoCoordinate(48.7209, 0.280649), QGeoCoordinate(47.8222, 0.327952), QGeoCoordinate(47.8456, 1.6634)}), QGeoPath({QGeoCoordinate(48.745, 1.63973), QGeoCoordinate(47.8456, 1.66342), QGeoCoordinate(47.8533, 2.99999)}), QGeoPath({QGeoCoordinate(48.7209, 5.71937), QGeoCoordinate(47.8222, 5.67206), QGeoCoordinate(47.8141, 5.99999)}), QGeoPath({QGeoCoordinate(48.745, 4.36029), QGeoCoordinate(47.8456, 4.3366), QGeoCoordinate(47.8222, 5.67205)}), QGeoPath({QGeoCoordinate(48.745, 7.63973), QGeoCoordinate(47.8456, 7.66342), QGeoCoordinate(47.8533, 8.99999)}), QGeoPath({QGeoCoordinate(47.8456, 7.66342), QGeoCoordinate(46.946, 7.68598), QGeoCoordinate(46.9535, 8.99999)}), QGeoPath({QGeoCoordinate(48.745, 13.6397), QGeoCoordinate(47.8456, 13.6634), QGeoCoordinate(47.8533, 15)}), QGeoPath({QGeoCoordinate(47.8456, 13.6634), QGeoCoordinate(46.946, 13.686), QGeoCoordinate(46.9535, 15)}), QGeoPath({QGeoCoordinate(48.7209, 17.7194), QGeoCoordinate(47.8222, 17.6721), QGeoCoordinate(47.8141, 18)}), QGeoPath({QGeoCoordinate(48.745, 16.3603), QGeoCoordinate(47.8456, 16.3366), QGeoCoordinate(47.8222, 17.672)}), QGeoPath({QGeoCoordinate(43.3262, 18.5332), QGeoCoordinate(42.4265, 18.5688), QGeoCoordinate(42.4459, 19.784)}), QGeoPath({QGeoCoordinate(44.2138, 18), QGeoCoordinate(44.2258, 18.4959)}), QGeoPath({QGeoCoordinate(45.1251, 17.5431), QGeoCoordinate(44.2258, 17.5041), QGeoCoordinate(44.2138, 18)}), QGeoPath({QGeoCoordinate(45.1141, 18), QGeoCoordinate(45.1252, 18.4569)}), QGeoPath({QGeoCoordinate(46.0243, 17.5841), QGeoCoordinate(45.1252, 17.5431), QGeoCoordinate(45.1141, 18)}), QGeoPath({QGeoCoordinate(43.3262, 12.5332), QGeoCoordinate(42.4265, 12.5688), QGeoCoordinate(42.4459, 13.784)}), QGeoPath({QGeoCoordinate(44.2138, 12), QGeoCoordinate(44.2258, 12.4959)}), QGeoPath({QGeoCoordinate(45.1251, 11.5431), QGeoCoordinate(44.2258, 11.5041), QGeoCoordinate(44.2138, 12)}), QGeoPath({QGeoCoordinate(43.3262, 6.53322), QGeoCoordinate(42.4265, 6.56882), QGeoCoordinate(42.4459, 7.784)}), QGeoPath({QGeoCoordinate(44.2258, 6.49594), QGeoCoordinate(43.3263, 6.53322), QGeoCoordinate(43.3462, 7.76618)}), QGeoPath({QGeoCoordinate(43.3262, 0.533224), QGeoCoordinate(42.4265, 0.568824), QGeoCoordinate(42.4459, 1.784)}), QGeoPath({QGeoCoordinate(44.2258, -0.49593), QGeoCoordinate(43.3263, -0.533212), QGeoCoordinate(43.3135, -5.14735e-06)}), QGeoPath({QGeoCoordinate(43.3135, 5.14735e-06), QGeoCoordinate(43.3263, 0.533212)}), QGeoPath({QGeoCoordinate(44.2258, 0.495943), QGeoCoordinate(43.3263, 0.533224), QGeoCoordinate(43.3462, 1.76618)}), QGeoPath({QGeoCoordinate(45.1251, 0.456883), QGeoCoordinate(44.2258, 0.495942), QGeoCoordinate(44.2464, 1.74752)}), QGeoPath({QGeoCoordinate(45.1251, -0.45687), QGeoCoordinate(44.2258, -0.49593), QGeoCoordinate(44.2138, -5.84107e-06)}), QGeoPath({QGeoCoordinate(44.2138, 5.84107e-06), QGeoCoordinate(44.2258, 0.49593)}), QGeoPath({QGeoCoordinate(46.0243, -0.415929), QGeoCoordinate(45.1252, -0.45687), QGeoCoordinate(45.1141, -6.30242e-06)}), QGeoPath({QGeoCoordinate(45.1141, 6.30242e-06), QGeoCoordinate(45.1252, 0.45687)}), QGeoPath({QGeoCoordinate(46.9234, -0.372992), QGeoCoordinate(46.0244, -0.415928), QGeoCoordinate(46.0142, -5.43924e-06)}), QGeoPath({QGeoCoordinate(46.0142, 5.43924e-06), QGeoCoordinate(46.0244, 0.415928)}), QGeoPath({QGeoCoordinate(46.9234, 0.373005), QGeoCoordinate(46.0244, 0.415941), QGeoCoordinate(46.0463, 1.70746)}), QGeoPath({QGeoCoordinate(46.0243, 0.415942), QGeoCoordinate(45.1252, 0.456883), QGeoCoordinate(45.1464, 1.72796)}), QGeoPath({QGeoCoordinate(46.0463, 1.70747), QGeoCoordinate(45.1464, 1.72797), QGeoCoordinate(45.1535, 2.99999)}), QGeoPath({QGeoCoordinate(45.1464, 1.72797), QGeoCoordinate(44.2464, 1.74753), QGeoCoordinate(44.2532, 2.99999)}), QGeoPath({QGeoCoordinate(44.2464, 1.74753), QGeoCoordinate(43.3462, 1.76619), QGeoCoordinate(43.3529, 2.99999)}), QGeoPath({QGeoCoordinate(43.3462, 1.76619), QGeoCoordinate(42.4459, 1.78402), QGeoCoordinate(42.4523, 2.99999)}), QGeoPath({QGeoCoordinate(43.3529, 3.00001), QGeoCoordinate(42.4523, 3.00001), QGeoCoordinate(42.4459, 4.21598)}), QGeoPath({QGeoCoordinate(44.2532, 3.00001), QGeoCoordinate(43.3529, 3.00001), QGeoCoordinate(43.3462, 4.23381)}), QGeoPath({QGeoCoordinate(45.1535, 3.00001), QGeoCoordinate(44.2532, 3.00001), QGeoCoordinate(44.2464, 4.25247)}), QGeoPath({QGeoCoordinate(46.0536, 3.00001), QGeoCoordinate(45.1535, 3.00001), QGeoCoordinate(45.1464, 4.27203)}), QGeoPath({QGeoCoordinate(46.0463, 4.29254), QGeoCoordinate(45.1464, 4.27204), QGeoCoordinate(45.1252, 5.54312)}), QGeoPath({QGeoCoordinate(45.1464, 4.27204), QGeoCoordinate(44.2464, 4.25248), QGeoCoordinate(44.2258, 5.50406)}), QGeoPath({QGeoCoordinate(43.3462, 4.23382), QGeoCoordinate(42.4459, 4.216), QGeoCoordinate(42.4265, 5.43118)}), QGeoPath({QGeoCoordinate(44.2464, 4.25248), QGeoCoordinate(43.3462, 4.23382), QGeoCoordinate(43.3263, 5.46678)}), QGeoPath({QGeoCoordinate(44.2258, 5.50407), QGeoCoordinate(43.3263, 5.46679), QGeoCoordinate(43.3135, 5.99999)}), QGeoPath({QGeoCoordinate(43.3135, 6.00001), QGeoCoordinate(43.3263, 6.53321)}), QGeoPath({QGeoCoordinate(45.1251, 5.54313), QGeoCoordinate(44.2258, 5.50407), QGeoCoordinate(44.2138, 5.99999)}), QGeoPath({QGeoCoordinate(44.2138, 6.00001), QGeoCoordinate(44.2258, 6.49593)}), QGeoPath({QGeoCoordinate(46.0243, 5.58407), QGeoCoordinate(45.1252, 5.54313), QGeoCoordinate(45.1141, 5.99999)}), QGeoPath({QGeoCoordinate(45.1141, 6.00001), QGeoCoordinate(45.1252, 6.45687)}), QGeoPath({QGeoCoordinate(46.0243, 6.41594), QGeoCoordinate(45.1252, 6.45688), QGeoCoordinate(45.1464, 7.72796)}), QGeoPath({QGeoCoordinate(45.1251, 6.45688), QGeoCoordinate(44.2258, 6.49594), QGeoCoordinate(44.2464, 7.74752)}), QGeoPath({QGeoCoordinate(45.1464, 7.72797), QGeoCoordinate(44.2464, 7.74753), QGeoCoordinate(44.2532, 8.99999)}), QGeoPath({QGeoCoordinate(44.2464, 7.74753), QGeoCoordinate(43.3462, 7.76619), QGeoCoordinate(43.3529, 8.99999)}), QGeoPath({QGeoCoordinate(43.3462, 7.76619), QGeoCoordinate(42.4459, 7.78402), QGeoCoordinate(42.4523, 8.99999)}), QGeoPath({QGeoCoordinate(43.3529, 9.00001), QGeoCoordinate(42.4523, 9.00001), QGeoCoordinate(42.4459, 10.216)}), QGeoPath({QGeoCoordinate(44.2532, 9.00001), QGeoCoordinate(43.3529, 9.00001), QGeoCoordinate(43.3462, 10.2338)}), QGeoPath({QGeoCoordinate(45.1535, 9.00001), QGeoCoordinate(44.2532, 9.00001), QGeoCoordinate(44.2464, 10.2525)}), QGeoPath({QGeoCoordinate(45.1464, 10.272), QGeoCoordinate(44.2464, 10.2525), QGeoCoordinate(44.2258, 11.5041)}), QGeoPath({QGeoCoordinate(43.3462, 10.2338), QGeoCoordinate(42.4459, 10.216), QGeoCoordinate(42.4265, 11.4312)}), QGeoPath({QGeoCoordinate(44.2464, 10.2525), QGeoCoordinate(43.3462, 10.2338), QGeoCoordinate(43.3263, 11.4668)}), QGeoPath({QGeoCoordinate(44.2258, 11.5041), QGeoCoordinate(43.3263, 11.4668), QGeoCoordinate(43.3135, 12)}), QGeoPath({QGeoCoordinate(43.3135, 12), QGeoCoordinate(43.3263, 12.5332)}), QGeoPath({QGeoCoordinate(44.2258, 12.4959), QGeoCoordinate(43.3263, 12.5332), QGeoCoordinate(43.3462, 13.7662)}), QGeoPath({QGeoCoordinate(45.1251, 12.4569), QGeoCoordinate(44.2258, 12.4959), QGeoCoordinate(44.2464, 13.7475)}), QGeoPath({QGeoCoordinate(46.0243, 11.5841), QGeoCoordinate(45.1252, 11.5431), QGeoCoordinate(45.1141, 12)}), QGeoPath({QGeoCoordinate(45.1141, 12), QGeoCoordinate(45.1252, 12.4569)}), QGeoPath({QGeoCoordinate(46.0243, 12.4159), QGeoCoordinate(45.1252, 12.4569), QGeoCoordinate(45.1464, 13.728)}), QGeoPath({QGeoCoordinate(46.0463, 13.7075), QGeoCoordinate(45.1464, 13.728), QGeoCoordinate(45.1535, 15)}), QGeoPath({QGeoCoordinate(45.1464, 13.728), QGeoCoordinate(44.2464, 13.7475), QGeoCoordinate(44.2532, 15)}), QGeoPath({QGeoCoordinate(44.2464, 13.7475), QGeoCoordinate(43.3462, 13.7662), QGeoCoordinate(43.3529, 15)}), QGeoPath({QGeoCoordinate(43.3462, 13.7662), QGeoCoordinate(42.4459, 13.784), QGeoCoordinate(42.4523, 15)}), QGeoPath({QGeoCoordinate(43.3529, 15), QGeoCoordinate(42.4523, 15), QGeoCoordinate(42.4459, 16.216)}), QGeoPath({QGeoCoordinate(44.2532, 15), QGeoCoordinate(43.3529, 15), QGeoCoordinate(43.3462, 16.2338)}), QGeoPath({QGeoCoordinate(45.1535, 15), QGeoCoordinate(44.2532, 15), QGeoCoordinate(44.2464, 16.2525)}), QGeoPath({QGeoCoordinate(46.0536, 15), QGeoCoordinate(45.1535, 15), QGeoCoordinate(45.1464, 16.272)}), QGeoPath({QGeoCoordinate(46.0463, 16.2925), QGeoCoordinate(45.1464, 16.272), QGeoCoordinate(45.1252, 17.5431)}), QGeoPath({QGeoCoordinate(45.1464, 16.272), QGeoCoordinate(44.2464, 16.2525), QGeoCoordinate(44.2258, 17.5041)}), QGeoPath({QGeoCoordinate(43.3462, 16.2338), QGeoCoordinate(42.4459, 16.216), QGeoCoordinate(42.4265, 17.4312)}), QGeoPath({QGeoCoordinate(44.2464, 16.2525), QGeoCoordinate(43.3462, 16.2338), QGeoCoordinate(43.3263, 17.4668)}), QGeoPath({QGeoCoordinate(44.2258, 17.5041), QGeoCoordinate(43.3263, 17.4668), QGeoCoordinate(43.3135, 18)}), QGeoPath({QGeoCoordinate(43.3135, 18), QGeoCoordinate(43.3263, 18.5332)}), QGeoPath({QGeoCoordinate(44.2258, 18.4959), QGeoCoordinate(43.3263, 18.5332), QGeoCoordinate(43.3462, 19.7662)}), QGeoPath({QGeoCoordinate(45.1251, 18.4569), QGeoCoordinate(44.2258, 18.4959), QGeoCoordinate(44.2464, 19.7475)}), QGeoPath({QGeoCoordinate(46.0243, 18.4159), QGeoCoordinate(45.1252, 18.4569), QGeoCoordinate(45.1464, 19.728)}), QGeoPath({QGeoCoordinate(46.9234, 18.373), QGeoCoordinate(46.0244, 18.4159), QGeoCoordinate(46.0463, 19.7075)}), QGeoPath({QGeoCoordinate(46.9234, 17.627), QGeoCoordinate(46.0244, 17.5841), QGeoCoordinate(46.0142, 18)}), QGeoPath({QGeoCoordinate(46.0142, 18), QGeoCoordinate(46.0244, 18.4159)}), QGeoPath({QGeoCoordinate(47.8222, 17.6721), QGeoCoordinate(46.9234, 17.627), QGeoCoordinate(46.9142, 18)}), QGeoPath({QGeoCoordinate(47.8456, 16.3366), QGeoCoordinate(46.946, 16.314), QGeoCoordinate(46.9234, 17.627)}), QGeoPath({QGeoCoordinate(46.946, 16.314), QGeoCoordinate(46.0463, 16.2925), QGeoCoordinate(46.0244, 17.5841)}), QGeoPath({QGeoCoordinate(46.9535, 15), QGeoCoordinate(46.0536, 15), QGeoCoordinate(46.0463, 16.2925)}), QGeoPath({QGeoCoordinate(46.946, 13.686), QGeoCoordinate(46.0463, 13.7075), QGeoCoordinate(46.0536, 15)}), QGeoPath({QGeoCoordinate(46.9234, 12.373), QGeoCoordinate(46.0244, 12.4159), QGeoCoordinate(46.0463, 13.7075)}), QGeoPath({QGeoCoordinate(46.0142, 12), QGeoCoordinate(46.0244, 12.4159)}), QGeoPath({QGeoCoordinate(46.9234, 11.627), QGeoCoordinate(46.0244, 11.5841), QGeoCoordinate(46.0142, 12)}), QGeoPath({QGeoCoordinate(46.946, 10.314), QGeoCoordinate(46.0463, 10.2925), QGeoCoordinate(46.0244, 11.5841)}), QGeoPath({QGeoCoordinate(46.0463, 10.2925), QGeoCoordinate(45.1464, 10.272), QGeoCoordinate(45.1252, 11.5431)}), QGeoPath({QGeoCoordinate(46.0536, 9.00001), QGeoCoordinate(45.1535, 9.00001), QGeoCoordinate(45.1464, 10.272)}), QGeoPath({QGeoCoordinate(46.0463, 7.70747), QGeoCoordinate(45.1464, 7.72797), QGeoCoordinate(45.1535, 8.99999)}), QGeoPath({QGeoCoordinate(46.946, 7.68598), QGeoCoordinate(46.0463, 7.70747), QGeoCoordinate(46.0536, 8.99999)}), QGeoPath({QGeoCoordinate(46.9234, 6.37301), QGeoCoordinate(46.0244, 6.41594), QGeoCoordinate(46.0463, 7.70746)}), QGeoPath({QGeoCoordinate(46.9234, 5.62701), QGeoCoordinate(46.0244, 5.58407), QGeoCoordinate(46.0142, 5.99999)}), QGeoPath({QGeoCoordinate(46.0142, 6.00001), QGeoCoordinate(46.0244, 6.41593)}), QGeoPath({QGeoCoordinate(47.8222, 5.67206), QGeoCoordinate(46.9234, 5.62701), QGeoCoordinate(46.9142, 6)}), QGeoPath({QGeoCoordinate(47.8456, 4.3366), QGeoCoordinate(46.946, 4.31404), QGeoCoordinate(46.9234, 5.627)}), QGeoPath({QGeoCoordinate(46.946, 4.31404), QGeoCoordinate(46.0463, 4.29254), QGeoCoordinate(46.0244, 5.58406)}), QGeoPath({QGeoCoordinate(46.9535, 3.00001), QGeoCoordinate(46.0536, 3.00001), QGeoCoordinate(46.0463, 4.29253)}), QGeoPath({QGeoCoordinate(46.946, 1.68598), QGeoCoordinate(46.0463, 1.70747), QGeoCoordinate(46.0536, 2.99999)}), QGeoPath({QGeoCoordinate(47.8456, 1.66342), QGeoCoordinate(46.946, 1.68598), QGeoCoordinate(46.9535, 2.99999)}), QGeoPath({QGeoCoordinate(46.9142, 2.27026e-06), QGeoCoordinate(46.9234, 0.372992)}), QGeoPath({QGeoCoordinate(47.8222, 0.327952), QGeoCoordinate(46.9234, 0.373005), QGeoCoordinate(46.946, 1.68596)}), QGeoPath({QGeoCoordinate(48.7209, 0.280649), QGeoCoordinate(47.8222, 0.327952), QGeoCoordinate(47.8456, 1.6634)}), QGeoPath({QGeoCoordinate(48.7139, 1.28178e-05), QGeoCoordinate(48.7209, 0.280634)}), QGeoPath({QGeoCoordinate(49.6194, 0.230949), QGeoCoordinate(48.7209, 0.280648), QGeoCoordinate(48.745, 1.63971)}), QGeoPath({QGeoCoordinate(49.6137, 1.23315e-05), QGeoCoordinate(49.6194, 0.230935)}), QGeoPath({QGeoCoordinate(50.5177, 0.178696), QGeoCoordinate(49.6194, 0.230949), QGeoCoordinate(49.6443, 1.61482)}), QGeoPath({QGeoCoordinate(50.5434, 1.58867), QGeoCoordinate(49.6443, 1.61484), QGeoCoordinate(49.6525, 2.99999)}), QGeoPath({QGeoCoordinate(49.6443, 1.61484), QGeoCoordinate(48.745, 1.63973), QGeoCoordinate(48.753, 2.99999)}), QGeoPath({QGeoCoordinate(48.745, 1.63973), QGeoCoordinate(47.8456, 1.66342), QGeoCoordinate(47.8533, 2.99999)}), QGeoPath({QGeoCoordinate(48.753, 3.00001), QGeoCoordinate(47.8533, 3.00001), QGeoCoordinate(47.8456, 4.33658)}), QGeoPath({QGeoCoordinate(47.8533, 3.00001), QGeoCoordinate(46.9535, 3.00001), QGeoCoordinate(46.946, 4.31402)}), QGeoPath({QGeoCoordinate(48.753, 3.00001), QGeoCoordinate(47.8533, 3.00001), QGeoCoordinate(47.8456, 4.33658)}), QGeoPath({QGeoCoordinate(49.6525, 3.00001), QGeoCoordinate(48.753, 3.00001), QGeoCoordinate(48.745, 4.36027)}), QGeoPath({QGeoCoordinate(50.5519, 3.00001), QGeoCoordinate(49.6525, 3.00001), QGeoCoordinate(49.6443, 4.38516)}), QGeoPath({QGeoCoordinate(50.5434, 4.41135), QGeoCoordinate(49.6443, 4.38518), QGeoCoordinate(49.6194, 5.76905)}), QGeoPath({QGeoCoordinate(49.6194, 5.76906), QGeoCoordinate(48.7209, 5.71937), QGeoCoordinate(48.7139, 5.99999)}), QGeoPath({QGeoCoordinate(49.6443, 4.38518), QGeoCoordinate(48.745, 4.36029), QGeoCoordinate(48.7209, 5.71935)}), QGeoPath({QGeoCoordinate(48.745, 4.36029), QGeoCoordinate(47.8456, 4.3366), QGeoCoordinate(47.8222, 5.67205)}), QGeoPath({QGeoCoordinate(47.8141, 6.00001), QGeoCoordinate(47.8222, 6.32794)}), QGeoPath({QGeoCoordinate(48.7209, 6.28065), QGeoCoordinate(47.8222, 6.32795), QGeoCoordinate(47.8456, 7.6634)}), QGeoPath({QGeoCoordinate(46.9142, 6), QGeoCoordinate(46.9234, 6.37299)}), QGeoPath({QGeoCoordinate(47.8222, 6.32795), QGeoCoordinate(46.9234, 6.373), QGeoCoordinate(46.946, 7.68596)}), QGeoPath({QGeoCoordinate(48.7209, 6.28065), QGeoCoordinate(47.8222, 6.32795), QGeoCoordinate(47.8456, 7.6634)}), QGeoPath({QGeoCoordinate(48.7139, 6.00001), QGeoCoordinate(48.7209, 6.28063)}), QGeoPath({QGeoCoordinate(49.6194, 6.23095), QGeoCoordinate(48.7209, 6.28065), QGeoCoordinate(48.745, 7.63971)}), QGeoPath({QGeoCoordinate(49.6137, 6.00001), QGeoCoordinate(49.6194, 6.23094)}), QGeoPath({QGeoCoordinate(50.5177, 6.1787), QGeoCoordinate(49.6194, 6.23095), QGeoCoordinate(49.6443, 7.61482)}), QGeoPath({QGeoCoordinate(50.5434, 7.58867), QGeoCoordinate(49.6443, 7.61484), QGeoCoordinate(49.6525, 8.99999)}), QGeoPath({QGeoCoordinate(49.6443, 7.61484), QGeoCoordinate(48.745, 7.63973), QGeoCoordinate(48.753, 8.99999)}), QGeoPath({QGeoCoordinate(48.745, 7.63973), QGeoCoordinate(47.8456, 7.66342), QGeoCoordinate(47.8533, 8.99999)}), QGeoPath({QGeoCoordinate(48.753, 9.00001), QGeoCoordinate(47.8533, 9.00001), QGeoCoordinate(47.8456, 10.3366)}), QGeoPath({QGeoCoordinate(48.745, 10.3603), QGeoCoordinate(47.8456, 10.3366), QGeoCoordinate(47.8222, 11.672)}), QGeoPath({QGeoCoordinate(47.8222, 11.6721), QGeoCoordinate(46.9234, 11.627), QGeoCoordinate(46.9142, 12)}), QGeoPath({QGeoCoordinate(47.8456, 10.3366), QGeoCoordinate(46.946, 10.314), QGeoCoordinate(46.9234, 11.627)}), QGeoPath({QGeoCoordinate(48.745, 10.3603), QGeoCoordinate(47.8456, 10.3366), QGeoCoordinate(47.8222, 11.672)}), QGeoPath({QGeoCoordinate(47.8141, 12), QGeoCoordinate(47.8222, 12.3279)}), QGeoPath({QGeoCoordinate(48.7209, 12.2806), QGeoCoordinate(47.8222, 12.328), QGeoCoordinate(47.8456, 13.6634)}), QGeoPath({QGeoCoordinate(46.9142, 12), QGeoCoordinate(46.9234, 12.373)}), QGeoPath({QGeoCoordinate(47.8222, 12.328), QGeoCoordinate(46.9234, 12.373), QGeoCoordinate(46.946, 13.686)}), QGeoPath({QGeoCoordinate(48.7209, 12.2806), QGeoCoordinate(47.8222, 12.328), QGeoCoordinate(47.8456, 13.6634)}), QGeoPath({QGeoCoordinate(48.7139, 12), QGeoCoordinate(48.7209, 12.2806)}), QGeoPath({QGeoCoordinate(49.6194, 12.2309), QGeoCoordinate(48.7209, 12.2806), QGeoCoordinate(48.745, 13.6397)}), QGeoPath({QGeoCoordinate(49.6443, 13.6148), QGeoCoordinate(48.745, 13.6397), QGeoCoordinate(48.753, 15)}), QGeoPath({QGeoCoordinate(48.745, 13.6397), QGeoCoordinate(47.8456, 13.6634), QGeoCoordinate(47.8533, 15)}), QGeoPath({QGeoCoordinate(48.753, 15), QGeoCoordinate(47.8533, 15), QGeoCoordinate(47.8456, 16.3366)}), QGeoPath({QGeoCoordinate(47.8533, 15), QGeoCoordinate(46.9535, 15), QGeoCoordinate(46.946, 16.314)}), QGeoPath({QGeoCoordinate(48.753, 15), QGeoCoordinate(47.8533, 15), QGeoCoordinate(47.8456, 16.3366)}), QGeoPath({QGeoCoordinate(49.6525, 15), QGeoCoordinate(48.753, 15), QGeoCoordinate(48.745, 16.3603)}), QGeoPath({QGeoCoordinate(49.6443, 16.3852), QGeoCoordinate(48.745, 16.3603), QGeoCoordinate(48.7209, 17.7194)}), QGeoPath({QGeoCoordinate(48.745, 16.3603), QGeoCoordinate(47.8456, 16.3366), QGeoCoordinate(47.8222, 17.672)}), QGeoPath({QGeoCoordinate(47.8141, 18), QGeoCoordinate(47.8222, 18.3279)}), QGeoPath({QGeoCoordinate(48.7209, 18.2806), QGeoCoordinate(47.8222, 18.328), QGeoCoordinate(47.8456, 19.6634)}), QGeoPath({QGeoCoordinate(46.9142, 18), QGeoCoordinate(46.9234, 18.373)}), QGeoPath({QGeoCoordinate(47.8222, 18.328), QGeoCoordinate(46.9234, 18.373), QGeoCoordinate(46.946, 19.686)}), QGeoPath({QGeoCoordinate(48.7209, 18.2806), QGeoCoordinate(47.8222, 18.328), QGeoCoordinate(47.8456, 19.6634)}), QGeoPath({QGeoCoordinate(48.7139, 18), QGeoCoordinate(48.7209, 18.2806)}), QGeoPath({QGeoCoordinate(49.6194, 18.2309), QGeoCoordinate(48.7209, 18.2806), QGeoCoordinate(48.745, 19.6397)}), QGeoPath({QGeoCoordinate(49.6137, 18), QGeoCoordinate(49.6194, 18.2309)}), QGeoPath({QGeoCoordinate(50.5177, 18.1787), QGeoCoordinate(49.6194, 18.2309), QGeoCoordinate(49.6443, 19.6148)}), QGeoPath({QGeoCoordinate(50.5177, 17.8213), QGeoCoordinate(49.6194, 17.7691), QGeoCoordinate(49.6137, 18)}), QGeoPath({QGeoCoordinate(50.5434, 16.4113), QGeoCoordinate(49.6443, 16.3852), QGeoCoordinate(49.6194, 17.7691)}), QGeoPath({QGeoCoordinate(50.5519, 15), QGeoCoordinate(49.6525, 15), QGeoCoordinate(49.6443, 16.3852)}), QGeoPath({QGeoCoordinate(50.5434, 13.5887), QGeoCoordinate(49.6443, 13.6148), QGeoCoordinate(49.6525, 15)}), QGeoPath({QGeoCoordinate(49.6137, 12), QGeoCoordinate(49.6194, 12.2309)}), QGeoPath({QGeoCoordinate(50.5177, 12.1787), QGeoCoordinate(49.6194, 12.2309), QGeoCoordinate(49.6443, 13.6148)}), QGeoPath({QGeoCoordinate(50.5177, 11.8213), QGeoCoordinate(49.6194, 11.7691), QGeoCoordinate(49.6137, 12)}), QGeoPath({QGeoCoordinate(50.5434, 10.4113), QGeoCoordinate(49.6443, 10.3852), QGeoCoordinate(49.6194, 11.7691)}), QGeoPath({QGeoCoordinate(50.5519, 9.00001), QGeoCoordinate(49.6525, 9.00001), QGeoCoordinate(49.6443, 10.3852)}), QGeoPath({QGeoCoordinate(49.6525, 9.00001), QGeoCoordinate(48.753, 9.00001), QGeoCoordinate(48.745, 10.3603)}), QGeoPath({QGeoCoordinate(48.753, 9.00001), QGeoCoordinate(47.8533, 9.00001), QGeoCoordinate(47.8456, 10.3366)}), QGeoPath({QGeoCoordinate(47.8533, 9.00001), QGeoCoordinate(46.9535, 9.00001), QGeoCoordinate(46.946, 10.314)}), QGeoPath({QGeoCoordinate(46.9535, 9.00001), QGeoCoordinate(46.0536, 9.00001), QGeoCoordinate(46.0463, 10.2925)})});
    paths = extractPaths(values);
    QVERIFY(compare(paths, refPaths3));

    mapGrid.geometryChanged(6.32, QGeoCoordinate(65.685, -16.7669), QGeoCoordinate(54.3621, 27.1139));
    QList<QString> refLabels2({"26U", "26V", "26W", "27U", "27V", "27W", "28U", "28V", "28W", "29U", "29V", "29W", "30U", "30V", "30W", "31U", "31V", "31W", "32U", "32V", "32W", "33U", "33V", "33W", "34U", "34V", "34W", "35U", "35V", "35W", "36U", "36V", "36W", "37U", "37V", "37W"});
    labels = extractLabelTexts(values);
    QVERIFY(compare(labels, refLabels2));

    QList<QGeoPath> refPaths2({QGeoPath({QGeoCoordinate(56, -60), QGeoCoordinate(56, 0)}), QGeoPath({QGeoCoordinate(56, 0), QGeoCoordinate(56, 60)}), QGeoPath({QGeoCoordinate(64, -60), QGeoCoordinate(64, 0)}), QGeoPath({QGeoCoordinate(64, 0), QGeoCoordinate(64, 60)}), QGeoPath({QGeoCoordinate(-80, -30), QGeoCoordinate(84, -30)}), QGeoPath({QGeoCoordinate(-80, -24), QGeoCoordinate(84, -24)}), QGeoPath({QGeoCoordinate(-80, -18), QGeoCoordinate(84, -18)}), QGeoPath({QGeoCoordinate(-80, -12), QGeoCoordinate(84, -12)}), QGeoPath({QGeoCoordinate(-80, -6), QGeoCoordinate(84, -6)}), QGeoPath({QGeoCoordinate(-80, 0), QGeoCoordinate(84, 0)}), QGeoPath({QGeoCoordinate(56, 3), QGeoCoordinate(64, 3)}), QGeoPath({QGeoCoordinate(64, 6), QGeoCoordinate(72, 6)}), QGeoPath({QGeoCoordinate(-80, 6), QGeoCoordinate(56, 6)}), QGeoPath({QGeoCoordinate(-80, 12), QGeoCoordinate(72, 12)}), QGeoPath({QGeoCoordinate(-80, 18), QGeoCoordinate(72, 18)}), QGeoPath({QGeoCoordinate(-80, 24), QGeoCoordinate(72, 24)}), QGeoPath({QGeoCoordinate(-80, 30), QGeoCoordinate(72, 30)}), QGeoPath({QGeoCoordinate(-80, 36), QGeoCoordinate(72, 36)})});
    paths = extractPaths(values);
    QVERIFY(compare(paths, refPaths2));

    mapGrid.geometryChanged(4.12367, QGeoCoordinate(73.9408, -92.2633), QGeoCoordinate(7.12298, 108.848));
    QList<QString> refLabels1({"01C", "01D", "01E", "01F", "01G", "01H", "01J", "01K", "01L", "01M", "01N", "01P", "01Q", "01R", "01S", "01T", "01U", "01V", "01W", "01X", "02C", "02D", "02E", "02F", "02G", "02H", "02J", "02K", "02L", "02M", "02N", "02P", "02Q", "02R", "02S", "02T", "02U", "02V", "02W", "02X", "03C", "03D", "03E", "03F", "03G", "03H", "03J", "03K", "03L", "03M", "03N", "03P", "03Q", "03R", "03S", "03T", "03U", "03V", "03W", "03X", "04C", "04D", "04E", "04F", "04G", "04H", "04J", "04K", "04L", "04M", "04N", "04P", "04Q", "04R", "04S", "04T", "04U", "04V", "04W", "04X", "05C", "05D", "05E", "05F", "05G", "05H", "05J", "05K", "05L", "05M", "05N", "05P", "05Q", "05R", "05S", "05T", "05U", "05V", "05W", "05X", "06C", "06D", "06E", "06F", "06G", "06H", "06J", "06K", "06L", "06M", "06N", "06P", "06Q", "06R", "06S", "06T", "06U", "06V", "06W", "06X", "07C", "07D", "07E", "07F", "07G", "07H", "07J", "07K", "07L", "07M", "07N", "07P", "07Q", "07R", "07S", "07T", "07U", "07V", "07W", "07X", "08C", "08D", "08E", "08F", "08G", "08H", "08J", "08K", "08L", "08M", "08N", "08P", "08Q", "08R", "08S", "08T", "08U", "08V", "08W", "08X", "09C", "09D", "09E", "09F", "09G", "09H", "09J", "09K", "09L", "09M", "09N", "09P", "09Q", "09R", "09S", "09T", "09U", "09V", "09W", "09X", "10C", "10D", "10E", "10F", "10G", "10H", "10J", "10K", "10L", "10M", "10N", "10P", "10Q", "10R", "10S", "10T", "10U", "10V", "10W", "10X", "11C", "11D", "11E", "11F", "11G", "11H", "11J", "11K", "11L", "11M", "11N", "11P", "11Q", "11R", "11S", "11T", "11U", "11V", "11W", "11X", "12C", "12D", "12E", "12F", "12G", "12H", "12J", "12K", "12L", "12M", "12N", "12P", "12Q", "12R", "12S", "12T", "12U", "12V", "12W", "12X", "13C", "13D", "13E", "13F", "13G", "13H", "13J", "13K", "13L", "13M", "13N", "13P", "13Q", "13R", "13S", "13T", "13U", "13V", "13W", "13X", "14C", "14D", "14E", "14F", "14G", "14H", "14J", "14K", "14L", "14M", "14N", "14P", "14Q", "14R", "14S", "14T", "14U", "14V", "14W", "14X", "15C", "15D", "15E", "15F", "15G", "15H", "15J", "15K", "15L", "15M", "15N", "15P", "15Q", "15R", "15S", "15T", "15U", "15V", "15W", "15X", "16C", "16D", "16E", "16F", "16G", "16H", "16J", "16K", "16L", "16M", "16N", "16P", "16Q", "16R", "16S", "16T", "16U", "16V", "16W", "16X", "17C", "17D", "17E", "17F", "17G", "17H", "17J", "17K", "17L", "17M", "17N", "17P", "17Q", "17R", "17S", "17T", "17U", "17V", "17W", "17X", "18C", "18D", "18E", "18F", "18G", "18H", "18J", "18K", "18L", "18M", "18N", "18P", "18Q", "18R", "18S", "18T", "18U", "18V", "18W", "18X", "19C", "19D", "19E", "19F", "19G", "19H", "19J", "19K", "19L", "19M", "19N", "19P", "19Q", "19R", "19S", "19T", "19U", "19V", "19W", "19X", "20C", "20D", "20E", "20F", "20G", "20H", "20J", "20K", "20L", "20M", "20N", "20P", "20Q", "20R", "20S", "20T", "20U", "20V", "20W", "20X", "21C", "21D", "21E", "21F", "21G", "21H", "21J", "21K", "21L", "21M", "21N", "21P", "21Q", "21R", "21S", "21T", "21U", "21V", "21W", "21X", "22C", "22D", "22E", "22F", "22G", "22H", "22J", "22K", "22L", "22M", "22N", "22P", "22Q", "22R", "22S", "22T", "22U", "22V", "22W", "22X", "23C", "23D", "23E", "23F", "23G", "23H", "23J", "23K", "23L", "23M", "23N", "23P", "23Q", "23R", "23S", "23T", "23U", "23V", "23W", "23X", "24C", "24D", "24E", "24F", "24G", "24H", "24J", "24K", "24L", "24M", "24N", "24P", "24Q", "24R", "24S", "24T", "24U", "24V", "24W", "24X", "25C", "25D", "25E", "25F", "25G", "25H", "25J", "25K", "25L", "25M", "25N", "25P", "25Q", "25R", "25S", "25T", "25U", "25V", "25W", "25X", "26C", "26D", "26E", "26F", "26G", "26H", "26J", "26K", "26L", "26M", "26N", "26P", "26Q", "26R", "26S", "26T", "26U", "26V", "26W", "26X", "27C", "27D", "27E", "27F", "27G", "27H", "27J", "27K", "27L", "27M", "27N", "27P", "27Q", "27R", "27S", "27T", "27U", "27V", "27W", "27X", "28C", "28D", "28E", "28F", "28G", "28H", "28J", "28K", "28L", "28M", "28N", "28P", "28Q", "28R", "28S", "28T", "28U", "28V", "28W", "28X", "29C", "29D", "29E", "29F", "29G", "29H", "29J", "29K", "29L", "29M", "29N", "29P", "29Q", "29R", "29S", "29T", "29U", "29V", "29W", "29X", "30C", "30D", "30E", "30F", "30G", "30H", "30J", "30K", "30L", "30M", "30N", "30P", "30Q", "30R", "30S", "30T", "30U", "30V", "30W", "30X", "31C", "31D", "31E", "31F", "31G", "31H", "31J", "31K", "31L", "31M", "31N", "31P", "31Q", "31R", "31S", "31T", "31U", "31V", "31W", "31X", "32C", "32D", "32E", "32F", "32G", "32H", "32J", "32K", "32L", "32M", "32N", "32P", "32Q", "32R", "32S", "32T", "32U", "32V", "32W", "33C", "33D", "33E", "33F", "33G", "33H", "33J", "33K", "33L", "33M", "33N", "33P", "33Q", "33R", "33S", "33T", "33U", "33V", "33W", "33X", "34C", "34D", "34E", "34F", "34G", "34H", "34J", "34K", "34L", "34M", "34N", "34P", "34Q", "34R", "34S", "34T", "34U", "34V", "34W", "35C", "35D", "35E", "35F", "35G", "35H", "35J", "35K", "35L", "35M", "35N", "35P", "35Q", "35R", "35S", "35T", "35U", "35V", "35W", "35X", "36C", "36D", "36E", "36F", "36G", "36H", "36J", "36K", "36L", "36M", "36N", "36P", "36Q", "36R", "36S", "36T", "36U", "36V", "36W", "37C", "37D", "37E", "37F", "37G", "37H", "37J", "37K", "37L", "37M", "37N", "37P", "37Q", "37R", "37S", "37T", "37U", "37V", "37W", "37X", "38C", "38D", "38E", "38F", "38G", "38H", "38J", "38K", "38L", "38M", "38N", "38P", "38Q", "38R", "38S", "38T", "38U", "38V", "38W", "38X", "39C", "39D", "39E", "39F", "39G", "39H", "39J", "39K", "39L", "39M", "39N", "39P", "39Q", "39R", "39S", "39T", "39U", "39V", "39W", "39X", "40C", "40D", "40E", "40F", "40G", "40H", "40J", "40K", "40L", "40M", "40N", "40P", "40Q", "40R", "40S", "40T", "40U", "40V", "40W", "40X", "41C", "41D", "41E", "41F", "41G", "41H", "41J", "41K", "41L", "41M", "41N", "41P", "41Q", "41R", "41S", "41T", "41U", "41V", "41W", "41X", "42C", "42D", "42E", "42F", "42G", "42H", "42J", "42K", "42L", "42M", "42N", "42P", "42Q", "42R", "42S", "42T", "42U", "42V", "42W", "42X", "43C", "43D", "43E", "43F", "43G", "43H", "43J", "43K", "43L", "43M", "43N", "43P", "43Q", "43R", "43S", "43T", "43U", "43V", "43W", "43X", "44C", "44D", "44E", "44F", "44G", "44H", "44J", "44K", "44L", "44M", "44N", "44P", "44Q", "44R", "44S", "44T", "44U", "44V", "44W", "44X", "45C", "45D", "45E", "45F", "45G", "45H", "45J", "45K", "45L", "45M", "45N", "45P", "45Q", "45R", "45S", "45T", "45U", "45V", "45W", "45X", "46C", "46D", "46E", "46F", "46G", "46H", "46J", "46K", "46L", "46M", "46N", "46P", "46Q", "46R", "46S", "46T", "46U", "46V", "46W", "46X", "47C", "47D", "47E", "47F", "47G", "47H", "47J", "47K", "47L", "47M", "47N", "47P", "47Q", "47R", "47S", "47T", "47U", "47V", "47W", "47X", "48C", "48D", "48E", "48F", "48G", "48H", "48J", "48K", "48L", "48M", "48N", "48P", "48Q", "48R", "48S", "48T", "48U", "48V", "48W", "48X", "49C", "49D", "49E", "49F", "49G", "49H", "49J", "49K", "49L", "49M", "49N", "49P", "49Q", "49R", "49S", "49T", "49U", "49V", "49W", "49X", "50C", "50D", "50E", "50F", "50G", "50H", "50J", "50K", "50L", "50M", "50N", "50P", "50Q", "50R", "50S", "50T", "50U", "50V", "50W", "50X", "51C", "51D", "51E", "51F", "51G", "51H", "51J", "51K", "51L", "51M", "51N", "51P", "51Q", "51R", "51S", "51T", "51U", "51V", "51W", "51X", "52C", "52D", "52E", "52F", "52G", "52H", "52J", "52K", "52L", "52M", "52N", "52P", "52Q", "52R", "52S", "52T", "52U", "52V", "52W", "52X", "53C", "53D", "53E", "53F", "53G", "53H", "53J", "53K", "53L", "53M", "53N", "53P", "53Q", "53R", "53S", "53T", "53U", "53V", "53W", "53X", "54C", "54D", "54E", "54F", "54G", "54H", "54J", "54K", "54L", "54M", "54N", "54P", "54Q", "54R", "54S", "54T", "54U", "54V", "54W", "54X", "55C", "55D", "55E", "55F", "55G", "55H", "55J", "55K", "55L", "55M", "55N", "55P", "55Q", "55R", "55S", "55T", "55U", "55V", "55W", "55X", "56C", "56D", "56E", "56F", "56G", "56H", "56J", "56K", "56L", "56M", "56N", "56P", "56Q", "56R", "56S", "56T", "56U", "56V", "56W", "56X", "57C", "57D", "57E", "57F", "57G", "57H", "57J", "57K", "57L", "57M", "57N", "57P", "57Q", "57R", "57S", "57T", "57U", "57V", "57W", "57X", "58C", "58D", "58E", "58F", "58G", "58H", "58J", "58K", "58L", "58M", "58N", "58P", "58Q", "58R", "58S", "58T", "58U", "58V", "58W", "58X", "59C", "59D", "59E", "59F", "59G", "59H", "59J", "59K", "59L", "59M", "59N", "59P", "59Q", "59R", "59S", "59T", "59U", "59V", "59W", "59X", "60C", "60D", "60E", "60F", "60G", "60H", "60J", "60K", "60L", "60M", "60N", "60P", "60Q", "60R", "60S", "60T", "60U", "60V", "60W", "60X"});
    labels = extractLabelTexts(values);
    QVERIFY(compare(labels, refLabels1));

    generateReferenceTestData(6.32, QGeoCoordinate(65.685, -16.7669), QGeoCoordinate(54.3621, 27.1139), values);

    QList<QGeoPath> refPaths1({QGeoPath({QGeoCoordinate(-80, -180), QGeoCoordinate(-80, -120)}), QGeoPath({QGeoCoordinate(-80, -120), QGeoCoordinate(-80, -60)}), QGeoPath({QGeoCoordinate(-80, -60), QGeoCoordinate(-80, 0)}), QGeoPath({QGeoCoordinate(-80, 0), QGeoCoordinate(-80, 60)}), QGeoPath({QGeoCoordinate(-80, 60), QGeoCoordinate(-80, 120)}), QGeoPath({QGeoCoordinate(-80, 120), QGeoCoordinate(-80, 180)}), QGeoPath({QGeoCoordinate(-72, -180), QGeoCoordinate(-72, -120)}), QGeoPath({QGeoCoordinate(-72, -120), QGeoCoordinate(-72, -60)}), QGeoPath({QGeoCoordinate(-72, -60), QGeoCoordinate(-72, 0)}), QGeoPath({QGeoCoordinate(-72, 0), QGeoCoordinate(-72, 60)}), QGeoPath({QGeoCoordinate(-72, 60), QGeoCoordinate(-72, 120)}), QGeoPath({QGeoCoordinate(-72, 120), QGeoCoordinate(-72, 180)}), QGeoPath({QGeoCoordinate(-64, -180), QGeoCoordinate(-64, -120)}), QGeoPath({QGeoCoordinate(-64, -120), QGeoCoordinate(-64, -60)}), QGeoPath({QGeoCoordinate(-64, -60), QGeoCoordinate(-64, 0)}), QGeoPath({QGeoCoordinate(-64, 0), QGeoCoordinate(-64, 60)}), QGeoPath({QGeoCoordinate(-64, 60), QGeoCoordinate(-64, 120)}), QGeoPath({QGeoCoordinate(-64, 120), QGeoCoordinate(-64, 180)}), QGeoPath({QGeoCoordinate(-56, -180), QGeoCoordinate(-56, -120)}), QGeoPath({QGeoCoordinate(-56, -120), QGeoCoordinate(-56, -60)}), QGeoPath({QGeoCoordinate(-56, -60), QGeoCoordinate(-56, 0)}), QGeoPath({QGeoCoordinate(-56, 0), QGeoCoordinate(-56, 60)}), QGeoPath({QGeoCoordinate(-56, 60), QGeoCoordinate(-56, 120)}), QGeoPath({QGeoCoordinate(-56, 120), QGeoCoordinate(-56, 180)}), QGeoPath({QGeoCoordinate(-48, -180), QGeoCoordinate(-48, -120)}), QGeoPath({QGeoCoordinate(-48, -120), QGeoCoordinate(-48, -60)}), QGeoPath({QGeoCoordinate(-48, -60), QGeoCoordinate(-48, 0)}), QGeoPath({QGeoCoordinate(-48, 0), QGeoCoordinate(-48, 60)}), QGeoPath({QGeoCoordinate(-48, 60), QGeoCoordinate(-48, 120)}), QGeoPath({QGeoCoordinate(-48, 120), QGeoCoordinate(-48, 180)}), QGeoPath({QGeoCoordinate(-40, -180), QGeoCoordinate(-40, -120)}), QGeoPath({QGeoCoordinate(-40, -120), QGeoCoordinate(-40, -60)}), QGeoPath({QGeoCoordinate(-40, -60), QGeoCoordinate(-40, 0)}), QGeoPath({QGeoCoordinate(-40, 0), QGeoCoordinate(-40, 60)}), QGeoPath({QGeoCoordinate(-40, 60), QGeoCoordinate(-40, 120)}), QGeoPath({QGeoCoordinate(-40, 120), QGeoCoordinate(-40, 180)}), QGeoPath({QGeoCoordinate(-32, -180), QGeoCoordinate(-32, -120)}), QGeoPath({QGeoCoordinate(-32, -120), QGeoCoordinate(-32, -60)}), QGeoPath({QGeoCoordinate(-32, -60), QGeoCoordinate(-32, 0)}), QGeoPath({QGeoCoordinate(-32, 0), QGeoCoordinate(-32, 60)}), QGeoPath({QGeoCoordinate(-32, 60), QGeoCoordinate(-32, 120)}), QGeoPath({QGeoCoordinate(-32, 120), QGeoCoordinate(-32, 180)}), QGeoPath({QGeoCoordinate(-24, -180), QGeoCoordinate(-24, -120)}), QGeoPath({QGeoCoordinate(-24, -120), QGeoCoordinate(-24, -60)}), QGeoPath({QGeoCoordinate(-24, -60), QGeoCoordinate(-24, 0)}), QGeoPath({QGeoCoordinate(-24, 0), QGeoCoordinate(-24, 60)}), QGeoPath({QGeoCoordinate(-24, 60), QGeoCoordinate(-24, 120)}), QGeoPath({QGeoCoordinate(-24, 120), QGeoCoordinate(-24, 180)}), QGeoPath({QGeoCoordinate(-16, -180), QGeoCoordinate(-16, -120)}), QGeoPath({QGeoCoordinate(-16, -120), QGeoCoordinate(-16, -60)}), QGeoPath({QGeoCoordinate(-16, -60), QGeoCoordinate(-16, 0)}), QGeoPath({QGeoCoordinate(-16, 0), QGeoCoordinate(-16, 60)}), QGeoPath({QGeoCoordinate(-16, 60), QGeoCoordinate(-16, 120)}), QGeoPath({QGeoCoordinate(-16, 120), QGeoCoordinate(-16, 180)}), QGeoPath({QGeoCoordinate(-8, -180), QGeoCoordinate(-8, -120)}), QGeoPath({QGeoCoordinate(-8, -120), QGeoCoordinate(-8, -60)}), QGeoPath({QGeoCoordinate(-8, -60), QGeoCoordinate(-8, 0)}), QGeoPath({QGeoCoordinate(-8, 0), QGeoCoordinate(-8, 60)}), QGeoPath({QGeoCoordinate(-8, 60), QGeoCoordinate(-8, 120)}), QGeoPath({QGeoCoordinate(-8, 120), QGeoCoordinate(-8, 180)}), QGeoPath({QGeoCoordinate(0, -180), QGeoCoordinate(0, -120)}), QGeoPath({QGeoCoordinate(0, -120), QGeoCoordinate(0, -60)}), QGeoPath({QGeoCoordinate(0, -60), QGeoCoordinate(0, 0)}), QGeoPath({QGeoCoordinate(0, 0), QGeoCoordinate(0, 60)}), QGeoPath({QGeoCoordinate(0, 60), QGeoCoordinate(0, 120)}), QGeoPath({QGeoCoordinate(0, 120), QGeoCoordinate(0, 180)}), QGeoPath({QGeoCoordinate(8, -180), QGeoCoordinate(8, -120)}), QGeoPath({QGeoCoordinate(8, -120), QGeoCoordinate(8, -60)}), QGeoPath({QGeoCoordinate(8, -60), QGeoCoordinate(8, 0)}), QGeoPath({QGeoCoordinate(8, 0), QGeoCoordinate(8, 60)}), QGeoPath({QGeoCoordinate(8, 60), QGeoCoordinate(8, 120)}), QGeoPath({QGeoCoordinate(8, 120), QGeoCoordinate(8, 180)}), QGeoPath({QGeoCoordinate(16, -180), QGeoCoordinate(16, -120)}), QGeoPath({QGeoCoordinate(16, -120), QGeoCoordinate(16, -60)}), QGeoPath({QGeoCoordinate(16, -60), QGeoCoordinate(16, 0)}), QGeoPath({QGeoCoordinate(16, 0), QGeoCoordinate(16, 60)}), QGeoPath({QGeoCoordinate(16, 60), QGeoCoordinate(16, 120)}), QGeoPath({QGeoCoordinate(16, 120), QGeoCoordinate(16, 180)}), QGeoPath({QGeoCoordinate(24, -180), QGeoCoordinate(24, -120)}), QGeoPath({QGeoCoordinate(24, -120), QGeoCoordinate(24, -60)}), QGeoPath({QGeoCoordinate(24, -60), QGeoCoordinate(24, 0)}), QGeoPath({QGeoCoordinate(24, 0), QGeoCoordinate(24, 60)}), QGeoPath({QGeoCoordinate(24, 60), QGeoCoordinate(24, 120)}), QGeoPath({QGeoCoordinate(24, 120), QGeoCoordinate(24, 180)}), QGeoPath({QGeoCoordinate(32, -180), QGeoCoordinate(32, -120)}), QGeoPath({QGeoCoordinate(32, -120), QGeoCoordinate(32, -60)}), QGeoPath({QGeoCoordinate(32, -60), QGeoCoordinate(32, 0)}), QGeoPath({QGeoCoordinate(32, 0), QGeoCoordinate(32, 60)}), QGeoPath({QGeoCoordinate(32, 60), QGeoCoordinate(32, 120)}), QGeoPath({QGeoCoordinate(32, 120), QGeoCoordinate(32, 180)}), QGeoPath({QGeoCoordinate(40, -180), QGeoCoordinate(40, -120)}), QGeoPath({QGeoCoordinate(40, -120), QGeoCoordinate(40, -60)}), QGeoPath({QGeoCoordinate(40, -60), QGeoCoordinate(40, 0)}), QGeoPath({QGeoCoordinate(40, 0), QGeoCoordinate(40, 60)}), QGeoPath({QGeoCoordinate(40, 60), QGeoCoordinate(40, 120)}), QGeoPath({QGeoCoordinate(40, 120), QGeoCoordinate(40, 180)}), QGeoPath({QGeoCoordinate(48, -180), QGeoCoordinate(48, -120)}), QGeoPath({QGeoCoordinate(48, -120), QGeoCoordinate(48, -60)}), QGeoPath({QGeoCoordinate(48, -60), QGeoCoordinate(48, 0)}), QGeoPath({QGeoCoordinate(48, 0), QGeoCoordinate(48, 60)}), QGeoPath({QGeoCoordinate(48, 60), QGeoCoordinate(48, 120)}), QGeoPath({QGeoCoordinate(48, 120), QGeoCoordinate(48, 180)}), QGeoPath({QGeoCoordinate(56, -180), QGeoCoordinate(56, -120)}), QGeoPath({QGeoCoordinate(56, -120), QGeoCoordinate(56, -60)}), QGeoPath({QGeoCoordinate(56, -60), QGeoCoordinate(56, 0)}), QGeoPath({QGeoCoordinate(56, 0), QGeoCoordinate(56, 60)}), QGeoPath({QGeoCoordinate(56, 60), QGeoCoordinate(56, 120)}), QGeoPath({QGeoCoordinate(56, 120), QGeoCoordinate(56, 180)}), QGeoPath({QGeoCoordinate(64, -180), QGeoCoordinate(64, -120)}), QGeoPath({QGeoCoordinate(64, -120), QGeoCoordinate(64, -60)}), QGeoPath({QGeoCoordinate(64, -60), QGeoCoordinate(64, 0)}), QGeoPath({QGeoCoordinate(64, 0), QGeoCoordinate(64, 60)}), QGeoPath({QGeoCoordinate(64, 60), QGeoCoordinate(64, 120)}), QGeoPath({QGeoCoordinate(64, 120), QGeoCoordinate(64, 180)}), QGeoPath({QGeoCoordinate(72, -180), QGeoCoordinate(72, -120)}), QGeoPath({QGeoCoordinate(72, -120), QGeoCoordinate(72, -60)}), QGeoPath({QGeoCoordinate(72, -60), QGeoCoordinate(72, 0)}), QGeoPath({QGeoCoordinate(72, 0), QGeoCoordinate(72, 60)}), QGeoPath({QGeoCoordinate(72, 60), QGeoCoordinate(72, 120)}), QGeoPath({QGeoCoordinate(72, 120), QGeoCoordinate(72, 180)}), QGeoPath({QGeoCoordinate(84, -180), QGeoCoordinate(84, -120)}), QGeoPath({QGeoCoordinate(84, -120), QGeoCoordinate(84, -60)}), QGeoPath({QGeoCoordinate(84, -60), QGeoCoordinate(84, 0)}), QGeoPath({QGeoCoordinate(84, 0), QGeoCoordinate(84, 60)}), QGeoPath({QGeoCoordinate(84, 60), QGeoCoordinate(84, 120)}), QGeoPath({QGeoCoordinate(84, 120), QGeoCoordinate(84, 180)}), QGeoPath({QGeoCoordinate(-80, -180), QGeoCoordinate(84, -180)}), QGeoPath({QGeoCoordinate(-80, -174), QGeoCoordinate(84, -174)}), QGeoPath({QGeoCoordinate(-80, -168), QGeoCoordinate(84, -168)}), QGeoPath({QGeoCoordinate(-80, -162), QGeoCoordinate(84, -162)}), QGeoPath({QGeoCoordinate(-80, -156), QGeoCoordinate(84, -156)}), QGeoPath({QGeoCoordinate(-80, -150), QGeoCoordinate(84, -150)}), QGeoPath({QGeoCoordinate(-80, -144), QGeoCoordinate(84, -144)}), QGeoPath({QGeoCoordinate(-80, -138), QGeoCoordinate(84, -138)}), QGeoPath({QGeoCoordinate(-80, -132), QGeoCoordinate(84, -132)}), QGeoPath({QGeoCoordinate(-80, -126), QGeoCoordinate(84, -126)}), QGeoPath({QGeoCoordinate(-80, -120), QGeoCoordinate(84, -120)}), QGeoPath({QGeoCoordinate(-80, -114), QGeoCoordinate(84, -114)}), QGeoPath({QGeoCoordinate(-80, -108), QGeoCoordinate(84, -108)}), QGeoPath({QGeoCoordinate(-80, -102), QGeoCoordinate(84, -102)}), QGeoPath({QGeoCoordinate(-80, -96), QGeoCoordinate(84, -96)}), QGeoPath({QGeoCoordinate(-80, -90), QGeoCoordinate(84, -90)}), QGeoPath({QGeoCoordinate(-80, -84), QGeoCoordinate(84, -84)}), QGeoPath({QGeoCoordinate(-80, -78), QGeoCoordinate(84, -78)}), QGeoPath({QGeoCoordinate(-80, -72), QGeoCoordinate(84, -72)}), QGeoPath({QGeoCoordinate(-80, -66), QGeoCoordinate(84, -66)}), QGeoPath({QGeoCoordinate(-80, -60), QGeoCoordinate(84, -60)}), QGeoPath({QGeoCoordinate(-80, -54), QGeoCoordinate(84, -54)}), QGeoPath({QGeoCoordinate(-80, -48), QGeoCoordinate(84, -48)}), QGeoPath({QGeoCoordinate(-80, -42), QGeoCoordinate(84, -42)}), QGeoPath({QGeoCoordinate(-80, -36), QGeoCoordinate(84, -36)}), QGeoPath({QGeoCoordinate(-80, -30), QGeoCoordinate(84, -30)}), QGeoPath({QGeoCoordinate(-80, -24), QGeoCoordinate(84, -24)}), QGeoPath({QGeoCoordinate(-80, -18), QGeoCoordinate(84, -18)}), QGeoPath({QGeoCoordinate(-80, -12), QGeoCoordinate(84, -12)}), QGeoPath({QGeoCoordinate(-80, -6), QGeoCoordinate(84, -6)}), QGeoPath({QGeoCoordinate(-80, 0), QGeoCoordinate(84, 0)}), QGeoPath({QGeoCoordinate(56, 3), QGeoCoordinate(64, 3)}), QGeoPath({QGeoCoordinate(64, 6), QGeoCoordinate(72, 6)}), QGeoPath({QGeoCoordinate(-80, 6), QGeoCoordinate(56, 6)}), QGeoPath({QGeoCoordinate(-80, 12), QGeoCoordinate(72, 12)}), QGeoPath({QGeoCoordinate(-80, 18), QGeoCoordinate(72, 18)}), QGeoPath({QGeoCoordinate(-80, 24), QGeoCoordinate(72, 24)}), QGeoPath({QGeoCoordinate(-80, 30), QGeoCoordinate(72, 30)}), QGeoPath({QGeoCoordinate(-80, 36), QGeoCoordinate(72, 36)}), QGeoPath({QGeoCoordinate(-80, 42), QGeoCoordinate(84, 42)}), QGeoPath({QGeoCoordinate(-80, 48), QGeoCoordinate(84, 48)}), QGeoPath({QGeoCoordinate(-80, 54), QGeoCoordinate(84, 54)}), QGeoPath({QGeoCoordinate(-80, 60), QGeoCoordinate(84, 60)}), QGeoPath({QGeoCoordinate(-80, 66), QGeoCoordinate(84, 66)}), QGeoPath({QGeoCoordinate(-80, 72), QGeoCoordinate(84, 72)}), QGeoPath({QGeoCoordinate(-80, 78), QGeoCoordinate(84, 78)}), QGeoPath({QGeoCoordinate(-80, 84), QGeoCoordinate(84, 84)}), QGeoPath({QGeoCoordinate(-80, 90), QGeoCoordinate(84, 90)}), QGeoPath({QGeoCoordinate(-80, 96), QGeoCoordinate(84, 96)}), QGeoPath({QGeoCoordinate(-80, 102), QGeoCoordinate(84, 102)}), QGeoPath({QGeoCoordinate(-80, 108), QGeoCoordinate(84, 108)}), QGeoPath({QGeoCoordinate(-80, 114), QGeoCoordinate(84, 114)}), QGeoPath({QGeoCoordinate(-80, 120), QGeoCoordinate(84, 120)}), QGeoPath({QGeoCoordinate(-80, 126), QGeoCoordinate(84, 126)}), QGeoPath({QGeoCoordinate(-80, 132), QGeoCoordinate(84, 132)}), QGeoPath({QGeoCoordinate(-80, 138), QGeoCoordinate(84, 138)}), QGeoPath({QGeoCoordinate(-80, 144), QGeoCoordinate(84, 144)}), QGeoPath({QGeoCoordinate(-80, 150), QGeoCoordinate(84, 150)}), QGeoPath({QGeoCoordinate(-80, 156), QGeoCoordinate(84, 156)}), QGeoPath({QGeoCoordinate(-80, 162), QGeoCoordinate(84, 162)}), QGeoPath({QGeoCoordinate(-80, 168), QGeoCoordinate(84, 168)}), QGeoPath({QGeoCoordinate(-80, 174), QGeoCoordinate(84, 174)}), QGeoPath({QGeoCoordinate(-80, 180), QGeoCoordinate(84, 180)}), QGeoPath({QGeoCoordinate(72, 9), QGeoCoordinate(84, 9)}), QGeoPath({QGeoCoordinate(72, 21), QGeoCoordinate(84, 21)}), QGeoPath({QGeoCoordinate(72, 33), QGeoCoordinate(84, 33)})});
    paths = extractPaths(values);
    QVERIFY(compare(paths, refPaths1));

    // This query is subset of previous one and should not return any elements
    mapGrid.geometryChanged(6.32, QGeoCoordinate(65.685, -16.7669), QGeoCoordinate(54.3621, 27.1139));
    labels = extractLabelTexts(values);
    QVERIFY(labels.count() == 0);

    disconnect(&mapGrid, &MapGridMGRS::updateValues, this, &MapGridTests::updateValues);
}

QTEST_MAIN(MapGridTests)
#include "MapGridTests.moc"
