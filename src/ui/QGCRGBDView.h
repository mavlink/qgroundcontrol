#ifndef QGCRGBDVIEW_H
#define QGCRGBDVIEW_H

#include "HUD.h"

class QGCRGBDView : public HUD
{
    Q_OBJECT
public:
    explicit QGCRGBDView(int width=640, int height=480, QWidget *parent = 0);

signals:

public slots:
    void setActiveUAS(UASInterface* uas);

    void clearData(void);
    void enableRGB(bool enabled);
    void enableDepth(bool enabled);
    void updateData(UASInterface *uas);

protected:
    bool rgbEnabled;
    bool depthEnabled;
    QAction* enableRGBAction;
    QAction* enableDepthAction;

    void contextMenuEvent (QContextMenuEvent* event);
};

#endif // QGCRGBDVIEW_H
