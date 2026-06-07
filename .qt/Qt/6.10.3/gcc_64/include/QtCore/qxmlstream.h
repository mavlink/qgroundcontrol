// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QXMLSTREAM_H
#define QXMLSTREAM_H

#include <QtCore/qiodevice.h>

#if QT_CONFIG(xmlstream)

#include <QtCore/qcompare.h>
#include <QtCore/qlist.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qstring.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

class QXmlString {
    QStringPrivate m_string;
public:
    QXmlString(QStringPrivate &&d) : m_string(std::move(d)) {}
    QXmlString(const QString &s) : m_string(s.data_ptr()) {}
    QXmlString & operator=(const QString &s) { m_string = s.data_ptr(); return *this; }
    QXmlString & operator=(QString &&s) { m_string.swap(s.data_ptr()); return *this; }
    inline constexpr QXmlString() {}

    void swap(QXmlString &other) noexcept
    {
        m_string.swap(other.m_string);
    }

    operator QStringView() const noexcept { return QStringView(m_string.data(), m_string.size); }
    qsizetype size() const noexcept { return m_string.size; }
    bool isNull() const noexcept { return m_string.isNull(); }

private:
    friend bool comparesEqual(const QXmlString &lhs, const QXmlString &rhs) noexcept
    {
        return QStringView(lhs) == QStringView(rhs);
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QXmlString)
};

}
Q_DECLARE_SHARED_NS_EXT(QtPrivate, QXmlString)


class QXmlStreamReaderPrivate;
class QXmlStreamAttributes;
class Q_CORE_EXPORT QXmlStreamAttribute {
    QtPrivate::QXmlString m_name, m_namespaceUri, m_qualifiedName, m_value;
    uint m_isDefault : 1;
    friend class QXmlStreamReaderPrivate;
    friend class QXmlStreamAttributes;
public:
    QXmlStreamAttribute();
    QXmlStreamAttribute(const QString &qualifiedName, const QString &value);
    QXmlStreamAttribute(const QString &namespaceUri, const QString &name, const QString &value);

    inline QStringView namespaceUri() const { return m_namespaceUri; }
    inline QStringView name() const { return m_name; }
    inline QStringView qualifiedName() const { return m_qualifiedName; }
    inline QStringView prefix() const {
        return QStringView(m_qualifiedName).left(qMax(0, m_qualifiedName.size() - m_name.size() - 1));
    }
    inline QStringView value() const { return m_value; }
    inline bool isDefault() const { return m_isDefault; }
#if QT_CORE_REMOVED_SINCE(6, 8)
    inline bool operator==(const QXmlStreamAttribute &other) const
    { return comparesEqual(*this, other); }
    inline bool operator!=(const QXmlStreamAttribute &other) const
    { return !operator==(other); }
#endif

private:
    friend bool comparesEqual(const QXmlStreamAttribute &lhs,
                              const QXmlStreamAttribute &rhs) noexcept
    {
        if (lhs.m_value != rhs.m_value)
            return false;
        if (lhs.m_namespaceUri.isNull())
            return lhs.m_qualifiedName == rhs.m_qualifiedName;
        return lhs.m_namespaceUri == rhs.m_namespaceUri
            && lhs.m_name == rhs.m_name;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QXmlStreamAttribute)
};

Q_DECLARE_TYPEINFO(QXmlStreamAttribute, Q_RELOCATABLE_TYPE);

// We export each out-of-line method individually to prevent MSVC from
// exporting the whole QList class.
class QXmlStreamAttributes : public QList<QXmlStreamAttribute>
{
public:
    inline QXmlStreamAttributes() {}
#if QT_CORE_REMOVED_SINCE(6, 6)
    Q_CORE_EXPORT QStringView value(const QString &namespaceUri, const QString &name) const;
    Q_CORE_EXPORT QStringView value(const QString &namespaceUri, QLatin1StringView name) const;
    Q_CORE_EXPORT QStringView value(QLatin1StringView namespaceUri, QLatin1StringView name) const;
    Q_CORE_EXPORT QStringView value(const QString &qualifiedName) const;
    Q_CORE_EXPORT QStringView value(QLatin1StringView qualifiedName) const;
#endif
    Q_CORE_EXPORT QStringView value(QAnyStringView namespaceUri, QAnyStringView name) const noexcept;
    Q_CORE_EXPORT QStringView value(QAnyStringView qualifiedName) const noexcept;

    Q_CORE_EXPORT void append(const QString &namespaceUri, const QString &name, const QString &value);
    Q_CORE_EXPORT void append(const QString &qualifiedName, const QString &value);

    bool hasAttribute(QAnyStringView qualifiedName) const
    {
        return !value(qualifiedName).isNull();
    }

