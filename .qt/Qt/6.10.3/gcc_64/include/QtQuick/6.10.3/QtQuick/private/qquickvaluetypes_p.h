// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKVALUETYPES_P_H
#define QQUICKVALUETYPES_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qqml.h>
#include <private/qtquickglobal_p.h>
#include <private/qqmlvaluetype_p.h>

#include <QtGui/QColor>
#include <QtGui/QColorSpace>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtGui/QVector4D>
#include <QtGui/QQuaternion>
#include <QtGui/QMatrix4x4>
#include <QtGui/QFont>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickColorValueType : public QColor
{
    Q_PROPERTY(qreal r READ r WRITE setR FINAL)
    Q_PROPERTY(qreal g READ g WRITE setG FINAL)
    Q_PROPERTY(qreal b READ b WRITE setB FINAL)
    Q_PROPERTY(qreal a READ a WRITE setA FINAL)
    Q_PROPERTY(qreal hsvHue READ hsvHue WRITE setHsvHue FINAL)
    Q_PROPERTY(qreal hsvSaturation READ hsvSaturation WRITE setHsvSaturation FINAL)
    Q_PROPERTY(qreal hsvValue READ hsvValue WRITE setHsvValue FINAL)
    Q_PROPERTY(qreal hslHue READ hslHue WRITE setHslHue FINAL)
    Q_PROPERTY(qreal hslSaturation READ hslSaturation WRITE setHslSaturation FINAL)
    Q_PROPERTY(qreal hslLightness READ hslLightness WRITE setHslLightness FINAL)
    Q_PROPERTY(bool valid READ isValid FINAL)
    Q_GADGET
    QML_ADDED_IN_VERSION(2, 0)
    QML_FOREIGN(QColor)
    QML_VALUE_TYPE(color)
    QML_EXTENDED(QQuickColorValueType)
    QML_STRUCTURED_VALUE

public:
    static QVariant create(const QJSValue &params);

    Q_INVOKABLE QQuickColorValueType() = default;
    Q_INVOKABLE QQuickColorValueType(const QColor &color) : QColor(color) {}
    Q_INVOKABLE QQuickColorValueType(const QString &string);
    Q_INVOKABLE QString toString() const;

    Q_INVOKABLE QColor alpha(qreal value) const;
    Q_INVOKABLE QColor lighter(qreal factor = 1.5) const;
    Q_INVOKABLE QColor darker(qreal factor = 2.0) const;
    Q_INVOKABLE QColor tint(const QColor &tintColor) const;

    qreal r() const;
    qreal g() const;
    qreal b() const;
    qreal a() const;
    qreal hsvHue() const;
    qreal hsvSaturation() const;
    qreal hsvValue() const;
    qreal hslHue() const;
    qreal hslSaturation() const;
    qreal hslLightness() const;
    bool isValid() const;
    void setR(qreal);
    void setG(qreal);
    void setB(qreal);
    void setA(qreal);
    void setHsvHue(qreal);
    void setHsvSaturation(qreal);
    void setHsvValue(qreal);
    void setHslHue(qreal);
    void setHslSaturation(qreal);
    void setHslLightness(qreal);
};

class Q_QUICK_EXPORT QQuickVector2DValueType : public QVector2D
{
    Q_PROPERTY(qreal x READ x WRITE setX FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY FINAL)
    Q_GADGET
    QML_ADDED_IN_VERSION(2, 0)
    QML_FOREIGN(QVector2D)
    QML_VALUE_TYPE(vector2d)
    QML_EXTENDED(QQuickVector2DValueType)
    QML_STRUCTURED_VALUE

public:
    static QVariant create(const QJSValue &params);

    Q_INVOKABLE QQuickVector2DValueType() = default;
    Q_INVOKABLE QQuickVector2DValueType(const QVector2D &vector2D) : QVector2D(vector2D) {}
    Q_INVOKABLE QString toString() const;

    qreal x() const;
    qreal y() const;
    void setX(qreal);
    void setY(qreal);

