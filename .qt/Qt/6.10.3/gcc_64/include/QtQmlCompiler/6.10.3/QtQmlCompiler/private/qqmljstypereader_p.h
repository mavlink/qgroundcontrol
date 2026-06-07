// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSTYPEREADER_P_H
#define QQMLJSTYPEREADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsscope_p.h"
#include "qqmljsimporter_p.h"

#include <QtQml/private/qqmljsastfwd_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>

#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class QQmlJSTypeReader
{
public:
    QQmlJSTypeReader(QQmlJSImporter *importer, const QString &file)
        : m_importer(importer)
        , m_file(file)
    {}

    bool operator()(const QSharedPointer<QQmlJSScope> &scope);
    QList<QQmlJS::DiagnosticMessage> errors() const { return m_errors; }

private:
    QQmlJSImporter *m_importer;
    QString m_file;
    QStringList m_qmldirFiles;
    QList<QQmlJS::DiagnosticMessage> m_errors;
};

QT_END_NAMESPACE

#endif // QQMLJSTYPEREADER_P_H
