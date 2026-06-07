// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTQUICKFOREIGN_P_H
#define QTQUICKFOREIGN_P_H

#include <QtQuick/private/qtquickglobal_p.h>

#include <QtGui/qaccessibilityhints.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qeventpoint.h>
#if QT_CONFIG(im)
#include <QtGui/qinputmethod.h>
#endif
#if QT_CONFIG(validator)
#include <QtGui/qvalidator.h>
#endif
#if QT_CONFIG(shortcut)
#include <QtGui/qkeysequence.h>
#endif

#include <QtGui/qfontvariableaxis.h>

#include <QtGui/QScreen>

#include <QtQml/qqml.h>

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

QT_BEGIN_NAMESPACE

struct QAccessibilityHintsForeign
{
    Q_GADGET
    QML_FOREIGN(QAccessibilityHints)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 10)
};

struct QStyleHintsForeign
{
    Q_GADGET
    QML_FOREIGN(QStyleHints)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 4)
};

struct QImageForeign
{
    Q_GADGET
    QML_FOREIGN(QImage)
    QML_ANONYMOUS
};

#if QT_CONFIG(validator)
struct QValidatorForeign
{
    Q_GADGET
    QML_FOREIGN(QValidator)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)
};

#if QT_CONFIG(regularexpression)
struct QRegularExpressionValidatorForeign
{
    Q_GADGET
    QML_FOREIGN(QRegularExpressionValidator)
    QML_NAMED_ELEMENT(RegularExpressionValidator)
    QML_ADDED_IN_VERSION(2, 14)
};
#endif // QT_CONFIG(regularexpression)

#endif // QT_CONFIG(validator)

#if QT_CONFIG(im)
struct QInputMethodForeign
{
    Q_GADGET
    QML_FOREIGN(QInputMethod)
    QML_NAMED_ELEMENT(InputMethod)
    QML_ADDED_IN_VERSION(2, 0)
    QML_REMOVED_IN_VERSION(6, 4)
    QML_UNCREATABLE("InputMethod is an abstract class.")
};
#endif // QT_CONFIG(im)

#if QT_CONFIG(shortcut)
namespace QKeySequenceForeign
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(QKeySequence)
    QML_NAMED_ELEMENT(StandardKey)
    QML_ADDED_IN_VERSION(2, 2)
};
#endif // QT_CONFIG(shortcut)

struct QEventPointForeign
{
    Q_GADGET
    QML_FOREIGN(QEventPoint)
    QML_VALUE_TYPE(eventPoint)
    QML_ADDED_IN_VERSION(6, 5)
};

// Prevent the same QEventPoint type from being exported into qmltypes
// twice, as a value type and a namespace.
// TODO: Remove once QTBUG-115855 is fixed.
struct QEventPointDerived : public QEventPoint
{
    Q_GADGET
};

namespace QEventPointForeignNamespace
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(QEventPointDerived)
    QML_NAMED_ELEMENT(EventPoint)
    QML_ADDED_IN_VERSION(6, 6)
};

struct QFontVariableAxisForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 9)
    QML_FOREIGN(QFontVariableAxis)
};

struct QScreenForeign
{
    Q_GADGET
    QML_FOREIGN(QScreen)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 3) // used in ScreenInfo
};

QT_END_NAMESPACE

#endif // QTQUICKFOREIGN_P_H