    Q_INVOKABLE qreal dotProduct(const QVector2D &vec) const;
    Q_INVOKABLE QVector2D times(const QVector2D &vec) const;
    Q_INVOKABLE QVector2D times(qreal scalar) const;
    Q_INVOKABLE QVector2D plus(const QVector2D &vec) const;
    Q_INVOKABLE QVector2D minus(const QVector2D &vec) const;
    Q_INVOKABLE QVector2D normalized() const;
    Q_INVOKABLE qreal length() const;
    Q_INVOKABLE QVector3D toVector3d() const;
    Q_INVOKABLE QVector4D toVector4d() const;
    Q_INVOKABLE bool fuzzyEquals(const QVector2D &vec, qreal epsilon) const;
    Q_INVOKABLE bool fuzzyEquals(const QVector2D &vec) const;
};

class Q_QUICK_EXPORT QQuickVector3DValueType : public QVector3D
{
    Q_PROPERTY(qreal x READ x WRITE setX FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY FINAL)
    Q_PROPERTY(qreal z READ z WRITE setZ FINAL)
    Q_GADGET
    QML_ADDED_IN_VERSION(2, 0)
    QML_FOREIGN(QVector3D)
    QML_VALUE_TYPE(vector3d)
    QML_EXTENDED(QQuickVector3DValueType)
    QML_STRUCTURED_VALUE

public:
    static QVariant create(const QJSValue &params);

    Q_INVOKABLE QQuickVector3DValueType() = default;
    Q_INVOKABLE QQuickVector3DValueType(const QVector3D &vector3D) : QVector3D(vector3D) {}
    Q_INVOKABLE QString toString() const;

    qreal x() const;
    qreal y() const;
    qreal z() const;
    void setX(qreal);
    void setY(qreal);
    void setZ(qreal);

    Q_INVOKABLE QVector3D crossProduct(const QVector3D &vec) const;
    Q_INVOKABLE qreal dotProduct(const QVector3D &vec) const;
    Q_INVOKABLE QVector3D times(const QMatrix4x4 &m) const;
    Q_INVOKABLE QVector3D times(const QVector3D &vec) const;
    Q_INVOKABLE QVector3D times(qreal scalar) const;
    Q_INVOKABLE QVector3D plus(const QVector3D &vec) const;
    Q_INVOKABLE QVector3D minus(const QVector3D &vec) const;
    Q_INVOKABLE QVector3D normalized() const;
    Q_INVOKABLE qreal length() const;
    Q_INVOKABLE QVector2D toVector2d() const;
    Q_INVOKABLE QVector4D toVector4d() const;
    Q_INVOKABLE bool fuzzyEquals(const QVector3D &vec, qreal epsilon) const;
    Q_INVOKABLE bool fuzzyEquals(const QVector3D &vec) const;
};

class Q_QUICK_EXPORT QQuickVector4DValueType : public QVector4D
{
    Q_PROPERTY(qreal x READ x WRITE setX FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY FINAL)
    Q_PROPERTY(qreal z READ z WRITE setZ FINAL)
    Q_PROPERTY(qreal w READ w WRITE setW FINAL)
    Q_GADGET
    QML_ADDED_IN_VERSION(2, 0)
    QML_FOREIGN(QVector4D)
    QML_VALUE_TYPE(vector4d)
    QML_EXTENDED(QQuickVector4DValueType)
    QML_STRUCTURED_VALUE

public:
    static QVariant create(const QJSValue &params);

    Q_INVOKABLE QQuickVector4DValueType() = default;
    Q_INVOKABLE QQuickVector4DValueType(const QVector4D &vector4d) : QVector4D(vector4d) {}
    Q_INVOKABLE QString toString() const;

    qreal x() const;
    qreal y() const;
    qreal z() const;
    qreal w() const;
    void setX(qreal);
    void setY(qreal);
    void setZ(qreal);
    void setW(qreal);

    Q_INVOKABLE qreal dotProduct(const QVector4D &vec) const;
    Q_INVOKABLE QVector4D times(const QVector4D &vec) const;
    Q_INVOKABLE QVector4D times(const QMatrix4x4 &m) const;
    Q_INVOKABLE QVector4D times(qreal scalar) const;
    Q_INVOKABLE QVector4D plus(const QVector4D &vec) const;
    Q_INVOKABLE QVector4D minus(const QVector4D &vec) const;
    Q_INVOKABLE QVector4D normalized() const;
    Q_INVOKABLE qreal length() const;
    Q_INVOKABLE QVector2D toVector2d() const;
    Q_INVOKABLE QVector3D toVector3d() const;
    Q_INVOKABLE bool fuzzyEquals(const QVector4D &vec, qreal epsilon) const;
    Q_INVOKABLE bool fuzzyEquals(const QVector4D &vec) const;
};

