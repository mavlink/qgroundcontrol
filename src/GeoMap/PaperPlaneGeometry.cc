/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PaperPlaneGeometry.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QtGlobal>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>

#include <cmath>

namespace {

// Silhouette from vehicleArrowOpaque.svg (72x72 viewBox scaled to length 100):
// nose top-center, wingtips at the rear corners, tail notch on the spine.
// Wingtips raised and keel dropped for the folded-paper dihedral.
constexpr QVector3D kNose(0.0f, 50.0f, 0.0f);
constexpr QVector3D kTailNotch(0.0f, -25.0f, 0.0f);
constexpr QVector3D kLeftTip(-36.0f, -50.0f, 14.0f);
constexpr QVector3D kRightTip(36.0f, -50.0f, 14.0f);
constexpr QVector3D kKeelBottom(0.0f, -48.0f, -16.0f);

// Vertex colors multiply in linear space (linear tonemap re-encodes to sRGB)
QVector4D linearColor(float r, float g, float b)
{
    const auto toLinear = [](float c) {
        return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
    };
    return QVector4D(toLinear(r), toLinear(g), toLinear(b), 1.0f);
}

}  // namespace

PaperPlaneGeometry::PaperPlaneGeometry(QQuick3DObject* parent) : QQuick3DGeometry(parent)
{
    // Wing reds from vehicleArrowOpaque.svg; paper-white keel fin as the
    // fuselage, visible in tilted 3D views
    const QVector4D leftWingColor = linearColor(0xc7 / 255.0f, 0x2b / 255.0f, 0x27 / 255.0f);
    const QVector4D rightWingColor = linearColor(0xee / 255.0f, 0x34 / 255.0f, 0x24 / 255.0f);
    const QVector4D fuselageColor = linearColor(0xf5 / 255.0f, 0xf5 / 255.0f, 0xf5 / 255.0f);

    struct Triangle
    {
        QVector3D a, b, c;
        QVector4D color;
    };

    // CCW seen from the normal side: wings face up(ish), keel faces +x
    const Triangle triangles[kTriangleCount] = {
        {kNose, kLeftTip, kTailNotch, leftWingColor},
        {kNose, kTailNotch, kRightTip, rightWingColor},
        {kNose, kTailNotch, kKeelBottom, fuselageColor},
    };

    QList<float> floats;
    floats.reserve(kTriangleCount * 3 * kFloatsPerVertex);
    QVector3D minBounds = kNose;
    QVector3D maxBounds = kNose;
    for (const Triangle& triangle : triangles) {
        const QVector3D normal = QVector3D::crossProduct(triangle.b - triangle.a, triangle.c - triangle.a).normalized();
        for (const QVector3D& position : {triangle.a, triangle.b, triangle.c}) {
            floats << position.x() << position.y() << position.z();
            floats << normal.x() << normal.y() << normal.z();
            floats << triangle.color.x() << triangle.color.y() << triangle.color.z() << triangle.color.w();
            minBounds = QVector3D(qMin(minBounds.x(), position.x()), qMin(minBounds.y(), position.y()),
                                  qMin(minBounds.z(), position.z()));
            maxBounds = QVector3D(qMax(maxBounds.x(), position.x()), qMax(maxBounds.y(), position.y()),
                                  qMax(maxBounds.z(), position.z()));
        }
    }

    setStride(kFloatsPerVertex * sizeof(float));
    setVertexData(QByteArray(reinterpret_cast<const char*>(floats.constData()),
                             static_cast<qsizetype>(floats.size() * sizeof(float))));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 3 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::ColorSemantic, 6 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    setBounds(minBounds, maxBounds);
}
