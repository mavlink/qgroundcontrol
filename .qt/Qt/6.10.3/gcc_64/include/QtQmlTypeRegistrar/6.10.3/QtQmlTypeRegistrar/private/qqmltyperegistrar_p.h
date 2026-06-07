// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QMLTYPEREGISTRAR_P_H
#define QMLTYPEREGISTRAR_P_H

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

#include <QtCore/qcbormap.h>
#include <QtCore/qversionnumber.h>

#include <cstdlib>

#include "qmetatypesjsonprocessor_p.h"

QT_BEGIN_NAMESPACE

class QmlTypeRegistrar
{
    QString m_module;
    QString m_targetNamespace;
    QTypeRevision m_moduleVersion;
    QList<quint8> m_pastMajorVersions;
    QList<QString> m_includes;
    bool m_followForeignVersioning = false;
    QVector<MetaType> m_types;
    QVector<MetaType> m_foreignTypes;
    QList<QAnyStringView> m_referencedTypes;
    QList<UsingDeclaration> m_usingDeclarations;

    MetaType findType(QAnyStringView name) const;
    MetaType findTypeForeign(QAnyStringView name) const;

public:
    void write(QTextStream &os, QAnyStringView outFileName) const;
    bool generatePluginTypes(const QString &pluginTypesFile, bool generatingJSRoot = false);
    void setModuleNameAndNamespace(const QString &module, const QString &targetNamespace);
    void setModuleVersions(QTypeRevision moduleVersion, const QList<quint8> &pastMajorVersions,
                           bool followForeignVersioning);
    void setIncludes(const QList<QString> &includes);
    void setTypes(const QVector<MetaType> &types, const QVector<MetaType> &foreignTypes);
    void setReferencedTypes(const QList<QAnyStringView> &referencedTypes);
    void setUsingDeclarations(const QList<UsingDeclaration> &usingDeclarations);

    static bool argumentsFromCommandLineAndFile(QStringList &allArguments,
                                                const QStringList &arguments);
    static int runExtract(
            const QString &baseName, const QString &nameSpace,
            const MetaTypesJsonProcessor &processor);
};

QT_END_NAMESPACE
#endif // QMLTYPEREGISTRAR_P_H
