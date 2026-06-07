// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QDOCHTMLEXTRACTOR_P_H
#define QDOCHTMLEXTRACTOR_P_H

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

#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QString>

QT_BEGIN_NAMESPACE

class HtmlExtractor
{
public:
    enum class ExtractionMode : char { Simplified, Extended };

    virtual QString extract(const QString &code, const QString &keyword, ExtractionMode mode) = 0;
    virtual ~HtmlExtractor() = default;
};

class ExtractDocumentation
{
public:
    ExtractDocumentation(QQmlJS::Dom::DomType domType);
    QString execute(const QString &code, const QString &keyword, HtmlExtractor::ExtractionMode mode);
private:
    std::unique_ptr<HtmlExtractor> m_extractor;
};

QT_END_NAMESPACE

#endif // QDOCHTMLEXTRACTOR_P_H
