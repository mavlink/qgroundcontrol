// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSVALUEITERATOR_H
#define QJSVALUEITERATOR_H

#include <QtQml/qjsvalue.h>
#include <QtQml/qtqmlglobal.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE


class QString;

class QJSValueIteratorPrivate;
class Q_QML_EXPORT QJSValueIterator
{
public:
    QJSValueIterator(const QJSValue &value);
    ~QJSValueIterator();

    bool hasNext() const;
    bool next();

    QString name() const;

    QJSValue value() const;
    QJSValueIterator& operator=(QJSValue &value);

private:
    QScopedPointer<QJSValueIteratorPrivate> d_ptr;

    Q_DECLARE_PRIVATE(QJSValueIterator)
    Q_DISABLE_COPY(QJSValueIterator)
};

QT_END_NAMESPACE

#endif // QJSVALUEITERATOR_H
