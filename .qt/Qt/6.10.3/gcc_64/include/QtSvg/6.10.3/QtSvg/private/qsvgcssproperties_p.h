// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSVGCSSPROPERTIES_P_H
#define QSVGCSSPROPERTIES_P_H

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

#include <QtCore/qxmlstream.h>
#include <QtCore/qstringview.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qlist.h>
#include <QtCore/qstringview.h>

QT_BEGIN_NAMESPACE

// Holds the parsed value for each animation-*
struct QSvgAnimationProperty
{
    QStringView name;
    int duration = 0;
    int delay = 0;
    int iteration = 1;
};

class QSvgCssAnimationProperties
{
public:
    QSvgCssAnimationProperties(const QXmlStreamAttributes &attributes);
    QList<QSvgAnimationProperty> parse() const;

private:
    void shortHandtoLonghandForm(QStringView value);

private:
    QList<QStringView> m_names;
    QList<QStringView> m_durations;
    QList<QStringView> m_delays;
    QList<QStringView> m_iterationCounts;
    QList<QStringView> m_directions;
    QList<QStringView> m_timingFunctions;
    QList<QStringView> m_fillModes;
    QList<QStringView> m_playStates;
};

QT_END_NAMESPACE

#endif //QSVGCSSPROPERTIES_P_H
