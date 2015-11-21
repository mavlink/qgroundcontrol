/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#ifndef QGCMESSAGEVIEW_H
#define QGCMESSAGEVIEW_H

#include <QWidget>
#include <UASInterface.h>
#include <QVBoxLayout>
#include <QAction>

#include "QGCUnconnectedInfoWidget.h"
#include "UASMessageHandler.h"

class UASMessage;

namespace Ui {
class UASMessageView;
}

// Message View base class
class UASMessageView : public QWidget
{
    Q_OBJECT

public:
    explicit UASMessageView(QWidget *parent = 0);
    virtual ~UASMessageView();
    Ui::UASMessageView* ui() { return _ui; }

private:
    Ui::UASMessageView* _ui;
};

// Message View Widget (used in the Info View tabbed Widget)
class UASMessageViewWidget : public UASMessageView
{
    Q_OBJECT

public:
    explicit UASMessageViewWidget(UASMessageHandler* uasMessageHandler, QWidget *parent = 0);
    ~UASMessageViewWidget();

public slots:
    void handleTextMessage(UASMessage* message);
    void clearMessages();

private:
    QGCUnconnectedInfoWidget*   _unconnectedWidget;
    UASMessageHandler*          _uasMessageHandler;
};

#endif // QGCMESSAGEVIEW_H
