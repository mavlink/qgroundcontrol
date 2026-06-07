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

#ifndef QDESIGNERPROMOTION_H
#define QDESIGNERPROMOTION_H

#include "shared_global_p.h"

#include <QtDesigner/abstractpromotioninterface.h>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;

namespace qdesigner_internal {

    class  QDESIGNER_SHARED_EXPORT  QDesignerPromotion : public QDesignerPromotionInterface
    {
    public:
        explicit QDesignerPromotion(QDesignerFormEditorInterface *core);

        PromotedClasses promotedClasses() const override;

        QSet<QString> referencedPromotedClassNames() const override;

        bool addPromotedClass(const QString &baseClass,
                              const QString &className,
                              const QString &includeFile,
                              QString *errorMessage) override;

        bool removePromotedClass(const QString &className, QString *errorMessage) override;

        bool changePromotedClassName(const QString &oldclassName, const QString &newClassName, QString *errorMessage) override;

        bool setPromotedClassIncludeFile(const QString &className, const QString &includeFile, QString *errorMessage) override;

        QList<QDesignerWidgetDataBaseItemInterface *> promotionBaseClasses() const override;

    private:
        bool canBePromoted(const QDesignerWidgetDataBaseItemInterface *) const;
        void refreshObjectInspector();

        QDesignerFormEditorInterface *m_core;
    };
}

QT_END_NAMESPACE

#endif // QDESIGNERPROMOTION_H
