// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef PROMOTIONMODEL_H
#define PROMOTIONMODEL_H

#include <QtGui/qstandarditemmodel.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerWidgetDataBaseItemInterface;

namespace qdesigner_internal {

    // Item model representing the promoted widgets.
    class PromotionModel : public QStandardItemModel {
        Q_OBJECT

    public:
        struct ModelData {
            bool isValid() const { return promotedItem != nullptr; }

            QDesignerWidgetDataBaseItemInterface *baseItem{nullptr};
            QDesignerWidgetDataBaseItemInterface *promotedItem{nullptr};
            bool referenced{false};
        };

        explicit PromotionModel(QDesignerFormEditorInterface *core);

        void updateFromWidgetDatabase();

        ModelData modelData(const QModelIndex &index) const;
        ModelData modelData(const QStandardItem *item) const;

        QModelIndex indexOfClass(const QString &className) const;

   signals:
        void includeFileChanged(QDesignerWidgetDataBaseItemInterface *, const QString &includeFile);
        void classNameChanged(QDesignerWidgetDataBaseItemInterface *, const QString &newName);

    private slots:
        void slotItemChanged(QStandardItem * item);

    private:
        void initializeHeaders();

        QDesignerFormEditorInterface *m_core;
    };
} // namespace qdesigner_internal

QT_END_NAMESPACE

Q_DECLARE_METATYPE(qdesigner_internal::PromotionModel::ModelData)

#endif // PROMOTIONMODEL_H