class Q_QUICK_EXPORT QQuickQuaternionValueType : public QQuaternion
{
    Q_PROPERTY(qreal scalar READ scalar WRITE setScalar FINAL)
    Q_PROPERTY(qreal x READ x WRITE setX FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY FINAL)
    Q_PROPERTY(qreal z READ z WRITE setZ FINAL)
    Q_GADGET
    QML_ADDED_IN_VERSION(2, 0)
    QML_FOREIGN(QQuaternion)
    QML_VALUE_TYPE(quaternion)
    QML_EXTENDED(QQuickQuaternionValueType)
    QML_STRUCTURED_VALUE

public:
    static QVariant create(const QJSValue &params);

    Q_INVOKABLE QQuickQuaternionValueType() = default;
    Q_INVOKABLE QQuickQuaternionValueType(const QQuaternion &quat) : QQuaternion(quat) {}
    Q_INVOKABLE QString toString() const;

    qreal scalar() const;
    qreal x() const;
    qreal y() const;
    qreal z() const;
    void setScalar(qreal);
    void setX(qreal);
    void setY(qreal);
    void setZ(qreal);

    Q_INVOKABLE qreal dotProduct(const QQuaternion &q) const;
    Q_INVOKABLE QQuaternion times(const QQuaternion &q) const;
    Q_INVOKABLE QVector3D times(const QVector3D &vec) const;
    Q_INVOKABLE QQuaternion times(qreal factor) const;
    Q_INVOKABLE QQuaternion plus(const QQuaternion &q) const;
    Q_INVOKABLE QQuaternion minus(const QQuaternion &q) const;

    Q_INVOKABLE QQuaternion normalized() const;
    Q_INVOKABLE QQuaternion inverted() const;
    Q_INVOKABLE QQuaternion conjugated() const;
    Q_INVOKABLE qreal length() const;

    Q_INVOKABLE QVector3D toEulerAngles() const;
    Q_INVOKABLE QVector4D toVector4d() const;

    Q_INVOKABLE bool fuzzyEquals(const QQuaternion &q, qreal epsilon) const;
    Q_INVOKABLE bool fuzzyEquals(const QQuaternion &q) const;
};

class Q_QUICK_EXPORT QQuickMatrix4x4ValueType : public QMatrix4x4
{
    Q_PROPERTY(qreal m11 READ m11 WRITE setM11 FINAL)
    Q_PROPERTY(qreal m12 READ m12 WRITE setM12 FINAL)
    Q_PROPERTY(qreal m13 READ m13 WRITE setM13 FINAL)
    Q_PROPERTY(qreal m14 READ m14 WRITE setM14 FINAL)
    Q_PROPERTY(qreal m21 READ m21 WRITE setM21 FINAL)
    Q_PROPERTY(qreal m22 READ m22 WRITE setM22 FINAL)
    Q_PROPERTY(qreal m23 READ m23 WRITE setM23 FINAL)
    Q_PROPERTY(qreal m24 READ m24 WRITE setM24 FINAL)
    Q_PROPERTY(qreal m31 READ m31 WRITE setM31 FINAL)
    Q_PROPERTY(qreal m32 READ m32 WRITE setM32 FINAL)
    Q_PROPERTY(qreal m33 READ m33 WRITE setM33 FINAL)
    Q_PROPERTY(qreal m34 READ m34 WRITE setM34 FINAL)
    Q_PROPERTY(qreal m41 READ m41 WRITE setM41 FINAL)
    Q_PROPERTY(qreal m42 READ m42 WRITE setM42 FINAL)
    Q_PROPERTY(qreal m43 READ m43 WRITE setM43 FINAL)
    Q_PROPERTY(qreal m44 READ m44 WRITE setM44 FINAL)
    Q_GADGET
    QML_ADDED_IN_VERSION(2, 0)
    QML_FOREIGN(QMatrix4x4)
    QML_VALUE_TYPE(matrix4x4)
    QML_EXTENDED(QQuickMatrix4x4ValueType)
    QML_STRUCTURED_VALUE

public:
    static QVariant create(const QJSValue &params);

    Q_INVOKABLE QQuickMatrix4x4ValueType() = default;

