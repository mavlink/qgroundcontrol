// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKVALIDATOR_P_H
#define QQUICKVALIDATOR_P_H

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

#include <private/qtquickglobal_p.h>

#include <QtQml/qqml.h>

#include <QtGui/qvalidator.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(validator)
class Q_QUICK_EXPORT QQuickIntValidator : public QIntValidator
{
    Q_OBJECT
    Q_PROPERTY(QString locale READ localeName WRITE setLocaleName RESET resetLocaleName NOTIFY localeNameChanged)
    QML_NAMED_ELEMENT(IntValidator)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickIntValidator(QObject *parent = nullptr);

    QString localeName() const;
    void setLocaleName(const QString &name);
    void resetLocaleName();

Q_SIGNALS:
    void localeNameChanged();
};

class Q_QUICK_EXPORT QQuickDoubleValidator : public QDoubleValidator
{
    Q_OBJECT
    Q_PROPERTY(QString locale READ localeName WRITE setLocaleName RESET resetLocaleName NOTIFY localeNameChanged)
    QML_NAMED_ELEMENT(DoubleValidator)
    QML_ADDED_IN_VERSION(2, 0)
public:
    QQuickDoubleValidator(QObject *parent = nullptr);

    QString localeName() const;
    void setLocaleName(const QString &name);
    void resetLocaleName();

Q_SIGNALS:
    void localeNameChanged();
};
#endif

QT_END_NAMESPACE

#endif // QQUICKVALIDATOR_P_H