    bool hasAttribute(QAnyStringView namespaceUri, QAnyStringView name) const
    {
        return !value(namespaceUri, name).isNull();
    }

    using QList<QXmlStreamAttribute>::append;
};

class Q_CORE_EXPORT QXmlStreamNamespaceDeclaration {
    QtPrivate::QXmlString m_prefix, m_namespaceUri;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamNamespaceDeclaration();
    QXmlStreamNamespaceDeclaration(const QString &prefix, const QString &namespaceUri);

    inline QStringView prefix() const { return m_prefix; }
    inline QStringView namespaceUri() const { return m_namespaceUri; }
#if QT_CORE_REMOVED_SINCE(6, 8)
    inline bool operator==(const QXmlStreamNamespaceDeclaration &other) const
    { return comparesEqual(*this, other); }
    inline bool operator!=(const QXmlStreamNamespaceDeclaration &other) const
    { return !operator==(other); }
#endif
private:
    friend bool comparesEqual(const QXmlStreamNamespaceDeclaration &lhs,
                              const QXmlStreamNamespaceDeclaration &rhs) noexcept
    {
        return lhs.m_prefix == rhs.m_prefix
            && lhs.m_namespaceUri == rhs.m_namespaceUri;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QXmlStreamNamespaceDeclaration)
};

Q_DECLARE_TYPEINFO(QXmlStreamNamespaceDeclaration, Q_RELOCATABLE_TYPE);
typedef QList<QXmlStreamNamespaceDeclaration> QXmlStreamNamespaceDeclarations;

class Q_CORE_EXPORT QXmlStreamNotationDeclaration {
    QtPrivate::QXmlString m_name, m_systemId, m_publicId;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamNotationDeclaration();

    inline QStringView name() const { return m_name; }
    inline QStringView systemId() const { return m_systemId; }
    inline QStringView publicId() const { return m_publicId; }
#if QT_CORE_REMOVED_SINCE(6, 8)
    inline bool operator==(const QXmlStreamNotationDeclaration &other) const
    { return comparesEqual(*this, other); }
    inline bool operator!=(const QXmlStreamNotationDeclaration &other) const
    { return !operator==(other); }
#endif
private:
    friend bool comparesEqual(const QXmlStreamNotationDeclaration &lhs,
                              const QXmlStreamNotationDeclaration &rhs) noexcept
    {
        return lhs.m_name == rhs.m_name && lhs.m_systemId == rhs.m_systemId
                && lhs.m_publicId == rhs.m_publicId;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QXmlStreamNotationDeclaration)
};

Q_DECLARE_TYPEINFO(QXmlStreamNotationDeclaration, Q_RELOCATABLE_TYPE);
typedef QList<QXmlStreamNotationDeclaration> QXmlStreamNotationDeclarations;

class Q_CORE_EXPORT QXmlStreamEntityDeclaration {
    QtPrivate::QXmlString m_name, m_notationName, m_systemId, m_publicId, m_value;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamEntityDeclaration();

    inline QStringView name() const { return m_name; }
    inline QStringView notationName() const { return m_notationName; }
    inline QStringView systemId() const { return m_systemId; }
    inline QStringView publicId() const { return m_publicId; }
    inline QStringView value() const { return m_value; }
#if QT_CORE_REMOVED_SINCE(6, 8)
    inline bool operator==(const QXmlStreamEntityDeclaration &other) const
    { return comparesEqual(*this, other); }
    inline bool operator!=(const QXmlStreamEntityDeclaration &other) const
    { return !operator==(other); }
#endif

private:
    friend bool comparesEqual(const QXmlStreamEntityDeclaration &lhs,
                              const QXmlStreamEntityDeclaration &rhs) noexcept
    {
        return lhs.m_name == rhs.m_name
            && lhs.m_notationName == rhs.m_notationName
            && lhs.m_systemId == rhs.m_systemId
            && lhs.m_publicId == rhs.m_publicId
            && lhs.m_value == rhs.m_value;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(QXmlStreamEntityDeclaration)
};

Q_DECLARE_TYPEINFO(QXmlStreamEntityDeclaration, Q_RELOCATABLE_TYPE);
typedef QList<QXmlStreamEntityDeclaration> QXmlStreamEntityDeclarations;

class Q_CORE_EXPORT QXmlStreamEntityResolver
{
    Q_DISABLE_COPY_MOVE(QXmlStreamEntityResolver)
public:
    QXmlStreamEntityResolver() = default;
    virtual ~QXmlStreamEntityResolver();
    virtual QString resolveEntity(const QString& publicId, const QString& systemId);
    virtual QString resolveUndeclaredEntity(const QString &name);
};

#if QT_CONFIG(xmlstreamreader)
class Q_CORE_EXPORT QXmlStreamReader
{
    QDOC_PROPERTY(bool namespaceProcessing READ namespaceProcessing WRITE setNamespaceProcessing)
public:
    enum TokenType {
        NoToken = 0,
        Invalid,
        StartDocument,
        EndDocument,
        StartElement,
        EndElement,
        Characters,
        Comment,
        DTD,
        EntityReference,
        ProcessingInstruction
    };


