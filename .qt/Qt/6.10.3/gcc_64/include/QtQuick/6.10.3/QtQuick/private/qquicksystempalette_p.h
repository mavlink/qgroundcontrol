// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSYSTEMPALETTE_H
#define QQUICKSYSTEMPALETTE_H

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

#include <private/qtquickglobal_p.h>

#include <QtCore/qobject.h>

#include <QtGui/qpalette.h>

#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickSystemPalettePrivate;
class Q_QUICK_EXPORT QQuickSystemPalette : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickSystemPalette)

    Q_PROPERTY(QQuickSystemPalette::ColorGroup colorGroup READ colorGroup WRITE setColorGroup NOTIFY paletteChanged)
    Q_PROPERTY(QColor window READ window NOTIFY paletteChanged)
    Q_PROPERTY(QColor windowText READ windowText NOTIFY paletteChanged)
    Q_PROPERTY(QColor base READ base NOTIFY paletteChanged)
    Q_PROPERTY(QColor text READ text NOTIFY paletteChanged)
    Q_PROPERTY(QColor alternateBase READ alternateBase NOTIFY paletteChanged)
    Q_PROPERTY(QColor button READ button NOTIFY paletteChanged)
    Q_PROPERTY(QColor buttonText READ buttonText NOTIFY paletteChanged)
    Q_PROPERTY(QColor light READ light NOTIFY paletteChanged)
    Q_PROPERTY(QColor midlight READ midlight NOTIFY paletteChanged)
    Q_PROPERTY(QColor dark READ dark NOTIFY paletteChanged)
    Q_PROPERTY(QColor mid READ mid NOTIFY paletteChanged)
    Q_PROPERTY(QColor shadow READ shadow NOTIFY paletteChanged)
    Q_PROPERTY(QColor highlight READ highlight NOTIFY paletteChanged)
    Q_PROPERTY(QColor highlightedText READ highlightedText NOTIFY paletteChanged)
    Q_PROPERTY(QColor placeholderText READ placeholderText NOTIFY paletteChanged REVISION(6, 2))
    Q_PROPERTY(QColor accent READ accent NOTIFY paletteChanged REVISION(6, 7) FINAL)
    QML_NAMED_ELEMENT(SystemPalette)
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickSystemPalette(QObject *parent=nullptr);

    enum ColorGroup { Active = QPalette::Active, Inactive = QPalette::Inactive, Disabled = QPalette::Disabled };
    Q_ENUM(ColorGroup)

    QColor window() const;
    QColor windowText() const;

    QColor base() const;
    QColor text() const;
    QColor alternateBase() const;

    QColor button() const;
    QColor buttonText() const;

    QColor light() const;
    QColor midlight() const;
    QColor dark() const;
    QColor mid() const;
    QColor shadow() const;

    QColor highlight() const;
    QColor highlightedText() const;

    QColor placeholderText() const;
    QColor accent() const;

    QQuickSystemPalette::ColorGroup colorGroup() const;
    void setColorGroup(QQuickSystemPalette::ColorGroup);

Q_SIGNALS:
    void paletteChanged();
};

QT_END_NAMESPACE

#endif // QQUICKSYSTEMPALETTE_H
