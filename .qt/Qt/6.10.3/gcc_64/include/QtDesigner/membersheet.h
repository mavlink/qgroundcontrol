// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MEMBERSHEET_H
#define MEMBERSHEET_H

#include <QtDesigner/extension.h>

#include <QtCore/qlist.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE

class QString; // FIXME: fool syncqt

class QDesignerMemberSheetExtension
{
public:
    Q_DISABLE_COPY_MOVE(QDesignerMemberSheetExtension)

    QDesignerMemberSheetExtension() = default;
    virtual ~QDesignerMemberSheetExtension() = default;

    virtual int count() const = 0;

    virtual int indexOf(const QString &name) const = 0;

    virtual QString memberName(int index) const = 0;
    virtual QString memberGroup(int index) const = 0;
    virtual void setMemberGroup(int index, const QString &group) = 0;

    virtual bool isVisible(int index) const = 0;
    virtual void setVisible(int index, bool b) = 0;

    virtual bool isSignal(int index) const = 0;
    virtual bool isSlot(int index) const = 0;

    virtual bool inheritedFromWidget(int index) const = 0;

    virtual QString declaredInClass(int index) const = 0;

    virtual QString signature(int index) const = 0;
    virtual QList<QByteArray> parameterTypes(int index) const = 0;
    virtual QList<QByteArray> parameterNames(int index) const = 0;
};
Q_DECLARE_EXTENSION_INTERFACE(QDesignerMemberSheetExtension, "org.qt-project.Qt.Designer.MemberSheet")

QT_END_NAMESPACE

#endif // MEMBERSHEET_H
