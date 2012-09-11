/**
 ******************************************************************************
 *
 * @file       parameteraction.h
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

#ifndef PARAMETERACTION_H
#define PARAMETERACTION_H

#include "utils_global.h"

#include <QtGui/QAction>

namespace Utils {

/* ParameterAction: Intended for actions that act on a 'current',
 * string-type parameter (typically file name) and have 2 states:
 * 1) <no current parameter> displaying "Do XX" (empty text)
 * 2) <parameter present> displaying "Do XX with %1".
 * Provides a slot to set the parameter, changing display
 * and enabled state accordingly.
 * The text passed in should already be translated; parameterText
 * should contain a %1 where the parameter is to be inserted. */

class QTCREATOR_UTILS_EXPORT ParameterAction : public QAction
{
    Q_ENUMS(EnablingMode)
    Q_PROPERTY(QString emptyText READ emptyText WRITE setEmptyText)
    Q_PROPERTY(QString parameterText READ parameterText WRITE setParameterText)
    Q_PROPERTY(EnablingMode enablingMode READ enablingMode WRITE setEnablingMode)
    Q_OBJECT
public:
    enum EnablingMode { AlwaysEnabled, EnabledWithParameter };

    explicit ParameterAction(const QString &emptyText,
                             const QString &parameterText,
                             EnablingMode em = AlwaysEnabled,
                             QObject* parent = 0);

    QString emptyText() const;
    void setEmptyText(const QString &);

    QString parameterText() const;
    void setParameterText(const QString &);

    EnablingMode enablingMode() const;
    void setEnablingMode(EnablingMode m);

public slots:
    void setParameter(const QString &);

private:
    QString m_emptyText;
    QString m_parameterText;
    EnablingMode m_enablingMode;
};

}

#endif // PARAMETERACTION_H
