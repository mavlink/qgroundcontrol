// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKIMAGINESTYLE_P_H
#define QQUICKIMAGINESTYLE_P_H

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

#include <QtCore/qvariant.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2/qquickattachedpropertypropagator.h>
#include <QtQuickControls2Imagine/qtquickcontrols2imagineexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKCONTROLS2IMAGINE_EXPORT QQuickImagineStyle : public QQuickAttachedPropertyPropagator
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath RESET resetPath NOTIFY pathChanged FINAL)
    Q_PROPERTY(QUrl url READ url NOTIFY pathChanged FINAL)
    QML_NAMED_ELEMENT(Imagine)
    QML_ATTACHED(QQuickImagineStyle)
    QML_UNCREATABLE("")
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickImagineStyle(QObject *parent = nullptr);

    static QQuickImagineStyle *qmlAttachedProperties(QObject *object);

    QString path() const;
    void setPath(const QString &path);
    void inheritPath(const QString &path);
    void propagatePath();
    void resetPath();

    QUrl url() const;

Q_SIGNALS:
    void pathChanged();

protected:
    void attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent) override;

private:
    void init();

    bool m_explicitPath = false;
    QString m_path;
};

QT_END_NAMESPACE

#endif // QQUICKIMAGINESTYLE_P_H
