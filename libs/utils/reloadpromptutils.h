/**
 ******************************************************************************
 *
 * @file       reloadpromptutils.h
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

#ifndef RELOADPROMPTUTILS_H
#define RELOADPROMPTUTILS_H

#include "utils_global.h"

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace Utils {

enum ReloadPromptAnswer { ReloadCurrent, ReloadAll, ReloadSkipCurrent, ReloadNone };

QTCREATOR_UTILS_EXPORT ReloadPromptAnswer reloadPrompt(const QString &fileName, bool modified, QWidget *parent);
QTCREATOR_UTILS_EXPORT ReloadPromptAnswer reloadPrompt(const QString &title, const QString &prompt, QWidget *parent);

} // namespace Utils

#endif // RELOADPROMPTUTILS_H
