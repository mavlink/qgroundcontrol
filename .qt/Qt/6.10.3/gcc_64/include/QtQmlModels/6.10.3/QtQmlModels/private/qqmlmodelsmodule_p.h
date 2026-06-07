// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLMODELSMODULE_H
#define QQMLMODELSMODULE_H

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

#include <QtQml/qqml.h>

#if QT_CONFIG(itemmodel)
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qitemselectionmodel.h>
#endif

#include <private/qtqmlmodelsglobal_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(itemmodel)
struct QItemSelectionModelForeign
{
    Q_GADGET
    QML_FOREIGN(QItemSelectionModel)
    QML_NAMED_ELEMENT(ItemSelectionModel)
    QML_ADDED_IN_VERSION(2, 2)
};

struct QAbstractItemModelForeign
{
    Q_GADGET
    QML_FOREIGN(QAbstractItemModel)
    QML_NAMED_ELEMENT(AbstractItemModel)
    QML_ADDED_IN_VERSION(6, 5)
    QML_UNCREATABLE("QAbstractItemModel is abstract in C++.")
};

struct QAbstractListModelForeign
{
    Q_GADGET
    QML_FOREIGN(QAbstractListModel)
    QML_NAMED_ELEMENT(AbstractListModel)
    QML_ADDED_IN_VERSION(6, 5)
    QML_UNCREATABLE("QAbstractListModel is abstract in C++.")
};
#endif

QT_END_NAMESPACE

#endif
