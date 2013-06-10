#ifndef QGCMESSAGEVIEW_H
#define QGCMESSAGEVIEW_H

#include <QWidget>
#include <UASInterface.h>
#include <QVBoxLayout>
#include <QAction>
#include "QGCUnconnectedInfoWidget.h"

namespace Ui {
class QGCMessageView;
}

class QGCMessageView : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCMessageView(QWidget *parent = 0);
    ~QGCMessageView();

public slots:
    /**
     * @brief Set currently active UAS
     * @param uas the current active UAS
     */
    void setActiveUAS(UASInterface* uas);
    /**
     * @brief Handle text message from current active UAS
     * @param uasid
     * @param componentid
     * @param severity
     * @param text
     */
    void handleTextMessage(int uasid, int componentid, int severity, QString text);

    /**
     * @brief Hand context menu event
     * @param event
     */
    virtual void contextMenuEvent(QContextMenuEvent* event);

protected:
    UASInterface* activeUAS;
    QVBoxLayout* initialLayout;
    QGCUnconnectedInfoWidget *connectWidget;
    QAction* clearAction;
    
private:
    Ui::QGCMessageView *ui;
};

#endif // QGCMESSAGEVIEW_H
