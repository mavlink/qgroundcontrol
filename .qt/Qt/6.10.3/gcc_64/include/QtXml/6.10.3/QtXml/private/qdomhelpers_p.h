// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QDOMHELPERS_P_H
#define QDOMHELPERS_P_H

#include <qcoreapplication.h>
#include <qdom.h>
#include <private/qglobal_p.h>

QT_REQUIRE_CONFIG(dom);
QT_BEGIN_NAMESPACE

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qxml.cpp and qdom.cpp. This header file may change from version to version without
// notice, or even be removed.
//
// We mean it.
//

class QDomDocumentPrivate;
class QDomNodePrivate;
class QXmlStreamReader;
class QXmlStreamAttributes;

/**************************************************************
 *
 * QDomBuilder
 *
 **************************************************************/

class QDomBuilder
{
public:
    QDomBuilder(QDomDocumentPrivate *d, QXmlStreamReader *r, QDomDocument::ParseOptions options);
    ~QDomBuilder();

    bool endDocument();
    bool startElement(const QString &nsURI, const QString &qName, const QXmlStreamAttributes &atts);
    bool endElement();
    bool characters(const QString &characters, bool cdata = false);
    bool processingInstruction(const QString &target, const QString &data);
    bool skippedEntity(const QString &name);
    bool startEntity(const QString &name);
    bool endEntity();
    bool startDTD(const QString &name, const QString &publicId, const QString &systemId);
    bool parseDTD(const QString &dtd);
    bool comment(const QString &characters);
    bool externalEntityDecl(const QString &name, const QString &publicId, const QString &systemId);
    bool notationDecl(const QString &name, const QString &publicId, const QString &systemId);
    bool unparsedEntityDecl(const QString &name, const QString &publicId, const QString &systemId,
                            const QString &notationName);

    void fatalError(const QString &message);
    QDomDocument::ParseResult result() const { return parseResult; }

    bool preserveSpacingOnlyNodes() const
    { return parseOptions & QDomDocument::ParseOption::PreserveSpacingOnlyNodes; }

private:
    QString dtdInternalSubset(const QString &dtd);

    QDomDocument::ParseResult parseResult;
    QDomDocumentPrivate *doc;
    QDomNodePrivate *node;
    QXmlStreamReader *reader;
    QString entityName;
    QDomDocument::ParseOptions parseOptions;
};

/**************************************************************
 *
 * QDomParser
 *
 **************************************************************/

class QDomParser
{
    Q_DECLARE_TR_FUNCTIONS(QDomParser)
public:
    QDomParser(QDomDocumentPrivate *d, QXmlStreamReader *r, QDomDocument::ParseOptions options);

    bool parse();
    QDomDocument::ParseResult result() const { return domBuilder.result(); }

private:
    bool parseProlog();
    bool parseBody();
    bool parseMarkupDecl();

    QXmlStreamReader *reader;
    QDomBuilder domBuilder;
};

QT_END_NAMESPACE

#endif // QDOMHELPERS_P_H
