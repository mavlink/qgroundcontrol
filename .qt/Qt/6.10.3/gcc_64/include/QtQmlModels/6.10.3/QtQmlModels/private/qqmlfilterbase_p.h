// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLFILTERBASE_H
#define QQMLFILTERBASE_H

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

#include <QtCore/QObject>
#include <QtCore/QAbstractItemModel>
#include <QtQml/private/qqmlcustomparser_p.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlFilterBasePrivate;

class Q_QMLMODELS_EXPORT QQmlFilterBase: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool invert READ invert WRITE setInvert NOTIFY invertChanged FINAL)
    Q_PROPERTY(int column READ column WRITE setColumn NOTIFY columnChanged FINAL)
    QML_NAMED_ELEMENT(FilterBase)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQmlFilterBase(QQmlFilterBasePrivate *privObj, QObject *parent = nullptr);
    virtual ~QQmlFilterBase() = default;

    bool enabled() const;
    void setEnabled(const bool bEnable);

    bool invert() const;
    void setInvert(const bool bInvert);

    int column() const;
    void setColumn(const int column);

    virtual bool filterAcceptsRowInternal(int, const QModelIndex&, const QQmlSortFilterProxyModel *) const { return true; }
    virtual bool filterAcceptsColumnInternal(int, const QModelIndex&, const QQmlSortFilterProxyModel *) const { return true; }
    virtual void update(const QQmlSortFilterProxyModel *) { /* do nothing */ };

Q_SIGNALS:
    void invalidateModel();
    void invalidateCache(QQmlFilterBase *filter);
    void enabledChanged();
    void invertChanged();
    void columnChanged();

public slots:
    void invalidate(bool updateCache = false);

private:
    Q_DECLARE_PRIVATE(QQmlFilterBase)
};

class QQmlFilterBasePrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlFilterBase)

private:
    bool m_enabled = true;
    bool m_invert = false;
    int m_filterColumn = -1;
};

QT_END_NAMESPACE

#endif // QQMLFILTERBASE_H
