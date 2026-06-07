// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTABLEMODELCOLUMN_P_H
#define QQMLTABLEMODELCOLUMN_P_H

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

#include "qqmlmodelsglobal_p.h"

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtQml/qqml.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQml/qjsvalue.h>

QT_REQUIRE_CONFIG(qml_table_model);

QT_BEGIN_NAMESPACE

class Q_LABSQMLMODELS_EXPORT QQmlTableModelColumn : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QJSValue display READ display WRITE setDisplay NOTIFY displayChanged FINAL)
    Q_PROPERTY(QJSValue decoration READ decoration WRITE setDecoration NOTIFY decorationChanged FINAL)
    Q_PROPERTY(QJSValue edit READ edit WRITE setEdit NOTIFY editChanged FINAL)
    Q_PROPERTY(QJSValue toolTip READ toolTip WRITE setToolTip NOTIFY toolTipChanged FINAL)
    Q_PROPERTY(QJSValue statusTip READ statusTip WRITE setStatusTip NOTIFY statusTipChanged FINAL)
    Q_PROPERTY(QJSValue whatsThis READ whatsThis WRITE setWhatsThis NOTIFY whatsThisChanged FINAL)

    Q_PROPERTY(QJSValue font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QJSValue textAlignment READ textAlignment WRITE setTextAlignment NOTIFY textAlignmentChanged FINAL)
    Q_PROPERTY(QJSValue background READ background WRITE setBackground NOTIFY backgroundChanged FINAL)
    Q_PROPERTY(QJSValue foreground READ foreground WRITE setForeground NOTIFY foregroundChanged FINAL)
    Q_PROPERTY(QJSValue checkState READ checkState WRITE setCheckState NOTIFY checkStateChanged FINAL)

    Q_PROPERTY(QJSValue accessibleText READ accessibleText WRITE setAccessibleText NOTIFY accessibleTextChanged FINAL)
    Q_PROPERTY(QJSValue accessibleDescription READ accessibleDescription
        WRITE setAccessibleDescription NOTIFY accessibleDescriptionChanged FINAL)

    Q_PROPERTY(QJSValue sizeHint READ sizeHint WRITE setSizeHint NOTIFY sizeHintChanged FINAL)
    QML_NAMED_ELEMENT(TableModelColumn)
    QML_ADDED_IN_VERSION(1, 0)

public:
    QQmlTableModelColumn(QObject *parent = nullptr);
    ~QQmlTableModelColumn() override;

    QJSValue display() const;
    void setDisplay(const QJSValue &stringOrFunction);

    QJSValue decoration() const;
    void setDecoration(const QJSValue &stringOrFunction);

    QJSValue edit() const;
    void setEdit(const QJSValue &stringOrFunction);

    QJSValue toolTip() const;
    void setToolTip(const QJSValue &stringOrFunction);

    QJSValue statusTip() const;
    void setStatusTip(const QJSValue &stringOrFunction);

    QJSValue whatsThis() const;
    void setWhatsThis(const QJSValue &stringOrFunction);

    QJSValue font() const;
    void setFont(const QJSValue &stringOrFunction);

    QJSValue textAlignment() const;
    void setTextAlignment(const QJSValue &stringOrFunction);

    QJSValue background() const;
    void setBackground(const QJSValue &stringOrFunction);

    QJSValue foreground() const;
    void setForeground(const QJSValue &stringOrFunction);

    QJSValue checkState() const;
    void setCheckState(const QJSValue &stringOrFunction);

    QJSValue accessibleText() const;
    void setAccessibleText(const QJSValue &stringOrFunction);

    QJSValue accessibleDescription() const;
    void setAccessibleDescription(const QJSValue &stringOrFunction);

    QJSValue sizeHint() const;
    void setSizeHint(const QJSValue &stringOrFunction);

    QJSValue getterAtRole(const QString &roleName);

    const QHash<QString, QJSValue> getters() const;

    static const QHash<int, QString> supportedRoleNames();

Q_SIGNALS:
    void indexChanged();
    void displayChanged();
    void decorationChanged();
    void editChanged();
    void toolTipChanged();
    void statusTipChanged();
    void whatsThisChanged();

    void fontChanged();
    void textAlignmentChanged();
    void backgroundChanged();
    void foregroundChanged();
    void checkStateChanged();

    void accessibleTextChanged();
    void accessibleDescriptionChanged();
    void sizeHintChanged();

private:
    // We store these in a hash because QQuickTableModel needs string-based lookup in certain situations.
    QHash<QString, QJSValue> mGetters;
};

QT_END_NAMESPACE

#endif // QQMLTABLEMODELCOLUMN_P_H
