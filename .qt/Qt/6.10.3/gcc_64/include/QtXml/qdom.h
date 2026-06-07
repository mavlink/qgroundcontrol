// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDOM_H
#define QDOM_H

#include <QtXml/qtxmlglobal.h>

#include <QtCore/qcompare.h>
#include <QtCore/qcontainertools_impl.h>
#include <QtCore/qstring.h>

#include <iterator>

#if QT_CONFIG(dom)

class tst_QDom;

QT_BEGIN_NAMESPACE

class QIODevice;
class QTextStream;

class QDomDocumentPrivate;
class QDomDocumentTypePrivate;
class QDomDocumentFragmentPrivate;
class QDomNodePrivate;
class QDomNodeListPrivate;
class QDomImplementationPrivate;
class QDomElementPrivate;
class QDomNotationPrivate;
class QDomEntityPrivate;
class QDomEntityReferencePrivate;
class QDomProcessingInstructionPrivate;
class QDomAttrPrivate;
class QDomCharacterDataPrivate;
class QDomTextPrivate;
class QDomCommentPrivate;
class QDomCDATASectionPrivate;
class QDomNamedNodeMapPrivate;
class QDomImplementationPrivate;

class QDomNodeList;
class QDomElement;
class QDomText;
class QDomComment;
class QDomCDATASection;
class QDomProcessingInstruction;
class QDomAttr;
class QDomEntityReference;
class QDomDocument;
class QDomNamedNodeMap;
class QDomDocument;
class QDomDocumentFragment;
class QDomDocumentType;
class QDomImplementation;
class QDomNode;
class QDomEntity;
class QDomNotation;
class QDomCharacterData;
class QXmlStreamReader;

class Q_XML_EXPORT QDomImplementation
{
public:
    QDomImplementation();
    QDomImplementation(const QDomImplementation &implementation);
    ~QDomImplementation();
    QDomImplementation& operator=(const QDomImplementation &other);
    bool operator==(const QDomImplementation &other) const;
    bool operator!=(const QDomImplementation &other) const;

    // functions
    bool hasFeature(const QString& feature, const QString& version) const;
    QDomDocumentType createDocumentType(const QString& qName, const QString& publicId, const QString& systemId);
    QDomDocument createDocument(const QString& nsURI, const QString& qName, const QDomDocumentType& doctype);

    enum InvalidDataPolicy { AcceptInvalidChars = 0, DropInvalidChars, ReturnNullNode };
    static InvalidDataPolicy invalidDataPolicy();
    static void setInvalidDataPolicy(InvalidDataPolicy policy);

    // Qt extension
    bool isNull();

private:
    QDomImplementationPrivate* impl;
    QDomImplementation(QDomImplementationPrivate*);

    friend class QDomDocument;
};

class Q_XML_EXPORT QDomNode
{
public:
    enum NodeType {
        ElementNode               = 1,
        AttributeNode             = 2,
        TextNode                  = 3,
        CDATASectionNode          = 4,
        EntityReferenceNode       = 5,
        EntityNode                = 6,
        ProcessingInstructionNode = 7,
        CommentNode               = 8,
        DocumentNode              = 9,
        DocumentTypeNode          = 10,
        DocumentFragmentNode      = 11,
        NotationNode              = 12,
        BaseNode                  = 21,// this is not in the standard
        CharacterDataNode         = 22 // this is not in the standard
    };

    enum EncodingPolicy
    {
        EncodingFromDocument      = 1,
        EncodingFromTextStream    = 2
    };

    QDomNode();
    QDomNode(const QDomNode &node);
    QDomNode& operator=(const QDomNode &other);
    bool operator==(const QDomNode &other) const;
    bool operator!=(const QDomNode &other) const;
    ~QDomNode();

    // DOM functions
    QDomNode insertBefore(const QDomNode& newChild, const QDomNode& refChild);
    QDomNode insertAfter(const QDomNode& newChild, const QDomNode& refChild);
    QDomNode replaceChild(const QDomNode& newChild, const QDomNode& oldChild);
    QDomNode removeChild(const QDomNode& oldChild);
    QDomNode appendChild(const QDomNode& newChild);
    bool hasChildNodes() const;
    QDomNode cloneNode(bool deep = true) const;
    void normalize();
    bool isSupported(const QString& feature, const QString& version) const;

