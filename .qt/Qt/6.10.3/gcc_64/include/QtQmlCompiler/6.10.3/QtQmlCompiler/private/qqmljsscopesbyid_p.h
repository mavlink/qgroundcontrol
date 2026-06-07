// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSSCOPESBYID_P_H
#define QQMLJSSCOPESBYID_P_H

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

#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

enum QQmlJSScopesByIdOption: char {
    Default = 0,
    AssumeComponentsAreBound = 1,
};
Q_DECLARE_FLAGS(QQmlJSScopesByIdOptions, QQmlJSScopesByIdOption);

class QQmlJSScopesById
{
public:
    enum class Confidence: quint8 { Certain, Possible };
    enum class CallbackResult: bool { StopSearch = false, ContinueSearch = true };
    enum class Success: bool { No = false, Yes = true };

    template<typename T>
    struct CertainCallback
    {
        CallbackResult operator() (const T &candidate, Confidence confidence) {
            // Here, we only accept certain results, and abort the search otherwise.
            switch (confidence) {
            case Confidence::Certain:
                result = candidate;
                return CallbackResult::ContinueSearch;
            case Confidence::Possible:
                break;
            }
            return CallbackResult::StopSearch;
        }

        T result = {};
    };

    template<typename T>
    struct MostLikelyCallback
    {
        CallbackResult operator() (const T &candidate, Confidence confidence) {
            // The last one in a chain of candidates is the one that's definitely a component,
            // by virtue of either being the file root component or a recognized inline component,
            // or QQmlComponent property.
            Q_UNUSED(confidence);
            result = candidate;
            return CallbackResult::ContinueSearch;
        }

        T result = {};
    };

    bool componentsAreBound() const { return m_componentsAreBound; }
    void setComponentsAreBound(bool bound) { m_componentsAreBound = bound; }

    void setSignaturesAreEnforced(bool enforced) { m_signaturesAreEnforced = enforced; }
    bool signaturesAreEnforced() const { return m_signaturesAreEnforced; }

    void setValueTypesAreAddressable(bool addressable) { m_valueTypesAreAddressable = addressable; }
    bool valueTypesAreAddressable() const { return m_valueTypesAreAddressable; }

    /*!
        \internal
        Find the possible IDs for \a scope as seen by \a referrer. There can be at most one
        ID for a scope. Depending on whether we can determine the component boundaries of the
        \a scope and the \a referrer we may or may not be able to tell whether it's visible.

        We can generally determine the relevant component boundaries for each scope. However,
        if the scope or any of its parents is assigned to a property of which we cannot see the
        type, we don't know whether the type of that property happens to be Component. In that
        case, we can't say.

        Returns \c Success::Yes if either no ID was found or the \a callback returned
        \c CallbackResult::ContinueSearch for the ID found. Returns \c Success::No if the
        \a callback returned \c CallbackResult::StopSearch.
     */
    template<typename F>
    Success possibleIds(
            const QQmlJSScope::ConstPtr &scope, const QQmlJSScope::ConstPtr &referrer,
            QQmlJSScopesByIdOptions options, F &&callback) const
    {
        Q_ASSERT(!scope.isNull());

        // A scope can only have one ID.
        const QString key = m_scopesById.key(scope);
        if (key.isEmpty())
            return Success::Yes;

        Success result = Success::Yes;
        possibleComponentRoots(
                referrer, [&](const QQmlJSScope::ConstPtr &referrerRoot,
                              QQmlJSScope::IsComponentRoot referrerConfidence) {
            return possibleComponentRoots(
                            scope, [&](const QQmlJSScope::ConstPtr &referredRoot,
                                       QQmlJSScope::IsComponentRoot referredConfidence) {
                if (isComponentVisible(referredRoot, referrerRoot, options)) {
                    // The key won't change and our confidence won't change either. No need to
                    // call this again for each combination of scopes.
                    if (callback(key, confidence(referrerConfidence, referredConfidence))
                            == CallbackResult::StopSearch) {
                        result = Success::No;
                    }

                    return CallbackResult::StopSearch;
                }
                return CallbackResult::ContinueSearch;
            });
        });

        return result;
    }

    /*!
        \internal
        Returns the id of \a scope in the component to which \a referrer belongs to.
        If \a scope is not visible from \a referrer or has no ID, an empty string is returned.
        An empty string is also returned if we can't determine the component boundaries for either
        \a scope or \a referrer.
     */
    QString id(const QQmlJSScope::ConstPtr &scope, const QQmlJSScope::ConstPtr &referrer,
               QQmlJSScopesByIdOptions options = Default) const
    {
        CertainCallback<QString> result;
        const Success isCertain = possibleIds(scope, referrer, options, result);

        // The default callback only assigns the result if it's certain.
        // We can't have "possible" results after a certain one.
        Q_ASSERT(isCertain == Success::Yes || result.result.isEmpty());

        return result.result;
    }

