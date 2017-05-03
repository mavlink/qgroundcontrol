#ifndef QGCTABBEDINFOVIEW_H
#define QGCTABBEDINFOVIEW_H

#include "QGCDockWidget.h"
#include "MAVLinkDecoder.h"
#include "UASMessageView.h"
#include "UASQuickView.h"

#include "ui_QGCTabbedInfoView.h"

class QGCTabbedInfoView : public QGCDockWidget
{
    Q_OBJECT
    
public:
    explicit QGCTabbedInfoView(const QString& title, QAction* action, QWidget *parent = 0);
    ~QGCTabbedInfoView();
    void addSource(MAVLinkDecoder *decoder);
private:
    MAVLinkDecoder *m_decoder;
    Ui::QGCTabbedInfoView ui;
    UASMessageViewWidget *messageView;
    UASQuickView *quickView;
};

#endif // QGCTABBEDINFOVIEW_H