    qreal m11() const { return (*this)(0, 0); }
    qreal m12() const { return (*this)(0, 1); }
    qreal m13() const { return (*this)(0, 2); }
    qreal m14() const { return (*this)(0, 3); }
    qreal m21() const { return (*this)(1, 0); }
    qreal m22() const { return (*this)(1, 1); }
    qreal m23() const { return (*this)(1, 2); }
    qreal m24() const { return (*this)(1, 3); }
    qreal m31() const { return (*this)(2, 0); }
    qreal m32() const { return (*this)(2, 1); }
    qreal m33() const { return (*this)(2, 2); }
    qreal m34() const { return (*this)(2, 3); }
    qreal m41() const { return (*this)(3, 0); }
    qreal m42() const { return (*this)(3, 1); }
    qreal m43() const { return (*this)(3, 2); }
    qreal m44() const { return (*this)(3, 3); }

    void setM11(qreal value) { (*this)(0, 0) = value; }
    void setM12(qreal value) { (*this)(0, 1) = value; }
    void setM13(qreal value) { (*this)(0, 2) = value; }
    void setM14(qreal value) { (*this)(0, 3) = value; }
    void setM21(qreal value) { (*this)(1, 0) = value; }
    void setM22(qreal value) { (*this)(1, 1) = value; }
    void setM23(qreal value) { (*this)(1, 2) = value; }
    void setM24(qreal value) { (*this)(1, 3) = value; }
    void setM31(qreal value) { (*this)(2, 0) = value; }
    void setM32(qreal value) { (*this)(2, 1) = value; }
    void setM33(qreal value) { (*this)(2, 2) = value; }
    void setM34(qreal value) { (*this)(2, 3) = value; }
    void setM41(qreal value) { (*this)(3, 0) = value; }
    void setM42(qreal value) { (*this)(3, 1) = value; }
    void setM43(qreal value) { (*this)(3, 2) = value; }
    void setM44(qreal value) { (*this)(3, 3) = value; }

    Q_INVOKABLE void translate(const QVector3D &t) { QMatrix4x4::translate(t); }
    Q_INVOKABLE void rotate(float angle, const QVector3D &axis) { QMatrix4x4::rotate(angle, axis); }
    Q_INVOKABLE void rotate(const QQuaternion &q) { QMatrix4x4::rotate(q); }
    Q_INVOKABLE void scale(float s) { QMatrix4x4::scale(s); }
    Q_INVOKABLE void scale(float sx, float sy, float sz) { QMatrix4x4::scale(sx, sy, sz); }
    Q_INVOKABLE void scale(const QVector3D &s) { QMatrix4x4::scale(s); }
    Q_INVOKABLE void lookAt(const QVector3D &eye, const QVector3D &center, const QVector3D &up)
    {
        QMatrix4x4::lookAt(eye, center, up);
    }

    Q_INVOKABLE QMatrix4x4 times(const QMatrix4x4 &m) const;
    Q_INVOKABLE QVector4D times(const QVector4D &vec) const;
    Q_INVOKABLE QVector3D times(const QVector3D &vec) const;
    Q_INVOKABLE QMatrix4x4 times(qreal factor) const;
    Q_INVOKABLE QMatrix4x4 plus(const QMatrix4x4 &m) const;
    Q_INVOKABLE QMatrix4x4 minus(const QMatrix4x4 &m) const;

    Q_INVOKABLE QVector4D row(int n) const;
    Q_INVOKABLE QVector4D column(int m) const;

    Q_INVOKABLE qreal determinant() const;
    Q_INVOKABLE QMatrix4x4 inverted() const;
    Q_INVOKABLE QMatrix4x4 transposed() const;

    Q_INVOKABLE QPointF map(const QPointF p) const;
    Q_INVOKABLE QRectF mapRect(const QRectF r) const;

    Q_INVOKABLE bool fuzzyEquals(const QMatrix4x4 &m, qreal epsilon) const;
    Q_INVOKABLE bool fuzzyEquals(const QMatrix4x4 &m) const;
};

class Q_QUICK_EXPORT QQuickPlanarTransform : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(PlanarTransform)
    QML_ADDED_IN_VERSION(6, 8)