    QXmlStreamReader();
    explicit QXmlStreamReader(QIODevice *device);
#if QT_CORE_REMOVED_SINCE(6, 5)
    explicit QXmlStreamReader(const QByteArray &data);
    explicit QXmlStreamReader(const QString &data);
    explicit QXmlStreamReader(const char * data);
#endif // QT_CORE_REMOVED_SINCE(6, 5)
    Q_WEAK_OVERLOAD
    explicit QXmlStreamReader(const QByteArray &data)
        : QXmlStreamReader(data, PrivateConstructorTag{}) { }
    explicit QXmlStreamReader(QAnyStringView data);
    ~QXmlStreamReader();

    void setDevice(QIODevice *device);
    QIODevice *device() const;
#if QT_CORE_REMOVED_SINCE(6, 5)
    void addData(const QByteArray &data);
    void addData(const QString &data);
    void addData(const char *data);
#endif // QT_CORE_REMOVED_SINCE(6, 5)
    Q_WEAK_OVERLOAD
    void addData(const QByteArray &data) { addDataImpl(data); }
    void addData(QAnyStringView data);
    void clear();


    bool atEnd() const;
    TokenType readNext();

    bool readNextStartElement();
    void skipCurrentElement();
    QString readRawInnerData();

    TokenType tokenType() const;
    QString tokenString() const;

    void setNamespaceProcessing(bool);
    bool namespaceProcessing() const;

    inline bool isStartDocument() const { return tokenType() == StartDocument; }
    inline bool isEndDocument() const { return tokenType() == EndDocument; }
    inline bool isStartElement() const { return tokenType() == StartElement; }
    inline bool isEndElement() const { return tokenType() == EndElement; }
    inline bool isCharacters() const { return tokenType() == Characters; }
    bool isWhitespace() const;
    bool isCDATA() const;
    inline bool isComment() const { return tokenType() == Comment; }
    inline bool isDTD() const { return tokenType() == DTD; }
    inline bool isEntityReference() const { return tokenType() == EntityReference; }
    inline bool isProcessingInstruction() const { return tokenType() == ProcessingInstruction; }

    bool isStandaloneDocument() const;
    bool hasStandaloneDeclaration() const;
    QStringView documentVersion() const;
    QStringView documentEncoding() const;

    qint64 lineNumber() const;
    qint64 columnNumber() const;
    qint64 characterOffset() const;

    QXmlStreamAttributes attributes() const;

    enum ReadElementTextBehaviour {
        ErrorOnUnexpectedElement,
        IncludeChildElements,
        SkipChildElements
    };
    QString readElementText(ReadElementTextBehaviour behaviour = ErrorOnUnexpectedElement);

    QStringView name() const;
    QStringView namespaceUri() const;
    QStringView qualifiedName() const;
    QStringView prefix() const;

    QStringView processingInstructionTarget() const;
    QStringView processingInstructionData() const;

    QStringView text() const;

    QXmlStreamNamespaceDeclarations namespaceDeclarations() const;
    void addExtraNamespaceDeclaration(const QXmlStreamNamespaceDeclaration &extraNamespaceDeclaraction);
    void addExtraNamespaceDeclarations(const QXmlStreamNamespaceDeclarations &extraNamespaceDeclaractions);
    QXmlStreamNotationDeclarations notationDeclarations() const;
    QXmlStreamEntityDeclarations entityDeclarations() const;
    QStringView dtdName() const;
    QStringView dtdPublicId() const;
    QStringView dtdSystemId() const;

    int entityExpansionLimit() const;
    void setEntityExpansionLimit(int limit);

    enum Error {
        NoError,
        UnexpectedElementError,
        CustomError,
        NotWellFormedError,
        PrematureEndOfDocumentError
    };
    void raiseError(const QString& message = QString());
    QString errorString() const;
    Error error() const;

    inline bool hasError() const
    {
        return error() != NoError;
    }

    void setEntityResolver(QXmlStreamEntityResolver *resolver);
    QXmlStreamEntityResolver *entityResolver() const;

private:
    struct PrivateConstructorTag { };
    QXmlStreamReader(const QByteArray &data, PrivateConstructorTag);
    void addDataImpl(const QByteArray &data);

