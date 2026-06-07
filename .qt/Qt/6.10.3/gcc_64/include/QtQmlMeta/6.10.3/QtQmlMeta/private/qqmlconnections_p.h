// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLCONNECTIONS_H
#define QQMLCONNECTIONS_H

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

#include <private/qqmlcustomparser_p.h>

#include <QtQmlMeta/qtqmlmetaexports.h>
#include <QtQml/qqml.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QQmlBoundSignal;
class QQmlContext;
class QQmlConnectionsPrivate;
class Q_QMLMETA_EXPORT QQmlConnections : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlConnections)

    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QObject *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged REVISION(2, 3))
    Q_PROPERTY(bool ignoreUnknownSignals READ ignoreUnknownSignals WRITE setIgnoreUnknownSignals)
    QML_NAMED_ELEMENT(Connections)
    QML_ADDED_IN_VERSION(2, 0)
    QML_CUSTOMPARSER

public:
    QQmlConnections(QObject *parent = nullptr);
    ~QQmlConnections();

    QObject *target() const;
    void setTarget(QObject *);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool ignoreUnknownSignals() const;
    void setIgnoreUnknownSignals(bool ignore);

protected:
    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void targetChanged();
    Q_REVISION(2, 3) void enabledChanged();

private:
    void connectSignals();
    void connectSignalsToMethods();
    void connectSignalsToBindings();
};

// TODO: Drop this class as soon as we can
class QQmlConnectionsParser : public QQmlCustomParser
{
public:
    void verifyBindings(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit,
            const QList<const QV4::CompiledData::Binding *> &props) override;
    void applyBindings(QObject *object, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QList<const QV4::CompiledData::Binding *> &bindings) override;
};

// TODO: We won't need Connections to be a custom type anymore once we can drop the
//       automatic signal handler inference from undeclared properties.
template<>
inline QQmlCustomParser *qmlCreateCustomParser<QQmlConnections>()
{
    return new QQmlConnectionsParser;
}

QT_END_NAMESPACE

#endif