    // DOM read-only attributes
    QString nodeName() const;
    NodeType nodeType() const;
    QDomNode parentNode() const;
    QDomNodeList childNodes() const;
    QDomNode firstChild() const;
    QDomNode lastChild() const;
    QDomNode previousSibling() const;
    QDomNode nextSibling() const;
    QDomNamedNodeMap attributes() const;
    QDomDocument ownerDocument() const;
    QString namespaceURI() const;
    QString localName() const;
    bool hasAttributes() const;

    // DOM attributes
    QString nodeValue() const;
    void setNodeValue(const QString &value);
    QString prefix() const;
    void setPrefix(const QString& pre);

    // Qt extensions
    bool isAttr() const;
    bool isCDATASection() const;
    bool isDocumentFragment() const;
    bool isDocument() const;
    bool isDocumentType() const;
    bool isElement() const;
    bool isEntityReference() const;
    bool isText() const;
    bool isEntity() const;
    bool isNotation() const;
    bool isProcessingInstruction() const;
    bool isCharacterData() const;
    bool isComment() const;

    /**
     * Shortcut to avoid dealing with QDomNodeList
     * all the time.
     */
    QDomNode namedItem(const QString& name) const;

    bool isNull() const;
    void clear();

    QDomAttr toAttr() const;
    QDomCDATASection toCDATASection() const;
    QDomDocumentFragment toDocumentFragment() const;
    QDomDocument toDocument() const;
    QDomDocumentType toDocumentType() const;
    QDomElement toElement() const;
    QDomEntityReference toEntityReference() const;
    QDomText toText() const;
    QDomEntity toEntity() const;
    QDomNotation toNotation() const;
    QDomProcessingInstruction toProcessingInstruction() const;
    QDomCharacterData toCharacterData() const;
    QDomComment toComment() const;

    void save(QTextStream&, int, EncodingPolicy=QDomNode::EncodingFromDocument) const;

    QDomElement firstChildElement(const QString &tagName = QString(), const QString &namespaceURI = QString()) const;
    QDomElement lastChildElement(const QString &tagName = QString(), const QString &namespaceURI = QString()) const;
    QDomElement previousSiblingElement(const QString &tagName = QString(), const QString &namespaceURI = QString()) const;
    QDomElement nextSiblingElement(const QString &taName = QString(), const QString &namespaceURI = QString()) const;

    int lineNumber() const;
    int columnNumber() const;

protected:
    QDomNodePrivate* impl;
    QDomNode(QDomNodePrivate*);

private:
    friend class ::tst_QDom;
    friend class QDomDocument;
    friend class QDomDocumentType;
    friend class QDomNodeList;
    friend class QDomNamedNodeMap;
};

class Q_XML_EXPORT QDomNodeList
{
public:
    QDomNodeList();
    QDomNodeList(const QDomNodeList &nodeList);
    QDomNodeList& operator=(const QDomNodeList &other);
#if QT_XML_REMOVED_SINCE(6, 9)
    bool operator==(const QDomNodeList &other) const;
    bool operator!=(const QDomNodeList &other) const;
#endif
    ~QDomNodeList();

    // DOM functions
    QDomNode item(int index) const;
    inline QDomNode at(int index) const { return item(index); } // Qt API consistency

    // DOM read only attributes
    int length() const;
    inline int count() const { return length(); } // Qt API consitancy
    inline int size() const { return length(); } // Qt API consistency
    inline bool isEmpty() const { return length() == 0; } // Qt API consistency

private:
    Q_XML_EXPORT friend bool comparesEqual(const QDomNodeList &lhs, const QDomNodeList &rhs) noexcept;
    Q_DECLARE_EQUALITY_COMPARABLE(QDomNodeList)

    int noexceptLength() const noexcept;

    class It
    {
        const QDomNodeListPrivate *parent;
        QDomNodePrivate *current;

        friend class QDomNodeList;
        friend class QDomNodeListPrivate;
        Q_XML_EXPORT It(const QDomNodeListPrivate *lp, bool start) noexcept;

        friend constexpr bool comparesEqual(const It &lhs, const It &rhs)
        { Q_ASSERT(lhs.parent == rhs.parent); return lhs.current == rhs.current; }
        Q_DECLARE_EQUALITY_COMPARABLE_NON_NOEXCEPT(It);

