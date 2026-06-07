// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSSTREAMWRITER_P_H
#define QQMLJSSTREAMWRITER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/QIODevice>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QScopedPointer>
#include <utility>

QT_BEGIN_NAMESPACE

class QQmlJSStreamWriter
{
public:
    QQmlJSStreamWriter(QByteArray *array);

    void writeStartDocument();
    void writeEndDocument();
    void writeLibraryImport(
        QByteArrayView uri, int majorVersion, int minorVersion, QByteArrayView as = {});
    void writeStartObject(QByteArrayView component);
    void writeEndObject();
    void writeScriptBinding(QByteArrayView name, QByteArrayView rhs);
    void writeStringBinding(QByteArrayView name, QAnyStringView value);
    void writeNumberBinding(QByteArrayView name, qint64 value);

    // TODO: Drop this once we can drop qmlplugindump. It is substantially weird.
    void writeEnumObjectLiteralBinding(
        QByteArrayView name, const QList<std::pair<QAnyStringView, int>> &keyValue);

    // TODO: these would look better with generator functions.
    void writeArrayBinding(QByteArrayView name, const QByteArrayList &elements);
    void writeStringListBinding(QByteArrayView name, const QList<QAnyStringView> &elements);

    void write(QByteArrayView data);
    void writeBooleanBinding(QByteArrayView name, bool value);

private:
    void writeIndent();
    void writePotentialLine(const QByteArray &line);
    void flushPotentialLinesWithNewlines();

    template<typename String, typename ElementHandler>
    void doWriteArrayBinding(
            QByteArrayView name, const QList<String> &elements, ElementHandler &&handler);

    int m_indentDepth;
    QList<QByteArray> m_pendingLines;
    int m_pendingLineLength;
    bool m_maybeOneline;
    QScopedPointer<QIODevice> m_stream;
};

QT_END_NAMESPACE

#endif // QQMLJSSTREAMWRITER_P_H
