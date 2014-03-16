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

protected:
    // Stores the UAS that we're currently receiving messages from.
    UASInterface* activeUAS;
    // Stores the connect widget that is displayed when no UAS is active.
    QGCUnconnectedInfoWidget* connectWidget;
    
private:
    Ui::QGCMessageView *ui;
};

#endif // QGCMESSAGEVIEW_H