        Q_XML_EXPORT static QDomNodePrivate *findNextInOrder(const QDomNodeListPrivate *parent, QDomNodePrivate *current);
        Q_XML_EXPORT static QDomNodePrivate *findPrevInOrder(const QDomNodeListPrivate *parent, QDomNodePrivate *current);

    public:
        // Rule Of Zero applies
        It() = default;

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = QDomNode;
        using element_type = const QDomNode;
        using difference_type = qptrdiff; // difference to [container.reqmts]
        using reference = value_type;     // difference to [container.reqmts]
        using pointer = QtPrivate::ArrowProxy<reference>;

        reference operator*() const { return QDomNode(current); }
        pointer operator->() const { return { **this }; }

        It &operator++() { current = findNextInOrder(parent, current); return *this; }
        It operator++(int) { auto copy = *this; ++*this; return copy; }

        It &operator--() { current = findPrevInOrder(parent, current); return *this; }
        It operator--(int) { auto copy = *this; --*this; return copy; }
    };

public:
    using const_iterator = It;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    using value_type = It::value_type;
    using difference_type = It::difference_type;
    using reference = It::reference;
    using const_reference = reference;
    using pointer = It::pointer;
    using const_pointer = pointer;

    [[nodiscard]] const_iterator begin()  const noexcept { return It{impl, true}; }
    [[nodiscard]] const_iterator end()    const noexcept { return It{impl, false}; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend()   const noexcept { return end(); }

    [[nodiscard]] const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator{end()}; }
    [[nodiscard]] const_reverse_iterator rend()    const noexcept { return const_reverse_iterator{begin()}; }
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    [[nodiscard]] const_reverse_iterator crend()   const noexcept { return rend(); }

    [[nodiscard]] const_iterator constBegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator constEnd()   const noexcept { return end(); }

private:
    QDomNodeListPrivate* impl;
    QDomNodeList(QDomNodeListPrivate*);

    friend class QDomNode;
    friend class QDomElement;
    friend class QDomDocument;
    friend class ::tst_QDom;
};

class Q_XML_EXPORT QDomDocumentType : public QDomNode
{
public:
    QDomDocumentType();
    QDomDocumentType(const QDomDocumentType &documentType);
    QDomDocumentType& operator=(const QDomDocumentType &other);

    // DOM read only attributes
    QString name() const;
    QDomNamedNodeMap entities() const;
    QDomNamedNodeMap notations() const;
    QString publicId() const;
    QString systemId() const;
    QString internalSubset() const;

    // Overridden from QDomNode
    inline QDomNode::NodeType nodeType() const { return DocumentTypeNode; }

private:
    QDomDocumentType(QDomDocumentTypePrivate*);

    friend class QDomImplementation;
    friend class QDomDocument;
    friend class QDomNode;
    friend class ::tst_QDom;
};

class Q_XML_EXPORT QDomDocument : public QDomNode
{
public:
    enum class ParseOption {
        Default = 0x00,
        UseNamespaceProcessing = 0x01,
        PreserveSpacingOnlyNodes = 0x02,
    };
    Q_DECLARE_FLAGS(ParseOptions, ParseOption)

    struct ParseResult
    {
        QString errorMessage;
        qsizetype errorLine = 0;
        qsizetype errorColumn = 0;

        explicit operator bool() const noexcept { return errorMessage.isEmpty(); }
    };

    QDomDocument();
    explicit QDomDocument(const QString& name);
    explicit QDomDocument(const QDomDocumentType& doctype);
    QDomDocument(const QDomDocument &document);
    QDomDocument& operator=(const QDomDocument &other);
    ~QDomDocument();

    // DOM functions
    QDomElement createElement(const QString& tagName);
    QDomDocumentFragment createDocumentFragment();
    QDomText createTextNode(const QString& data);
    QDomComment createComment(const QString& data);
    QDomCDATASection createCDATASection(const QString& data);
    QDomProcessingInstruction createProcessingInstruction(const QString& target, const QString& data);
    QDomAttr createAttribute(const QString& name);
    QDomEntityReference createEntityReference(const QString& name);
    QDomNodeList elementsByTagName(const QString& tagname) const;
    QDomNode importNode(const QDomNode& importedNode, bool deep);
    QDomElement createElementNS(const QString& nsURI, const QString& qName);
    QDomAttr createAttributeNS(const QString& nsURI, const QString& qName);
    QDomNodeList elementsByTagNameNS(const QString& nsURI, const QString& localName);
    QDomElement elementById(const QString& elementId);

