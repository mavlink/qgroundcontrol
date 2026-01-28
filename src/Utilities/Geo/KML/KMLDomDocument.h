#pragma once

#include <QtCore/QLoggingCategory>
#include <QtGui/QColor>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

class QColor;
class QGeoCoordinate;

Q_DECLARE_LOGGING_CATEGORY(KMLDomDocumentLog)

/// Used to convert a Plan to a KML document
class KMLDomDocument : public QDomDocument
{
public:
    KMLDomDocument(const QString &name);

    void appendChildToRoot(const QDomNode &child);
    QDomElement addFolder(const QString &name);
    QDomElement addPlacemark(const QString &name, bool visible);
    void addTextElement(QDomElement &parentElement, const QString &name, const QString &value);
    void addLookAt(QDomElement &parentElement, const QGeoCoordinate &coord);

    // Content helpers
    void addDescription(QDomElement &parent, const QString &content);

    // Style helpers
    QDomElement addStyle(const QString &id);
    void addLineStyle(QDomElement &styleElement, const QColor &color, int width = 1, double opacity = 1.0);
    void addPolyStyle(QDomElement &styleElement, const QColor &color, double opacity = 1.0);

    // Geometry element helpers
    QDomElement addPoint(QDomElement &parent, const QGeoCoordinate &coord,
                         const QString &altitudeMode = QLatin1String("absolute"), bool extrude = true);
    QDomElement addLineString(QDomElement &parent, const QList<QGeoCoordinate> &coords,
                              const QString &altitudeMode = QLatin1String("absolute"),
                              bool extrude = true, bool tessellate = true);
    QDomElement addPolygon(QDomElement &parent, const QList<QGeoCoordinate> &coords,
                           const QString &altitudeMode = QLatin1String("clampToGround"));

    // Formatting utilities (static - can be used without instantiating)
    static QString kmlColorString(const QColor &color, double opacity = 1);
    static QString kmlCoordString(const QGeoCoordinate &coord);

    // KML constants
    static constexpr const char *kmlNamespace = "http://www.opengis.net/kml/2.2";
    static constexpr const char *kmlSchemaLocation = "https://schemas.opengis.net/kml/2.2.0/ogckml22.xsd";
    static constexpr const char *xsiNamespace = "http://www.w3.org/2001/XMLSchema-instance";
    static constexpr const char *balloonStyleName = "BalloonStyle";

protected:
    QDomElement _rootDocumentElement;

private:
    void _addStandardStyles();
};
