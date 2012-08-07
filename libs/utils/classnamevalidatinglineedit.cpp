/**
 ******************************************************************************
 *
 * @file       classnamevalidatinglineedit.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief      
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   
 * @{
 * 
 *****************************************************************************/
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "classnamevalidatinglineedit.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

namespace Utils {

struct ClassNameValidatingLineEditPrivate {
    ClassNameValidatingLineEditPrivate();

    const QRegExp m_nameRegexp;
    const QString m_namespaceDelimiter;
    bool m_namespacesEnabled;
    bool m_lowerCaseFileName;
};

// Match something like "Namespace1::Namespace2::ClassName".
ClassNameValidatingLineEditPrivate:: ClassNameValidatingLineEditPrivate() :
    m_nameRegexp(QLatin1String("[a-zA-Z_][a-zA-Z0-9_]*(::[a-zA-Z_][a-zA-Z0-9_]*)*")),
    m_namespaceDelimiter(QLatin1String("::")),
    m_namespacesEnabled(false),
    m_lowerCaseFileName(true)
{
    QTC_ASSERT(m_nameRegexp.isValid(), return);
}

// --------------------- ClassNameValidatingLineEdit
ClassNameValidatingLineEdit::ClassNameValidatingLineEdit(QWidget *parent) :
    Utils::BaseValidatingLineEdit(parent),
    m_d(new ClassNameValidatingLineEditPrivate)
{
}

ClassNameValidatingLineEdit::~ClassNameValidatingLineEdit()
{
    delete m_d;
}

bool ClassNameValidatingLineEdit::namespacesEnabled() const
{
    return m_d->m_namespacesEnabled;
}

void ClassNameValidatingLineEdit::setNamespacesEnabled(bool b)
{
    m_d->m_namespacesEnabled = b;
}

bool ClassNameValidatingLineEdit::validate(const QString &value, QString *errorMessage) const
{
    if (!m_d->m_namespacesEnabled && value.contains(QLatin1Char(':'))) {
        if (errorMessage)
            *errorMessage = tr("The class name must not contain namespace delimiters.");
        return false;
    } else if (value.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("Please enter a class name.");
        return false;
    } else if (!m_d->m_nameRegexp.exactMatch(value)) {
        if (errorMessage)
            *errorMessage = tr("The class name contains invalid characters.");
        return false;
    }
    return true;
}

void ClassNameValidatingLineEdit::slotChanged(const QString &t)
{
    Utils::BaseValidatingLineEdit::slotChanged(t);
    if (isValid()) {
        // Suggest file names, strip namespaces
        QString fileName = m_d->m_lowerCaseFileName ? t.toLower() : t;
        if (m_d->m_namespacesEnabled) {
            const int namespaceIndex = fileName.lastIndexOf(m_d->m_namespaceDelimiter);
            if (namespaceIndex != -1)
                fileName.remove(0, namespaceIndex + m_d->m_namespaceDelimiter.size());
        }
        emit updateFileName(fileName);
    }
}

QString ClassNameValidatingLineEdit::createClassName(const QString &name)
{
    // Remove spaces and convert the adjacent characters to uppercase
    QString className = name;
    QRegExp spaceMatcher(QLatin1String(" +(\\w)"), Qt::CaseSensitive, QRegExp::RegExp2);
    QTC_ASSERT(spaceMatcher.isValid(), /**/);
    int pos;
    while ((pos = spaceMatcher.indexIn(className)) != -1) {
        className.replace(pos, spaceMatcher.matchedLength(),
                          spaceMatcher.cap(1).toUpper());
    }

    // Filter out any remaining invalid characters
    className.remove(QRegExp(QLatin1String("[^a-zA-Z0-9_]")));

    // If the first character is numeric, prefix the name with a "_"
    if (className.at(0).isNumber()) {
        className.prepend(QLatin1Char('_'));
    } else {
        // Convert the first character to uppercase
        className.replace(0, 1, className.left(1).toUpper());
    }

    return className;
}

bool ClassNameValidatingLineEdit::lowerCaseFileName() const
{
    return m_d->m_lowerCaseFileName;
}

void ClassNameValidatingLineEdit::setLowerCaseFileName(bool v)
{
    m_d->m_lowerCaseFileName = v;
}

} // namespace Utils