    // DOM read only attributes
    QDomDocumentType doctype() const;
    QDomImplementation implementation() const;
    QDomElement documentElement() const;

    // Overridden from QDomNode
    inline QDomNode::NodeType nodeType() const { return DocumentNode; }

    // Qt extensions
#if QT_DEPRECATED_SINCE(6, 8)
    QT_DEPRECATED_VERSION_X_6_8("Use the overload taking ParseOptions instead.")
    bool setContent(const QByteArray &text, bool namespaceProcessing, QString *errorMsg = nullptr, int *errorLine = nullptr, int *errorColumn = nullptr);
    QT_DEPRECATED_VERSION_X_6_8("Use the overload taking ParseOptions instead.")
    bool setContent(const QString &text, bool namespaceProcessing, QString *errorMsg = nullptr, int *errorLine = nullptr, int *errorColumn = nullptr);
    QT_DEPRECATED_VERSION_X_6_8("Use the overload taking ParseOptions instead.")
    bool setContent(QIODevice *dev, bool namespaceProcessing, QString *errorMsg = nullptr, int *errorLine = nullptr, int *errorColumn = nullptr);
    QT_DEPRECATED_VERSION_X_6_8("Use the overload returning ParseResult instead.")
    bool setContent(const QByteArray &text, QString *errorMsg, int *errorLine = nullptr, int *errorColumn = nullptr);
    QT_DEPRECATED_VERSION_X_6_8("Use the overload returning ParseResult instead.")
    bool setContent(const QString &text, QString *errorMsg, int *errorLine = nullptr, int *errorColumn = nullptr);
    QT_DEPRECATED_VERSION_X_6_8("Use the overload returning ParseResult instead.")
    bool setContent(QIODevice *dev, QString *errorMsg, int *errorLine = nullptr, int *errorColumn = nullptr);
    QT_DEPRECATED_VERSION_X_6_8("Use the overload taking ParseOptions instead.")
    bool setContent(QXmlStreamReader *reader, bool namespaceProcessing, QString *errorMsg = nullptr,
                    int *errorLine = nullptr, int *errorColumn = nullptr);
#endif // QT_DEPRECATED_SINCE(6, 8)

    Q_WEAK_OVERLOAD
    ParseResult setContent(const QByteArray &data, ParseOptions options = ParseOption::Default)
    { return setContentImpl(data, options); }
    ParseResult setContent(QAnyStringView data, ParseOptions options = ParseOption::Default);
    ParseResult setContent(QIODevice *device, ParseOptions options = ParseOption::Default);
    ParseResult setContent(QXmlStreamReader *reader, ParseOptions options = ParseOption::Default);

    // Qt extensions
    QString toString(int indent = 1) const;
    QByteArray toByteArray(int indent = 1) const;

private:
    ParseResult setContentImpl(const QByteArray &data, ParseOptions options);

    QDomDocument(QDomDocumentPrivate*);

    friend class QDomNode;
};

class Q_XML_EXPORT QDomNamedNodeMap
{
public:
    QDomNamedNodeMap();
    QDomNamedNodeMap(const QDomNamedNodeMap &namedNodeMap);
    QDomNamedNodeMap& operator=(const QDomNamedNodeMap &other);
    bool operator==(const QDomNamedNodeMap &other) const;
    bool operator!=(const QDomNamedNodeMap &other) const;
    ~QDomNamedNodeMap();

    // DOM functions
    QDomNode namedItem(const QString& name) const;
    QDomNode setNamedItem(const QDomNode& newNode);
    QDomNode removeNamedItem(const QString& name);
    QDomNode item(int index) const;
    QDomNode namedItemNS(const QString& nsURI, const QString& localName) const;
    QDomNode setNamedItemNS(const QDomNode& newNode);
    QDomNode removeNamedItemNS(const QString& nsURI, const QString& localName);

    // DOM read only attributes
    int length() const;
    int count() const { return length(); } // Qt API consitancy
    inline int size() const { return length(); } // Qt API consistency
    inline bool isEmpty() const { return length() == 0; } // Qt API consistency

    // Qt extension
    bool contains(const QString& name) const;

private:
    QDomNamedNodeMapPrivate* impl;
    QDomNamedNodeMap(QDomNamedNodeMapPrivate*);

