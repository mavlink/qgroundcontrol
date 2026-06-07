// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFONTINFO_P_H
#define QQUICKFONTINFO_P_H

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
#include <QtGui/qfontinfo.h>
#include <QtCore/qobject.h>

#include <QtQuick/private/qquickvaluetypes_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickFontInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QString family READ family NOTIFY fontChanged FINAL)
    Q_PROPERTY(QString styleName READ styleName NOTIFY fontChanged FINAL)
    Q_PROPERTY(int pixelSize READ pixelSize NOTIFY fontChanged FINAL)
    Q_PROPERTY(qreal pointSize READ pointSize NOTIFY fontChanged FINAL)
    Q_PROPERTY(bool italic READ italic NOTIFY fontChanged FINAL)
    Q_PROPERTY(int weight READ weight NOTIFY fontChanged FINAL)
    Q_PROPERTY(bool bold READ bold NOTIFY fontChanged FINAL)
    Q_PROPERTY(bool fixedPitch READ fixedPitch NOTIFY fontChanged FINAL)
    Q_PROPERTY(QQuickFontEnums::Style style READ style NOTIFY fontChanged FINAL)
    Q_PROPERTY(QList<QFontVariableAxis> variableAxes READ variableAxes NOTIFY fontChanged FINAL)
    QML_NAMED_ELEMENT(FontInfo)
    QML_ADDED_IN_VERSION(6, 9)
public:
    explicit QQuickFontInfo(QObject *parent = nullptr);
    ~QQuickFontInfo() override;

    QFont font() const;
    void setFont(QFont font);

    QString family() const;
    QString styleName() const;
    int pixelSize() const;
    qreal pointSize() const;
    bool italic() const;
    int weight() const;
    bool bold() const;
    bool underline() const;
    bool overline() const;
    bool strikeOut() const;
    bool fixedPitch() const;
    QQuickFontEnums::Style style() const;
    QList<QFontVariableAxis> variableAxes() const;

Q_SIGNALS:
    void fontChanged();

private:
    QFont m_font;
    QFontInfo m_info;
};

QT_END_NAMESPACE

#endif // QQUICKFONTINFO_P_H
