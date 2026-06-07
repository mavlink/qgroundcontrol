// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDATETIMEEDIT_P_H
#define QDATETIMEEDIT_P_H

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
#include <QtCore/qcalendar.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qtimezone.h>
#include "QtWidgets/qcalendarwidget.h"
#include "QtWidgets/qspinbox.h"
#include "QtWidgets/qtoolbutton.h"
#include "QtWidgets/qmenu.h"
#include "QtWidgets/qdatetimeedit.h"
#include "private/qabstractspinbox_p.h"
#include "private/qdatetimeparser_p.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QCalendarPopup;
class Q_AUTOTEST_EXPORT QDateTimeEditPrivate : public QAbstractSpinBoxPrivate, public QDateTimeParser
{
    Q_DECLARE_PUBLIC(QDateTimeEdit)
public:
    QDateTimeEditPrivate(const QTimeZone &zone = QTimeZone::LocalTime);

    void init(const QVariant &var);
    void readLocaleSettings();

    QDateTime validateAndInterpret(QString &input, int &, QValidator::State &state,
                                   bool fixup = false) const;
    void clearSection(int index);

    // Override QAbstractSpinBoxPrivate:
    void emitSignals(EmitPolicy ep, const QVariant &old) override;
    QString textFromValue(const QVariant &f) const override;
    QVariant valueFromText(const QString &f) const override;
    void editorCursorPositionChanged(int oldpos, int newpos) override;
    void interpret(EmitPolicy ep) override;
    void clearCache() const override;
    QStyle::SubControl newHoverControl(const QPoint &pos) override;
    void updateEditFieldGeometry() override;
    QVariant getZeroVariant() const override;
    void setRange(const QVariant &min, const QVariant &max) override;
    void updateEdit() override;

    // Override QDateTimeParser:
    QString displayText() const override { return edit->text(); }
    QDateTime getMinimum(const QTimeZone &zone) const override;
    QDateTime getMaximum(const QTimeZone &zone) const override;
    QLocale locale() const override { return q_func()->locale(); }
    int cursorPosition() const override { return edit ? edit->cursorPosition() : -1; }

    int absoluteIndex(QDateTimeEdit::Section s, int index) const;
    int absoluteIndex(SectionNode s) const;
    QDateTime stepBy(int index, int steps, bool test = false) const;
    int sectionAt(int pos) const;
    int closestSection(int index, bool forward) const;
    int nextPrevSection(int index, bool forward) const;
    void setSelected(int index, bool forward = false);

    void updateCache(const QVariant &val, const QString &str) const;

    QDateTime convertTimeZone(const QDateTime &datetime);
    void updateTimeZone();
    QString valueToText(const QVariant &var) const { return textFromValue(var); }
    QDateTime dateTimeValue(QDate date, QTime time) const;

    void _q_resetButton();
    void updateArrow(QStyle::StateFlag state);
    bool calendarPopupEnabled() const;
    void syncCalendarWidget();

    bool isSeparatorKey(const QKeyEvent *k) const;

    static QDateTimeEdit::Sections convertSections(QDateTimeParser::Sections s);
    static QDateTimeEdit::Section convertToPublic(QDateTimeParser::Section s);

    void initCalendarPopup(QCalendarWidget *cw = nullptr);
    void positionCalendarPopup();

    QDateTimeEdit::Sections sections = {};
    mutable bool cacheGuard = false;

    QString defaultDateFormat, defaultTimeFormat, defaultDateTimeFormat, unreversedFormat;
    mutable QVariant conflictGuard;
    bool hasHadFocus = false, formatExplicitlySet = false, calendarPopup = false;
    QStyle::StateFlag arrowState = QStyle::State_None;
    QCalendarPopup *monthCalendar = nullptr;

#ifdef QT_KEYPAD_NAVIGATION
    bool focusOnButton = false;
#endif

    QTimeZone timeZone;
};


class QCalendarPopup : public QWidget
{
    Q_OBJECT
public:
    explicit QCalendarPopup(QWidget *parent = nullptr, QCalendarWidget *cw = nullptr,
                            QCalendar ca = QCalendar());
    QDate selectedDate() { return verifyCalendarInstance()->selectedDate(); }
    void setDate(QDate date);
    void setDateRange(QDate min, QDate max);
    void setFirstDayOfWeek(Qt::DayOfWeek dow) { verifyCalendarInstance()->setFirstDayOfWeek(dow); }
    QCalendarWidget *calendarWidget() const { return const_cast<QCalendarPopup*>(this)->verifyCalendarInstance(); }
    void setCalendarWidget(QCalendarWidget *cw);
Q_SIGNALS:
    void activated(QDate date);
    void newDateSelected(QDate newDate);
    void hidingCalendar(QDate oldDate);
    void resetButton();

private Q_SLOTS:
    void dateSelected(QDate date);
    void dateSelectionChanged();

protected:
    void hideEvent(QHideEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    bool event(QEvent *e) override;

private:
    QCalendarWidget *verifyCalendarInstance();

    QPointer<QCalendarWidget> calendar;
    QDate oldDate;
    bool dateChanged;
    QCalendar calendarSystem;
};

QT_END_NAMESPACE

#endif // QDATETIMEEDIT_P_H