    friend class QDomNode;
    friend class QDomDocumentType;
    friend class QDomElement;
};

class Q_XML_EXPORT QDomDocumentFragment : public QDomNode
{
public:
    QDomDocumentFragment();
    QDomDocumentFragment(const QDomDocumentFragment &documentFragment);
    QDomDocumentFragment& operator=(const QDomDocumentFragment &other);

    // Overridden from QDomNode
    inline QDomNode::NodeType nodeType() const { return DocumentFragmentNode; }

private:
    QDomDocumentFragment(QDomDocumentFragmentPrivate*);

    friend class QDomDocument;
    friend class QDomNode;
};

class Q_XML_EXPORT QDomCharacterData : public QDomNode
{
public:
    QDomCharacterData();
    QDomCharacterData(const QDomCharacterData &characterData);
    QDomCharacterData& operator=(const QDomCharacterData &other);

    // DOM functions
    QString substringData(unsigned long offset, unsigned long count);
    void appendData(const QString& arg);
    void insertData(unsigned long offset, const QString& arg);
    void deleteData(unsigned long offset, unsigned long count);
    void replaceData(unsigned long offset, unsigned long count, const QString& arg);

    // DOM read only attributes
    int length() const;

    // DOM attributes
    QString data() const;
    void setData(const QString &data);

    // Overridden from QDomNode
    QDomNode::NodeType nodeType() const;

private:
    QDomCharacterData(QDomCharacterDataPrivate*);

    friend class QDomDocument;
    friend class QDomText;
    friend class QDomComment;
    friend class QDomNode;
};

class Q_XML_EXPORT QDomAttr : public QDomNode
{
public:
    QDomAttr();
    QDomAttr(const QDomAttr &attr);
    QDomAttr& operator=(const QDomAttr &other);

    // DOM read only attributes
    QString name() const;
    bool specified() const;
    QDomElement ownerElement() const;

    // DOM attributes
    QString value() const;
    void setValue(const QString &value);

    // Overridden from QDomNode
    inline QDomNode::NodeType nodeType() const { return AttributeNode; }

private:
    QDomAttr(QDomAttrPrivate*);

    friend class QDomDocument;
    friend class QDomElement;
    friend class QDomNode;
};

class Q_XML_EXPORT QDomElement : public QDomNode
{
public:
    QDomElement();
    QDomElement(const QDomElement &element);
    QDomElement& operator=(const QDomElement &other);

    // DOM functions
    QString attribute(const QString& name, const QString& defValue = QString() ) const;
    void setAttribute(const QString& name, const QString& value);
    void setAttribute(const QString& name, qlonglong value);
    void setAttribute(const QString& name, qulonglong value);
    inline void setAttribute(const QString& name, int value)
        { setAttribute(name, qlonglong(value)); }
    inline void setAttribute(const QString& name, uint value)
        { setAttribute(name, qulonglong(value)); }
    void setAttribute(const QString& name, float value);
    void setAttribute(const QString& name, double value);
    void removeAttribute(const QString& name);
    QDomAttr attributeNode(const QString& name);
    QDomAttr setAttributeNode(const QDomAttr& newAttr);
    QDomAttr removeAttributeNode(const QDomAttr& oldAttr);
    QDomNodeList elementsByTagName(const QString& tagname) const;
    bool hasAttribute(const QString& name) const;

    QString attributeNS(const QString& nsURI, const QString& localName, const QString& defValue = QString()) const;
    void setAttributeNS(const QString& nsURI, const QString& qName, const QString& value);
    inline void setAttributeNS(const QString& nsURI, const QString& qName, int value)
        { setAttributeNS(nsURI, qName, qlonglong(value)); }
    inline void setAttributeNS(const QString& nsURI, const QString& qName, uint value)
        { setAttributeNS(nsURI, qName, qulonglong(value)); }
    void setAttributeNS(const QString& nsURI, const QString& qName, qlonglong value);
    void setAttributeNS(const QString& nsURI, const QString& qName, qulonglong value);
    void setAttributeNS(const QString& nsURI, const QString& qName, double value);
    void removeAttributeNS(const QString& nsURI, const QString& localName);
    QDomAttr attributeNodeNS(const QString& nsURI, const QString& localName);
    QDomAttr setAttributeNodeNS(const QDomAttr& newAttr);
    QDomNodeList elementsByTagNameNS(const QString& nsURI, const QString& localName) const;
    bool hasAttributeNS(const QString& nsURI, const QString& localName) const;

