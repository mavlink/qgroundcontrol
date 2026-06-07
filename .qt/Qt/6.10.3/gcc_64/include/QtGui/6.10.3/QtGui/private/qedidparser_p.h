// Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEDIDPARSER_P_H
#define QEDIDPARSER_P_H

#include <QtCore/QMap>
#include <QtCore/QPointF>
#include <QtCore/QSize>

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

#include <QtGui/qtguiglobal.h>
#include <QtCore/qlist.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QEdidParser
{
public:
    bool parse(const QByteArray &blob);

    QString identifier;
    QString manufacturer;
    QString model;
    QString serialNumber;
    QSizeF physicalSize;
    qreal gamma;
    QPointF redChromaticity;
    QPointF greenChromaticity;
    QPointF blueChromaticity;
    QPointF whiteChromaticity;
    QList<QList<uint16_t>> tables;
    bool sRgb;
    bool useTables;

private:
    QString parseEdidString(const quint8 *data);
};

QT_END_NAMESPACE

#endif // QEDIDPARSER_P_H
