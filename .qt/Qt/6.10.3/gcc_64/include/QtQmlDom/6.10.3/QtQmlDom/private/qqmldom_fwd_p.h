// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLDOM_FWD_P_H
#define QQMLDOM_FWD_P_H

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

#include "qqmldom_global.h"
#include "private/qglobal_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {
namespace FileLocations {
class Node;
}
class AstComments;
class Binding;
class Comment;
class CommentedElement;
class ConstantData;
class DomBase;
enum DomCreationOption : char;
class DomEnvironment;
class DomItem;
class DomTop;
class DomUniverse;
class Empty;
class EnumDecl;
class Export;
class ExternalItemInfoBase;
class ExternalItemPairBase;
class ExternalOwningItem;
enum FileLocationRegion : int;
class FileWriter;
class GlobalComponent;
class GlobalScope;
class MockObject;
class MockOwner;
class Id;
class Import;
class JsFile;
class JsResource;
class List;
class LoadInfo;
class Map;
class MethodInfo;
class ModuleIndex;
class ModuleScope;
class MutableDomItem;
class ObserversTrie;
class OutWriter;
class OutWriterState;
class OwningItem;
class Path;
class Pragma;
class PropertyDefinition;
class PropertyInfo;
class QQmlDomAstCreator;
class QmlComponent;
class QmlDirectory;
class QmldirFile;
class QmlFile;
class QmlObject;
class QmltypesComponent;
class QmltypesFile;
class Reference;
class RegionComments;
class ScriptExpression;
class Source;
class TestDomItem;
class Version;

namespace ScriptElements {
class BlockStatement;
class IdentifierExpression;
class Literal;
class ForStatement;
class IfStatement;
class BinaryExpression;
class VariableDeclaration;
class VariableDeclarationEntry;
class GenericScriptElement;
// TODO: add new script classes here, as qqmldomitem_p.h cannot include qqmldomscriptelements_p.h
// without creating circular dependencies
class ReturnStatement;

} // end namespace ScriptElements

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOM_FWD_P_H