    // DOM read only attributes
    QString tagName() const;
    void setTagName(const QString& name); // Qt extension

    // Overridden from QDomNode
    QDomNamedNodeMap attributes() const;
    inline QDomNode::NodeType nodeType() const { return ElementNode; }

    QString text() const;

private:
    QDomElement(QDomElementPrivate*);

    friend class QDomDocument;
    friend class QDomNode;
    friend class QDomAttr;
};

class Q_XML_EXPORT QDomText : public QDomCharacterData
{
public:
    QDomText();
    QDomText(const QDomText &text);
    QDomText& operator=(const QDomText &other);

    // DOM functions
    QDomText splitText(int offset);

    // Overridden from QDomCharacterData
    inline QDomNode::NodeType nodeType() const { return TextNode; }

private:
    QDomText(QDomTextPrivate*);

    friend class QDomCDATASection;
    friend class QDomDocument;
    friend class QDomNode;
};

class Q_XML_EXPORT QDomComment : public QDomCharacterData
{
public:
    QDomComment();
    QDomComment(const QDomComment &comment);
    QDomComment& operator=(const QDomComment &other);

    // Overridden from QDomCharacterData
    inline QDomNode::NodeType nodeType() const { return CommentNode; }

private:
    QDomComment(QDomCommentPrivate*);

    friend class QDomDocument;
    friend class QDomNode;
};

class Q_XML_EXPORT QDomCDATASection : public QDomText
{
public:
    QDomCDATASection();
    QDomCDATASection(const QDomCDATASection &cdataSection);
    QDomCDATASection& operator=(const QDomCDATASection &other);

    // Overridden from QDomText
    inline QDomNode::NodeType nodeType() const { return CDATASectionNode; }

private:
    QDomCDATASection(QDomCDATASectionPrivate*);

    friend class QDomDocument;
    friend class QDomNode;
};

class Q_XML_EXPORT QDomNotation : public QDomNode
{
public:
    QDomNotation();
    QDomNotation(const QDomNotation &notation);
    QDomNotation& operator=(const QDomNotation &other);

    // DOM read only attributes
    QString publicId() const;
    QString systemId() const;

    // Overridden from QDomNode
    inline QDomNode::NodeType nodeType() const { return NotationNode; }

private:
    QDomNotation(QDomNotationPrivate*);

    friend class QDomDocument;
    friend class QDomNode;
};

class Q_XML_EXPORT QDomEntity : public QDomNode
{
public:
    QDomEntity();
    QDomEntity(const QDomEntity &entity);
    QDomEntity& operator=(const QDomEntity &other);

    // DOM read only attributes
    QString publicId() const;
    QString systemId() const;
    QString notationName() const;

    // Overridden from QDomNode
    inline QDomNode::NodeType nodeType() const { return EntityNode; }

private:
    QDomEntity(QDomEntityPrivate*);

    friend class QDomNode;
};

class Q_XML_EXPORT QDomEntityReference : public QDomNode
{
public:
    QDomEntityReference();
    QDomEntityReference(const QDomEntityReference &entityReference);
    QDomEntityReference& operator=(const QDomEntityReference &other);

    // Overridden from QDomNode
    inline QDomNode::NodeType nodeType() const { return EntityReferenceNode; }

private:
    QDomEntityReference(QDomEntityReferencePrivate*);

    friend class QDomDocument;
    friend class QDomNode;
};

class Q_XML_EXPORT QDomProcessingInstruction : public QDomNode
{
public:
    QDomProcessingInstruction();
    QDomProcessingInstruction(const QDomProcessingInstruction &processingInstruction);
    QDomProcessingInstruction& operator=(const QDomProcessingInstruction &other);

    // DOM read only attributes
    QString target() const;

    // DOM attributes
    QString data() const;
    void setData(const QString &data);

    // Overridden from QDomNode
    inline QDomNode::NodeType nodeType() const { return ProcessingInstructionNode; }

private:
    QDomProcessingInstruction(QDomProcessingInstructionPrivate*);

    friend class QDomDocument;
    friend class QDomNode;
};


Q_XML_EXPORT QTextStream& operator<<(QTextStream& stream, const QDomNode& node);

QT_END_NAMESPACE

#endif // feature dom

#endif // QDOM_H
