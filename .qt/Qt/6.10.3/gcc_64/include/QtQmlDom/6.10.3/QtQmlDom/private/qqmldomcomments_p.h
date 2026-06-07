// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDOMCOMMENTS_P_H
#define QQMLDOMCOMMENTS_P_H

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

#include "qqmldom_fwd_p.h"
#include "qqmldomconstants_p.h"
#include "qqmldomitem_p.h"
#include "qqmldomfilelocations_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#include <QtCore/QMultiMap>
#include <QtCore/QHash>
#include <QtCore/QStack>
#include <QtCore/QCoreApplication>

#include <memory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT CommentInfo
{
    Q_DECLARE_TR_FUNCTIONS(CommentInfo)
public:
    CommentInfo(QStringView, QQmlJS::SourceLocation loc);

    QStringView preWhitespace() const { return rawComment.mid(0, commentBegin); }

    QStringView comment() const { return rawComment.mid(commentBegin, commentEnd - commentBegin); }

    QStringView commentContent() const
    {
        return rawComment.mid(commentContentBegin, commentContentEnd - commentContentEnd);
    }

    QStringView postWhitespace() const
    {
        return rawComment.mid(commentEnd, rawComment.size() - commentEnd);
    }

    // Comment source location populated during lexing doesn't include start strings // or /*
    // Returns the location starting from // or /*
    QQmlJS::SourceLocation sourceLocation() const { return commentLocation; }

    quint32 commentBegin = 0;
    quint32 commentEnd = 0;
    quint32 commentContentBegin = 0;
    quint32 commentContentEnd = 0;
    QStringView commentStartStr;
    QStringView commentEndStr;
    bool hasStartNewline = false;
    bool hasEndNewline = false;
    int nContentNewlines = 0;
    QStringView rawComment;
    QStringList warnings;
    QQmlJS::SourceLocation commentLocation;
};

/*!
\internal
Anchor comments at an AST::Node* (DefaultLocation) or at an QQmlJS::SourceLocation inside of the
AST::Node*. In the latter case, the member location takes the value of the anchor's
QQmlJS::SourceLocation::offset, so that the formatter can differentiate between multiple anchors
inside the same AST::Node*.
*/
struct CommentAnchor
{
    qsizetype location = -1;
    static CommentAnchor from(const SourceLocation &sl) { return CommentAnchor{ sl.begin() }; }
    friend bool comparesEqual(const CommentAnchor &a, const CommentAnchor &b) noexcept
    {
        return a.location == b.location;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(CommentAnchor)
};

inline size_t qHash(const CommentAnchor &key, size_t seed = 0) noexcept
{
    return qHashMulti(seed, key.location);
}

class QMLDOM_EXPORT Comment
{
public:
    constexpr static DomType kindValue = DomType::Comment;
    DomType kind() const { return kindValue; }

    enum CommentType {Pre, Post};

    Comment(const QString &c, const QQmlJS::SourceLocation &loc, int newlinesBefore = 1,
            CommentType type = Pre)
        : m_comment(c), m_location(loc), m_newlinesBefore(newlinesBefore), m_type(type)
    {
    }
    Comment(QStringView c, const QQmlJS::SourceLocation &loc, int newlinesBefore = 1,
            CommentType type = Pre)
        : m_comment(c), m_location(loc), m_newlinesBefore(newlinesBefore), m_type(type)
    {
    }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;
    int newlinesBefore() const { return m_newlinesBefore; }
    void setNewlinesBefore(int n) { m_newlinesBefore = n; }
    QStringView rawComment() const { return m_comment; }
    CommentInfo info() const { return CommentInfo(m_comment, m_location); }
    void write(OutWriter &lw) const;

    CommentType type() const { return m_type; }

    friend bool operator==(const Comment &c1, const Comment &c2)
    {
        return c1.m_newlinesBefore == c2.m_newlinesBefore && c1.m_comment == c2.m_comment;
    }
    friend bool operator!=(const Comment &c1, const Comment &c2) { return !(c1 == c2); }

private:
    QStringView m_comment;
    QQmlJS::SourceLocation m_location;
    int m_newlinesBefore;
    CommentType m_type;
};

class QMLDOM_EXPORT CommentedElement
{
public:
    constexpr static DomType kindValue = DomType::CommentedElement;
    DomType kind() const { return kindValue; }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;
    void writePre(OutWriter &lw) const;
    void writePost(OutWriter &lw) const;

    friend bool operator==(const CommentedElement &c1, const CommentedElement &c2)
    {
        return c1.m_preComments == c2.m_preComments && c1.m_postComments == c2.m_postComments;
    }
    friend bool operator!=(const CommentedElement &c1, const CommentedElement &c2)
    {
        return !(c1 == c2);
    }

    void addComment(const Comment &comment)
    {
        if (comment.type() == Comment::CommentType::Pre)
            m_preComments.append(comment);
        else
            m_postComments.append(comment);
    }

    QList<Comment> preComments() const { return m_preComments; }
    QList<Comment> postComments() const { return m_postComments; }

private:
    QList<Comment> m_preComments;
    QList<Comment> m_postComments;
};

class QMLDOM_EXPORT RegionComments
{
public:
    constexpr static DomType kindValue = DomType::RegionComments;
    DomType kind() const { return kindValue; }

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const;