    Q_DISABLE_COPY(QXmlStreamReader)
    Q_DECLARE_PRIVATE(QXmlStreamReader)
    std::unique_ptr<QXmlStreamReaderPrivate> d_ptr;

};
#endif // feature xmlstreamreader

#if QT_CONFIG(xmlstreamwriter)

class QXmlStreamWriterPrivate;

class Q_CORE_EXPORT QXmlStreamWriter
{
    QDOC_PROPERTY(bool autoFormatting READ autoFormatting WRITE setAutoFormatting)
    QDOC_PROPERTY(int autoFormattingIndent READ autoFormattingIndent WRITE setAutoFormattingIndent)
    QDOC_PROPERTY(bool stopWritingOnError READ stopWritingOnError WRITE setStopWritingOnError)
public:
    QXmlStreamWriter();
    explicit QXmlStreamWriter(QIODevice *device);
    explicit QXmlStreamWriter(QByteArray *array);
    explicit QXmlStreamWriter(QString *string);
    ~QXmlStreamWriter();

    void setDevice(QIODevice *device);
    QIODevice *device() const;

    void setAutoFormatting(bool);
    bool autoFormatting() const;

    void setAutoFormattingIndent(int spacesOrTabs);
    int autoFormattingIndent() const;

    void setStopWritingOnError(bool stop);
    bool stopWritingOnError() const;

#if QT_CORE_REMOVED_SINCE(6,5)
    void writeAttribute(const QString &qualifiedName, const QString &value);
    void writeAttribute(const QString &namespaceUri, const QString &name, const QString &value);
#endif
    void writeAttribute(QAnyStringView qualifiedName, QAnyStringView value);
    void writeAttribute(QAnyStringView namespaceUri, QAnyStringView name, QAnyStringView value);

    void writeAttribute(const QXmlStreamAttribute& attribute);
    void writeAttributes(const QXmlStreamAttributes& attributes);

#if QT_CORE_REMOVED_SINCE(6,5)
    void writeCDATA(const QString &text);
    void writeCharacters(const QString &text);
    void writeComment(const QString &text);

    void writeDTD(const QString &dtd);

    void writeEmptyElement(const QString &qualifiedName);
    void writeEmptyElement(const QString &namespaceUri, const QString &name);

    void writeTextElement(const QString &qualifiedName, const QString &text);
    void writeTextElement(const QString &namespaceUri, const QString &name, const QString &text);
#endif
    void writeCDATA(QAnyStringView text);
    void writeCharacters(QAnyStringView text);
    void writeComment(QAnyStringView text);

    void writeDTD(QAnyStringView dtd);

    void writeEmptyElement(QAnyStringView qualifiedName);
    void writeEmptyElement(QAnyStringView namespaceUri, QAnyStringView name);

    void writeTextElement(QAnyStringView qualifiedName, QAnyStringView text);
    void writeTextElement(QAnyStringView namespaceUri, QAnyStringView name, QAnyStringView text);


    void writeEndDocument();
    void writeEndElement();

#if QT_CORE_REMOVED_SINCE(6,5)
    void writeEntityReference(const QString &name);
    void writeNamespace(const QString &namespaceUri, const QString &prefix);
    void writeDefaultNamespace(const QString &namespaceUri);
    void writeProcessingInstruction(const QString &target, const QString &data);
#endif
    void writeEntityReference(QAnyStringView name);
    void writeNamespace(QAnyStringView namespaceUri, QAnyStringView prefix = {});
    void writeDefaultNamespace(QAnyStringView namespaceUri);
    void writeProcessingInstruction(QAnyStringView target, QAnyStringView data = {});

    void writeStartDocument();
#if QT_CORE_REMOVED_SINCE(6,5)
    void writeStartDocument(const QString &version);
    void writeStartDocument(const QString &version, bool standalone);
    void writeStartElement(const QString &qualifiedName);
    void writeStartElement(const QString &namespaceUri, const QString &name);
#endif
    void writeStartDocument(QAnyStringView version);
    void writeStartDocument(QAnyStringView version, bool standalone);
    void writeStartElement(QAnyStringView qualifiedName);
    void writeStartElement(QAnyStringView namespaceUri, QAnyStringView name);

#if QT_CONFIG(xmlstreamreader)
    void writeCurrentToken(const QXmlStreamReader &reader);
#endif

    enum class Error {
        None,
        IO,
        Encoding,
        InvalidCharacter,
        Custom,
    };

    void raiseError(QAnyStringView message);
    QString errorString() const;
    Error error() const;
    bool hasError() const;

private:
    Q_DISABLE_COPY(QXmlStreamWriter)
    Q_DECLARE_PRIVATE(QXmlStreamWriter)
    std::unique_ptr<QXmlStreamWriterPrivate> d_ptr;
};
#endif // feature xmlstreamwriter

QT_END_NAMESPACE

#endif // feature xmlstream

#endif // QXMLSTREAM_H
