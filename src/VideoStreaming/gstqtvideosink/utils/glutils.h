/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief QGC Video Item
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef GLUTILS_H
#define GLUTILS_H

#ifdef __mobile__
#include <QOpenGLFunctions>
#define getQOpenGLFunctions() QOpenGLContext::currentContext()->functions()
#define QOpenGLFunctionsDef QOpenGLFunctions
#endif

#ifdef __rasp_pi2__
#include <QOpenGLFunctions_ES2>
#define getQOpenGLFunctions() QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_ES2>()
#define QOpenGLFunctionsDef QOpenGLFunctions_ES2
#endif

#ifndef QOpenGLFunctionsDef
#include <QOpenGLFunctions_2_0>
#define getQOpenGLFunctions() QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_2_0>()
#define QOpenGLFunctionsDef QOpenGLFunctions_2_0
#endif

#endif
