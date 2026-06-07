// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QCOMBOBOX_P_H
#define QCOMBOBOX_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "QtWidgets/qcombobox.h"

#include "QtWidgets/qabstractslider.h"
#include "QtWidgets/qapplication.h"
#include "QtWidgets/qstyleditemdelegate.h"
#include "QtGui/qstandarditemmodel.h"
#include "QtWidgets/qlineedit.h"
#include "QtWidgets/qlistview.h"
#include "QtGui/qpainter.h"
#include "QtWidgets/qstyle.h"
#include "QtWidgets/qstyleoption.h"
#include "QtCore/qtimer.h"
#include "private/qwidget_p.h"
#include "QtCore/qpointer.h"
#if QT_CONFIG(completer)
#include "QtWidgets/qcompleter.h"
#endif
#include "QtGui/qevent.h"

#include <limits.h>

QT_REQUIRE_CONFIG(combobox);

QT_BEGIN_NAMESPACE

class QPlatformMenu;

class QComboBoxListView : public QListView
{
    Q_OBJECT
public:
    explicit QComboBoxListView(QComboBox *cmb = nullptr);
    ~QComboBoxListView() override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void initViewItemOption(QStyleOptionViewItem *option) const override;
    void paintEvent(QPaintEvent *e) override;

private:
    QComboBox *combo;
};

class Q_AUTOTEST_EXPORT QComboBoxPrivateScroller : public QWidget
{
    Q_OBJECT

public:
    explicit QComboBoxPrivateScroller(QAbstractSlider::SliderAction action, QWidget *parent);
    ~QComboBoxPrivateScroller() override;

    QSize sizeHint() const override;

protected:
    void stopTimer();
    void startTimer();

    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    void timerEvent(QTimerEvent *e) override;
    void hideEvent(QHideEvent *) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *) override;

Q_SIGNALS:
    void doScroll(int action);

private:
    QAbstractSlider::SliderAction sliderAction;
    QBasicTimer timer;
    bool fast = false;
};

class Q_WIDGETS_EXPORT QComboBoxPrivateContainer : public QFrame
{
    Q_OBJECT

public:
    QComboBoxPrivateContainer(QAbstractItemView *itemView, QComboBox *parent);
    ~QComboBoxPrivateContainer();
    QAbstractItemView *itemView() const;
    void setItemView(QAbstractItemView *itemView);
    int spacing() const;
    int topMargin() const;
    int bottomMargin() const { return topMargin(); }
    void updateTopBottomMargin();
    void updateStyleSettings();

    QTimer blockMouseReleaseTimer;
    QBasicTimer adjustSizeTimer;
    QPoint initialClickPosition;

public Q_SLOTS:
    void scrollItemView(int action);
    void hideScrollers();
    void updateScrollers();
    void viewDestroyed();

protected:
    void changeEvent(QEvent *e) override;
    bool eventFilter(QObject *o, QEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void timerEvent(QTimerEvent *timerEvent) override;
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    QStyleOptionComboBox comboStyleOption() const;

Q_SIGNALS:
    void itemSelected(const QModelIndex &);
    void resetButton();

private:
    QComboBox *combo;
    QAbstractItemView *view = nullptr;
    QComboBoxPrivateScroller *top = nullptr;
    QComboBoxPrivateScroller *bottom = nullptr;
    QElapsedTimer popupTimer;
    bool maybeIgnoreMouseButtonRelease = false;

    friend class QComboBox;
    friend class QComboBoxPrivate;
};

class Q_AUTOTEST_EXPORT QComboMenuDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit QComboMenuDelegate(QObject *parent, QComboBox *cmb);
    ~QComboMenuDelegate() override;

protected:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index) override;

private:
    QStyleOptionMenuItem getStyleOption(const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const;
    QComboBox *mCombo;
    int pressedIndex;
};

class Q_AUTOTEST_EXPORT QComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit QComboBoxDelegate(QObject *parent, QComboBox *cmb);
    ~QComboBoxDelegate() override;

    static bool isSeparator(const QModelIndex &index);
    static void setSeparator(QAbstractItemModel *model, const QModelIndex &index);

