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

class QGCUasMessage;
class QGCToolBar;

namespace Ui {
class QGCMessageView;
}

// Message View base class
class QGCMessageView : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCMessageView(QWidget *parent = 0);
    virtual ~QGCMessageView();
    Ui::QGCMessageView* ui() { return _ui; }
private:
    Ui::QGCMessageView* _ui;
};

// Message View Widget (used in the Info View tabbed Widget)
class QGCMessageViewWidget : public QGCMessageView
{
    Q_OBJECT
public:
    explicit QGCMessageViewWidget(QWidget *parent = 0);
    ~QGCMessageViewWidget();
public slots:
    void handleTextMessage(QGCUasMessage* message);
private:
    QGCUnconnectedInfoWidget* _unconnectedWidget;
};

// Roll down Message View
class QGCMessageViewRollDown : public QGCMessageView
{
    Q_OBJECT
public:
    explicit QGCMessageViewRollDown(QWidget *parent, QGCToolBar* toolBar);
    ~QGCMessageViewRollDown();
public slots:
    void handleTextMessage(QGCUasMessage* message);
protected:
    void leaveEvent(QEvent* event);
private:
    QGCToolBar* _toolBar;
};

#endif // QGCMESSAGEVIEW_H
