// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSEARCHFIELD_P_H
#define QQUICKSEARCHFIELD_P_H

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

#include <QtQuickTemplates2/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickSearchFieldPrivate;
class QQuickTextField;
class QQuickPopup;
class QQmlInstanceModel;
class QQuickIndicatorButton;
class QQmlComponent;

class Q_QUICKTEMPLATES2_EXPORT QQuickSearchField : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(QVariant suggestionModel READ suggestionModel WRITE setSuggestionModel
                       NOTIFY suggestionModelChanged FINAL)
    Q_PROPERTY(QQmlInstanceModel *delegateModel READ delegateModel NOTIFY delegateModelChanged FINAL)
    Q_PROPERTY(int suggestionCount READ suggestionCount NOTIFY suggestionCountChanged FINAL)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged
                       FINAL)
    Q_PROPERTY(int highlightedIndex READ highlightedIndex NOTIFY highlightedIndexChanged FINAL)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QString textRole READ textRole WRITE setTextRole NOTIFY textRoleChanged FINAL)
    Q_PROPERTY(bool live READ isLive WRITE setLive NOTIFY liveChanged)
    Q_PROPERTY(QQuickIndicatorButton *searchIndicator READ searchIndicator CONSTANT FINAL)
    Q_PROPERTY(QQuickIndicatorButton *clearIndicator READ clearIndicator CONSTANT FINAL)
    Q_PROPERTY(QQuickPopup *popup READ popup WRITE setPopup NOTIFY popupChanged FINAL)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)

    QML_NAMED_ELEMENT(SearchField)
    QML_ADDED_IN_VERSION(6, 10)

public:
    explicit QQuickSearchField(QQuickItem *parent = nullptr);
    ~QQuickSearchField();

    QVariant suggestionModel() const;
    void setSuggestionModel(const QVariant &model);

    QQmlInstanceModel *delegateModel() const;

    int suggestionCount() const;

    int currentIndex() const;
    void setCurrentIndex(int index);

    int highlightedIndex() const;

    QString text() const;
    void setText(const QString &text);

    QString textRole() const;
    void setTextRole(const QString &textRole);

    bool isLive() const;
    void setLive(const bool live);

    QQuickIndicatorButton *searchIndicator() const;
    QQuickIndicatorButton *clearIndicator() const;

    QQuickPopup *popup() const;
    void setPopup(QQuickPopup *popup);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

Q_SIGNALS:
    void activated(int index);
    void highlighted(int index);
    void accepted();
    void searchTriggered();
    void textEdited();
    void suggestionModelChanged();
    void delegateModelChanged();
    void suggestionCountChanged();
    void currentIndexChanged();
    void highlightedIndexChanged();
    void textChanged();
    void textRoleChanged();
    void liveChanged();
    void popupChanged();
    void delegateChanged();

    void searchButtonPressed();
    void clearButtonPressed();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void classBegin() override;
    void componentComplete() override;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data) override;

private:
    Q_DISABLE_COPY(QQuickSearchField)
    Q_DECLARE_PRIVATE(QQuickSearchField)
};

QT_END_NAMESPACE

#endif // QQUICKSEARCHFIELD_P_H
