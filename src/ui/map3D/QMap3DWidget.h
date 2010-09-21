#ifndef QMAP3DWIDGET_H
#define QMAP3DWIDGET_H

#include "Q3DWidget.h"

class CheetahModel;
class FTTextureFont;
class UASInterface;

class QMap3DWidget : public Q3DWidget
{
    Q_OBJECT

public:
    explicit QMap3DWidget(QWidget* parent);
    ~QMap3DWidget();

    void init(void);

    static void display(void* clientData);
    void displayHandler(void);

    static void timer(void* clientData);
    void timerHandler(void);

    double getTime(void) const;

public slots:
    void setActiveUAS(UASInterface* uas);

protected:
    UASInterface* uas;

private:
    double lastRedrawTime;

    boost::scoped_ptr<CheetahModel> cheetahModel;
    boost::scoped_ptr<FTTextureFont> font;
};

#endif // QMAP3DWIDGET_H
