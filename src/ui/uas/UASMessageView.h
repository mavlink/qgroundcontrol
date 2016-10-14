/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
