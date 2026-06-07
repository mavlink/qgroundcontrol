// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QQUICKGEOMETRYTESTUTIL_P_H
#define QQUICKGEOMETRYTESTUTIL_P_H

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

#include <QObject>
#include <QVector>
#include <QSize>
#include <private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickItem;

class QSizeChangeListener : public QObject, public QVector<QSize>
{
    Q_OBJECT
public:
    explicit QSizeChangeListener(QQuickItem *item);
private Q_SLOTS:
    void onSizeChanged();
private:
    QQuickItem *item;
};

QT_END_NAMESPACE

#endif // QQUICKGEOMETRYTESTUTIL_P_H
