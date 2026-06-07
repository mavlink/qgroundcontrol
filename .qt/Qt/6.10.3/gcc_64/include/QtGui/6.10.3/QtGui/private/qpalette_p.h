// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPALETTE_P_H
#define QPALETTE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qpalette.h"

#include <QtCore/qatomic.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QPalettePrivate
{
public:
    class Data : public QSharedData {
    public:
        // Every instance of Data has to have a unique serial number, even
        // if it gets created by copying another - we wouldn't create a copy
        // in the first place if the serial number should be the same!
        Data(const Data &other)
            : QSharedData(other)
        {
            for (int grp = 0; grp < int(QPalette::NColorGroups); grp++) {
                for (int role = 0; role < int(QPalette::NColorRoles); role++)
                    br[grp][role] = other.br[grp][role];
            }
        }
        Data() = default;

        QBrush br[QPalette::NColorGroups][QPalette::NColorRoles];
        const int ser_no = qt_palette_count++;
    };

    QPalettePrivate(const QExplicitlySharedDataPointer<Data> &data)
        : ref(1), data(data)
    { }
    QPalettePrivate()
        : QPalettePrivate(QExplicitlySharedDataPointer<Data>(new Data))
    { }

    static constexpr QPalette::ResolveMask colorRoleOffset(QPalette::ColorGroup colorGroup)
    {
        // Exclude NoRole; that bit is used for Accent
        return (qToUnderlying(QPalette::NColorRoles) - 1) * qToUnderlying(colorGroup);
    }

    static constexpr QPalette::ResolveMask bitPosition(QPalette::ColorGroup colorGroup,
                                                       QPalette::ColorRole colorRole)
    {
        // Map Accent into NoRole for resolving purposes
        if (colorRole == QPalette::Accent)
            colorRole = QPalette::NoRole;

        return colorRole + colorRoleOffset(colorGroup);
    }

    QAtomicInt ref;
    QPalette::ResolveMask resolveMask = {0};
    static QAtomicInt qt_palette_count;
    static QAtomicInt qt_palette_private_count;
    int detach_no = ++qt_palette_private_count;
    QExplicitlySharedDataPointer<Data> data;
};

QT_END_NAMESPACE

#endif // QPALETTE_P_H