    /*!
        \internal
        Find all possible scopes for \a id as seen by \a referrer. There can be multiple
        possibilities if we cannot determine component boundaries for any candidate or the
        referrer.

        We can generally determine the relevant component boundaries for each scope. However,
        if the scope or any of its parents is assigned to a property of which we cannot see the
        type, we don't know whether the type of that property happens to be Component. In that
        case, we can't say.

        Returns \c Success::Yes if either no suitable scope was found or the \a callback returned
        \c CallbackResult::ContinueSearch for all scopes found. Returns \c Success::No if the
        \a callback returns \c CallbackResult::StopSearch for any scope found. It also stops the
        search at that point.
     */
    template<typename F>
    Success possibleScopes(
            const QString &id, const QQmlJSScope::ConstPtr &referrer,
            QQmlJSScopesByIdOptions options, F &&callback) const
    {
        Q_ASSERT(!id.isEmpty());
        Success result = Success::Yes;

        const auto range =  m_scopesById.equal_range(id);
        for (auto it = range.first; it != range.second; ++it) {
            possibleComponentRoots(
                    *it, [&](const QQmlJSScope::ConstPtr &referredRoot,
                             QQmlJSScope::IsComponentRoot referredConfidence) {

                possibleComponentRoots(
                    referrer, [&](const QQmlJSScope::ConstPtr &referrerRoot,
                                  QQmlJSScope::IsComponentRoot referrerConfidence) {

                    if (!isComponentVisible(referredRoot, referrerRoot, options))
                        return CallbackResult::ContinueSearch;

                    if (callback(*it, confidence(referrerConfidence, referredConfidence))
                            == CallbackResult::StopSearch) {
                        // Propagate the negative result from the callback.
                        result = Success::No;
                    }

                    // Once we've reported *it, we don't care about the other possible referrerRoots
                    // anymore. They are not reported after all. The confidence can't change
                    // anymore, either.
                    return CallbackResult::StopSearch;
                });

                // If nothing matched or the callback was successful, consider the next candidate.
                // If the callback failed, stop here.
                return result == Success::Yes
                        ? CallbackResult::ContinueSearch
                        : CallbackResult::StopSearch;
            });

            // If the callback failed, return right away.
            if (result == Success::No)
                return result;
        }

        Q_ASSERT(result == Success::Yes);
        return Success::Yes;
    }

    /*!
        \internal
        Returns the scope that has id \a id in the component to which \a referrer belongs to.
        If no such scope exists, a null scope is returned.
        A null scope is also returned if we cannot determine the component boundaries for any
        candidate or the \a referrer.
     */
    QQmlJSScope::ConstPtr scope(const QString &id, const QQmlJSScope::ConstPtr &referrer,
                                QQmlJSScopesByIdOptions options = Default) const
    {
        CertainCallback<QQmlJSScope::ConstPtr> result;
        const Success isCertain = possibleScopes(id, referrer, options, result);

        // The default callback only assigns the result if it's certain.
        // We can't have "possible" results after a certain one.
        Q_ASSERT(isCertain == Success::Yes || result.result.isNull());

        return result.result;
    }

    void insert(const QString &id, const QQmlJSScope::ConstPtr &scope)
    {
        Q_ASSERT(!id.isEmpty());
        m_scopesById.insert(id, scope);
    }

    void clear() { m_scopesById.clear(); }

    /*!
        \internal
        Returns \c true if \a id exists anywhere in the current document.
        This is still allowed if the other occurrence is in a different (inline) component.
        Check the return value of scope to know whether the id has already been assigned
        in a givne scope.
    */
    bool existsAnywhereInDocument(const QString &id) const { return m_scopesById.contains(id); }

private:
    template<typename F>
    static CallbackResult possibleComponentRoots(const QQmlJSScope::ConstPtr &inner, F &&callback)
    {
        QQmlJSScope::ConstPtr scope = inner;
        QQmlJSScope::IsComponentRoot maxConfidence = QQmlJSScope::IsComponentRoot::Yes;
        while (scope) {
            switch (scope->componentRootStatus()) {
            case QQmlJSScope::IsComponentRoot::Maybe:
                if (callback(scope, QQmlJSScope::IsComponentRoot::Maybe)
                        == CallbackResult::StopSearch) {
                    return CallbackResult::StopSearch;
                }
                // If we've seen one "maybe", then there is no certainty anymore.
                // The "maybe" ones are always processed first since the properties of unknown
                // type are inside the elements they belong to.
                maxConfidence = QQmlJSScope::IsComponentRoot::Maybe;
                Q_FALLTHROUGH();
            case QQmlJSScope::IsComponentRoot::No:
                scope = scope->parentScope();
                continue;
            case QQmlJSScope::IsComponentRoot::Yes:
                return callback(scope, maxConfidence);
            }
        }

        return CallbackResult::ContinueSearch;
    }

    static Confidence confidence(
            QQmlJSScope::IsComponentRoot a, QQmlJSScope::IsComponentRoot b) {
        switch (a) {
        case QQmlJSScope::IsComponentRoot::Yes:
            return b == QQmlJSScope::IsComponentRoot::Yes
                    ? Confidence::Certain
                    : Confidence::Possible;
        case QQmlJSScope::IsComponentRoot::Maybe:
            return Confidence::Possible;
        default:
            break;
        }

        Q_UNREACHABLE_RETURN(Confidence::Certain);
    }

    bool isComponentVisible(const QQmlJSScope::ConstPtr &observed,
                            const QQmlJSScope::ConstPtr &observer,
                            QQmlJSScopesByIdOptions options) const
    {
        if (!m_componentsAreBound && !options.testAnyFlag(AssumeComponentsAreBound))
            return observed == observer;

        for (QQmlJSScope::ConstPtr scope = observer; scope; scope = scope->parentScope()) {
            if (scope == observed)
                return true;
        }

        return false;
    }

    QMultiHash<QString, QQmlJSScope::ConstPtr> m_scopesById;
    bool m_componentsAreBound = false;
    bool m_signaturesAreEnforced = true;
    bool m_valueTypesAreAddressable = false;
};

QT_END_NAMESPACE

#endif // QQMLJSSCOPESBYID_P_H
