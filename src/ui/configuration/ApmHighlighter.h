/*=====================================================================

APM_PLANNER Open Source Ground Control Station

(c) 2013, Bill Bonney <billbonney@communistech.com>

This file is part of the APM_PLANNER project

    APM_PLANNER is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    APM_PLANNER is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with APM_PLANNER. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief APM Highligther for ArduPilot Console.
 *
 *   @author Bill Bonney <billbonney@communistech.com>
 *
 */

#ifndef APMHIGHLIGHTER_H
#define APMHIGHLIGHTER_H

#include "ApmHighlighter.h"
#include <QSyntaxHighlighter>

class APMHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit APMHighlighter(QObject *parent = 0);
    void highlightBlock(const QString &text);

signals:
    
public slots:
    
};

#endif // APMHIGHLIGHTER_H