    friend bool operator==(const RegionComments &c1, const RegionComments &c2)
    {
        return c1.m_regionComments == c2.m_regionComments;
    }
    friend bool operator!=(const RegionComments &c1, const RegionComments &c2)
    {
        return !(c1 == c2);
    }

    const QMap<FileLocationRegion, CommentedElement> &regionComments() const { return m_regionComments;}
    Path addComment(const Comment &comment, FileLocationRegion region)
    {
        if (comment.type() == Comment::CommentType::Pre)
            return addPreComment(comment, region);
        else
            return addPostComment(comment, region);
    }

private:
    Path addPreComment(const Comment &comment, FileLocationRegion region)
    {
        const auto preList = m_regionComments[region].preComments();
        index_type idx = preList.size();
        m_regionComments[region].addComment(comment);
        return Path::fromField(Fields::regionComments)
                .withKey(fileLocationRegionName(region))
                .withField(Fields::preComments)
                .withIndex(idx);
    }

    Path addPostComment(const Comment &comment, FileLocationRegion region)
    {
        const auto postList = m_regionComments[region].postComments();
        index_type idx = postList.size();
        m_regionComments[region].addComment(comment);
        return Path::fromField(Fields::regionComments)
                .withKey(fileLocationRegionName(region))
                .withField(Fields::postComments)
                .withIndex(idx);
    }

    QMap<FileLocationRegion, CommentedElement> m_regionComments;
};

class QMLDOM_EXPORT AstComments final : public OwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return std::make_shared<AstComments>(*this);
    }

    using CommentKey = std::pair<AST::Node *, CommentAnchor>;

public:
    constexpr static DomType kindValue = DomType::AstComments;
    DomType kind() const override { return kindValue; }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor)  const override;
    std::shared_ptr<AstComments> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<AstComments>(doCopy(self));
    }

    Path canonicalPath(const DomItem &self) const override { return self.m_ownerPath; }
    AstComments(const std::shared_ptr<Engine> &e) : m_engine(e) { }
    AstComments(const AstComments &o)
        : OwningItem(o), m_engine(o.m_engine), m_commentedElements(o.m_commentedElements)
    {
    }

    const CommentedElement *commentForNode(AST::Node *n, CommentAnchor location) const
    {
        const auto it = m_commentedElements.constFind(CommentKey{ n, location });
        return it == m_commentedElements.end() ? nullptr : &*it;
    }

    std::pair<QList<Comment>, QList<Comment>> collectPreAndPostComments() const
    {
        QList<Comment> pre;
        QList<Comment> post;
        for (const auto &element : m_commentedElements) {
            pre.append(element.preComments());
            post.append(element.postComments());
        }
        return std::make_pair(pre, post);
    }

    CommentedElement *ensureCommentForNode(AST::Node *n, CommentAnchor location)
    {
        const CommentKey key{ n, location };
        return &m_commentedElements[key];
    }

    QMultiMap<quint32, const QList<Comment> *> allCommentsInNode(AST::Node *n);

private:
    std::shared_ptr<Engine> m_engine;
    QHash<CommentKey, CommentedElement> m_commentedElements;
};

class CommentCollector
{
public:
    CommentCollector() = default;
    CommentCollector(MutableDomItem item);
    void collectComments();
    void collectComments(const std::shared_ptr<Engine> &engine, AST::Node *rootNode,
                         const std::shared_ptr<AstComments> &astComments);

private:
    MutableDomItem m_rootItem;
    FileLocations::Tree m_fileLocations;
};

class VisitAll : public AST::Visitor
{
public:
    VisitAll() = default;

    static QSet<int> uiKinds();

    void throwRecursionDepthError() override { }

    bool visit(AST::UiPublicMember *el) override
    {
        AST::Node::accept(el->annotations, this);
        AST::Node::accept(el->memberType, this);
        return true;
    }

    bool visit(AST::UiSourceElement *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiObjectDefinition *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiObjectBinding *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiScriptBinding *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiArrayBinding *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiParameterList *el) override
    {
        AST::Node::accept(el->type, this);
        return true;
    }

    bool visit(AST::UiQualifiedId *el) override
    {
        AST::Node::accept(el->next, this);
        return true;
    }

    bool visit(AST::UiEnumDeclaration *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    bool visit(AST::UiInlineComponent *el) override
    {
        AST::Node::accept(el->annotations, this);
        return true;
    }

    void endVisit(AST::UiImport *el) override { AST::Node::accept(el->version, this); }
    void endVisit(AST::UiPublicMember *el) override { AST::Node::accept(el->parameters, this); }

    void endVisit(AST::UiParameterList *el) override
    {
        AST::Node::accept(el->next, this); // put other args at the same level as this one...
    }

    void endVisit(AST::UiEnumMemberList *el) override
    {
        AST::Node::accept(el->next,
                          this); // put other enum members at the same level as this one...
    }

    bool visit(AST::TemplateLiteral *el) override
    {
        AST::Node::accept(el->expression, this);
        return true;
    }

    void endVisit(AST::Elision *el) override
    {
        AST::Node::accept(el->next, this); // emit other elisions at the same level
    }
};
} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif // QQMLDOMCOMMENTS_P_H
