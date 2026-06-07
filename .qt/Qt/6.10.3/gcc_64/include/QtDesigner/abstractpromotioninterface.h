// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ABSTRACTPROMOTIONINTERFACE_H
#define ABSTRACTPROMOTIONINTERFACE_H

#include <QtDesigner/sdk_global.h>

#include <QtCore/qpair.h>
#include <QtCore/qlist.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class QDesignerWidgetDataBaseItemInterface;

class QDESIGNER_SDK_EXPORT QDesignerPromotionInterface
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerPromotionInterface)

    QDesignerPromotionInterface() = default;
    virtual ~QDesignerPromotionInterface() = default;

    struct PromotedClass
    {
        QDesignerWidgetDataBaseItemInterface *baseItem;
        QDesignerWidgetDataBaseItemInterface *promotedItem;
    };

    using PromotedClasses = QList<PromotedClass>;

    virtual PromotedClasses promotedClasses() const = 0;

    virtual QSet<QString> referencedPromotedClassNames()  const = 0;

    virtual bool addPromotedClass(const QString &baseClass,
                                  const QString &className,
                                  const QString &includeFile,
                                  QString *errorMessage) = 0;

    virtual bool removePromotedClass(const QString &className, QString *errorMessage) = 0;

    virtual bool changePromotedClassName(const QString &oldClassName, const QString &newClassName, QString *errorMessage) = 0;

    virtual bool setPromotedClassIncludeFile(const QString &className, const QString &includeFile, QString *errorMessage) = 0;

    virtual QList<QDesignerWidgetDataBaseItemInterface *> promotionBaseClasses() const = 0;
};

QT_END_NAMESPACE

#endif // ABSTRACTPROMOTIONINTERFACE_H