public:
    explicit QQuickPlanarTransform(QObject *parent = nullptr);

    Q_INVOKABLE static QMatrix4x4 identity();
    Q_INVOKABLE static QMatrix4x4 fromAffineMatrix(float scaleX, float shearY,
                                                   float shearX, float scaleY,
                                                   float translateX, float translateY);
    Q_INVOKABLE static QMatrix4x4 fromTranslate(float translateX, float translateY);
    Q_INVOKABLE static QMatrix4x4 fromScale(float scaleX, float scaleY,
                                            float originX = 0, float originY = 0);
    Q_INVOKABLE static QMatrix4x4 fromRotate(float angle,float originX = 0, float originY = 0);
    Q_INVOKABLE static QMatrix4x4 fromShear(float shearX, float shearY,
                                            float originX = 0, float originY = 0);
};

namespace QQuickFontEnums
{
Q_NAMESPACE_EXPORT(Q_QUICK_EXPORT)

QML_NAMED_ELEMENT(Font)
QML_ADDED_IN_VERSION(2, 0)

enum FontWeight { Thin = QFont::Thin,
                  ExtraLight = QFont::ExtraLight,
                  Light = QFont::Light,
                  Normal = QFont::Normal,
                  Medium = QFont::Medium,
                  DemiBold = QFont::DemiBold,
                  Bold = QFont::Bold,
                  ExtraBold = QFont::ExtraBold,
                  Black = QFont::Black };
Q_ENUM_NS(FontWeight)
enum Capitalization { MixedCase = QFont::MixedCase,
                       AllUppercase = QFont::AllUppercase,
                       AllLowercase = QFont::AllLowercase,
                       SmallCaps = QFont::SmallCaps,
                       Capitalize = QFont::Capitalize };
Q_ENUM_NS(Capitalization)

enum HintingPreference {
    PreferDefaultHinting = QFont::PreferDefaultHinting,
    PreferNoHinting = QFont::PreferNoHinting,
    PreferVerticalHinting = QFont::PreferVerticalHinting,
    PreferFullHinting = QFont::PreferFullHinting
};
Q_ENUM_NS(HintingPreference)

enum Style {
    StyleNormal = QFont::StyleNormal,
    StyleItalic = QFont::StyleItalic,
    StyleOblique = QFont::StyleOblique,
};
Q_ENUM_NS(Style)
};

class Q_QUICK_EXPORT QQuickFontValueType : public QFont
{
    Q_GADGET

    Q_PROPERTY(QString family READ family WRITE setFamily FINAL)
    Q_PROPERTY(QString styleName READ styleName WRITE setStyleName FINAL)
    Q_PROPERTY(bool bold READ bold WRITE setBold FINAL)
    Q_PROPERTY(int weight READ weight WRITE setWeight FINAL)
    Q_PROPERTY(bool italic READ italic WRITE setItalic FINAL)
    Q_PROPERTY(bool underline READ underline WRITE setUnderline FINAL)
    Q_PROPERTY(bool overline READ overline WRITE setOverline FINAL)
    Q_PROPERTY(bool strikeout READ strikeout WRITE setStrikeout FINAL)
    Q_PROPERTY(qreal pointSize READ pointSize WRITE setPointSize FINAL)
    Q_PROPERTY(int pixelSize READ pixelSize WRITE setPixelSize FINAL)
    Q_PROPERTY(QQuickFontEnums::Capitalization capitalization READ capitalization WRITE setCapitalization FINAL)
    Q_PROPERTY(qreal letterSpacing READ letterSpacing WRITE setLetterSpacing FINAL)
    Q_PROPERTY(qreal wordSpacing READ wordSpacing WRITE setWordSpacing FINAL)
    Q_PROPERTY(QQuickFontEnums::HintingPreference hintingPreference READ hintingPreference WRITE setHintingPreference FINAL)
    Q_PROPERTY(bool kerning READ kerning WRITE setKerning FINAL)
    Q_PROPERTY(bool preferShaping READ preferShaping WRITE setPreferShaping FINAL)
    Q_PROPERTY(QVariantMap features READ features WRITE setFeatures FINAL)
    Q_PROPERTY(QVariantMap variableAxes READ variableAxes WRITE setVariableAxes FINAL)
    Q_PROPERTY(bool contextFontMerging READ contextFontMerging WRITE setContextFontMerging FINAL)
    Q_PROPERTY(bool preferTypoLineMetrics READ preferTypoLineMetrics WRITE setPreferTypoLineMetrics FINAL)

