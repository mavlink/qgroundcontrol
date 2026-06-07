// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTEXTSELECTION_H
#define QQUICKTEXTSELECTION_H

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

#include <QtQuick/qquicktextdocument.h>

#include <QtQml/qqml.h>

#include <QtGui/qtextcursor.h>

QT_BEGIN_NAMESPACE

class QFont;
class QQuickTextControl;

class Q_QUICK_EXPORT QQuickTextSelection : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)

    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 7)

public:
    explicit QQuickTextSelection(QObject *parent = nullptr);

    QString text() const;
    void setText(const QString &text);

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(QColor color);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment align);

Q_SIGNALS:
    void textChanged();
    void fontChanged();
    void colorChanged();
    void alignmentChanged();

private:
    QTextCursor cursor() const;
    void updateFromCharFormat(const QTextCharFormat &fmt);
    void updateFromBlockFormat();

private:
    QTextCursor m_cursor;
    QTextCharFormat m_charFormat;
    QTextBlockFormat m_blockFormat;
    QQuickTextControl *m_control = nullptr;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTextSelection)

#endif // QQUICKTEXTSELECTION_H
