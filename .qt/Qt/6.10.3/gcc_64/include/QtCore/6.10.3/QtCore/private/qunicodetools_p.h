// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:header-decls-only

#ifndef QUNICODETOOLS_P_H
#define QUNICODETOOLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qchar.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

struct QCharAttributes
{
    uchar graphemeBoundary : 1;
    uchar wordBreak        : 1;
    uchar sentenceBoundary : 1;
    uchar lineBreak        : 1;
    uchar whiteSpace       : 1;
    uchar wordStart        : 1;
    uchar wordEnd          : 1;
    uchar mandatoryBreak   : 1;
};
Q_DECLARE_TYPEINFO(QCharAttributes, Q_PRIMITIVE_TYPE);

namespace QUnicodeTools {

struct ScriptItem
{
    qsizetype position;
    QChar::Script script;
};

using ScriptItemArray = QVarLengthArray<ScriptItem, 64>;

} // namespace QUnicodeTools
Q_DECLARE_TYPEINFO(QUnicodeTools::ScriptItem, Q_PRIMITIVE_TYPE);
namespace QUnicodeTools {

enum CharAttributeOption {
    GraphemeBreaks = 0x01,
    WordBreaks = 0x02,
    SentenceBreaks = 0x04,
    LineBreaks = 0x08,
    WhiteSpaces = 0x10,
    HangulLineBreakTailoring = 0x20,

    DontClearAttributes = 0x1000
};
Q_DECLARE_FLAGS(CharAttributeOptions, CharAttributeOption)

// attributes buffer has to have a length of string length + 1
Q_CORE_EXPORT void initCharAttributes(QStringView str,
                                      const ScriptItem *items, qsizetype numItems,
                                      QCharAttributes *attributes, CharAttributeOptions options);


Q_CORE_EXPORT void initScripts(QStringView str, ScriptItemArray *scripts);

} // namespace QUnicodeTools

QT_END_NAMESPACE

#endif // QUNICODETOOLS_P_H