    QML_VALUE_TYPE(font)
    QML_FOREIGN(QFont)
    QML_ADDED_IN_VERSION(2, 0)
    QML_EXTENDED(QQuickFontValueType)
    QML_STRUCTURED_VALUE

public:
    static QVariant create(const QJSValue &value);

    Q_INVOKABLE QQuickFontValueType() = default;
    Q_INVOKABLE QQuickFontValueType(const QFont &font) : QFont(font) {}
    Q_INVOKABLE QString toString() const;

    QString family() const;
    void setFamily(const QString &);

    QString styleName() const;
    void setStyleName(const QString &);

    bool bold() const;
    void setBold(bool b);

    int weight() const;
    void setWeight(int);

    bool italic() const;
    void setItalic(bool b);

    bool underline() const;
    void setUnderline(bool b);

    bool overline() const;
    void setOverline(bool b);

    bool strikeout() const;
    void setStrikeout(bool b);

    qreal pointSize() const;
    void setPointSize(qreal size);

    int pixelSize() const;
    void setPixelSize(int size);

    QQuickFontEnums::Capitalization capitalization() const;
    void setCapitalization(QQuickFontEnums::Capitalization);

    qreal letterSpacing() const;
    void setLetterSpacing(qreal spacing);

    qreal wordSpacing() const;
    void setWordSpacing(qreal spacing);

    QQuickFontEnums::HintingPreference hintingPreference() const;
    void setHintingPreference(QQuickFontEnums::HintingPreference);

    bool kerning() const;
    void setKerning(bool b);

    bool preferShaping() const;
    void setPreferShaping(bool b);

    QVariantMap features() const;
    void setFeatures(const QVariantMap &features);

    QVariantMap variableAxes() const;
    void setVariableAxes(const QVariantMap &variableAxes);

    bool contextFontMerging() const;
    void setContextFontMerging(bool b);

    bool preferTypoLineMetrics() const;
    void setPreferTypoLineMetrics(bool b);
};

namespace QQuickColorSpaceEnums
{
Q_NAMESPACE_EXPORT(Q_QUICK_EXPORT)
QML_NAMED_ELEMENT(ColorSpace)
QML_ADDED_IN_VERSION(2, 15)
Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

enum NamedColorSpace {
    Unknown = 0,
    SRgb,
    SRgbLinear,
    AdobeRgb,
    DisplayP3,
    ProPhotoRgb
};
Q_ENUM_NS(NamedColorSpace)

enum class Primaries {
    Custom = 0,
    SRgb,
    AdobeRgb,
    DciP3D65,
    ProPhotoRgb
};
Q_ENUM_NS(Primaries)
enum class TransferFunction {
    Custom = 0,
    Linear,
    Gamma,
    SRgb,
    ProPhotoRgb
};
Q_ENUM_NS(TransferFunction)
}

class Q_QUICK_EXPORT QQuickColorSpaceValueType : public QColorSpace
{
    Q_GADGET

    Q_PROPERTY(QQuickColorSpaceEnums::NamedColorSpace namedColorSpace READ namedColorSpace WRITE setNamedColorSpace FINAL)
    Q_PROPERTY(QQuickColorSpaceEnums::Primaries primaries READ primaries WRITE setPrimaries FINAL)
    Q_PROPERTY(QQuickColorSpaceEnums::TransferFunction transferFunction READ transferFunction WRITE setTransferFunction FINAL)
    Q_PROPERTY(float gamma READ gamma WRITE setGamma FINAL)

    QML_ANONYMOUS
    QML_FOREIGN(QColorSpace)
    QML_ADDED_IN_VERSION(2, 15)
    QML_EXTENDED(QQuickColorSpaceValueType)
    QML_STRUCTURED_VALUE

public:
    static QVariant create(const QJSValue &params);

    QQuickColorSpaceEnums::NamedColorSpace namedColorSpace() const noexcept;
    void setNamedColorSpace(QQuickColorSpaceEnums::NamedColorSpace namedColorSpace);
    QQuickColorSpaceEnums::Primaries primaries() const noexcept;
    void setPrimaries(QQuickColorSpaceEnums::Primaries primariesId);
    QQuickColorSpaceEnums::TransferFunction transferFunction() const noexcept;
    void setTransferFunction(QQuickColorSpaceEnums::TransferFunction transferFunction);
    float gamma() const noexcept;
    void setGamma(float gamma);
};

QT_END_NAMESPACE

#endif // QQUICKVALUETYPES_P_H
