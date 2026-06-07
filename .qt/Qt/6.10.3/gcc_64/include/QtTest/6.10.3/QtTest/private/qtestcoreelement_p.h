// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTESTCOREELEMENT_P_H
#define QTESTCOREELEMENT_P_H

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

#include <QtTest/qttestglobal.h>
#include <QtTest/private/qtestelementattribute_p.h>

#include <vector>

QT_BEGIN_NAMESPACE


template <class ElementType>
class QTestCoreElement
{
    public:
        QTestCoreElement(QTest::LogElementType type = QTest::LET_Undefined);
        virtual ~QTestCoreElement();

        void addAttribute(const QTest::AttributeIndex index, const char *value);
        const std::vector<QTestElementAttribute*> &attributes() const;
        const char *attributeValue(QTest::AttributeIndex index) const;
        const char *attributeName(QTest::AttributeIndex index) const;
        const QTestElementAttribute *attribute(QTest::AttributeIndex index) const;

        const char *elementName() const;
        QTest::LogElementType elementType() const;

    private:
        std::vector<QTestElementAttribute*> listOfAttributes;
        QTest::LogElementType type;
};

template<class ElementType>
QTestCoreElement<ElementType>::QTestCoreElement(QTest::LogElementType t)
    : type(t)
{
}

template<class ElementType>
QTestCoreElement<ElementType>::~QTestCoreElement()
{
    for (auto *attribute : listOfAttributes)
        delete attribute;
}

template <class ElementType>
void QTestCoreElement<ElementType>::addAttribute(const QTest::AttributeIndex attributeIndex, const char *value)
{
    if (attributeIndex == -1 || attribute(attributeIndex))
        return;

    QTestElementAttribute *testAttribute = new QTestElementAttribute;
    testAttribute->setPair(attributeIndex, value);
    listOfAttributes.push_back(testAttribute);
}

template <class ElementType>
const std::vector<QTestElementAttribute*> &QTestCoreElement<ElementType>::attributes() const
{
    return listOfAttributes;
}

template <class ElementType>
const char *QTestCoreElement<ElementType>::attributeValue(QTest::AttributeIndex index) const
{
    const QTestElementAttribute *attrb = attribute(index);
    if (attrb)
        return attrb->value();

    return nullptr;
}

template <class ElementType>
const char *QTestCoreElement<ElementType>::attributeName(QTest::AttributeIndex index) const
{
    const QTestElementAttribute *attrb = attribute(index);
    if (attrb)
        return attrb->name();

    return nullptr;
}

template <class ElementType>
const char *QTestCoreElement<ElementType>::elementName() const
{
    const char *xmlElementNames[] =
    {
        "property",
        "properties",
        "failure",
        "error",
        "testcase",
        "testsuite",
        "message",
        "system-err",
        "system-out",
        "skipped"
    };

    if (type != QTest::LET_Undefined)
        return xmlElementNames[type];

    return nullptr;
}

template <class ElementType>
QTest::LogElementType QTestCoreElement<ElementType>::elementType() const
{
    return type;
}

template <class ElementType>
const QTestElementAttribute *QTestCoreElement<ElementType>::attribute(QTest::AttributeIndex index) const
{
    for (auto *attribute : listOfAttributes) {
        if (attribute->index() == index)
            return attribute;
    }

    return nullptr;
}

QT_END_NAMESPACE

#endif