protected:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
private:
    QComboBox *mCombo;
};

class Q_AUTOTEST_EXPORT QComboBoxPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QComboBox)
public:
    QComboBoxPrivate();
    ~QComboBoxPrivate();
    void init();
    QComboBoxPrivateContainer* viewContainer();
    void updateLineEditGeometry();
    Qt::MatchFlags matchFlags() const;
    void editingFinished();
    void returnPressed();
    void complete();
    void itemSelected(const QModelIndex &item);
    bool contains(const QString &text, int role);
    void emitActivated(const QModelIndex &index);
    void emitHighlighted(const QModelIndex &index);
    void emitCurrentIndexChanged(const QModelIndex &index);
    void modelDestroyed();
    void modelReset();
    void updateMicroFocus() { q_func()->updateMicroFocus(); } // PMF connect doesn't handle default args
#if QT_CONFIG(completer)
    void completerActivated(const QModelIndex &index);
#endif
    void resetButton();
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void updateIndexBeforeChange();
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    void updateArrow(QStyle::StateFlag state);
    bool updateHoverControl(const QPoint &pos);
    void trySetValidIndex();
    QRect popupGeometry(const QPoint &globalPos) const;
    QStyle::SubControl newHoverControl(const QPoint &pos);
    int computeWidthHint() const;
    QSize recomputeSizeHint(QSize &sh) const;
    void adjustComboBoxSize();
    QString itemText(const QModelIndex &index) const;
    QIcon itemIcon(const QModelIndex &index) const;
    int itemRole() const;
    void updateLayoutDirection();
    void setCurrentIndex(const QModelIndex &index);
    void updateDelegate(bool force = false);
    void initViewItemOption(QStyleOptionViewItem *option) const;
    void keyboardSearchString(const QString &text);
    void modelChanged();
    void updateViewContainerPaletteAndOpacity();
    void updateFocusPolicy();
    void showPopupFromMouseEvent(QMouseEvent *e);
    void doHidePopup();
    void updateCurrentText(const QString &text);
    void connectModel();
    void disconnectModel();

#ifdef Q_OS_MAC
    void cleanupNativePopup();
    bool showNativePopup();
    struct IndexSetter {
        int index;
        QComboBox *cb;

        void operator()(void)
        {
            cb->setCurrentIndex(index);
            cb->d_func()->emitActivated(cb->d_func()->currentIndex);
        }
    };
#endif

    std::array<QMetaObject::Connection, 8> modelConnections;
    QAbstractItemModel *model = nullptr;
    QLineEdit *lineEdit = nullptr;
    QPointer<QComboBoxPrivateContainer> container;
#ifdef Q_OS_MAC
    QPlatformMenu *m_platformMenu = nullptr;
#endif
    QPersistentModelIndex currentIndex;
    QPersistentModelIndex root;
    QString placeholderText;
    QString currentText;
    QRect hoverRect;
    QSize iconSize;
    mutable QSize minimumSizeHint;
    mutable QSize sizeHint;
    QComboBox::InsertPolicy insertPolicy = QComboBox::InsertAtBottom;
    QComboBox::SizeAdjustPolicy sizeAdjustPolicy = QComboBox::AdjustToContentsOnFirstShow;
    QStyle::StateFlag arrowState = QStyle::State_None;
    QStyle::SubControl hoverControl = QStyle::SC_None;
    QComboBox::LabelDrawingMode labelDrawingMode = QComboBox::LabelDrawingMode::UseStyle;
    int minimumContentsLength = 0;
    int indexBeforeChange = -1;
    int maxVisibleItems = 10;
    int maxCount = (std::numeric_limits<int>::max)();
    int modelColumn = 0;
    int placeholderIndex = -1;
    bool shownOnce : 1;
    bool duplicatesEnabled : 1;
    bool frame : 1;
    bool inserting : 1;
    bool hidingPopup : 1;
};

QT_END_NAMESPACE

#endif // QCOMBOBOX_P_H
