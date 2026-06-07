// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QMLTYPESCREATOR_P_H
#define QMLTYPESCREATOR_P_H

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

#include "qqmltypesclassdescription_p.h"
#include "qqmljsstreamwriter_p.h"

#include <QtCore/qstring.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class QmlTypesCreator
{
public:
    QmlTypesCreator() : m_qml(&m_output) {}

    bool generate(const QString &outFileName);

    void setOwnTypes(QVector<MetaType> ownTypes) { m_ownTypes = std::move(ownTypes); }
    void setForeignTypes(QVector<MetaType> foreignTypes) { m_foreignTypes = std::move(foreignTypes); }
    void setReferencedTypes(QList<QAnyStringView> referencedTypes) { m_referencedTypes = std::move(referencedTypes); }
    void setModule(QByteArray module) { m_module = std::move(module); }
    void setVersion(QTypeRevision version) { m_version = version; }
    void setUsingDeclarations(QList<UsingDeclaration> usingDeclarations) { m_usingDeclarations = std::move(usingDeclarations);}
    void setGeneratingJSRoot(bool jsroot) { m_generatingJSRoot = jsroot; }

private:
    void writeComponent(const QmlTypesClassDescription &collector);
    void writeClassProperties(const QmlTypesClassDescription &collector);
    void writeType(QAnyStringView type);
    void writeProperties(const Property::Container &properties);
    void writeMethods(const Method::Container &methods, QLatin1StringView type);
    void writeEnums(const Enum::Container &enums);
    void writeComponents();
    void writeRootMethods(const MetaType &classDef);

    QByteArray m_output;
    QQmlJSStreamWriter m_qml;
    QVector<MetaType> m_ownTypes;
    QVector<MetaType> m_foreignTypes;
    QList<QAnyStringView> m_referencedTypes;
    QList<UsingDeclaration> m_usingDeclarations;
    QByteArray m_module;
    QTypeRevision m_version = QTypeRevision::zero();
    bool m_generatingJSRoot = false;
};

QT_END_NAMESPACE

#endif // QMLTYPESCREATOR_P_H
