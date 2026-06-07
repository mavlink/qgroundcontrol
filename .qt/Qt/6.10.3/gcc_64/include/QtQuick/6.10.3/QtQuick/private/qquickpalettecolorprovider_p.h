// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default
#ifndef QQUICKPALETTECOLORPROVIDER_P_H
#define QQUICKPALETTECOLORPROVIDER_P_H

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

#include <QtGui/QPalette>

#include <QtQuick/qtquickglobal.h>

#include <QtQuick/private/qtquickglobal_p.h>
#include <private/qlazilyallocated_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractPaletteProvider;

class Q_QUICK_EXPORT QQuickPaletteColorProvider
    : public std::enable_shared_from_this<QQuickPaletteColorProvider>
{
public:
    QQuickPaletteColorProvider();

    const QColor &color(QPalette::ColorGroup group, QPalette::ColorRole role) const;
    bool setColor(QPalette::ColorGroup group, QPalette::ColorRole role, QColor color);
    bool resetColor(QPalette::ColorGroup group, QPalette::ColorRole role);
    bool resetColor(QPalette::ColorGroup group);

    bool fromQPalette(QPalette p);
    QPalette palette() const;

    const QQuickAbstractPaletteProvider *paletteProvider() const;
    void setPaletteProvider(const QQuickAbstractPaletteProvider *paletteProvider);

    bool copyColorGroup(QPalette::ColorGroup cg, const QQuickPaletteColorProvider &p);

    bool reset();

    bool inheritPalette(const QPalette &palette);

private:
    bool doInheritPalette(const QPalette &palette);
    bool updateInheritedPalette();
    void ensureRequestedPalette();

    QPalette m_resolvedPalette;
    QLazilyAllocated<QPalette> m_requestedPalette;
    QLazilyAllocated<QPalette> m_lastInheritedPalette;

    using Deleter = std::function<void(const QQuickAbstractPaletteProvider*)>;
    using ProviderPtr = std::unique_ptr<const QQuickAbstractPaletteProvider, Deleter>;
    ProviderPtr m_paletteProvider;
};

QT_END_NAMESPACE

#endif // QQUICKPALETTECOLORPROVIDER_P_H
