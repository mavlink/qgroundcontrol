/**
 ******************************************************************************
 *
 * @file       linecolumnlabel.h
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

#ifndef LINECOLUMNLABEL_H
#define LINECOLUMNLABEL_H

#include "utils_global.h"
#include <QtGui/QLabel>

namespace Utils {

/* A label suitable for displaying cursor positions, etc. with a fixed
 * with derived from a sample text. */

class  QTCREATOR_UTILS_EXPORT LineColumnLabel : public QLabel
{
    Q_DISABLE_COPY(LineColumnLabel)
    Q_OBJECT
    Q_PROPERTY(QString maxText READ maxText WRITE setMaxText DESIGNABLE true)

public:
    explicit LineColumnLabel(QWidget *parent = 0);
    virtual ~LineColumnLabel();

    void setText(const QString &text, const QString &maxText);
    QSize sizeHint() const;

    QString maxText() const;
    void setMaxText(const QString &maxText);

private:
    QString m_maxText;
    void *m_unused;
};

} // namespace Utils

#endif // LINECOLUMNLABEL_H
